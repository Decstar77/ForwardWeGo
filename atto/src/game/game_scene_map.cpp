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

        f32 speed = camera.GetMoveSpeed() * deltaTime;

        if ( input.IsKeyDown( Key::W ) ) camera.MoveForward( speed );
        if ( input.IsKeyDown( Key::S ) ) camera.MoveForward( -speed );
        if ( input.IsKeyDown( Key::D ) ) camera.MoveRight( speed );
        if ( input.IsKeyDown( Key::A ) ) camera.MoveRight( -speed );
    }

    void GameMapScene::OnRender( Renderer & renderer ) {
        renderer.SetViewProjectionMatrix( camera.GetViewProjectionMatrix() );
        map.Render( renderer, 0.0, 1, -1 );
        //renderer.RenderStaticModelUnlit( playerHands, Mat4( 1 ) );
    }

    void GameMapScene::OnShutdown() {

    }

    void GameMapScene::OnResize( i32 width, i32 height ) {
        camera.SetViewportSize( width, height );
    }
}


