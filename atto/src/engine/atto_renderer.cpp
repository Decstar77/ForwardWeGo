#include "atto_renderer.h"
#include "atto_log.h"
#include "renderer/atto_render_model.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image/std_image.h>
#include <vector>
#include <cmath>

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

        if ( !skinnedLitShader.CreateFromFiles( "assets/shaders/skinned_lit.vert", "assets/shaders/model_lit.frag" ) ) {
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

        flatColorShader.Destroy();
        modelLitShader.Destroy();
        modelUnlitShader.Destroy();
        skinnedLitShader.Destroy();
        skyboxShader.Destroy();

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
        glPolygonMode( GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL );
    }

    void Renderer::UseUnlitShader() {
        staticModelShader = &modelUnlitShader;
    }

    void Renderer::UseLitShader() {
        staticModelShader = &modelLitShader;
    }

    void Renderer::RenderStaticModel( const StaticModel & model, const Mat4 & modelMatrix ) {
        RenderStaticModel( model, modelMatrix, Vec3( 0.8f ) );
    }

    void Renderer::RenderStaticModel( const StaticModel & model, const Mat4 & modelMatrix, const Vec3 & color ) {
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

        model.Draw( staticModelShader );

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

    const Texture * Renderer::GetOrLoadTexture( const char * filePath ) {
        const int count = textures.GetCount();
        for ( int i = 0; i < count; i++ ) {
            if ( textures[i].GetPath() == filePath ) {
                return &textures[i];
            }
        }

        Texture & texture = textures.AddEmpty();
        texture.LoadFromFile( filePath );
        return &texture;
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
