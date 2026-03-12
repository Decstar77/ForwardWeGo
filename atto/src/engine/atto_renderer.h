#pragma once

#include "atto_core.h"
#include "atto_math.h"

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
        void RenderStaticModelUnlit( const StaticModel & model, const Mat4 & modelMatrix );
        void RenderGrid( Vec3 axisH, Vec3 axisV, Vec3 center, f32 spacing, f32 halfExtentH, f32 halfExtentV );

    private:
        static u32  CompileShader( u32 type, const char * source );
        static u32  CreateShaderProgram( const char * vertexSrc, const char * fragmentSrc );

        Mat4 viewProjectionMatrix = Mat4( 1.0f );

        // Test triangle resources
        u32 testTriangleVAO = 0;
        u32 testTriangleVBO = 0;
        u32 testTriangleShader = 0;
        i32 testTriangleVPLoc = -1;

        // Grid resources (dynamic, reuses testTriangleShader)
        u32 gridVAO = 0;
        u32 gridVBO = 0;

        // Static model shader (lit)
        u32 modelShader = 0;
        i32 modelVPLoc = -1;
        i32 modelModelLoc = -1;
        i32 modelLightDirLoc = -1;
        i32 modelLightColorLoc = -1;
        i32 modelObjectColorLoc = -1;

        // Static model shader (unlit)
        u32 modelUnlitShader = 0;
        i32 modelUnlitVPLoc = -1;
        i32 modelUnlitModelLoc = -1;
        i32 modelUnlitColorLoc = -1;

        Color clearColor = Color( 0.1f, 0.1f, 0.12f, 1.0f );
    };

} // namespace atto
