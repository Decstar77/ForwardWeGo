#include "atto_renderer.h"
#include "atto_log.h"
#include "renderer/atto_render_model.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace atto {

    static const char * TEST_TRIANGLE_VERT = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;

        uniform mat4 uViewProjection;

        out vec3 vColor;

        void main() {
            gl_Position = uViewProjection * vec4(aPos, 1.0);
            vColor = aColor;
        }
    )";

    static const char * TEST_TRIANGLE_FRAG = R"(
        #version 330 core
        in vec3 vColor;

        out vec4 FragColor;

        void main() {
            FragColor = vec4(vColor, 1.0);
        }
    )";

    static const char * MODEL_VERT = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoords;

        uniform mat4 uViewProjection;
        uniform mat4 uModel;

        out vec3 vNormal;
        out vec3 vFragPos;

        void main() {
            vec4 worldPos = uModel * vec4(aPos, 1.0);
            vFragPos = worldPos.xyz;
            vNormal = mat3(transpose(inverse(uModel))) * aNormal;
            gl_Position = uViewProjection * worldPos;
        }
    )";

    static const char * MODEL_FRAG = R"(
        #version 330 core
        in vec3 vNormal;
        in vec3 vFragPos;

        uniform vec3 uLightDir;
        uniform vec3 uLightColor;
        uniform vec3 uObjectColor;

        out vec4 FragColor;

        void main() {
            vec3 norm = normalize(vNormal);
            vec3 lightDir = normalize(-uLightDir);

            float ambient = 0.15;
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 result = (ambient + diff) * uLightColor * uObjectColor;

            FragColor = vec4(result, 1.0);
        }
    )";

    u32 Renderer::CompileShader( u32 type, const char * source ) {
        u32 shader = glCreateShader( type );
        glShaderSource( shader, 1, &source, nullptr );
        glCompileShader( shader );

        i32 success = 0;
        glGetShaderiv( shader, GL_COMPILE_STATUS, &success );
        if ( !success ) {
            char infoLog[512];
            glGetShaderInfoLog( shader, sizeof( infoLog ), nullptr, infoLog );
            const char * typeStr = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
            LOG_ERROR( "Shader compilation failed (%s): %s", typeStr, infoLog );
            glDeleteShader( shader );
            return 0;
        }

        return shader;
    }

    u32 Renderer::CreateShaderProgram( const char * vertexSrc, const char * fragmentSrc ) {
        u32 vertShader = CompileShader( GL_VERTEX_SHADER, vertexSrc );
        if ( vertShader == 0 ) return 0;

        u32 fragShader = CompileShader( GL_FRAGMENT_SHADER, fragmentSrc );
        if ( fragShader == 0 ) {
            glDeleteShader( vertShader );
            return 0;
        }

        u32 program = glCreateProgram();
        glAttachShader( program, vertShader );
        glAttachShader( program, fragShader );
        glLinkProgram( program );

        i32 success = 0;
        glGetProgramiv( program, GL_LINK_STATUS, &success );
        if ( !success ) {
            char infoLog[512];
            glGetProgramInfoLog( program, sizeof( infoLog ), nullptr, infoLog );
            LOG_ERROR( "Shader program linking failed: %s", infoLog );
            glDeleteProgram( program );
            program = 0;
        }

        glDeleteShader( vertShader );
        glDeleteShader( fragShader );

        return program;
    }

    bool Renderer::Initialize() {
        // Compile test triangle shader
        testTriangleShader = CreateShaderProgram( TEST_TRIANGLE_VERT, TEST_TRIANGLE_FRAG );
        if ( testTriangleShader == 0 ) {
            LOG_ERROR( "Failed to create test triangle shader" );
            return false;
        }
        testTriangleVPLoc = glGetUniformLocation( testTriangleShader, "uViewProjection" );

        // Position (xyz) + Color (rgb)
        f32 vertices[] = {
            -0.5f, -0.5f, 0.0f,    1.0f, 0.0f, 0.0f,
             0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f,
             0.0f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
        };

        glGenVertexArrays( 1, &testTriangleVAO );
        glGenBuffers( 1, &testTriangleVBO );

        glBindVertexArray( testTriangleVAO );

        glBindBuffer( GL_ARRAY_BUFFER, testTriangleVBO );
        glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );

        // Position attribute (location = 0)
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( f32 ), (void *)0 );
        glEnableVertexAttribArray( 0 );

        // Color attribute (location = 1)
        glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( f32 ), (void *)( 3 * sizeof( f32 ) ) );
        glEnableVertexAttribArray( 1 );

        glBindVertexArray( 0 );

        // Compile model shader
        modelShader = CreateShaderProgram( MODEL_VERT, MODEL_FRAG );
        if ( modelShader == 0 ) {
            LOG_ERROR( "Failed to create model shader" );
            return false;
        }
        modelVPLoc = glGetUniformLocation( modelShader, "uViewProjection" );
        modelModelLoc = glGetUniformLocation( modelShader, "uModel" );
        modelLightDirLoc = glGetUniformLocation( modelShader, "uLightDir" );
        modelLightColorLoc = glGetUniformLocation( modelShader, "uLightColor" );
        modelObjectColorLoc = glGetUniformLocation( modelShader, "uObjectColor" );

        glEnable( GL_DEPTH_TEST );

        LOG_INFO( "Renderer initialized" );

        return true;
    }

    void Renderer::Shutdown() {
        if ( testTriangleVAO != 0 ) {
            glDeleteVertexArrays( 1, &testTriangleVAO );
            testTriangleVAO = 0;
        }

        if ( testTriangleVBO != 0 ) {
            glDeleteBuffers( 1, &testTriangleVBO );
            testTriangleVBO = 0;
        }

        if ( testTriangleShader != 0 ) {
            glDeleteProgram( testTriangleShader );
            testTriangleShader = 0;
        }

        if ( modelShader != 0 ) {
            glDeleteProgram( modelShader );
            modelShader = 0;
        }

        LOG_INFO( "Renderer shutdown" );
    }

    void Renderer::BeginFrame() {
        glClearColor( clearColor.r, clearColor.g, clearColor.b, clearColor.a );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    }

    void Renderer::EndFrame() {
    }

    void Renderer::SetClearColor( const Color & color ) {
        clearColor = color;
    }

    void Renderer::SetViewport( i32 x, i32 y, i32 width, i32 height ) {
        glViewport( x, y, width, height );
    }

    void Renderer::SetViewProjectionMatrix( const Mat4 & vp ) {
        viewProjectionMatrix = vp;
    }

    void Renderer::RenderTestTriangle() {
        glUseProgram( testTriangleShader );
        if ( testTriangleVPLoc >= 0 ) {
            glUniformMatrix4fv( testTriangleVPLoc, 1, GL_FALSE, glm::value_ptr( viewProjectionMatrix ) );
        }
        glBindVertexArray( testTriangleVAO );
        glDrawArrays( GL_TRIANGLES, 0, 3 );
        glBindVertexArray( 0 );
        glUseProgram( 0 );
    }

    void Renderer::RenderStaticModel( const StaticModel & model, const Mat4 & modelMatrix ) {
        glUseProgram( modelShader );

        if ( modelVPLoc >= 0 ) {
            glUniformMatrix4fv( modelVPLoc, 1, GL_FALSE, glm::value_ptr( viewProjectionMatrix ) );
        }
        if ( modelModelLoc >= 0 ) {
            glUniformMatrix4fv( modelModelLoc, 1, GL_FALSE, glm::value_ptr( modelMatrix ) );
        }
        if ( modelLightDirLoc >= 0 ) {
            Vec3 lightDir = Normalize( Vec3( -0.3f, -1.0f, -0.5f ) );
            glUniform3fv( modelLightDirLoc, 1, glm::value_ptr( lightDir ) );
        }
        if ( modelLightColorLoc >= 0 ) {
            glUniform3f( modelLightColorLoc, 1.0f, 1.0f, 1.0f );
        }
        if ( modelObjectColorLoc >= 0 ) {
            glUniform3f( modelObjectColorLoc, 0.8f, 0.8f, 0.8f );
        }

        model.Draw();

        glUseProgram( 0 );
    }

} // namespace atto
