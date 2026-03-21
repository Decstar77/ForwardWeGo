#include "game_player_weapons.h"
#include "game_map.h"

namespace atto {

    // =========================================================================
    // PlayerWeaponKnife
    // =========================================================================

    void PlayerWeaponKnife::OnStart() {
        model.LoadFromFile( "assets/player/arms/knife.glb" );
        animator.PlayAnimation( model, "Armature|Knife_Idle_Anim", true );
        isEquipped = true;

        sndEquip.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndEquip.LoadSounds( {
            "knife/draw-01.wav",
            "knife/draw-02.wav",
            "knife/draw-03.wav",
            "knife/draw-04.wav",
        } );

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

    void PlayerWeaponKnife::OnEquip() {
        animator.PlayAnimation( model, "Armature|Knife_Draw_Anim", false );
        isAttacking = false;
        isEquipped  = false;
        sndEquip.Play();
    }

    void PlayerWeaponKnife::OnUpdate( f32 dt, bool isMoving, bool isSprinting, FPSCamera & camera, GameMap & map ) {
        ATTO_ASSERT( animator.GetCurrentAnimation(), "knife animator has no animation" );

        // Finish equip animation before allowing any input
        if ( !isEquipped ) {
            if ( animator.IsFinished() ) {
                isEquipped = true;
                const char * idleAnim = !isMoving   ? "Armature|Knife_Idle_Anim"
                                      : isSprinting ? "Armature|Knife_Run_Anim"
                                      :               "Armature|Knife_Walk_Anim";
                animator.PlayAnimation( model, idleAnim, true );
            }
            animator.Update( dt );
            return;
        }

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
        const Vec3 ArmsLocalOffset = Vec3( 0.0f, -0.15f, -0.06f );
        Mat4 cameraWorld     = glm::inverse( camera.GetViewMatrix() );
        Mat4 localCorrection = glm::rotate( Mat4( 1.0f ), PI, Vec3( 0.0f, 1.0f, 0.0f ) );
        Mat4 armsMatrix      = cameraWorld * glm::translate( Mat4( 1.0f ), ArmsLocalOffset ) * localCorrection;
        renderer.RenderAnimatedModel( model, animator, armsMatrix );
    }

    // =========================================================================
    // PlayerWeaponGlock
    // =========================================================================

    void PlayerWeaponGlock::OnStart() {
        model.LoadFromFile( "assets/player/arms/glock.glb" );
        animator.PlayAnimation( model, "Armature|Glock_Idle_Anim", true );
        isEquipped = true;

        sndEquip.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndEquip.LoadSounds( {
            "glock/gun_pistol_general_handling_01.wav",
            "glock/gun_pistol_general_handling_03.wav",
            "glock/gun_pistol_general_handling_05.wav",
            "glock/gun_pistol_general_handling_06.wav",
            "glock/gun_pistol_general_handling_07.wav",
            "glock/gun_pistol_general_handling_10.wav",
        } );

        sndCock.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndCock.LoadSounds( {
            "glock/gun_pistol_cock_01.wav",
            "glock/gun_pistol_cock_02.wav",
            "glock/gun_pistol_cock_03.wav",
            "glock/gun_pistol_cock_04.wav",
            "glock/gun_pistol_cock_05.wav",
            "glock/gun_pistol_cock_06.wav",
            "glock/gun_pistol_cock_07.wav",
        } );

        sndShoot.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndShoot.LoadSounds( {
            "glock/gun_pistol_shot_01.wav",
            "glock/gun_pistol_shot_02.wav",
            "glock/gun_pistol_shot_03.wav",
            "glock/gun_pistol_shot_04.wav",
            "glock/gun_pistol_shot_05.wav",
        } );

        sndRemoveMag.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndRemoveMag.LoadSounds( {
            "glock/gun_pistol_remove_mag_01.wav",
            "glock/gun_pistol_remove_mag_02.wav",
            "glock/gun_pistol_remove_mag_03.wav",
            "glock/gun_pistol_remove_mag_04.wav",
            "glock/gun_pistol_remove_mag_05.wav",
            "glock/gun_pistol_remove_mag_06.wav",
        } );

        sndInsertMag.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndInsertMag.LoadSounds( {
            "glock/gun_pistol_insert_mag_01.wav",
            "glock/gun_pistol_insert_mag_02.wav",
            "glock/gun_pistol_insert_mag_03.wav",
            "glock/gun_pistol_insert_mag_04.wav",
            "glock/gun_pistol_insert_mag_05.wav",
        } );

        sndDry.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndDry.LoadSounds( {
            "glock/gun_pistol_dry_fire_01.wav",
            "glock/gun_pistol_dry_fire_02.wav",
            "glock/gun_pistol_dry_fire_03.wav",
            "glock/gun_pistol_dry_fire_04.wav",
            "glock/gun_pistol_dry_fire_05.wav",
            "glock/gun_pistol_dry_fire_06.wav",
        } );
    }

    void PlayerWeaponGlock::OnEquip() {
        animator.PlayAnimation( model, "Armature|Glock_Draw_Anim", false );
        isAttacking = false;
        isEquipped  = false;
        sndEquip.Play();
    }

    void PlayerWeaponGlock::OnUpdate( f32 dt, bool isMoving, bool isSprinting, FPSCamera & camera, GameMap & map ) {
        ATTO_ASSERT( animator.GetCurrentAnimation(), "glock animator has no animation" );

        Input & input = Engine::Get().GetInput();
        const std::string & curAnim = animator.GetCurrentAnimation()->name;

        // Finish draw animation before allowing any input
        if ( !isEquipped ) {
            if ( animator.IsFinished() ) {
                isEquipped = true;
                const char * idleAnim = !isMoving   ? "Armature|Glock_Idle_Anim"
                                      : isSprinting ? "Armature|Glock_Run_Anim"
                                      :               "Armature|Glock_Walk_Anim";
                animator.PlayAnimation( model, idleAnim, true );
            }
            animator.Update( dt );
            return;
        }

        bool isIdleWalkOrRun = ( curAnim == "Armature|Glock_Idle_Anim"
                               || curAnim == "Armature|Glock_Walk_Anim"
                               || curAnim == "Armature|Glock_Run_Anim" );
        bool isADSIdle = ( curAnim == "Armature|Glock_ADS_Anim" );

        // Enter ADS — RMB held while in a normal locomotion state
        if ( input.IsMouseButtonDown( MouseButton::Right ) && !isADS && isIdleWalkOrRun && !isSprinting && !isReloading && !isAttacking ) {
            isADS = true;
            animator.PlayAnimation( model, "Armature|Glock_ADS_Anim", true );
        }

        // Exit ADS — RMB released while ADS idle (not mid-fire)
        if ( !input.IsMouseButtonDown( MouseButton::Right ) && isADS && isADSIdle ) {
            isADS = false;
            const char * returnAnim = !isMoving   ? "Armature|Glock_Idle_Anim"
                                    : isSprinting ? "Armature|Glock_Run_Anim"
                                    :               "Armature|Glock_Walk_Anim";
            animator.PlayAnimation( model, returnAnim, true );
        }

        // Fire — ADS
        if ( input.IsMouseButtonPressed( MouseButton::Left ) && isADS && isADSIdle ) {
            if ( ammo > 0 ) {
                animator.PlayAnimation( model, "Armature|Glock_ADS_Fire_Anim", false );
                isAttacking = true;
            }
            else {
                sndDry.Play();
            }
        }

        // Fire — hip
        if ( input.IsMouseButtonPressed( MouseButton::Left ) && !isADS && isIdleWalkOrRun && !isSprinting && !isReloading ) {
            if ( ammo > 0 ) {
                animator.PlayAnimation( model, "Armature|Glock_Fire_Anim", false );
                isAttacking = true;
            }
            else {
                sndDry.Play();
            }
        }

        // R to reload manually — not while ADS
        if ( input.IsKeyPressed( Key::R ) && !isADS && isIdleWalkOrRun && !isReloading && ammo < MaxAmmo ) {
            animator.PlayAnimation( model, "Armature|Glock_Reload_Anim", false );
            isReloading      = true;
            reloadSnd1Played = false;
            reloadSnd2Played = false;
            reloadSnd3Played = false;
        }

        // Reload sound cues
        if ( isReloading && curAnim == "Armature|Glock_Reload_Anim" ) {
            f32 pct = animator.GetPercentComplete();
            if ( !reloadSnd1Played ) {
                sndRemoveMag.Play( 0.5f );
                reloadSnd1Played = true;
            }
            if ( pct > 0.4f && !reloadSnd2Played ) {
                sndInsertMag.Play( 0.5f );
                reloadSnd2Played = true;
            }
            if ( pct > 0.7f && !reloadSnd3Played ) {
                sndCock.Play( 0.5f );
                reloadSnd3Played = true;
            }
        }

        // Hit detection at 65% — both hip and ADS fire
        bool isFiring = ( curAnim == "Armature|Glock_Fire_Anim" || curAnim == "Armature|Glock_ADS_Fire_Anim" );
        if ( isFiring && animator.GetPercentComplete() > 0.65f && isAttacking ) {
            isAttacking = false;
            ammo--;
            MapRaycastResult result;
            if ( map.Raycast( camera.GetPosition(), camera.GetForward(), result ) ) {
                if ( result.entity && result.distance <= 50.0f ) {
                    LOG_INFO( "Glock hit: %s at distance: %f", EntityTypeToString( result.entity->GetType() ), result.distance );
                    result.entity->TakeDamage( 25 );
                }
            }
            sndShoot.Play( 0.2f );
        }

        // Reload complete
        if ( isReloading && animator.IsFinished() ) {
            isReloading = false;
            ammo        = MaxAmmo;
            const char * returnAnim = !isMoving   ? "Armature|Glock_Idle_Anim"
                                    : isSprinting ? "Armature|Glock_Run_Anim"
                                    :               "Armature|Glock_Walk_Anim";
            animator.PlayAnimation( model, returnAnim, true );
        }

        // ADS fire finished — return to ADS if RMB still held, else hip
        if ( curAnim == "Armature|Glock_ADS_Fire_Anim" && animator.IsFinished() ) {
            isAttacking = false;
            if ( input.IsMouseButtonDown( MouseButton::Right ) ) {
                animator.PlayAnimation( model, "Armature|Glock_ADS_Anim", true );
            }
            else {
                isADS = false;
                const char * returnAnim = !isMoving   ? "Armature|Glock_Idle_Anim"
                                        : isSprinting ? "Armature|Glock_Run_Anim"
                                        :               "Armature|Glock_Walk_Anim";
                animator.PlayAnimation( model, returnAnim, true );
            }
        }

        // Hip fire finished — return to locomotion
        if ( curAnim == "Armature|Glock_Fire_Anim" && animator.IsFinished() ) {
            isAttacking = false;
            const char * returnAnim = !isMoving   ? "Armature|Glock_Idle_Anim"
                                    : isSprinting ? "Armature|Glock_Run_Anim"
                                    :               "Armature|Glock_Walk_Anim";
            animator.PlayAnimation( model, returnAnim, true );
        }

        // Locomotion transitions — only when fully in hip-fire mode
        if ( !isAttacking && !isReloading && !isADS ) {
            if ( !isMoving && curAnim != "Armature|Glock_Idle_Anim" ) {
                animator.PlayAnimation( model, "Armature|Glock_Idle_Anim", true );
            }
            else if ( isMoving && isSprinting && curAnim != "Armature|Glock_Run_Anim" ) {
                animator.PlayAnimation( model, "Armature|Glock_Run_Anim", true );
            }
            else if ( isMoving && !isSprinting && curAnim != "Armature|Glock_Walk_Anim" ) {
                animator.PlayAnimation( model, "Armature|Glock_Walk_Anim", true );
            }
        }

        animator.Update( dt );
    }

    void PlayerWeaponGlock::OnRender( Renderer & renderer, const FPSCamera & camera ) {
        Vec3 ArmsLocalOffset = Vec3( 0.05f, -0.22f, -0.1f );
        if (isADS == true) {
            ArmsLocalOffset = Vec3( 0.00f, -0.22f, -0.1f );
        }

        Mat4 cameraWorld     = glm::inverse( camera.GetViewMatrix() );
        Mat4 localCorrection = glm::rotate( Mat4( 1.0f ), PI, Vec3( 0.0f, 1.0f, 0.0f ) );
        Mat4 armsMatrix      = cameraWorld * glm::translate( Mat4( 1.0f ), ArmsLocalOffset ) * localCorrection;
        renderer.RenderAnimatedModel( model, animator, armsMatrix );
    }

}
