#include "game_scene_map.h"


namespace atto {

    void GameMapScene::OnStart( const char * args ) {
        JsonSerializer serializer( false );
        serializer.FromString( Engine::Get().GetAssetManager().ReadTextFile( args ) );
        map.SetPath( args );
        map.Serialize( serializer );
        map.Initialize();

        Renderer & renderer = Engine::Get().GetRenderer();
        renderer.LoadSkybox( "assets/textures/FS002_Day_Sunless.png" );

        player.OnStart( map.GetPlayerStart().spawnPos );

        crosshairTexture = renderer.GetOrLoadTexture( "assets/textures/crosshair008.png" );
        hudFont          = renderer.GetOrLoadFont( "assets/fonts/kenvector_future.ttf", 18.0f );

        Engine::Get().GetAudioSystem().SetMuted( false );
    }

    void GameMapScene::OnUpdate( f32 deltaTime ) {
        Input & input = Engine::Get().GetInput();

        if ( input.IsKeyPressed( Key::Escape ) ) {
            Engine::Get().TransitionToScene( "Editor", map.GetPath() );
        }

        if ( input.IsKeyDown( Key::LeftControl ) && input.IsKeyPressed( Key::S ) ) {
            Engine::Get().GetAudioSystem().ToggleMuted();
        }

        fps = fps * 0.9f + ( 1.0f / deltaTime ) * 0.1f;

        player.OnUpdate( deltaTime, map );

        map.Update( deltaTime );
    }

    void GameMapScene::OnRender( Renderer & renderer ) {
        const FPSCamera & camera = player.GetCamera();

        renderer.SetViewport( 0, 0, camera.GetViewportWidth(), camera.GetViewportHeight() );
        renderer.SetViewProjectionMatrix( camera.GetViewProjectionMatrix() );
        map.Render( renderer, 0.0, -1 );

        renderer.RenderSkybox( camera.GetViewMatrix(), camera.GetProjectionMatrix() );

        player.OnRender( renderer );

        renderer.RenderSprite( crosshairTexture, Vec2( 0.0f, 0.0f ), 32, 32, camera.GetViewportWidth(), camera.GetViewportHeight() );

        char fpsText[ 32 ];
        snprintf( fpsText, sizeof( fpsText ), "FPS: %.0f", fps );
        renderer.DrawText( hudFont, fpsText, 20.0f, 20.0f, Vec4( 1.0f, 1.0f, 1.0f, 1.0f ), camera.GetViewportWidth(), camera.GetViewportHeight() );
    }

    void GameMapScene::OnShutdown() {

    }

    void GameMapScene::OnResize( i32 width, i32 height ) {
        player.OnResize( width, height );
    }
}
