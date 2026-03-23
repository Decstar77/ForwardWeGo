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
        hudFont          = renderer.GetOrLoadFont( "assets/fonts/kenvector_future.ttf", 26.0f );
        hudFontSmall     = renderer.GetOrLoadFont( "assets/fonts/kenvector_future.ttf", 20.0f );

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

        ui.Sprite(
            UIConstraint::FromPreset( UIAnchorPreset::Center ),
            crosshairTexture, 32, 32
        );

        char fpsText[ 32 ];
        snprintf( fpsText, sizeof( fpsText ), "FPS: %.0f", fps );
        ui.Text(
            UIConstraint::FromPreset( UIAnchorPreset::TopLeft, Vec2( 20.0f, 20.0f ) ),
            hudFont, fpsText, Vec4( 1.0f )
        );

        // Health display — bottom left
        {
            char healthText[ 32 ];
            snprintf( healthText, sizeof( healthText ), "HP: %d", player.GetHealth() );
            Vec4 healthColor = player.GetHealth() > 30 ? Vec4( 1.0f ) : Vec4( 1.0f, 0.3f, 0.3f, 1.0f );
            ui.Text(
                UIConstraint::FromPreset( UIAnchorPreset::BottomLeft, Vec2( 20.0f, -20.0f ) ),
                hudFont, healthText, healthColor
            );
        }

        // Ammo / weapon display — bottom right
        const bool hasAmmo = player.GetActiveWeapon() == WeaponSlot::Glock;
        const char * weaponName = hasAmmo ? "Glock" : "Knife";

        if ( hasAmmo ) {
            char ammoText[ 32 ];
            snprintf( ammoText, sizeof( ammoText ), "%d / %d",
                      player.GetGlock().GetAmmo(), player.GetGlock().GetMaxAmmo() );
            ui.Text(
                UIConstraint::FromPreset( UIAnchorPreset::BottomRight, Vec2( -20.0f, -40.0f ) ),
                hudFont, ammoText, Vec4( 1.0f )
            );
        }

        ui.Text(
            UIConstraint::FromPreset( UIAnchorPreset::BottomRight, Vec2( -20.0f, -20.0f ) ),
            hudFontSmall, weaponName, Vec4( 0.7f, 0.7f, 0.7f, 1.0f )
        );

        ui.End( renderer );
    }

    void GameMapScene::OnShutdown() {

    }

    void GameMapScene::OnResize( i32 width, i32 height ) {
        player.OnResize( width, height );
    }
}
