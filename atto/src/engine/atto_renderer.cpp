#include "atto_renderer.h"
#include "atto_log.h"
#include "renderer/atto_render_model.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>

namespace atto {

    static const char * FLAT_COLOR_VERT = R"(
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

    static const char * FLAT_COLOR_FRAG = R"(
        #version 330 core
        in vec3 vColor;

        out vec4 FragColor;

        void main() {
            FragColor = vec4(vColor, 1.0);
        }
    )";

    static const char * MODEL_LIT_VERT = R"(
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

    static const char * MODEL_LIT_FRAG = R"(
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

    static const char * MODEL_UNLIT_VERT = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoords;

        uniform mat4 uViewProjection;
        uniform mat4 uModel;

        void main() {
            gl_Position = uViewProjection * uModel * vec4(aPos, 1.0);
        }
    )";

    static const char * MODEL_UNLIT_FRAG = R"(
        #version 330 core
        uniform vec3 uObjectColor;

        out vec4 FragColor;

        void main() {
            FragColor = vec4(uObjectColor, 1.0);
        }
    )";

    static const char * SKINNED_LIT_VERT = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoords;
        layout (location = 3) in ivec4 aBoneIDs;
        layout (location = 4) in vec4 aBoneWeights;

        const int MAX_BONES = 128;

        uniform mat4 uViewProjection;
        uniform mat4 uModel;
        uniform mat4 uBoneMatrices[MAX_BONES];

        out vec3 vNormal;
        out vec3 vFragPos;

        void main() {
            mat4 boneTransform = mat4(0.0);
            bool hasBones = false;
            for (int i = 0; i < 4; i++) {
                if (aBoneIDs[i] >= 0) {
                    boneTransform += uBoneMatrices[aBoneIDs[i]] * aBoneWeights[i];
                    hasBones = true;
                }
            }
            if (!hasBones) {
                boneTransform = mat4(1.0);
            }

            vec4 skinnedPos = boneTransform * vec4(aPos, 1.0);
            vec4 worldPos = uModel * skinnedPos;
            vFragPos = worldPos.xyz;

            vec3 skinnedNormal = mat3(boneTransform) * aNormal;
            vNormal = mat3(transpose(inverse(uModel))) * skinnedNormal;

            gl_Position = uViewProjection * worldPos;
        }
    )";

    bool Renderer::Initialize() {
        if ( !flatColorShader.CreateFromSource( FLAT_COLOR_VERT, FLAT_COLOR_FRAG ) ) {
            LOG_ERROR( "Failed to create flat color shader" );
            return false;
        }

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

        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( f32 ), (void *)0 );
        glEnableVertexAttribArray( 0 );

        glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( f32 ), (void *)(3 * sizeof( f32 )) );
        glEnableVertexAttribArray( 1 );

        glBindVertexArray( 0 );

        if ( !modelLitShader.CreateFromSource( MODEL_LIT_VERT, MODEL_LIT_FRAG ) ) {
            LOG_ERROR( "Failed to create lit model shader" );
            return false;
        }

        if ( !modelUnlitShader.CreateFromSource( MODEL_UNLIT_VERT, MODEL_UNLIT_FRAG ) ) {
            LOG_ERROR( "Failed to create unlit model shader" );
            return false;
        }

        if ( !skinnedLitShader.CreateFromSource( SKINNED_LIT_VERT, MODEL_LIT_FRAG ) ) {
            LOG_ERROR( "Failed to create skinned lit shader" );
            return false;
        }

        glGenVertexArrays( 1, &gridVAO );
        glGenBuffers( 1, &gridVBO );
        glBindVertexArray( gridVAO );
        glBindBuffer( GL_ARRAY_BUFFER, gridVBO );
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( f32 ), (void *)0 );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( f32 ), (void *)(3 * sizeof( f32 )) );
        glEnableVertexAttribArray( 1 );
        glBindVertexArray( 0 );

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

        if ( gridVAO != 0 ) {
            glDeleteVertexArrays( 1, &gridVAO );
            gridVAO = 0;
        }
        if ( gridVBO != 0 ) {
            glDeleteBuffers( 1, &gridVBO );
            gridVBO = 0;
        }

        flatColorShader.Destroy();
        modelLitShader.Destroy();
        modelUnlitShader.Destroy();
        skinnedLitShader.Destroy();

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

    void Renderer::SetWireframe( bool enabled ) {
        glPolygonMode( GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL );
    }

    void Renderer::RenderTestTriangle() {
        flatColorShader.Bind();
        flatColorShader.SetMat4( "uViewProjection", viewProjectionMatrix );

        glBindVertexArray( testTriangleVAO );
        glDrawArrays( GL_TRIANGLES, 0, 3 );
        glBindVertexArray( 0 );

        flatColorShader.Unbind();
    }

    void Renderer::RenderStaticModel( const StaticModel & model, const Mat4 & modelMatrix ) {
        RenderStaticModel( model, modelMatrix, Vec3( 0.8f ) );
    }

    void Renderer::RenderStaticModel( const StaticModel & model, const Mat4 & modelMatrix, const Vec3 & color ) {
        modelLitShader.Bind();

        modelLitShader.SetMat4( "uViewProjection", viewProjectionMatrix );
        modelLitShader.SetMat4( "uModel", modelMatrix );

        Vec3 lightDir = Normalize( Vec3( -0.3f, -1.0f, -0.5f ) );
        modelLitShader.SetVec3( "uLightDir", lightDir );
        modelLitShader.SetVec3( "uLightColor", Vec3( 1.0f ) );
        modelLitShader.SetVec3( "uObjectColor", color );

        model.Draw();

        modelLitShader.Unbind();
    }

    void Renderer::RenderStaticModelUnlit( const StaticModel & model, const Mat4 & modelMatrix ) {
        RenderStaticModelUnlit( model, modelMatrix, Vec3( 0.8f ) );
    }

    void Renderer::RenderStaticModelUnlit( const StaticModel & model, const Mat4 & modelMatrix, const Vec3 & color ) {
        modelUnlitShader.Bind();

        modelUnlitShader.SetMat4( "uViewProjection", viewProjectionMatrix );
        modelUnlitShader.SetMat4( "uModel", modelMatrix );
        modelUnlitShader.SetVec3( "uObjectColor", color );

        model.Draw();

        modelUnlitShader.Unbind();
    }

    void Renderer::RenderAnimatedModel( const AnimatedModel & model, const Animator & animator, const Mat4 & modelMatrix ) {
        RenderAnimatedModel( model, animator, modelMatrix, Vec3( 0.8f ) );
    }

    void Renderer::RenderAnimatedModel( const AnimatedModel & model, const Animator & animator, const Mat4 & modelMatrix, const Vec3 & color ) {
        skinnedLitShader.Bind();

        skinnedLitShader.SetMat4( "uViewProjection", viewProjectionMatrix );
        skinnedLitShader.SetMat4( "uModel", modelMatrix );

        skinnedLitShader.SetMat4Array( "uBoneMatrices[0]", animator.GetFinalBoneMatrices(), MAX_BONES );

        Vec3 lightDir = Normalize( Vec3( -0.3f, -1.0f, -0.5f ) );
        skinnedLitShader.SetVec3( "uLightDir", lightDir );
        skinnedLitShader.SetVec3( "uLightColor", Vec3( 1.0f ) );
        skinnedLitShader.SetVec3( "uObjectColor", color );

        model.Draw();

        skinnedLitShader.Unbind();
    }

    static Vec3 AxisColor( Vec3 axis ) {
        if ( Abs( axis.x ) > 0.5f ) return Vec3( 0.7f, 0.15f, 0.15f );
        if ( Abs( axis.y ) > 0.5f ) return Vec3( 0.15f, 0.7f, 0.15f );
        return Vec3( 0.15f, 0.15f, 0.7f );
    }

    void Renderer::RenderGrid( Vec3 axisH, Vec3 axisV, Vec3 center, f32 spacing, f32 halfExtentH, f32 halfExtentV ) {
        struct GridVert { Vec3 pos; Vec3 color; };

        const Vec3 minorColor( 0.22f );
        const Vec3 majorColor( 0.38f );
        const Vec3 axisHCol = AxisColor( axisH );
        const Vec3 axisVCol = AxisColor( axisV );

        f32 cH = Dot( center, axisH );
        f32 cV = Dot( center, axisV );

        f32 startH = cH - halfExtentH;
        f32 endH = cH + halfExtentH;
        f32 startV = cV - halfExtentV;
        f32 endV = cV + halfExtentV;

        i32 firstH = (i32)floorf( startH / spacing ) - 1;
        i32 lastH = (i32)ceilf( endH / spacing ) + 1;
        i32 firstV = (i32)floorf( startV / spacing ) - 1;
        i32 lastV = (i32)ceilf( endV / spacing ) + 1;

        i32 countH = lastH - firstH + 1;
        i32 countV = lastV - firstV + 1;
        if ( countH > 500 ) { firstH = -(250); lastH = 250; }
        if ( countV > 500 ) { firstV = -(250); lastV = 250; }

        f32 lineStartH = (firstH - 1) * spacing;
        f32 lineEndH = (lastH + 1) * spacing;
        f32 lineStartV = (firstV - 1) * spacing;
        f32 lineEndV = (lastV + 1) * spacing;

        std::vector<GridVert> verts;
        verts.reserve( ((lastV - firstV + 1) + (lastH - firstH + 1)) * 2 );

        for ( i32 i = firstV; i <= lastV; i++ ) {
            f32 v = i * spacing;
            Vec3 col = (i == 0) ? axisHCol : ((i % 10 == 0) ? majorColor : minorColor);
            verts.push_back( { axisH * lineStartH + axisV * v, col } );
            verts.push_back( { axisH * lineEndH + axisV * v, col } );
        }

        for ( i32 i = firstH; i <= lastH; i++ ) {
            f32 h = i * spacing;
            Vec3 col = (i == 0) ? axisVCol : ((i % 10 == 0) ? majorColor : minorColor);
            verts.push_back( { axisH * h + axisV * lineStartV, col } );
            verts.push_back( { axisH * h + axisV * lineEndV,   col } );
        }

        if ( verts.empty() ) return;

        glDisable( GL_DEPTH_TEST );

        glBindVertexArray( gridVAO );
        glBindBuffer( GL_ARRAY_BUFFER, gridVBO );
        glBufferData( GL_ARRAY_BUFFER, verts.size() * sizeof( GridVert ), verts.data(), GL_DYNAMIC_DRAW );

        flatColorShader.Bind();
        flatColorShader.SetMat4( "uViewProjection", viewProjectionMatrix );

        glDrawArrays( GL_LINES, 0, (i32)verts.size() );

        glBindVertexArray( 0 );
        flatColorShader.Unbind();

        glEnable( GL_DEPTH_TEST );
    }

} // namespace atto
