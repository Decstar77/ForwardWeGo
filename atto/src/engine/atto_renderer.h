#pragma once

#include "atto_core.h"
#include "atto_math.h"
#include "atto_shapes_3D.h"
#include "atto_containers.h"
#include "atto_ui.h"
#include "atto_particles.h"
#include "renderer/atto_render_model.h"
#include "renderer/atto_render_material.h"

namespace atto {

    class StaticModel;
    class AnimatedModel;
    class Animator;

    class Renderer {
    public:
        bool Initialize();
        void Shutdown();

        void BeginFrame();
        void EndFrame();

        void SetClearColor( const Color & color );
        void ClearDepthBuffer();
        void SetViewport( i32 x, i32 y, i32 width, i32 height );

        void SetViewProjectionMatrix( const Mat4 & vp );
        void SetWireframe( bool enabled );
        void UseUnlitShader();
        void UseLitShader();
        void RenderStaticModel( const StaticModel * model, const Mat4 & modelMatrix );
        void RenderStaticModel( const StaticModel * model, const Mat4 & modelMatrix, const Vec3 & color );
        void RenderAnimatedModel( const AnimatedModel & model, const Animator & animator, const Mat4 & modelMatrix );
        void RenderAnimatedModel( const AnimatedModel & model, const Animator & animator, const Mat4 & modelMatrix, const Vec3 & color );
        void RenderGrid( Vec3 axisH, Vec3 axisV, Vec3 center, f32 spacing, f32 halfExtentH, f32 halfExtentV );

        void LoadSkybox( const char * filePath );
        void RenderSkybox( const Mat4 & view, const Mat4 & projection );

        // 2D sprite rendering (NDC center + pixel size relative to viewport)
        void RenderSprite( const Texture * texture, Vec2 centerNDC, i32 pixelWidth, i32 pixelHeight, i32 viewportW, i32 viewportH );

        // Fullscreen red damage vignette overlay (alpha 0..1)
        void RenderDamageVignette( f32 alpha );

        // Text rendering — screen-space pixels, (0,0) at top-left
        void DrawText( const Font * font, const char * text, f32 x, f32 y, Vec4 color, i32 viewportW, i32 viewportH );

        const Texture * GetOrLoadTexture( const char * filePath, bool flip = false );
        const StaticModel * GetOrLoadStaticModel( const char * filePath, f32 loadScale = 1.0f );
        const Font * GetOrLoadFont( const char * path, f32 fontSize );

        // Billboard rendering — single textured quad in world space, always facing cameraPos
        // rotationRad rotates the quad around its facing axis
        void RenderBillboard( const Texture * texture, const Vec3 & worldPos, f32 size,
                              const Vec3 & cameraPos, const Vec3 & cameraUp,
                              f32 rotationRad = 0.0f, const Vec4 & color = Vec4( 1.0f ) );

        // World-space health bar — camera-facing colored bar with background
        void RenderWorldBar( const Vec3 & worldPos, f32 width, f32 height,
                             f32 fillFraction, const Vec4 & fillColor,
                             const Vec3 & cameraPos, const Vec3 & cameraUp,
                             const Vec4 & bgColor = Vec4( 0.0f, 0.0f, 0.0f, 0.6f ) );

        // Particle rendering (called by ParticleSystem::Render)
        void RenderParticles( const ParticleSystem::Particle * particles, i32 count, const Vec3 & cameraPos, const Vec3 & cameraUp );

        // Debug line drawing
        void DebugLine( const Vec3 & a, const Vec3 & b, const Vec3 & color = Vec3( 0.0f, 1.0f, 0.0f ) );
        void DebugSphere( const Sphere & sphere, const Vec3 & color = Vec3( 0.0f, 1.0f, 0.0f ) );
        void DebugAlignedBox( const AlignedBox & box, const Vec3 & color = Vec3( 0.0f, 1.0f, 0.0f ) );
        void DebugCapsule( const Capsule & cap, const Vec3 & color = Vec3( 0.0f, 1.0f, 0.0f ) );
        void DebugBox( const Box & box, const Vec3 & color = Vec3( 0.0f, 1.0f, 0.0f ) );

    private:
        void FlushDebugLines();
        Mat4 viewProjectionMatrix = Mat4( 1.0f );
        bool wireframe = false;

        // Test triangle / grid resources
        u32 testTriangleVAO = 0;
        u32 testTriangleVBO = 0;
        Shader flatColorShader;

        u32 gridVAO = 0;
        u32 gridVBO = 0;

        // Model shaders
        Shader modelLitShader;
        Shader modelUnlitShader;
        Shader skinnedLitShader;
        Shader * staticModelShader = nullptr;

        // Sprite resources
        Shader spriteShader;
        u32    spriteVAO = 0;
        u32    spriteVBO = 0;

        // Damage vignette
        Shader damageVignetteShader;

        // Skybox resources
        Shader skyboxShader;
        u32 skyboxVAO = 0;
        u32 skyboxVBO = 0;
        u32 skyboxTexture = 0;

        // Particle resources
        struct ParticleVert { Vec3 pos; Vec2 uv; Vec4 color; };
        Shader particleShader;
        u32 particleVAO = 0;
        u32 particleVBO = 0;

        // Debug line resources
        struct DebugLineVert { Vec3 pos; Vec3 color; };
        u32 debugLineVAO = 0;
        u32 debugLineVBO = 0;
        std::vector<DebugLineVert> debugLineVerts;

        Color clearColor = Color( 0.1f, 0.1f, 0.12f, 1.0f );

        // This could be std::vector< std::unquie_ptr<Texture>> ??
        // Textures
        FixedList<Texture, 1024> textures;

        // Static models
        FixedList<StaticModel, 1024> staticModels;

        // Fonts
        FixedList<Font, 64> fonts;

        // Text rendering resources
        Shader textShader;
        u32    textVAO = 0;
        u32    textVBO = 0;

    };

} // namespace atto
