#include "editor_scene.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace atto {

    void EditorScene::OnStart() {
        StartImgui();

        Vec2i windowSize = Engine::Get().GetWindowSize();

        flyCamera.SetViewportSize( windowSize.x, windowSize.y );
        flyCamera.SetPosition( Vec3( 0.0f, 0.0f, 3.0f ) );
        flyCamera.SetFOV( 60.0f );
        flyCamera.SetMoveSpeed( 5.0f );
        flyCamera.SetLookSensitivity( 0.1f );

        model.LoadFromFile( "assets/sm/SM_Env_Tree_02.fbx", 0.01f );
        texture.LoadFromFile( "assets/PolygonScifi_01_C.png" );
    }

    void EditorScene::StartImgui() {
        ImGui::CreateContext();
        ImGuiIO & io = ImGui::GetIO();
        (void)io;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Initialize ImGui GLFW and OpenGL3 bindings
        ImGui_ImplGlfw_InitForOpenGL( Engine::Get().GetWindowHandle(), true );
        ImGui_ImplOpenGL3_Init( "#version 330 core" );
    }

    void EditorScene::OnUpdate( f32 deltaTime ) {
        Input & input = Engine::Get().GetInput();

        // Right-click to engage mouse look
        if ( input.IsMouseButtonPressed( MouseButton::Right ) ) {
            input.SetCursorCaptured( true );
        }
        if ( input.IsMouseButtonReleased( MouseButton::Right ) ) {
            input.SetCursorCaptured( false );
        }

        if ( input.IsCursorCaptured() ) {
            Vec2 mouseDelta = input.GetMouseDelta();
            flyCamera.Rotate(
                mouseDelta.x * flyCamera.GetLookSensitivity() * DEG_TO_RAD,
                -mouseDelta.y * flyCamera.GetLookSensitivity() * DEG_TO_RAD
            );
        }

        f32 speed = flyCamera.GetMoveSpeed() * deltaTime;

        if ( input.IsKeyDown( Key::W ) ) flyCamera.MoveForward( speed );
        if ( input.IsKeyDown( Key::S ) ) flyCamera.MoveForward( -speed );
        if ( input.IsKeyDown( Key::D ) ) flyCamera.MoveRight( speed );
        if ( input.IsKeyDown( Key::A ) ) flyCamera.MoveRight( -speed );
        if ( input.IsKeyDown( Key::Space ) ) flyCamera.MoveUp( speed );
        if ( input.IsKeyDown( Key::LeftControl ) ) flyCamera.MoveUp( -speed );
    }

    void EditorScene::OnRender( Renderer & renderer ) {
        renderer.SetViewProjectionMatrix( flyCamera.GetViewProjectionMatrix() );

        renderer.RenderTestTriangle();
        renderer.RenderStaticModel( model, Mat4( 1.0f ) );

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
    }

    void EditorScene::OnShutdown() {
    }

    void EditorScene::OnResize( i32 width, i32 height ) {
        flyCamera.SetViewportSize( width, height );
    }

} // namespace atto
