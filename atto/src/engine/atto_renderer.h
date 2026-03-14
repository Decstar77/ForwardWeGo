#pragma once

#include "atto_core.h"
#include "atto_math.h"
#include "renderer/atto_render_model.h"
#include "renderer/atto_render_material.h"

namespace atto {

    class StaticModel;

    class Renderer {
    public:
        bool Initialize();
        void Shutdown();

        void BeginFrame();
        void EndFrame();

        void SetClearColor( const Color & color );
        void SetViewport( i32 x, i32 y, i32 width, i32 height );

        void SetViewProjectionMatrix( const Mat4 & vp );
        void SetWireframe( bool enabled );
        void RenderTestTriangle();
        void RenderStaticModel( const StaticModel & model, const Mat4 & modelMatrix );
        void RenderStaticModel( const StaticModel & model, const Mat4 & modelMatrix, const Vec3 & color );
        void RenderStaticModelUnlit( const StaticModel & model, const Mat4 & modelMatrix );
        void RenderStaticModelUnlit( const StaticModel & model, const Mat4 & modelMatrix, const Vec3 & color );
        void RenderGrid( Vec3 axisH, Vec3 axisV, Vec3 center, f32 spacing, f32 halfExtentH, f32 halfExtentV );

    private:
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

        Color clearColor = Color( 0.1f, 0.1f, 0.12f, 1.0f );
    };

} // namespace atto
