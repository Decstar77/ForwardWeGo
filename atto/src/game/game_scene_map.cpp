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

        coinTexture      = renderer.GetOrLoadTexture( "assets/textures/coin.png" );
        crosshairTexture = renderer.GetOrLoadTexture( "assets/textures/crosshair008.png" );
        hitMarkerTexture = renderer.GetOrLoadTexture( "assets/textures/crosshair/crosshair086.png" );
        hudFont          = renderer.GetOrLoadFont( "assets/fonts/kenvector_future.ttf", 26.0f );
        hudFontSmall     = renderer.GetOrLoadFont( "assets/fonts/kenvector_future.ttf", 20.0f );

        Engine::Get().GetAudioSystem().SetMuted( false );
    }

    void GameMapScene::OnUpdate( f32 deltaTime ) {
        Input & input = Engine::Get().GetInput();

        if ( input.IsKeyPressed( Key::Escape ) ) {
            Engine::Get().TransitionToScene( "Editor", map.GetPath() );
        }

        if ( input.IsKeyDown( Key::LeftControl ) && input.IsKeyPressed( Key::M ) ) {
            Engine::Get().GetAudioSystem().ToggleMuted();
        }

        fps = fps * 0.9f + ( 1.0f / deltaTime ) * 0.1f;

        const Vec3 oldPlayerPos = player.GetCamera().GetPosition();
        player.OnUpdate( deltaTime, map );
        const Vec3 newPlayerPos = player.GetCamera().GetPosition();
        const Vec3 playerVelocity = newPlayerPos - oldPlayerPos;

        // Feed the player position and rotation into the audio system
        Engine::Get().GetAudioSystem().SetListenerPosition( player.GetCamera().GetPosition() );
        Engine::Get().GetAudioSystem().SetListenerOrientation( player.GetCamera().GetForward(), player.GetCamera().GetUp() );
        Engine::Get().GetAudioSystem().SetListenerVelocity( playerVelocity );

        // Feed player position to the map so entities (AI) can query it
        map.SetPlayerPosition( player.GetCamera().GetPosition() );
        map.SetPlayerCameraUp( player.GetCamera().GetUp() );

        map.Update( deltaTime );

        // Apply any damage entities dealt to the player
        i32 pendingDmg = map.FlushPlayerDamage();
        if ( pendingDmg > 0 ) {
            player.TakeDamage( pendingDmg );
        }
    }

    void GameMapScene::OnRender( Renderer & renderer ) {
        const FPSCamera & camera = player.GetCamera();

        renderer.SetViewport( 0, 0, camera.GetViewportWidth(), camera.GetViewportHeight() );
        renderer.SetViewProjectionMatrix( camera.GetViewProjectionMatrix() );
        map.Render( renderer, 0.0, -1 );

        renderer.RenderSkybox( camera.GetViewMatrix(), camera.GetProjectionMatrix() );

        player.OnRender( renderer );

        // HUD
        const i32 vpW = camera.GetViewportWidth();
        const i32 vpH = camera.GetViewportHeight();

        ui.Begin( vpW, vpH );

        // Crosshair at screen center
        ui.DrawSprite( crosshairTexture, ui.GetCenterX(), ui.GetCenterY(), 32, 32 );

        if ( player.GetHitMarkerAlpha() > 0.0f ) {
            ui.DrawSprite( hitMarkerTexture, ui.GetCenterX(), ui.GetCenterY(), 48, 48 );
        }

        // FPS — top left
        char fpsText[ 32 ];
        snprintf( fpsText, sizeof( fpsText ), "FPS: %.0f", fps );
        ui.DrawText( hudFont, 20, 20, fpsText );

        // Health — bottom left
        {
            char healthText[ 32 ];
            snprintf( healthText, sizeof( healthText ), "HP: %d", player.GetHealth() );
            Vec4 healthColor = player.GetHealth() > 30 ? Vec4( 1.0f ) : Vec4( 1.0f, 0.3f, 0.3f, 1.0f );
            ui.DrawText( hudFont, 20, ui.GetHeight() - 20, healthText, healthColor,
                         UIAlignH::Left, UIAlignV::Bottom );
        }

        // Ammo / weapon display — bottom right
        const bool hasAmmo = player.GetActiveWeapon() == WeaponSlot::Glock;
        const char * weaponName = hasAmmo ? "Glock" : "Knife";

        if ( hasAmmo ) {
            char ammoText[ 32 ];
            snprintf( ammoText, sizeof( ammoText ), "%d / %d",
                      player.GetGlock().GetAmmo(), player.GetGlock().GetMaxAmmo() );
            ui.DrawText( hudFont, ui.GetWidth() - 20, ui.GetHeight() - 40, ammoText, Vec4( 1.0f ),
                         UIAlignH::Right, UIAlignV::Bottom );
        }

        ui.DrawText( hudFontSmall, ui.GetWidth() - 20, ui.GetHeight() - 20, weaponName,
                     Vec4( 0.7f, 0.7f, 0.7f, 1.0f ), UIAlignH::Right, UIAlignV::Bottom );

        ui.End( renderer );

        renderer.RenderDamageVignette( player.GetDamageVignetteAlpha() );
    }

    void GameMapScene::OnShutdown() {
        map.Shutdown();
    }

    void GameMapScene::OnResize( i32 width, i32 height ) {
        player.OnResize( width, height );
    }
}
