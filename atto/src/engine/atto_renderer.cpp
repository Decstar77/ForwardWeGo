#include "atto_renderer.h"
#include "atto_log.h"
#include "renderer/atto_render_model.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image/std_image.h>

#include <algorithm>
#include <vector>
#include <cmath>

#define STB_TRUETYPE_IMPLEMENTATION
#include "atto_engine.h"
#include "stb_truetype/stb_truetype.h"

namespace atto {


    static const f32 SKYBOX_CUBE_VERTICES[] = {
        -1.0f,  1.0f, -1.0f,   -1.0f, -1.0f, -1.0f,    1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,    1.0f,  1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,   -1.0f, -1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,    1.0f, -1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,    1.0f,  1.0f, -1.0f,    1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,    1.0f, -1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,    1.0f,  1.0f, -1.0f,    1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f,  1.0f,
    };


    bool Renderer::Initialize() {
        if ( !flatColorShader.CreateFromFiles( "assets/shaders/flat_color.vert", "assets/shaders/flat_color.frag" ) ) {
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

        if ( !modelLitShader.CreateFromFiles( "assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag" ) ) {
            LOG_ERROR( "Failed to create lit model shader" );
            return false;
        }

        if ( !modelUnlitShader.CreateFromFiles( "assets/shaders/model_unlit.vert", "assets/shaders/model_unlit.frag" ) ) {
            LOG_ERROR( "Failed to create unlit model shader" );
            return false;
        }

        if ( !skinnedLitShader.CreateFromFiles( "assets/shaders/skinned_unlit.vert", "assets/shaders/model_unlit.frag" ) ) {
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

        glGenVertexArrays( 1, &debugLineVAO );
        glGenBuffers( 1, &debugLineVBO );
        glBindVertexArray( debugLineVAO );
        glBindBuffer( GL_ARRAY_BUFFER, debugLineVBO );
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( f32 ), (void *)0 );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( f32 ), (void *)(3 * sizeof( f32 )) );
        glEnableVertexAttribArray( 1 );
        glBindVertexArray( 0 );

        if ( !spriteShader.CreateFromFiles( "assets/shaders/sprite.vert", "assets/shaders/sprite.frag" ) ) {
            LOG_ERROR( "Failed to create sprite shader" );
            return false;
        }

        {
            const char * rectVert = R"(
                #version 330 core
                layout (location = 0) in vec2 aPos;
                uniform vec2 uCenter;
                uniform vec2 uHalfSize;
                void main() {
                    gl_Position = vec4(uCenter + aPos * uHalfSize, 0.0, 1.0);
                }
            )";
            const char * rectFrag = R"(
                #version 330 core
                uniform vec4 uColor;
                out vec4 fragColor;
                void main() {
                    fragColor = uColor;
                }
            )";
            if ( !rectShader.CreateFromSource( rectVert, rectFrag ) ) {
                LOG_ERROR( "Failed to create rect shader" );
                return false;
            }
        }

        if ( !damageVignetteShader.CreateFromFiles( "assets/shaders/damage_vignette.vert", "assets/shaders/damage_vignette.frag" ) ) {
            LOG_ERROR( "Failed to create damage vignette shader" );
            return false;
        }

        if ( !textShader.CreateFromFiles( "assets/shaders/text.vert", "assets/shaders/text.frag" ) ) {
            LOG_ERROR( "Failed to create text shader" );
            return false;
        }

        glGenVertexArrays( 1, &textVAO );
        glGenBuffers( 1, &textVBO );
        glBindVertexArray( textVAO );
        glBindBuffer( GL_ARRAY_BUFFER, textVBO );
        glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof( f32 ), (void *)0 );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof( f32 ), (void *)(2 * sizeof( f32 )) );
        glEnableVertexAttribArray( 1 );
        glBindVertexArray( 0 );

        // Unit quad: two triangles, each vertex is (x, y, u, v)
        static const f32 SPRITE_QUAD[] = {
            -1.0f, -1.0f,   0.0f, 0.0f,
             1.0f, -1.0f,   1.0f, 0.0f,
             1.0f,  1.0f,   1.0f, 1.0f,
            -1.0f, -1.0f,   0.0f, 0.0f,
             1.0f,  1.0f,   1.0f, 1.0f,
            -1.0f,  1.0f,   0.0f, 1.0f,
        };

        glGenVertexArrays( 1, &spriteVAO );
        glGenBuffers( 1, &spriteVBO );
        glBindVertexArray( spriteVAO );
        glBindBuffer( GL_ARRAY_BUFFER, spriteVBO );
        glBufferData( GL_ARRAY_BUFFER, sizeof( SPRITE_QUAD ), SPRITE_QUAD, GL_STATIC_DRAW );
        glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof( f32 ), (void *)0 );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof( f32 ), (void *)(2 * sizeof( f32 )) );
        glEnableVertexAttribArray( 1 );
        glBindVertexArray( 0 );

        if ( !skyboxShader.CreateFromFiles( "assets/shaders/skybox.vert", "assets/shaders/skybox.frag" ) ) {
            LOG_ERROR( "Failed to create skybox shader" );
            return false;
        }

        glGenVertexArrays( 1, &skyboxVAO );
        glGenBuffers( 1, &skyboxVBO );
        glBindVertexArray( skyboxVAO );
        glBindBuffer( GL_ARRAY_BUFFER, skyboxVBO );
        glBufferData( GL_ARRAY_BUFFER, sizeof( SKYBOX_CUBE_VERTICES ), SKYBOX_CUBE_VERTICES, GL_STATIC_DRAW );
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( f32 ), (void *)0 );
        glEnableVertexAttribArray( 0 );
        glBindVertexArray( 0 );

        if ( !particleShader.CreateFromFiles( "assets/shaders/particle.vert", "assets/shaders/particle.frag" ) ) {
            LOG_ERROR( "Failed to create particle shader" );
            return false;
        }

        glGenVertexArrays( 1, &particleVAO );
        glGenBuffers( 1, &particleVBO );
        glBindVertexArray( particleVAO );
        glBindBuffer( GL_ARRAY_BUFFER, particleVBO );
        // pos (vec3)
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( ParticleVert ), (void *)0 );
        glEnableVertexAttribArray( 0 );
        // uv (vec2)
        glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof( ParticleVert ), (void *)offsetof( ParticleVert, uv ) );
        glEnableVertexAttribArray( 1 );
        // color (vec4)
        glVertexAttribPointer( 2, 4, GL_FLOAT, GL_FALSE, sizeof( ParticleVert ), (void *)offsetof( ParticleVert, color ) );
        glEnableVertexAttribArray( 2 );
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

        if ( debugLineVAO != 0 ) {
            glDeleteVertexArrays( 1, &debugLineVAO );
            debugLineVAO = 0;
        }
        if ( debugLineVBO != 0 ) {
            glDeleteBuffers( 1, &debugLineVBO );
            debugLineVBO = 0;
        }

        if ( spriteVAO != 0 ) {
            glDeleteVertexArrays( 1, &spriteVAO );
            spriteVAO = 0;
        }
        if ( spriteVBO != 0 ) {
            glDeleteBuffers( 1, &spriteVBO );
            spriteVBO = 0;
        }

        if ( skyboxVAO != 0 ) {
            glDeleteVertexArrays( 1, &skyboxVAO );
            skyboxVAO = 0;
        }
        if ( skyboxVBO != 0 ) {
            glDeleteBuffers( 1, &skyboxVBO );
            skyboxVBO = 0;
        }
        if ( skyboxTexture != 0 ) {
            glDeleteTextures( 1, &skyboxTexture );
            skyboxTexture = 0;
        }

        if ( textVAO != 0 ) { glDeleteVertexArrays( 1, &textVAO ); textVAO = 0; }
        if ( textVBO != 0 ) { glDeleteBuffers( 1, &textVBO );      textVBO = 0; }

        if ( particleVAO != 0 ) { glDeleteVertexArrays( 1, &particleVAO ); particleVAO = 0; }
        if ( particleVBO != 0 ) { glDeleteBuffers( 1, &particleVBO );      particleVBO = 0; }

        flatColorShader.Destroy();
        modelLitShader.Destroy();
        modelUnlitShader.Destroy();
        skinnedLitShader.Destroy();
        spriteShader.Destroy();
        rectShader.Destroy();
        textShader.Destroy();
        skyboxShader.Destroy();
        particleShader.Destroy();

        LOG_INFO( "Renderer shutdown" );
    }

    void Renderer::BeginFrame() {
        glClearColor( clearColor.r, clearColor.g, clearColor.b, clearColor.a );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    }

    void Renderer::EndFrame() {
        FlushDebugLines();
    }

    void Renderer::SetClearColor( const Color & color ) {
        clearColor = color;
    }

    void Renderer::ClearDepthBuffer() {
        glClear( GL_DEPTH_BUFFER_BIT );
    }

    void Renderer::SetViewport( i32 x, i32 y, i32 width, i32 height ) {
        glViewport( x, y, width, height );
    }

    void Renderer::SetViewProjectionMatrix( const Mat4 & vp ) {
        viewProjectionMatrix = vp;
    }

    void Renderer::SetWireframe( bool enabled ) {
        wireframe = enabled;
        glPolygonMode( GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL );
    }

    void Renderer::UseUnlitShader() {
        staticModelShader = &modelUnlitShader;
    }

    void Renderer::UseLitShader() {
        staticModelShader = &modelLitShader;
    }

    void Renderer::RenderStaticModel( const StaticModel * model, const Mat4 & modelMatrix ) {
        RenderStaticModel( model, modelMatrix, Vec3( 1.0f ) );
    }

    void Renderer::RenderStaticModel( const StaticModel * model, const Mat4 & modelMatrix, const Vec3 & color ) {
        if ( staticModelShader == nullptr ) {
            staticModelShader = &modelLitShader;
        }

        staticModelShader->Bind();

        staticModelShader->SetMat4( "uViewProjection", viewProjectionMatrix );
        staticModelShader->SetMat4( "uModel", modelMatrix );

        Vec3 lightDir = Normalize( Vec3( -0.3f, -1.0f, -0.5f ) );
        staticModelShader->SetVec3( "uLightDir", lightDir );
        staticModelShader->SetVec3( "uLightColor", Vec3( 1.0f ) );
        staticModelShader->SetVec3( "uObjectColor", color );

        if ( wireframe ) {
            staticModelShader->SetInt( "uAllowTextures", 0 );
        } else {
            staticModelShader->SetInt( "uAllowTextures", 1 );
        }

        model->Draw( staticModelShader );

        staticModelShader->Unbind();
    }

    // void Renderer::RenderStaticModelUnlit( const StaticModel & model, const Mat4 & modelMatrix, const Vec3 & color ) {
    //     modelUnlitShader.Bind();

    //     modelUnlitShader.SetMat4( "uViewProjection", viewProjectionMatrix );
    //     modelUnlitShader.SetMat4( "uModel", modelMatrix );
    //     modelUnlitShader.SetVec3( "uObjectColor", color );

    //     model.Draw();

    //     modelUnlitShader.Unbind();
    // }

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

        model.Draw( &skinnedLitShader );

        skinnedLitShader.Unbind();
    }

    void Renderer::LoadSkybox( const char * filePath ) {
        if ( skyboxTexture != 0 ) {
            glDeleteTextures( 1, &skyboxTexture );
            skyboxTexture = 0;
        }

        stbi_set_flip_vertically_on_load( true );

        glGenTextures( 1, &skyboxTexture );
        glBindTexture( GL_TEXTURE_2D, skyboxTexture );

        if ( stbi_is_hdr( filePath ) ) {
            int w, h, channels;
            f32 * data = stbi_loadf( filePath, &w, &h, &channels, 3 );
            if ( !data ) {
                LOG_ERROR( "Failed to load HDR skybox '%s'", filePath );
                glDeleteTextures( 1, &skyboxTexture );
                skyboxTexture = 0;
                return;
            }

            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, data );
            stbi_image_free( data );
        }
        else {
            int w, h, channels;
            stbi_uc * data = stbi_load( filePath, &w, &h, &channels, STBI_rgb_alpha );
            if ( !data ) {
                LOG_ERROR( "Failed to load skybox '%s'", filePath );
                glDeleteTextures( 1, &skyboxTexture );
                skyboxTexture = 0;
                return;
            }

            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
            stbi_image_free( data );
        }

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

        glBindTexture( GL_TEXTURE_2D, 0 );

        LOG_INFO( "Skybox loaded: %s", filePath );
    }

    void Renderer::RenderSkybox( const Mat4 & view, const Mat4 & projection ) {
        if ( skyboxTexture == 0 ) {
            return;
        }

        // Strip translation from the view matrix so the skybox stays centered on the camera
        Mat4 skyboxView = Mat4( Mat3( view ) );

        glDepthFunc( GL_LEQUAL );
        glDepthMask( GL_FALSE );

        skyboxShader.Bind();
        skyboxShader.SetMat4( "uView", skyboxView );
        skyboxShader.SetMat4( "uProjection", projection );
        skyboxShader.SetInt( "uEquirectMap", 0 );

        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, skyboxTexture );

        glBindVertexArray( skyboxVAO );
        glDrawArrays( GL_TRIANGLES, 0, 36 );
        glBindVertexArray( 0 );

        skyboxShader.Unbind();

        glDepthMask( GL_TRUE );
        glDepthFunc( GL_LESS );
    }

    void Renderer::RenderSprite( const Texture * texture, Vec2 centerNDC, i32 pixelWidth, i32 pixelHeight, i32 viewportW, i32 viewportH ) {
        if ( !texture || !texture->IsValid() ) {
            return;
        }

        Vec2 halfSize = Vec2(
            (f32)pixelWidth / (f32)viewportW,
            (f32)pixelHeight / (f32)viewportH
        );

        glDisable( GL_DEPTH_TEST );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        spriteShader.Bind();
        spriteShader.SetVec2( "uCenter", centerNDC );
        spriteShader.SetVec2( "uHalfSize", halfSize );
        spriteShader.SetInt( "uTexture", 0 );

        texture->Bind( 0 );

        glBindVertexArray( spriteVAO );
        glDrawArrays( GL_TRIANGLES, 0, 6 );
        glBindVertexArray( 0 );

        spriteShader.Unbind();

        glDisable( GL_BLEND );
        glEnable( GL_DEPTH_TEST );
    }

    void Renderer::RenderRect( Vec2 centerNDC, i32 pixelWidth, i32 pixelHeight, i32 viewportW, i32 viewportH, const Vec4 & color ) {
        Vec2 halfSize = Vec2(
            (f32)pixelWidth / (f32)viewportW,
            (f32)pixelHeight / (f32)viewportH
        );

        glDisable( GL_DEPTH_TEST );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        rectShader.Bind();
        rectShader.SetVec2( "uCenter", centerNDC );
        rectShader.SetVec2( "uHalfSize", halfSize );
        rectShader.SetVec4( "uColor", color );

        glBindVertexArray( spriteVAO );
        glDrawArrays( GL_TRIANGLES, 0, 6 );
        glBindVertexArray( 0 );

        rectShader.Unbind();

        glDisable( GL_BLEND );
        glEnable( GL_DEPTH_TEST );
    }

    void Renderer::RenderDamageVignette( f32 alpha ) {
        if ( alpha <= 0.0f ) {
            return;
        }

        glDisable( GL_DEPTH_TEST );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        damageVignetteShader.Bind();
        damageVignetteShader.SetFloat( "uAlpha", alpha );

        glBindVertexArray( spriteVAO );
        glDrawArrays( GL_TRIANGLES, 0, 6 );
        glBindVertexArray( 0 );

        damageVignetteShader.Unbind();

        glDisable( GL_BLEND );
        glEnable( GL_DEPTH_TEST );
    }

    const Texture * Renderer::GetOrLoadTexture( const char * filePath, bool flip ) {
        TextureCreateInfo defaultInfo = {};
        defaultInfo.flip = flip;
        return GetOrLoadTexture( filePath, defaultInfo );
    }

    const Texture * Renderer::GetOrLoadTexture( const char * filePath, TextureCreateInfo createInfo ) {
        const int count = textures.GetCount();
        for ( int i = 0; i < count; i++ ) {
            if ( textures[i].GetPath() == filePath ) {
                return &textures[i];
            }
        }

        BinarySerializer serializer( false );
        Engine::Get().GetAssetManager().LoadTextureData( filePath, serializer );
        serializer.Reset( true ); // Set to loading
        Texture & texture = textures.AddEmpty();
        texture.Serialize( serializer, createInfo );
        return &texture;
    }

    const StaticModel * Renderer::GetOrLoadStaticModel( const char * filePath, f32 loadScale ) {
        const int count = staticModels.GetCount();
        for ( int i = 0; i < count; i++ ) {
            if ( staticModels[i].GetPath() == filePath ) {
                return &staticModels[i];
            }
        }

        StaticModel & model = staticModels.ConstructEmpty();
        model.LoadFromFile( filePath, loadScale );
        return &model;
    }

    const AnimatedModel * Renderer::GetOrLoadAnimatedModel( const char * filePath, f32 loadScale ) {
        const int count = animatedModels.GetCount();
        for ( int i = 0 ; i < count; i++ ) {
            if ( animatedModels[i].GetPath()  == filePath ) {
                return &animatedModels[i];
            }
        }

        AnimatedModel & model = animatedModels.ConstructEmpty();
        model.LoadFromFile( filePath, loadScale );
        return &model;
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

    const Font * Renderer::GetOrLoadFont( const char * path, f32 fontSize ) {
        const i32 count = fonts.GetCount();
        for ( i32 i = 0; i < count; i++ ) {
            if ( fonts[i].GetPath() == path && fonts[i].GetFontSize() == fontSize ) {
                return &fonts[i];
            }
        }

        Font & font = fonts.AddEmpty();
        if ( !font.LoadFromFile( path, fontSize ) ) {
            LOG_ERROR( "Failed to load font: %s at size %.1f", path, fontSize );
        }
        return &font;
    }

    void Renderer::DrawText( const Font * font, const char * text, f32 x, f32 y, Vec4 color, i32 viewportW, i32 viewportH ) {
        if ( !font || !font->IsValid() || !text || *text == '\0' ) {
            return;
        }

        struct TextVert { f32 x, y, u, v; };
        static std::vector<TextVert> verts;
        verts.clear();

        const stbtt_bakedchar * charData = font->GetCharData();
        f32 xpos = x;
        f32 ypos = y;

        while ( *text ) {
            const char c = *text++;
            if ( c == '\n' ) {
                xpos  = x;
                ypos += font->GetFontSize();
                continue;
            }
            if ( c < FONT_FIRST_CHAR || c >= FONT_FIRST_CHAR + FONT_CHAR_COUNT ) {
                continue;
            }

            stbtt_aligned_quad q;
            stbtt_GetBakedQuad( charData, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT,
                                c - FONT_FIRST_CHAR, &xpos, &ypos, &q, 0 );

            verts.push_back( { q.x0, q.y0, q.s0, q.t0 } );
            verts.push_back( { q.x1, q.y0, q.s1, q.t0 } );
            verts.push_back( { q.x1, q.y1, q.s1, q.t1 } );
            verts.push_back( { q.x0, q.y0, q.s0, q.t0 } );
            verts.push_back( { q.x1, q.y1, q.s1, q.t1 } );
            verts.push_back( { q.x0, q.y1, q.s0, q.t1 } );
        }

        if ( verts.empty() ) {
            return;
        }

        // Orthographic projection: (0,0) top-left, (w,h) bottom-right
        Mat4 projection = glm::ortho( 0.0f, (f32)viewportW, (f32)viewportH, 0.0f );

        glDisable( GL_DEPTH_TEST );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        textShader.Bind();
        textShader.SetMat4( "uProjection", projection );
        textShader.SetVec4( "uColor",      color );
        textShader.SetInt(  "uFontAtlas",  0 );

        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, font->GetAtlasHandle() );

        glBindVertexArray( textVAO );
        glBindBuffer( GL_ARRAY_BUFFER, textVBO );
        glBufferData( GL_ARRAY_BUFFER, (GLsizeiptr)( verts.size() * sizeof( TextVert ) ), verts.data(), GL_DYNAMIC_DRAW );
        glDrawArrays( GL_TRIANGLES, 0, (i32)verts.size() );
        glBindVertexArray( 0 );

        textShader.Unbind();

        glDisable( GL_BLEND );
        glEnable( GL_DEPTH_TEST );
    }

    bool Font::LoadFromFile( const char * filePath, f32 inFontSize ) {
        path     = filePath;
        fontSize = inFontSize;

        // Read the TTF file into memory
        FILE * f = fopen( filePath, "rb" );
        if ( !f ) {
            LOG_ERROR( "Font::LoadFromFile — could not open %s", filePath );
            return false;
        }

        fseek( f, 0, SEEK_END );
        const long fileSize = ftell( f );
        fseek( f, 0, SEEK_SET );

        std::vector<u8> ttfBuffer( fileSize );
        fread( ttfBuffer.data(), 1, fileSize, f );
        fclose( f );

        // Bake the font atlas into a single-channel bitmap
        std::vector<u8> atlasBitmap( FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT );

        const i32 result = stbtt_BakeFontBitmap(
            ttfBuffer.data(), 0,
            inFontSize,
            atlasBitmap.data(), FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT,
            FONT_FIRST_CHAR, FONT_CHAR_COUNT,
            charData
        );

        if ( result <= 0 ) {
            LOG_ERROR( "Font::LoadFromFile — stbtt_BakeFontBitmap failed for %s (returned %d). Atlas may be too small.", filePath, result );
            // result == 0 means none fit; negative means some fit but atlas was full
            // Continue anyway — partially baked font is still usable
        }

        // Upload bitmap to OpenGL as a red-channel texture
        glGenTextures( 1, &atlasHandle );
        glBindTexture( GL_TEXTURE_2D, atlasHandle );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RED,
                      FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT,
                      0, GL_RED, GL_UNSIGNED_BYTE, atlasBitmap.data() );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glBindTexture( GL_TEXTURE_2D, 0 );

        return true;
    }

    void Renderer::DebugLine( const Vec3 & a, const Vec3 & b, const Vec3 & color ) {
        debugLineVerts.push_back( { a, color } );
        debugLineVerts.push_back( { b, color } );
    }

    void Renderer::DebugSphere( const Sphere & sphere, const Vec3 & color ) {
        constexpr i32 SEGMENTS = 24;
        constexpr f32 STEP = 2.0f * PI * (1.0f / (f32)SEGMENTS);

        for ( i32 i = 0; i < SEGMENTS; i++ ) {
            f32 a0 = i * STEP;
            f32 a1 = (i + 1) * STEP;
            f32 c0 = cosf( a0 ), s0 = sinf( a0 );
            f32 c1 = cosf( a1 ), s1 = sinf( a1 );

            Vec3 center = sphere.center;
            f32 r = sphere.radius;

            DebugLine( center + Vec3( c0 * r, s0 * r, 0.0f ), center + Vec3( c1 * r, s1 * r, 0.0f ), color );
            DebugLine( center + Vec3( c0 * r, 0.0f, s0 * r ), center + Vec3( c1 * r, 0.0f, s1 * r ), color );
            DebugLine( center + Vec3( 0.0f, c0 * r, s0 * r ), center + Vec3( 0.0f, c1 * r, s1 * r ), color );
        }
    }

    void Renderer::DebugAlignedBox( const AlignedBox & box, const Vec3 & color ) {
        Vec3 mn = box.min;
        Vec3 mx = box.max;

        Vec3 corners[8] = {
            { mn.x, mn.y, mn.z },
            { mx.x, mn.y, mn.z },
            { mx.x, mx.y, mn.z },
            { mn.x, mx.y, mn.z },
            { mn.x, mn.y, mx.z },
            { mx.x, mn.y, mx.z },
            { mx.x, mx.y, mx.z },
            { mn.x, mx.y, mx.z },
        };

        // Bottom face
        DebugLine( corners[0], corners[1], color );
        DebugLine( corners[1], corners[2], color );
        DebugLine( corners[2], corners[3], color );
        DebugLine( corners[3], corners[0], color );

        // Top face
        DebugLine( corners[4], corners[5], color );
        DebugLine( corners[5], corners[6], color );
        DebugLine( corners[6], corners[7], color );
        DebugLine( corners[7], corners[4], color );

        // Vertical edges
        DebugLine( corners[0], corners[4], color );
        DebugLine( corners[1], corners[5], color );
        DebugLine( corners[2], corners[6], color );
        DebugLine( corners[3], corners[7], color );
    }

    void Renderer::DebugCapsule( const Capsule & cap, const Vec3 & color ) {
        constexpr i32 SEGMENTS = 24;
        constexpr f32 STEP = TWO_PI / (f32)SEGMENTS;
        constexpr i32 HALF_SEGMENTS = SEGMENTS / 2;
        constexpr f32 HALF_STEP = PI / (f32)HALF_SEGMENTS;

        Vec3 bottom = cap.base;
        Vec3 top = cap.base + Vec3( 0.0f, cap.height, 0.0f );
        f32 r = cap.radius;

        for ( i32 i = 0; i < SEGMENTS; i++ ) {
            f32 a0 = i * STEP;
            f32 a1 = (i + 1) * STEP;
            f32 c0 = cosf( a0 ), s0 = sinf( a0 );
            f32 c1 = cosf( a1 ), s1 = sinf( a1 );

            // Bottom ring (XZ plane)
            DebugLine( bottom + Vec3( c0 * r, 0.0f, s0 * r ), bottom + Vec3( c1 * r, 0.0f, s1 * r ), color );
            // Top ring (XZ plane)
            DebugLine( top + Vec3( c0 * r, 0.0f, s0 * r ), top + Vec3( c1 * r, 0.0f, s1 * r ), color );
        }

        // Vertical lines connecting the two rings
        DebugLine( bottom + Vec3( r, 0.0f, 0.0f ), top + Vec3( r, 0.0f, 0.0f ), color );
        DebugLine( bottom + Vec3( -r, 0.0f, 0.0f ), top + Vec3( -r, 0.0f, 0.0f ), color );
        DebugLine( bottom + Vec3( 0.0f, 0.0f, r ), top + Vec3( 0.0f, 0.0f, r ), color );
        DebugLine( bottom + Vec3( 0.0f, 0.0f, -r ), top + Vec3( 0.0f, 0.0f, -r ), color );

        // Hemisphere arcs (semicircles)
        for ( i32 i = 0; i < HALF_SEGMENTS; i++ ) {
            f32 a0 = i * HALF_STEP;
            f32 a1 = (i + 1) * HALF_STEP;
            f32 c0 = cosf( a0 ), s0 = sinf( a0 );
            f32 c1 = cosf( a1 ), s1 = sinf( a1 );

            // Top hemisphere arcs (XY and ZY planes, curving upward)
            DebugLine( top + Vec3( c0 * r, s0 * r, 0.0f ), top + Vec3( c1 * r, s1 * r, 0.0f ), color );
            DebugLine( top + Vec3( 0.0f, s0 * r, c0 * r ), top + Vec3( 0.0f, s1 * r, c1 * r ), color );

            // Bottom hemisphere arcs (XY and ZY planes, curving downward)
            DebugLine( bottom + Vec3( c0 * r, -s0 * r, 0.0f ), bottom + Vec3( c1 * r, -s1 * r, 0.0f ), color );
            DebugLine( bottom + Vec3( 0.0f, -s0 * r, c0 * r ), bottom + Vec3( 0.0f, -s1 * r, c1 * r ), color );
        }
    }

    void Renderer::DebugBox( const Box & box, const Vec3 & color ) {
        Vec3 corners[8];
        box.GetCorners( corners );

        // Bottom face (corners 0,1,3,2 share -Y local)
        DebugLine( corners[0], corners[1], color );
        DebugLine( corners[1], corners[3], color );
        DebugLine( corners[3], corners[2], color );
        DebugLine( corners[2], corners[0], color );

        // Top face (corners 4,5,7,6 share +Y local)
        DebugLine( corners[4], corners[5], color );
        DebugLine( corners[5], corners[7], color );
        DebugLine( corners[7], corners[6], color );
        DebugLine( corners[6], corners[4], color );

        // Vertical edges
        DebugLine( corners[0], corners[4], color );
        DebugLine( corners[1], corners[5], color );
        DebugLine( corners[2], corners[6], color );
        DebugLine( corners[3], corners[7], color );
    }

    void Renderer::RenderWorldBar( const Vec3 & worldPos, f32 width, f32 height,
                                    f32 fillFraction, const Vec4 & fillColor,
                                    const Vec3 & cameraPos, const Vec3 & cameraUp,
                                    const Vec4 & bgColor ) {
        QueuedWorldBar entry = {};
        entry.worldPos = worldPos;
        entry.width = width;
        entry.height = height;
        entry.fillFraction = Clamp( fillFraction, 0.0f, 1.0f );
        entry.fillColor = fillColor;
        entry.cameraPos = cameraPos;
        entry.cameraUp = cameraUp;
        entry.bgColor = bgColor;

        TransparentEntry te = {};
        te.type = TransparentType::WorldBar;
        te.index = static_cast<i32>( queuedWorldBars.size() );
        te.distSq = LengthSquared( cameraPos - worldPos );

        queuedWorldBars.push_back( entry );
        transparentEntries.push_back( te );
    }

    void Renderer::RenderBillboard( const Texture * texture, const Vec3 & worldPos, f32 size,
                                     const Vec3 & cameraPos, const Vec3 & cameraUp,
                                     f32 rotationRad, const Vec4 & color ) {
        QueuedBillboard entry = {};
        entry.texture = texture;
        entry.worldPos = worldPos;
        entry.size = size;
        entry.cameraPos = cameraPos;
        entry.cameraUp = cameraUp;
        entry.rotationRad = rotationRad;
        entry.color = color;

        TransparentEntry te = {};
        te.type = TransparentType::Billboard;
        te.index = static_cast<i32>( queuedBillboards.size() );
        te.distSq = LengthSquared( cameraPos - worldPos );

        queuedBillboards.push_back( entry );
        transparentEntries.push_back( te );
    }

    void Renderer::FlushParticleBatch() {
        if ( particleBatchVerts.empty() ) {
            return;
        }

        glDepthMask( GL_FALSE );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        particleShader.Bind();
        particleShader.SetMat4( "uViewProjection", viewProjectionMatrix );

        if ( particleBatchTexture != nullptr && particleBatchTexture->IsValid() ) {
            particleShader.SetInt( "uHasTexture", 1 );
            particleShader.SetInt( "uTexture", 0 );
            particleBatchTexture->Bind( 0 );
        }
        else {
            particleShader.SetInt( "uHasTexture", 0 );
        }

        glBindVertexArray( particleVAO );
        glBindBuffer( GL_ARRAY_BUFFER, particleVBO );
        glBufferData( GL_ARRAY_BUFFER,
                      (GLsizeiptr)( particleBatchVerts.size() * sizeof( ParticleVert ) ),
                      particleBatchVerts.data(), GL_DYNAMIC_DRAW );
        glDrawArrays( GL_TRIANGLES, 0, (GLsizei)particleBatchVerts.size() );

        glBindVertexArray( 0 );
        particleShader.Unbind();

        glDepthMask( GL_TRUE );
        glDisable( GL_BLEND );

        particleBatchVerts.clear();
        particleBatchTexture = nullptr;
    }

    void Renderer::FlushTransparents( const Vec3 & cameraPos ) {
        if ( transparentEntries.empty() ) {
            return;
        }

        // Sort back-to-front (farthest first)
        std::sort( transparentEntries.begin(), transparentEntries.end(),
            []( const TransparentEntry & a, const TransparentEntry & b ) {
                return a.distSq > b.distSq;
            } );

        particleBatchVerts.clear();
        particleBatchTexture = nullptr;

        for ( const TransparentEntry & te : transparentEntries ) {
            switch ( te.type ) {
                case TransparentType::Particle: {
                    const QueuedParticle & qp = queuedParticles[te.index];

                    // If texture changed, flush the current batch
                    if ( !particleBatchVerts.empty() && qp.texture != particleBatchTexture ) {
                        FlushParticleBatch();
                    }

                    particleBatchTexture = qp.texture;
                    for ( i32 v = 0; v < 6; v++ ) {
                        particleBatchVerts.push_back( qp.verts[v] );
                    }
                } break;

                case TransparentType::Billboard: {
                    FlushParticleBatch();
                    const QueuedBillboard & bb = queuedBillboards[te.index];
                    DrawBillboardImmediate( bb.texture, bb.worldPos, bb.size,
                                            bb.cameraPos, bb.cameraUp,
                                            bb.rotationRad, bb.color );
                } break;

                case TransparentType::WorldBar: {
                    FlushParticleBatch();
                    const QueuedWorldBar & wb = queuedWorldBars[te.index];
                    DrawWorldBarImmediate( wb.worldPos, wb.width, wb.height,
                                           wb.fillFraction, wb.fillColor,
                                           wb.cameraPos, wb.cameraUp, wb.bgColor );
                } break;
            }
        }

        // Flush any remaining particle batch
        FlushParticleBatch();

        queuedBillboards.clear();
        queuedWorldBars.clear();
        queuedParticles.clear();
        transparentEntries.clear();
    }

    void Renderer::DrawWorldBarImmediate( const Vec3 & worldPos, f32 width, f32 height,
                                           f32 fillFraction, const Vec4 & fillColor,
                                           const Vec3 & cameraPos, const Vec3 & cameraUp,
                                           const Vec4 & bgColor ) {
        Vec3 toCamera = Normalize( cameraPos - worldPos );
        Vec3 right = Normalize( Cross( cameraUp, toCamera ) );
        Vec3 up = Normalize( Cross( toCamera, right ) );

        f32 halfW = width * 0.5f;
        f32 halfH = height * 0.5f;

        Vec3 bgBL = worldPos - right * halfW - up * halfH;
        Vec3 bgBR = worldPos + right * halfW - up * halfH;
        Vec3 bgTR = worldPos + right * halfW + up * halfH;
        Vec3 bgTL = worldPos - right * halfW + up * halfH;

        Vec3 fillLeft = worldPos - right * halfW;
        f32 fillW = width * fillFraction;
        Vec3 fBL = fillLeft - up * halfH;
        Vec3 fBR = fillLeft + right * fillW - up * halfH;
        Vec3 fTR = fillLeft + right * fillW + up * halfH;
        Vec3 fTL = fillLeft + up * halfH;

        ParticleVert verts[12] = {
            { bgBL, Vec2( 0.0f, 0.0f ), bgColor },
            { bgBR, Vec2( 1.0f, 0.0f ), bgColor },
            { bgTR, Vec2( 1.0f, 1.0f ), bgColor },
            { bgBL, Vec2( 0.0f, 0.0f ), bgColor },
            { bgTR, Vec2( 1.0f, 1.0f ), bgColor },
            { bgTL, Vec2( 0.0f, 1.0f ), bgColor },
            { fBL, Vec2( 0.0f, 0.0f ), fillColor },
            { fBR, Vec2( 1.0f, 0.0f ), fillColor },
            { fTR, Vec2( 1.0f, 1.0f ), fillColor },
            { fBL, Vec2( 0.0f, 0.0f ), fillColor },
            { fTR, Vec2( 1.0f, 1.0f ), fillColor },
            { fTL, Vec2( 0.0f, 1.0f ), fillColor },
        };

        glDepthMask( GL_FALSE );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        particleShader.Bind();
        particleShader.SetMat4( "uViewProjection", viewProjectionMatrix );
        particleShader.SetInt( "uHasTexture", 0 );

        glBindVertexArray( particleVAO );
        glBindBuffer( GL_ARRAY_BUFFER, particleVBO );
        glBufferData( GL_ARRAY_BUFFER, sizeof( verts ), verts, GL_DYNAMIC_DRAW );
        glDrawArrays( GL_TRIANGLES, 0, 12 );

        glBindVertexArray( 0 );
        particleShader.Unbind();

        glDepthMask( GL_TRUE );
        glDisable( GL_BLEND );
    }

    void Renderer::DrawBillboardImmediate( const Texture * texture, const Vec3 & worldPos, f32 size,
                                            const Vec3 & cameraPos, const Vec3 & cameraUp,
                                            f32 rotationRad, const Vec4 & color ) {
        f32 halfSize = size * 0.5f;

        Vec3 toCamera = Normalize( cameraPos - worldPos );
        Vec3 right = Normalize( Cross( cameraUp, toCamera ) ) * halfSize;
        Vec3 up = Normalize( Cross( toCamera, right ) ) * halfSize;

        if ( rotationRad != 0.0f ) {
            f32 cosA = cosf( rotationRad );
            f32 sinA = sinf( rotationRad );
            Vec3 newRight = right * cosA + up * sinA;
            Vec3 newUp = -right * sinA + up * cosA;
            right = newRight;
            up = newUp;
        }

        Vec3 bl = worldPos - right - up;
        Vec3 br = worldPos + right - up;
        Vec3 tr = worldPos + right + up;
        Vec3 tl = worldPos - right + up;

        ParticleVert verts[6] = {
            { bl, Vec2( 0.0f, 0.0f ), color },
            { br, Vec2( 1.0f, 0.0f ), color },
            { tr, Vec2( 1.0f, 1.0f ), color },
            { bl, Vec2( 0.0f, 0.0f ), color },
            { tr, Vec2( 1.0f, 1.0f ), color },
            { tl, Vec2( 0.0f, 1.0f ), color },
        };

        glDepthMask( GL_FALSE );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );

        particleShader.Bind();
        particleShader.SetMat4( "uViewProjection", viewProjectionMatrix );

        if ( texture != nullptr && texture->IsValid() ) {
            particleShader.SetInt( "uHasTexture", 1 );
            particleShader.SetInt( "uTexture", 0 );
            texture->Bind( 0 );
        }
        else {
            particleShader.SetInt( "uHasTexture", 0 );
        }

        glBindVertexArray( particleVAO );
        glBindBuffer( GL_ARRAY_BUFFER, particleVBO );
        glBufferData( GL_ARRAY_BUFFER, sizeof( verts ), verts, GL_DYNAMIC_DRAW );
        glDrawArrays( GL_TRIANGLES, 0, 6 );

        glBindVertexArray( 0 );
        particleShader.Unbind();

        glDepthMask( GL_TRUE );
        glDisable( GL_BLEND );
    }

    void Renderer::RenderParticles( const ParticleSystem::Particle * particles, i32 count, const Vec3 & cameraPos, const Vec3 & cameraUp ) {
        if ( count <= 0 ) {
            return;
        }

        for ( i32 i = 0; i < count; i++ ) {
            const auto & p = particles[i];

            f32 t = 1.0f - (p.lifetime / p.maxLifetime);
            f32 size = Lerp( p.startSize, p.endSize, t ) * 0.5f;
            Vec4 color = Vec4(
                Lerp( p.startColor.r, p.endColor.r, t ),
                Lerp( p.startColor.g, p.endColor.g, t ),
                Lerp( p.startColor.b, p.endColor.b, t ),
                Lerp( p.startColor.a, p.endColor.a, t )
            );

            Vec3 right, up;
            if ( p.velocityAligned && LengthSquared( p.velocity ) > 0.0001f ) {
                Vec3 velDir = Normalize( p.velocity );
                Vec3 toCamera = Normalize( cameraPos - p.position );
                right = Normalize( Cross( velDir, toCamera ) ) * size;
                up = velDir * size * p.stretchFactor;
            }
            else {
                Vec3 toCamera = Normalize( cameraPos - p.position );
                right = Normalize( Cross( cameraUp, toCamera ) ) * size;
                up = Normalize( Cross( toCamera, right ) ) * size;
            }

            Vec3 bl = p.position - right - up;
            Vec3 br = p.position + right - up;
            Vec3 tr = p.position + right + up;
            Vec3 tl = p.position - right + up;

            QueuedParticle qp = {};
            qp.texture = p.texture;
            qp.verts[0] = { bl, Vec2( 0.0f, 0.0f ), color };
            qp.verts[1] = { br, Vec2( 1.0f, 0.0f ), color };
            qp.verts[2] = { tr, Vec2( 1.0f, 1.0f ), color };
            qp.verts[3] = { bl, Vec2( 0.0f, 0.0f ), color };
            qp.verts[4] = { tr, Vec2( 1.0f, 1.0f ), color };
            qp.verts[5] = { tl, Vec2( 0.0f, 1.0f ), color };

            TransparentEntry te = {};
            te.type = TransparentType::Particle;
            te.index = static_cast<i32>( queuedParticles.size() );
            te.distSq = LengthSquared( cameraPos - p.position );

            queuedParticles.push_back( qp );
            transparentEntries.push_back( te );
        }
    }

    void Renderer::FlushDebugLines() {
        if ( debugLineVerts.empty() ) {
            return;
        }

        glDisable( GL_DEPTH_TEST );

        glBindVertexArray( debugLineVAO );
        glBindBuffer( GL_ARRAY_BUFFER, debugLineVBO );
        glBufferData( GL_ARRAY_BUFFER, debugLineVerts.size() * sizeof( DebugLineVert ), debugLineVerts.data(), GL_DYNAMIC_DRAW );

        flatColorShader.Bind();
        flatColorShader.SetMat4( "uViewProjection", viewProjectionMatrix );

        glDrawArrays( GL_LINES, 0, (i32)debugLineVerts.size() );

        glBindVertexArray( 0 );
        flatColorShader.Unbind();

        glEnable( GL_DEPTH_TEST );

        debugLineVerts.clear();
    }

} // namespace atto
