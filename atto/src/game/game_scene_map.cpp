#include "game_scene_map.h"


namespace atto {

    void GameMapScene::OnStart() {
        Vec2i windowSize = Engine::Get().GetWindowSize();

        camera.SetViewportSize( windowSize.x, windowSize.y );
        camera.SetPosition( Vec3( 0.0f, PlayerEyeHeight, 3.0f ) );
        camera.SetFOV( 60.0f );
        camera.SetMoveSpeed( 5.0f );
        camera.SetLookSensitivity( 0.1f );

        playerHands.LoadFromFile( "assets/player/Weapons/Arms_Combat_Knife.fbx" );
        
        playerHands.DebugPrint();

        JsonSerializer serializer( false );
        serializer.FromString( Engine::Get().GetAssetManager().ReadTextFile( "assets/maps/game.map" ) );
        map.Serialize( serializer );

        map.Initialize();

        animator.PlayAnimation( playerHands, 5, true );
    }

    void GameMapScene::OnUpdate( f32 deltaTime ) {
        Input & input = Engine::Get().GetInput();

        if ( input.IsKeyPressed( Key::Escape ) ) {
            Engine::Get().TransitionToScene( "Editor" );
        }

        if ( input.IsCursorCaptured() == false ) {
            input.SetCursorCaptured( true );
        }
        else {
            Vec2 mouseDelta = input.GetMouseDelta();
            camera.Rotate(
                mouseDelta.x * camera.GetLookSensitivity() * DEG_TO_RAD,
                -mouseDelta.y * camera.GetLookSensitivity() * DEG_TO_RAD
            );
        }

        animator.Update( deltaTime );

        f32 speed = camera.GetMoveSpeed() * deltaTime;

        // if ( input.IsKeyPressed( Key::G ) ) ArmsLocalOffset.y += 0.01f;
        // if ( input.IsKeyPressed( Key::H ) ) ArmsLocalOffset.y -= 0.01f;

        // if ( input.IsKeyPressed( Key::J ) ) ArmsLocalOffset.z += 0.01f;
        // if ( input.IsKeyPressed( Key::K ) ) ArmsLocalOffset.z -= 0.01f;

        // if ( input.IsKeyPressed( Key::L ) ) {
        //     LOG_INFO( "ArmsLocalOffset: (%f, %f, %f)", ArmsLocalOffset.x, ArmsLocalOffset.y, ArmsLocalOffset.z );
        // }

        if ( input.IsKeyDown( Key::W ) ) camera.MoveForward( speed );
        if ( input.IsKeyDown( Key::S ) ) camera.MoveForward( -speed );
        if ( input.IsKeyDown( Key::D ) ) camera.MoveRight( speed );
        if ( input.IsKeyDown( Key::A ) ) camera.MoveRight( -speed );
    }

    void GameMapScene::OnRender( Renderer & renderer ) {
        renderer.SetViewport( 0, 0, camera.GetViewportWidth(), camera.GetViewportHeight() );
        renderer.SetViewProjectionMatrix( camera.GetViewProjectionMatrix() );
        map.Render( renderer, 0.0, 1, -1 );

        Mat4 cameraWorld = glm::inverse( camera.GetViewMatrix() );

        Mat4 localCorrection = glm::rotate( Mat4( 1.0f ), PI, Vec3( 0.0f, 1.0f, 0.0f ) );
        localCorrection = glm::scale( localCorrection, Vec3( ArmsScale ) );

        Mat4 armsMatrix = cameraWorld * glm::translate( Mat4( 1.0f ), ArmsLocalOffset ) * localCorrection;

        renderer.RenderAnimatedModel( playerHands, animator, armsMatrix );
    }

    void GameMapScene::OnShutdown() {

    }

    void GameMapScene::OnResize( i32 width, i32 height ) {
        camera.SetViewportSize( width, height );
    }
}


