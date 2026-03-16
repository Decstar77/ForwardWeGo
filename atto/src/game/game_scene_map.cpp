#include "game_scene_map.h"


namespace atto {

    void GameMapScene::OnStart() {
        Vec2i windowSize = Engine::Get().GetWindowSize();

        camera.SetViewportSize( windowSize.x, windowSize.y );
        camera.SetPosition( Vec3( 0.0f, PlayerEyeHeight, 3.0f ) );
        camera.SetFOV( 60.0f );
        camera.SetMoveSpeed( 5.0f );
        camera.SetLookSensitivity( 0.1f );

        playerHands.LoadFromFile( "assets/player/arms/knife.glb" );

        JsonSerializer serializer( false );
        serializer.FromString( Engine::Get().GetAssetManager().ReadTextFile( "assets/maps/game.map" ) );
        map.Serialize( serializer );

        map.Initialize();

        animator.PlayAnimation( playerHands, "Armature|Knife_Idle_Anim", true );

        Renderer & renderer = Engine::Get().GetRenderer();
        renderer.LoadSkybox( "assets/FS002_Day_Sunless.png" );

        sndFootsteps.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndFootsteps.LoadSounds( {
            "footsteps/Light-Armor-Concrete-Walking-1.wav",
            "footsteps/Light-Armor-Concrete-Walking-2.wav",
            "footsteps/Light-Armor-Concrete-Walking-3.wav",
            "footsteps/Light-Armor-Concrete-Walking-4.wav",
            "footsteps/Light-Armor-Concrete-Walking-5.wav",
            "footsteps/Light-Armor-Concrete-Walking-6.wav",
            "footsteps/Light-Armor-Concrete-Walking-7.wav",
            "footsteps/Light-Armor-Concrete-Walking-8.wav",
            "footsteps/Light-Armor-Concrete-Walking-9.wav",
            "footsteps/Light-Armor-Concrete-Walking-10.wav",
            } );

        sndKnifeSwing1.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndKnifeSwing1.LoadSounds( {
            "knife/swing-1_1.wav",
            "knife/swing-1_2.wav",
            "knife/swing-1_3.wav",
            "knife/swing-1_4.wav",
            "knife/swing-1_5.wav",
            } );

        sndKnifeSwing2.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndKnifeSwing2.LoadSounds( {
            "knife/swing-3_1.wav",
            "knife/swing-3_2.wav",
            "knife/swing-3_3.wav",
            "knife/swing-3_4.wav",
            "knife/swing-3_5.wav",
            } );

        Engine::Get().GetAudioSystem().SetMuted( true );
    }

    void GameMapScene::OnUpdate( f32 deltaTime ) {
        Input & input = Engine::Get().GetInput();

        if ( input.IsKeyPressed( Key::Escape ) ) {
            Engine::Get().TransitionToScene( "Editor" );
        }

        if ( input.IsKeyDown( Key::LeftControl ) && input.IsKeyPressed( Key::S ) ) {
            Engine::Get().GetAudioSystem().ToggleMuted();
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

        ATTO_ASSERT( animator.GetCurrentAnimation(), "player hands are null ??" );

        if ( input.IsMouseButtonDown( MouseButton::Left )
            && animator.GetCurrentAnimation()->name == "Armature|Knife_Idle_Anim" ) {
            animator.PlayAnimation( playerHands, "Armature|Knife_Attack_1_Anim", false );
            sndKnifeSwing1.Play( 0.5f );

            MapRaycastResult result;
            if ( map.Raycast( camera.GetPosition(), camera.GetForward(), result ) ) {
                if ( result.entity && result.distance <= 1.25f ) {
                    LOG_INFO( "Hit entity: %s at distance: %f", EntityTypeToString( result.entity->GetType() ), result.distance );
                    result.entity->TakeDamage( 34 );
                }
            }
        }

        if ( input.IsMouseButtonDown( MouseButton::Right )
            && animator.GetCurrentAnimation()->name == "Armature|Knife_Idle_Anim" ) {
            animator.PlayAnimation( playerHands, "Armature|Knife_Attack_3_Anim", false );
            sndKnifeSwing2.Play( 0.5f );

            MapRaycastResult result;
            if ( map.Raycast( camera.GetPosition(), camera.GetForward(), result ) ) {
                if ( result.entity && result.distance <= 1.25f ) {
                    LOG_INFO( "Hit entity: %s at distance: %f", EntityTypeToString( result.entity->GetType() ), result.distance );
                    result.entity->TakeDamage( 55 );
                }
            }
        }

        if ( animator.IsFinished() && animator.GetCurrentAnimation()->name != "Armature|Knife_Idle_Anim" ) {
            animator.PlayAnimation( playerHands, "Armature|Knife_Idle_Anim", true );
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

        bool isMoving = false;
        if ( input.IsKeyDown( Key::W ) ) { camera.MoveForward( speed );  isMoving = true; }
        if ( input.IsKeyDown( Key::S ) ) { camera.MoveForward( -speed ); isMoving = true; }
        if ( input.IsKeyDown( Key::D ) ) { camera.MoveRight( speed );    isMoving = true; }
        if ( input.IsKeyDown( Key::A ) ) { camera.MoveRight( -speed );   isMoving = true; }

        Vec3 playerPos = camera.GetPosition();
        playerPos.y = 0.0f;
        playerCapsule = Capsule::FromTips( playerPos, playerPos + Vec3( 0, PlayerHeight, 0 ), 0.2f );

        Vec3 correction = map.ResolvePlayerCollision( playerCapsule );
        correction.y = 0.0f;
        if ( correction.x != 0.0f || correction.z != 0.0f ) {
            Vec3 camPos = camera.GetPosition();
            camPos += correction;
            camera.SetPosition( camPos );

            playerPos = camPos;
            playerPos.y = 0.0f;
            playerCapsule = Capsule::FromTips( playerPos, playerPos + Vec3( 0, PlayerHeight, 0 ), 0.2f );
        }

        if ( isMoving ) {
            footstepTimer += deltaTime;
            if ( footstepTimer >= footstepInterval ) {
                footstepTimer -= footstepInterval;
                sndFootsteps.Play( 0.5f );
            }
        }
        else {
            footstepTimer = footstepInterval;
        }
    }

    void GameMapScene::OnRender( Renderer & renderer ) {
        renderer.SetViewport( 0, 0, camera.GetViewportWidth(), camera.GetViewportHeight() );
        renderer.SetViewProjectionMatrix( camera.GetViewProjectionMatrix() );
        map.Render( renderer, 0.0, 1, -1 );

        Mat4 cameraWorld = glm::inverse( camera.GetViewMatrix() );

        Mat4 localCorrection = glm::rotate( Mat4( 1.0f ), PI, Vec3( 0.0f, 1.0f, 0.0f ) );
        Mat4 armsMatrix = cameraWorld * glm::translate( Mat4( 1.0f ), ArmsLocalOffset ) * localCorrection;

        renderer.RenderAnimatedModel( playerHands, animator, armsMatrix );
        renderer.RenderSkybox( camera.GetViewMatrix(), camera.GetProjectionMatrix() );

        //map.DebugDrawBrushCollision( renderer );
    }

    void GameMapScene::OnShutdown() {

    }

    void GameMapScene::OnResize( i32 width, i32 height ) {
        camera.SetViewportSize( width, height );
    }
}


