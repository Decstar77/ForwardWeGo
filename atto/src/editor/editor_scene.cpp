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
        static bool initialized = false;
        if ( initialized == true ) {
            return;
        }

        initialized = true;

        ImGui::CreateContext();
        ImGuiIO & io = ImGui::GetIO();

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Initialize ImGui GLFW and OpenGL3 bindings
        ImGui_ImplGlfw_InitForOpenGL( Engine::Get().GetWindowHandle(), true );
        ImGui_ImplOpenGL3_Init( "#version 330 core" );
    }

    void EditorScene::OnUpdate( f32 deltaTime ) {
        Input & input = Engine::Get().GetInput();

        bool alt = input.IsKeyDown( Key::LeftAlt ) || input.IsKeyDown( Key::RightAlt );
        if ( alt && input.IsKeyPressed( Key::Num1 ) ) { viewMode = EditorViewMode::XY;   input.SetCursorCaptured( false ); }
        if ( alt && input.IsKeyPressed( Key::Num2 ) ) { viewMode = EditorViewMode::ZY;   input.SetCursorCaptured( false ); }
        if ( alt && input.IsKeyPressed( Key::Num3 ) ) { viewMode = EditorViewMode::XZ;   input.SetCursorCaptured( false ); }
        if ( alt && input.IsKeyPressed( Key::Num4 ) ) { viewMode = EditorViewMode::Cam3D; }

        if ( viewMode == EditorViewMode::Cam3D ) {
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
        } else {
            // Ortho pan with right or middle mouse drag
            if ( input.IsMouseButtonDown( MouseButton::Right ) || input.IsMouseButtonDown( MouseButton::Middle ) ) {
                Vec2 mouseDelta = input.GetMouseDelta();
                Vec2i windowSize = Engine::Get().GetWindowSize();
                f32 scale = ( 2.0f * orthoSize ) / static_cast<f32>( windowSize.y );

                switch ( viewMode ) {
                    case EditorViewMode::XY:
                        orthoTarget.x -= mouseDelta.x * scale;
                        orthoTarget.y += mouseDelta.y * scale;
                        break;
                    case EditorViewMode::ZY:
                        orthoTarget.z -= mouseDelta.x * scale;
                        orthoTarget.y += mouseDelta.y * scale;
                        break;
                    case EditorViewMode::XZ:
                        orthoTarget.x -= mouseDelta.x * scale;
                        orthoTarget.z -= mouseDelta.y * scale;
                        break;
                    default: break;
                }
            }

            f32 scroll = input.GetScrollDelta();
            if ( scroll != 0.0f ) {
                orthoSize *= ( scroll > 0.0f ) ? 0.9f : 1.1f;
                orthoSize = Clamp( orthoSize, 0.1f, 1000.0f );
            }
        }
    }

    Mat4 EditorScene::GetOrthoViewProjectionMatrix() const {
        Vec2i windowSize = Engine::Get().GetWindowSize();
        f32 aspect = ( windowSize.y > 0 ) ? static_cast<f32>( windowSize.x ) / static_cast<f32>( windowSize.y ) : 1.0f;
        f32 halfH = orthoSize;
        f32 halfW = orthoSize * aspect;

        Mat4 proj = glm::ortho( -halfW, halfW, -halfH, halfH, -1000.0f, 1000.0f );

        constexpr f32 d = 500.0f;
        Mat4 view( 1.0f );
        switch ( viewMode ) {
            case EditorViewMode::XY:
                view = glm::lookAt( orthoTarget + Vec3( 0, 0, d ), orthoTarget, Vec3( 0, 1, 0 ) );
                break;
            case EditorViewMode::ZY:
                view = glm::lookAt( orthoTarget + Vec3( -d, 0, 0 ), orthoTarget, Vec3( 0, 1, 0 ) );
                break;
            case EditorViewMode::XZ:
                view = glm::lookAt( orthoTarget + Vec3( 0, d, 0 ), orthoTarget, Vec3( 0, 0, -1 ) );
                break;
            default: break;
        }

        return proj * view;
    }

    void EditorScene::OnRender( Renderer & renderer ) {
        if ( viewMode == EditorViewMode::Cam3D ) {
            renderer.SetViewProjectionMatrix( flyCamera.GetViewProjectionMatrix() );
        } else {
            renderer.SetViewProjectionMatrix( GetOrthoViewProjectionMatrix() );

            Vec2i windowSize = Engine::Get().GetWindowSize();
            f32 aspect = ( windowSize.y > 0 ) ? static_cast<f32>( windowSize.x ) / static_cast<f32>( windowSize.y ) : 1.0f;
            f32 halfH = orthoSize;
            f32 halfW = orthoSize * aspect;

            f32 spacing = powf( 10.0f, floorf( log10f( orthoSize * 0.2f ) ) );

            switch ( viewMode ) {
                case EditorViewMode::XY:
                    renderer.RenderGrid( Vec3( 1, 0, 0 ), Vec3( 0, 1, 0 ), orthoTarget, spacing, halfW, halfH );
                    break;
                case EditorViewMode::ZY:
                    renderer.RenderGrid( Vec3( 0, 0, 1 ), Vec3( 0, 1, 0 ), orthoTarget, spacing, halfW, halfH );
                    break;
                case EditorViewMode::XZ:
                    renderer.RenderGrid( Vec3( 1, 0, 0 ), Vec3( 0, 0, 1 ), orthoTarget, spacing, halfW, halfH );
                    break;
                default: break;
            }
        }

        if ( renderMode == EditorRenderMode::Wireframe ) {
            renderer.SetWireframe( true );
        }

        renderer.RenderTestTriangle();

        if ( renderMode == EditorRenderMode::Lit ) {
            renderer.RenderStaticModel( model, Mat4( 1.0f ) );
        } else {
            renderer.RenderStaticModelUnlit( model, Mat4( 1.0f ) );
        }

        renderer.SetWireframe( false );

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();

        static const char * modeLabels[] = { "XY (Front)", "ZY (Side)", "XZ (Top)", "3D Camera" };
        ImGui::SetNextWindowPos( ImVec2( 10, 10 ), ImGuiCond_Always );
        ImGui::SetNextWindowBgAlpha( 0.5f );
        ImGui::Begin( "##ViewMode", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav );
        ImGui::Text( "View: %s", modeLabels[static_cast<i32>( viewMode )] );
        ImGui::End();

        Vec2i windowSize = Engine::Get().GetWindowSize();
        ImGui::SetNextWindowPos( ImVec2( static_cast<f32>( windowSize.x ) - 10.0f, 10.0f ), ImGuiCond_Always, ImVec2( 1.0f, 0.0f ) );
        ImGui::SetNextWindowBgAlpha( 0.5f );
        ImGui::Begin( "##RenderMode", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav );
        i32 rm = static_cast<i32>( renderMode );
        ImGui::RadioButton( "Lit",       &rm, 0 ); ImGui::SameLine();
        ImGui::RadioButton( "Unlit",     &rm, 1 ); ImGui::SameLine();
        ImGui::RadioButton( "Wireframe", &rm, 2 );
        renderMode = static_cast<EditorRenderMode>( rm );
        ImGui::End();

        //ImGui::ShowDemoWindow();
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
    }

    void EditorScene::OnShutdown() {
    }

    void EditorScene::OnResize( i32 width, i32 height ) {
        flyCamera.SetViewportSize( width, height );
    }

} // namespace atto
