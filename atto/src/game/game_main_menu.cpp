#include "game_main_menu.h"

namespace atto {

    void MainMenuScene::OnStart() {
        Vec2i windowSize = Engine::Get().GetWindowSize();
        camera.SetViewportSize( windowSize.x, windowSize.y );
        camera.SetPosition( Vec3( 0.0f, 0.0f, 3.0f ) );
        camera.SetFOV( 60.0f );
        camera.SetMoveSpeed( 5.0f );
        camera.SetLookSensitivity( 0.1f );

        model.LoadFromFile( "assets/sm/SM_Env_Tree_02.fbx", 0.01f );
    }

    void MainMenuScene::OnUpdate( f32 deltaTime ) {
        Input & input = Engine::Get().GetInput();

        // Right-click to engage mouse look
        if ( input.IsMouseButtonPressed( MouseButton::Right ) ) {
            input.SetCursorCaptured( true );
        }
        if ( input.IsMouseButtonReleased( MouseButton::Right ) ) {
            input.SetCursorCaptured( false );
        }

        // Mouse look (only when captured)
        if ( input.IsCursorCaptured() ) {
            Vec2 mouseDelta = input.GetMouseDelta();
            camera.Rotate(
                mouseDelta.x * camera.GetLookSensitivity() * DEG_TO_RAD,
                -mouseDelta.y * camera.GetLookSensitivity() * DEG_TO_RAD
            );
        }

        // WASD movement
        f32 speed = camera.GetMoveSpeed() * deltaTime;

        if ( input.IsKeyDown( Key::W ) ) {
            camera.MoveForward( speed );
        }
        if ( input.IsKeyDown( Key::S ) ) {
            camera.MoveForward( -speed );
        }
        if ( input.IsKeyDown( Key::D ) ) {
            camera.MoveRight( speed );
        }
        if ( input.IsKeyDown( Key::A ) ) {
            camera.MoveRight( -speed );
        }

        // Global vertical movement
        if ( input.IsKeyDown( Key::Space ) ) {
            camera.MoveUp( speed );
        }
        if ( input.IsKeyDown( Key::LeftControl ) ) {
            camera.MoveUp( -speed );
        }
    }

    void MainMenuScene::OnRender( Renderer & renderer ) {
        renderer.SetViewProjectionMatrix( camera.GetViewProjectionMatrix() );
        renderer.RenderTestTriangle();
        renderer.RenderStaticModel( model, Mat4( 1.0f ) );
    }

    void MainMenuScene::OnShutdown() {
    }

    void MainMenuScene::OnResize( i32 width, i32 height ) {
        camera.SetViewportSize( width, height );
    }

} // namespace atto
