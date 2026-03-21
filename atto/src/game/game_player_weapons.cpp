#include "game_player_weapons.h"
#include "game_map.h"

namespace atto {

    static constexpr Vec3 ArmsLocalOffset = Vec3( 0.0f, -0.15f, -0.06f );

    // =========================================================================
    // PlayerWeaponKnife
    // =========================================================================

    void PlayerWeaponKnife::OnStart() {
        model.LoadFromFile( "assets/player/arms/knife.glb" );
        animator.PlayAnimation( model, "Armature|Knife_Idle_Anim", true );

        sndSwing1.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndSwing1.LoadSounds( {
            "knife/swing-1_1.wav",
            "knife/swing-1_2.wav",
            "knife/swing-1_3.wav",
            "knife/swing-1_4.wav",
            "knife/swing-1_5.wav",
        } );

        sndSwing2.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndSwing2.LoadSounds( {
            "knife/swing-3_1.wav",
            "knife/swing-3_2.wav",
            "knife/swing-3_3.wav",
            "knife/swing-3_4.wav",
            "knife/swing-3_5.wav",
        } );

        sndHitMetal1.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndHitMetal1.LoadSounds( {
            "knife/hit-metal-1_1.wav",
            "knife/hit-metal-1_2.wav",
            "knife/hit-metal-1_3.wav",
            "knife/hit-metal-1_4.wav",
            "knife/hit-metal-1_5.wav",
        } );

        sndHitMetal2.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndHitMetal2.LoadSounds( {
            "knife/hit-metal-3_1.wav",
            "knife/hit-metal-3_2.wav",
            "knife/hit-metal-3_3.wav",
            "knife/hit-metal-3_4.wav",
            "knife/hit-metal-3_5.wav",
        } );
    }

    void PlayerWeaponKnife::OnUpdate( f32 dt, bool isMoving, bool isSprinting, FPSCamera & camera, GameMap & map ) {
        ATTO_ASSERT( animator.GetCurrentAnimation(), "knife animator has no animation" );

        Input & input = Engine::Get().GetInput();
        const std::string & curAnim = animator.GetCurrentAnimation()->name;
        bool isIdleWalkOrRun = ( curAnim == "Armature|Knife_Idle_Anim"
                               || curAnim == "Armature|Knife_Walk_Anim"
                               || curAnim == "Armature|Knife_Run_Anim" );

        // Attack input — not allowed while sprinting
        if ( input.IsMouseButtonDown( MouseButton::Left ) && isIdleWalkOrRun && !isSprinting ) {
            animator.PlayAnimation( model, "Armature|Knife_Attack_1_Anim", false );
            sndSwing1.Play( 0.5f );
            isAttacking = true;
        }

        if ( input.IsMouseButtonDown( MouseButton::Right ) && isIdleWalkOrRun && !isSprinting ) {
            animator.PlayAnimation( model, "Armature|Knife_Attack_3_Anim", false );
            sndSwing2.Play( 0.5f );
            isAttacking = true;
        }

        // Hit detection at 75% through each attack
        if ( curAnim == "Armature|Knife_Attack_1_Anim"
            && animator.GetPercentComplete() > 0.75f
            && isAttacking ) {
            isAttacking = false;
            MapRaycastResult result;
            if ( map.Raycast( camera.GetPosition(), camera.GetForward(), result ) ) {
                if ( result.entity && result.distance <= 1.5f ) {
                    LOG_INFO( "Hit entity: %s at distance: %f", EntityTypeToString( result.entity->GetType() ), result.distance );
                    result.entity->TakeDamage( 34 );
                    sndHitMetal1.Play( 0.5f );
                }
            }
        }

        if ( curAnim == "Armature|Knife_Attack_3_Anim"
            && animator.GetPercentComplete() > 0.75f
            && isAttacking ) {
            isAttacking = false;
            MapRaycastResult result;
            if ( map.Raycast( camera.GetPosition(), camera.GetForward(), result ) ) {
                if ( result.entity && result.distance <= 1.5f ) {
                    LOG_INFO( "Hit entity: %s at distance: %f", EntityTypeToString( result.entity->GetType() ), result.distance );
                    result.entity->TakeDamage( 55 );
                    sndHitMetal2.Play( 0.5f );
                }
            }
        }

        // Return to locomotion anim after attack finishes
        if ( animator.IsFinished() && !isIdleWalkOrRun ) {
            isAttacking = false;
            const char * returnAnim = !isMoving   ? "Armature|Knife_Idle_Anim"
                                    : isSprinting ? "Armature|Knife_Run_Anim"
                                    :               "Armature|Knife_Walk_Anim";
            animator.PlayAnimation( model, returnAnim, true );
        }

        // Locomotion transitions
        if ( !isAttacking ) {
            if ( !isMoving && curAnim != "Armature|Knife_Idle_Anim" ) {
                animator.PlayAnimation( model, "Armature|Knife_Idle_Anim", true );
            }
            else if ( isMoving && isSprinting && curAnim != "Armature|Knife_Run_Anim" ) {
                animator.PlayAnimation( model, "Armature|Knife_Run_Anim", true );
            }
            else if ( isMoving && !isSprinting && curAnim != "Armature|Knife_Walk_Anim" ) {
                animator.PlayAnimation( model, "Armature|Knife_Walk_Anim", true );
            }
        }

        animator.Update( dt );
    }

    void PlayerWeaponKnife::OnRender( Renderer & renderer, const FPSCamera & camera ) {
        Mat4 cameraWorld     = glm::inverse( camera.GetViewMatrix() );
        Mat4 localCorrection = glm::rotate( Mat4( 1.0f ), PI, Vec3( 0.0f, 1.0f, 0.0f ) );
        Mat4 armsMatrix      = cameraWorld * glm::translate( Mat4( 1.0f ), ArmsLocalOffset ) * localCorrection;
        renderer.RenderAnimatedModel( model, animator, armsMatrix );
    }

    // =========================================================================
    // PlayerWeaponGlock
    // =========================================================================

    void PlayerWeaponGlock::OnStart() {
        
    }

    void PlayerWeaponGlock::OnUpdate( f32 dt, bool isMoving, bool isSprinting, FPSCamera & camera, GameMap & map ) {
    }

    void PlayerWeaponGlock::OnRender( Renderer & renderer, const FPSCamera & camera ) {
    }

}
