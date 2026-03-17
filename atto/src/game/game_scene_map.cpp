#include "game_scene_map.h"


namespace atto {

    void GameMapScene::OnStart() {
        JsonSerializer serializer( false );
        serializer.FromString( Engine::Get().GetAssetManager().ReadTextFile( "assets/maps/game.map" ) );
        map.Serialize( serializer );
        map.Initialize();

        Renderer & renderer = Engine::Get().GetRenderer();
        renderer.LoadSkybox( "assets/FS002_Day_Sunless.png" );

        player.OnStart();

        Engine::Get().GetAudioSystem().SetMuted( false );
    }

    void GameMapScene::OnUpdate( f32 deltaTime ) {
        Input & input = Engine::Get().GetInput();

        if ( input.IsKeyPressed( Key::Escape ) ) {
            Engine::Get().TransitionToScene( "Editor" );
        }

        if ( input.IsKeyDown( Key::LeftControl ) && input.IsKeyPressed( Key::S ) ) {
            Engine::Get().GetAudioSystem().ToggleMuted();
        }

        player.OnUpdate( deltaTime, map );
    }

    void GameMapScene::OnRender( Renderer & renderer ) {
        const FPSCamera & camera = player.GetCamera();

        renderer.SetViewport( 0, 0, camera.GetViewportWidth(), camera.GetViewportHeight() );
        renderer.SetViewProjectionMatrix( camera.GetViewProjectionMatrix() );
        map.Render( renderer, 0.0, -1 );

        renderer.RenderSkybox( camera.GetViewMatrix(), camera.GetProjectionMatrix() );

        player.OnRender( renderer );
    }

    void GameMapScene::OnShutdown() {

    }

    void GameMapScene::OnResize( i32 width, i32 height ) {
        player.OnResize( width, height );
    }
}
