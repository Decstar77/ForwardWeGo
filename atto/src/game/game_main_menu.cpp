#include "game_main_menu.h"

namespace atto {

    void MainMenuScene::OnStart() {
        Vec2i windowSize = Engine::Get().GetWindowSize();

        flyCamera.SetViewportSize( windowSize.x, windowSize.y );
        flyCamera.SetPosition( Vec3( 0.0f, 0.0f, 3.0f ) );
        flyCamera.SetFOV( 60.0f );
        flyCamera.SetMoveSpeed( 5.0f );
        flyCamera.SetLookSensitivity( 0.1f );

        fpsCamera.SetViewportSize( windowSize.x, windowSize.y );
        fpsCamera.SetPosition( Vec3( 0.0f, 0.0f, 3.0f ) );
        fpsCamera.SetFOV( 60.0f );
        fpsCamera.SetMoveSpeed( 5.0f );
        fpsCamera.SetLookSensitivity( 0.1f );

        model.LoadFromFile( "assets/sm/SM_Env_Tree_02.fbx", 0.01f );
    }

    void MainMenuScene::OnUpdate( f32 deltaTime ) {
        Input & input = Engine::Get().GetInput();

        // F5 toggles camera mode, syncing position and orientation
        if ( input.IsKeyPressed( Key::F5 ) ) {
            if ( cameraMode == CameraMode::Fly ) {
                fpsCamera.SetPosition( flyCamera.GetPosition() );
                fpsCamera.Rotate(
                    flyCamera.GetYaw() - fpsCamera.GetYaw(),
                    flyCamera.GetPitch() - fpsCamera.GetPitch()
                );
                cameraMode = CameraMode::FPS;
            }
            else {
                flyCamera.SetPosition( fpsCamera.GetPosition() );
                flyCamera.Rotate(
                    fpsCamera.GetYaw() - flyCamera.GetYaw(),
                    fpsCamera.GetPitch() - flyCamera.GetPitch()
                );
                cameraMode = CameraMode::Fly;
            }
        }

        // Right-click to engage mouse look
        if ( input.IsMouseButtonPressed( MouseButton::Right ) ) {
            input.SetCursorCaptured( true );
        }
        if ( input.IsMouseButtonReleased( MouseButton::Right ) ) {
            input.SetCursorCaptured( false );
        }

        if ( cameraMode == CameraMode::Fly ) {
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
        else {
            if ( input.IsCursorCaptured() ) {
                Vec2 mouseDelta = input.GetMouseDelta();
                fpsCamera.Rotate(
                    mouseDelta.x * fpsCamera.GetLookSensitivity() * DEG_TO_RAD,
                    -mouseDelta.y * fpsCamera.GetLookSensitivity() * DEG_TO_RAD
                );
            }

            f32 speed = fpsCamera.GetMoveSpeed() * deltaTime;

            if ( input.IsKeyDown( Key::W ) ) fpsCamera.MoveForward( speed );
            if ( input.IsKeyDown( Key::S ) ) fpsCamera.MoveForward( -speed );
            if ( input.IsKeyDown( Key::D ) ) fpsCamera.MoveRight( speed );
            if ( input.IsKeyDown( Key::A ) ) fpsCamera.MoveRight( -speed );
            if ( input.IsKeyDown( Key::Space ) ) fpsCamera.MoveUp( speed );
            if ( input.IsKeyDown( Key::LeftControl ) ) fpsCamera.MoveUp( -speed );
        }
    }

    void MainMenuScene::OnRender( Renderer & renderer ) {
        if ( cameraMode == CameraMode::Fly ) {
            renderer.SetViewProjectionMatrix( flyCamera.GetViewProjectionMatrix() );
        }
        else {
            renderer.SetViewProjectionMatrix( fpsCamera.GetViewProjectionMatrix() );
        }
        renderer.RenderTestTriangle();
        renderer.RenderStaticModel( model, Mat4( 1.0f ) );
    }

    void MainMenuScene::OnShutdown() {
    }

    void MainMenuScene::OnResize( i32 width, i32 height ) {
        flyCamera.SetViewportSize( width, height );
        fpsCamera.SetViewportSize( width, height );
    }

} // namespace atto
