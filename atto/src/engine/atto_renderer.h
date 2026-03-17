#pragma once

#include "atto_core.h"
#include "atto_math.h"
#include "atto_shapes_3D.h"
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
        void RenderStaticModel( const StaticModel & model, const Mat4 & modelMatrix );
        void RenderStaticModel( const StaticModel & model, const Mat4 & modelMatrix, const Vec3 & color );
        void RenderAnimatedModel( const AnimatedModel & model, const Animator & animator, const Mat4 & modelMatrix );
        void RenderAnimatedModel( const AnimatedModel & model, const Animator & animator, const Mat4 & modelMatrix, const Vec3 & color );
        void RenderGrid( Vec3 axisH, Vec3 axisV, Vec3 center, f32 spacing, f32 halfExtentH, f32 halfExtentV );

        void LoadSkybox( const char * filePath );
        void RenderSkybox( const Mat4 & view, const Mat4 & projection );

        // Debug line drawing
        void DebugLine( const Vec3 & a, const Vec3 & b, const Vec3 & color = Vec3( 0.0f, 1.0f, 0.0f ) );
        void DebugSphere( const Sphere & sphere, const Vec3 & color = Vec3( 0.0f, 1.0f, 0.0f ) );
        void DebugAlignedBox( const AlignedBox & box, const Vec3 & color = Vec3( 0.0f, 1.0f, 0.0f ) );
        void DebugCapsule( const Capsule & cap, const Vec3 & color = Vec3( 0.0f, 1.0f, 0.0f ) );

    private:
        void FlushDebugLines();
        Mat4 viewProjectionMatrix = Mat4( 1.0f );

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

        // Skybox resources
        Shader skyboxShader;
        u32 skyboxVAO = 0;
        u32 skyboxVBO = 0;
        u32 skyboxTexture = 0;

        // Debug line resources
        struct DebugLineVert { Vec3 pos; Vec3 color; };
        u32 debugLineVAO = 0;
        u32 debugLineVBO = 0;
        std::vector<DebugLineVert> debugLineVerts;

        Color clearColor = Color( 0.1f, 0.1f, 0.12f, 1.0f );
    };

} // namespace atto
