#include "game_player_controller.h"
#include "game_map.h"

namespace atto {

    void PlayerController::OnStart( const Vec3 & position ) {
        Vec2i windowSize = Engine::Get().GetWindowSize();

        camera.SetViewportSize( windowSize.x, windowSize.y );
        camera.SetPosition( Vec3( position.x, position.y + PlayerEyeHeight, position.z ) );
        camera.SetFOV( 60.0f );
        camera.SetMoveSpeed( 5.0f );
        camera.SetLookSensitivity( 0.1f );

        playerHands.LoadFromFile( "assets/player/arms/knife.glb" );
        animator.PlayAnimation( playerHands, "Armature|Knife_Idle_Anim", true );

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

        sndKnifeHitMetal1.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndKnifeHitMetal1.LoadSounds( {
            "knife/hit-metal-1_1.wav",
            "knife/hit-metal-1_2.wav",
            "knife/hit-metal-1_3.wav",
            "knife/hit-metal-1_4.wav",
            "knife/hit-metal-1_5.wav",
            } );

        sndKnifeHitMetal2.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndKnifeHitMetal2.LoadSounds( {
            "knife/hit-metal-3_1.wav",
            "knife/hit-metal-3_2.wav",
            "knife/hit-metal-3_3.wav",
            "knife/hit-metal-3_4.wav",
            "knife/hit-metal-3_5.wav",
            } );
    }

    void PlayerController::OnUpdate( f32 deltaTime, GameMap & map ) {
        Input & input = Engine::Get().GetInput();

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

        bool isMoving = false;
        if ( input.IsKeyDown( Key::W ) ) { camera.MoveForward( speed );  isMoving = true; }
        if ( input.IsKeyDown( Key::S ) ) { camera.MoveForward( -speed ); isMoving = true; }
        if ( input.IsKeyDown( Key::D ) ) { camera.MoveRight( speed );    isMoving = true; }
        if ( input.IsKeyDown( Key::A ) ) { camera.MoveRight( -speed );   isMoving = true; }

        ATTO_ASSERT( animator.GetCurrentAnimation(), "player hands are null ??" );

        const std::string & curAnim = animator.GetCurrentAnimation()->name;
        bool isIdleOrWalk = ( curAnim == "Armature|Knife_Idle_Anim" || curAnim == "Armature|Knife_Walk_Anim" );

        if ( input.IsMouseButtonDown( MouseButton::Left ) && isIdleOrWalk ) {
            animator.PlayAnimation( playerHands, "Armature|Knife_Attack_1_Anim", false );
            sndKnifeSwing1.Play( 0.5f );
            playerIsAttacking = true;
        }

        if ( input.IsMouseButtonDown( MouseButton::Right ) && isIdleOrWalk ) {
            animator.PlayAnimation( playerHands, "Armature|Knife_Attack_3_Anim", false );
            sndKnifeSwing2.Play( 0.5f );
            playerIsAttacking = true;
        }

        if ( curAnim == "Armature|Knife_Attack_1_Anim"
            && animator.GetPercentComplete() > 0.5f
            && playerIsAttacking == true ) {
            playerIsAttacking = false;
            MapRaycastResult result;
            if ( map.Raycast( camera.GetPosition(), camera.GetForward(), result ) ) {
                if ( result.entity && result.distance <= 1.5f ) {
                    LOG_INFO( "Hit entity: %s at distance: %f", EntityTypeToString( result.entity->GetType() ), result.distance );
                    result.entity->TakeDamage( 34 );
                    sndKnifeHitMetal1.Play( 0.5f );
                }
            }
        }

        if ( curAnim == "Armature|Knife_Attack_3_Anim"
            && animator.GetPercentComplete() > 0.5f
            && playerIsAttacking == true ) {
            playerIsAttacking = false;
            MapRaycastResult result;
            if ( map.Raycast( camera.GetPosition(), camera.GetForward(), result ) ) {
                if ( result.entity && result.distance <= 1.5f ) {
                    LOG_INFO( "Hit entity: %s at distance: %f", EntityTypeToString( result.entity->GetType() ), result.distance );
                    result.entity->TakeDamage( 55 );
                    sndKnifeHitMetal2.Play( 0.5f );
                }
            }
        }

        if ( animator.IsFinished() && !isIdleOrWalk ) {
            playerIsAttacking = false;
            animator.PlayAnimation( playerHands, isMoving ? "Armature|Knife_Walk_Anim" : "Armature|Knife_Idle_Anim", true );
        }

        if ( !playerIsAttacking ) {
            if ( isMoving && curAnim == "Armature|Knife_Idle_Anim" ) {
                animator.PlayAnimation( playerHands, "Armature|Knife_Walk_Anim", true );
            }
            else if ( !isMoving && curAnim == "Armature|Knife_Walk_Anim" ) {
                animator.PlayAnimation( playerHands, "Armature|Knife_Idle_Anim", true );
            }
        }

        animator.Update( deltaTime );

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

    void PlayerController::OnRender( Renderer & renderer ) {
        renderer.ClearDepthBuffer();
        Mat4 cameraWorld = glm::inverse( camera.GetViewMatrix() );
        Mat4 localCorrection = glm::rotate( Mat4( 1.0f ), PI, Vec3( 0.0f, 1.0f, 0.0f ) );
        Mat4 armsMatrix = cameraWorld * glm::translate( Mat4( 1.0f ), ArmsLocalOffset ) * localCorrection;
        renderer.RenderAnimatedModel( playerHands, animator, armsMatrix );
    }

    void PlayerController::OnResize( i32 width, i32 height ) {
        camera.SetViewportSize( width, height );
    }

}
