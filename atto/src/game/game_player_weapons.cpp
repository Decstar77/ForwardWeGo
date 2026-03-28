#include "game_player_weapons.h"
#include "game_map.h"

namespace atto {

    // =========================================================================
    // PlayerWeaponKnife
    // =========================================================================

    void PlayerWeaponKnife::OnStart() {
        Renderer & renderer = Engine::Get().GetRenderer();

        model = renderer.GetOrLoadAnimatedModel( "assets/player/arms/knife.glb" );
        animator.PlayAnimation( *model, "Armature|Knife_Idle_Anim", true );
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
        animator.PlayAnimation( *model, "Armature|Knife_Draw_Anim", false );
        isAttacking = false;
        isEquipped  = false;
        sndEquip.Play();
    }

    void PlayerWeaponKnife::OnUpdate( f32 dt, bool isMoving, bool isSprinting, bool isCrouching, FPSCamera & camera, GameMap & map ) {
        ATTO_ASSERT( animator.GetCurrentAnimation(), "knife animator has no animation" );

        // Finish equip animation before allowing any input
        if ( !isEquipped ) {
            if ( animator.IsFinished() ) {
                isEquipped = true;
                const char * idleAnim = !isMoving   ? "Armature|Knife_Idle_Anim"
                                      : isSprinting ? "Armature|Knife_Run_Anim"
                                      :               "Armature|Knife_Walk_Anim";
                animator.PlayAnimation( *model, idleAnim, true );
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
            animator.PlayAnimation( *model, "Armature|Knife_Attack_1_Anim", false );
            sndSwing1.Play( 0.5f );
            isAttacking = true;
        }

        if ( input.IsMouseButtonDown( MouseButton::Right ) && isIdleWalkOrRun && !isSprinting ) {
            animator.PlayAnimation( *model, "Armature|Knife_Attack_3_Anim", false );
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
                    constexpr i32 KnifeAttack1Damage = 34;
                    if ( result.entity->TakeDamage( KnifeAttack1Damage ) == TakeDamageResult::Success_HP ) {
                        didHitEntity = true;
                        AlignedBox bounds = result.entity->GetBounds();
                        Vec3 dmgPos = Vec3( result.entity->GetPosition().x, bounds.max.y + 0.2f, result.entity->GetPosition().z );
                        map.SpawnDamageNumber( dmgPos, KnifeAttack1Damage );
                    }
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
                    constexpr i32 KnifeAttack3Damage = 55;
                    if ( result.entity->TakeDamage( KnifeAttack3Damage ) == TakeDamageResult::Success_HP ) {
                        didHitEntity = true;
                        AlignedBox bounds = result.entity->GetBounds();
                        Vec3 dmgPos = Vec3( result.entity->GetPosition().x, bounds.max.y + 0.2f, result.entity->GetPosition().z );
                        map.SpawnDamageNumber( dmgPos, KnifeAttack3Damage );
                    }
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
            animator.PlayAnimation( *model, returnAnim, true );
        }

        // Locomotion transitions
        if ( !isAttacking ) {
            if ( !isMoving && curAnim != "Armature|Knife_Idle_Anim" ) {
                animator.PlayAnimation( *model, "Armature|Knife_Idle_Anim", true );
            }
            else if ( isMoving && isSprinting && curAnim != "Armature|Knife_Run_Anim" ) {
                animator.PlayAnimation( *model, "Armature|Knife_Run_Anim", true );
            }
            else if ( isMoving && !isSprinting && curAnim != "Armature|Knife_Walk_Anim" ) {
                animator.PlayAnimation( *model, "Armature|Knife_Walk_Anim", true );
            }
        }

        animator.Update( dt );
    }

    void PlayerWeaponKnife::OnRender( Renderer & renderer, const FPSCamera & camera ) {
        const Vec3 ArmsLocalOffset = Vec3( 0.0f, -0.15f, -0.06f );
        Mat4 cameraWorld     = glm::inverse( camera.GetViewMatrix() );
        Mat4 localCorrection = glm::rotate( Mat4( 1.0f ), PI, Vec3( 0.0f, 1.0f, 0.0f ) );
        Mat4 armsMatrix      = cameraWorld * glm::translate( Mat4( 1.0f ), ArmsLocalOffset ) * localCorrection;
        renderer.RenderAnimatedModel( *model, animator, armsMatrix );
    }

    // =========================================================================
    // PlayerWeaponGlock
    // =========================================================================

    void PlayerWeaponGlock::OnStart() {
        Renderer & renderer = Engine::Get().GetRenderer();
        model = renderer.GetOrLoadAnimatedModel( "assets/player/arms/glock.glb" );
        particleTextureSmoke = renderer.GetOrLoadTexture( "assets/textures/fx/synty/PolygonParticles_Smoke_01.png" );
        particleTextureTrace1 = renderer.GetOrLoadTexture( "assets/textures/fx/kenny/trace_06.png" );
        particleTextureTrace2 = renderer.GetOrLoadTexture( "assets/textures/fx/kenny/smoke_01.png" );
        animator.PlayAnimation( *model, "Armature|Glock_Idle_Anim", true );
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

        sndHit.Initialize( &Engine::Get().GetAudioSystem(), &Engine::Get().GetRNG() );
        sndHit.LoadSounds( {
            "bullet-hits/basic_hit_01.wav",
            "bullet-hits/basic_hit_02.wav",
            "bullet-hits/basic_hit_03.wav",
            "bullet-hits/basic_hit_04.wav",
            "bullet-hits/basic_hit_05.wav",
        } );
    }

    void PlayerWeaponGlock::OnEquip() {
        animator.PlayAnimation( *model, "Armature|Glock_Draw_Anim", false );
        isAttacking  = false;
        isEquipped   = false;
        reloadQueued = false;
        sndEquip.Play();
    }

    void PlayerWeaponGlock::OnUpdate( f32 dt, bool isMoving, bool isSprinting, bool isCrouching, FPSCamera & camera, GameMap & map ) {
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
                animator.PlayAnimation( *model, idleAnim, true );
            }
            animator.Update( dt );
            return;
        }

        bool isIdleWalkOrRun = ( curAnim == "Armature|Glock_Idle_Anim"
                               || curAnim == "Armature|Glock_Walk_Anim"
                               || curAnim == "Armature|Glock_Run_Anim" );
        bool isFiring = ( curAnim == "Armature|Glock_Fire_Anim" );

        // Fire
        if ( input.IsMouseButtonPressed( MouseButton::Left )
            && ( ( isIdleWalkOrRun && !isSprinting ) || ( isFiring && animator.GetPercentComplete() >= 0.125 ) )
            && !isReloading ) {
            if ( ammo > 0 ) {
                animator.PlayAnimation( *model, "Armature|Glock_Fire_Anim", false );
                isAttacking = true;

                ammo--;
                sndShoot.Play( 0.2f );

                // Accuracy spread based on player state
                // Crouching = perfect accuracy, standing still = small spread, walking = larger spread
                constexpr f32 SpreadStanding = 0.015f; // ~0.86 degrees
                constexpr f32 SpreadWalking  = 0.04f;  // ~2.3 degrees
                f32 spreadAmount = isCrouching ? 0.0f : ( isMoving ? SpreadWalking : SpreadStanding );

                Vec3 fireDir = camera.GetForward();
                if ( spreadAmount > 0.0f ) {
                    RNG & rng = Engine::Get().GetRNG();
                    Vec3 right = camera.GetRight();
                    Vec3 up    = camera.GetUp();
                    f32 offsetX = rng.Float( -spreadAmount, spreadAmount );
                    f32 offsetY = rng.Float( -spreadAmount, spreadAmount );
                    fireDir = glm::normalize( fireDir + right * offsetX + up * offsetY );
                }

                MapRaycastResult result;
                if ( map.Raycast( camera.GetPosition(), fireDir, result ) ) {
                    if ( result.entity && result.distance <= 50.0f ) {
                        LOG_INFO( "Glock hit: %s at distance: %f", EntityTypeToString( result.entity->GetType() ), result.distance );
                        constexpr i32 GlockDamage = 25;
                        if ( result.entity->TakeDamage( GlockDamage ) == TakeDamageResult::Success_HP ) {
                            sndHit.Play();
                            didHitEntity = true;
                            AlignedBox bounds = result.entity->GetBounds();
                            Vec3 dmgPos = Vec3( result.entity->GetPosition().x, bounds.max.y + 0.2f, result.entity->GetPosition().z );
                            map.SpawnDamageNumber( dmgPos, GlockDamage );
                        }
                    }

                    // Impact particles at hit point
                    Vec3 hitPoint = camera.GetPosition() + fireDir * result.distance;
                    Vec3 hitNormal = result.normal;
                    ParticleSystem & ps = map.GetParticleSystem();

                    // Impact sparks — velocity-aligned streaks that scatter off the surface
                    ParticleParms impactSparks;
                    impactSparks.position         = hitPoint + hitNormal * 0.02f;
                    impactSparks.positionVariance = Vec3( 0.01f );
                    impactSparks.velocity         = hitNormal * 2.0f;
                    impactSparks.velocityVariance = Vec3( 2.5f, 2.5f, 2.5f );
                    impactSparks.gravity          = Vec3( 0.0f, -6.0f, 0.0f );
                    impactSparks.drag             = 2.0f;
                    impactSparks.lifetime         = 0.2f;
                    impactSparks.lifetimeVariance = 0.1f;
                    impactSparks.startSize        = 0.04f * 10;
                    impactSparks.endSize          = 0.005f * 10;
                    impactSparks.startColor       = Color( 1.0f, 0.5f, 0.1f, 1.0f );
                    impactSparks.endColor         = Color( 0.8f, 0.15f, 0.0f, 0.0f );
                    impactSparks.texture          = particleTextureTrace1;
                    impactSparks.velocityAligned  = true;
                    impactSparks.stretchFactor    = 2.5f;
                    impactSparks.count            = 5;
                    ps.Emit( impactSparks );

                    // Impact dust/debris — slower, larger, fades out
                    ParticleParms impactDust;
                    impactDust.position         = hitPoint + hitNormal * 0.01f;
                    impactDust.positionVariance = Vec3( 0.03f );
                    impactDust.velocity         = hitNormal * 0.8f;
                    impactDust.velocityVariance = Vec3( 0.5f, 0.5f, 0.5f );
                    impactDust.gravity          = Vec3( 0.0f, 0.3f, 0.0f );
                    impactDust.drag             = 4.0f;
                    impactDust.lifetime         = 0.5f;
                    impactDust.lifetimeVariance = 0.2f;
                    impactDust.startSize        = 0.03f * 10;
                    impactDust.endSize          = 0.12f * 10;
                    impactDust.startColor       = Color( 0.9f, 0.85f, 0.7f, 0.6f );
                    impactDust.endColor         = Color( 0.5f, 0.5f, 0.5f, 0.0f );
                    impactDust.texture          = particleTextureTrace2;
                    impactDust.count            = 3;
                    ps.Emit( impactDust );
                }

                SpawnParticles( camera, map );
            }
            else {
                sndDry.Play();
            }
        }

        // R to reload manually
        if ( input.IsKeyPressed( Key::R ) && !isReloading && ammo < MaxAmmo ) {
            if ( isIdleWalkOrRun ) {
                animator.PlayAnimation( *model, "Armature|Glock_Reload_Anim", false );
                isReloading      = true;
                reloadQueued     = false;
                reloadSnd1Played = false;
                reloadSnd2Played = false;
                reloadSnd3Played = false;
            }
            else if ( isFiring ) {
                reloadQueued = true;
            }
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

        // Clear attack flag when fire animation completes
        if ( isFiring && animator.GetPercentComplete() >= 1.0f && isAttacking ) {
            isAttacking = false;
        }

        // Reload complete
        if ( isReloading && animator.IsFinished() ) {
            isReloading = false;
            ammo        = MaxAmmo;
            const char * returnAnim = !isMoving   ? "Armature|Glock_Idle_Anim"
                                    : isSprinting ? "Armature|Glock_Run_Anim"
                                    :               "Armature|Glock_Walk_Anim";
            animator.PlayAnimation( *model, returnAnim, true );
        }

        // Fire finished — start queued reload or return to locomotion
        if ( curAnim == "Armature|Glock_Fire_Anim" && animator.IsFinished() ) {
            isAttacking = false;
            if ( reloadQueued && ammo < MaxAmmo ) {
                animator.PlayAnimation( *model, "Armature|Glock_Reload_Anim", false );
                isReloading      = true;
                reloadQueued     = false;
                reloadSnd1Played = false;
                reloadSnd2Played = false;
                reloadSnd3Played = false;
            }
            else {
                reloadQueued = false;
                const char * returnAnim = !isMoving   ? "Armature|Glock_Idle_Anim"
                                        : isSprinting ? "Armature|Glock_Run_Anim"
                                        :               "Armature|Glock_Walk_Anim";
                animator.PlayAnimation( *model, returnAnim, true );
            }
        }

        // Locomotion transitions
        if ( !isAttacking && !isReloading ) {
            if ( !isMoving && curAnim != "Armature|Glock_Idle_Anim" ) {
                animator.PlayAnimation( *model, "Armature|Glock_Idle_Anim", true );
            }
            else if ( isMoving && isSprinting && curAnim != "Armature|Glock_Run_Anim" ) {
                animator.PlayAnimation( *model, "Armature|Glock_Run_Anim", true );
            }
            else if ( isMoving && !isSprinting && curAnim != "Armature|Glock_Walk_Anim" ) {
                animator.PlayAnimation( *model, "Armature|Glock_Walk_Anim", true );
            }
        }

        animator.Update( dt );
    }

    void PlayerWeaponGlock::OnRender( Renderer & renderer, const FPSCamera & camera ) {
        Vec3 ArmsLocalOffset = Vec3( 0.05f, -0.22f, -0.1f );

        Mat4 cameraWorld     = glm::inverse( camera.GetViewMatrix() );
        Mat4 localCorrection = glm::rotate( Mat4( 1.0f ), PI, Vec3( 0.0f, 1.0f, 0.0f ) );
        Mat4 armsMatrix      = cameraWorld * glm::translate( Mat4( 1.0f ), ArmsLocalOffset ) * localCorrection;
        renderer.RenderAnimatedModel( *model, animator, armsMatrix );
    }

    void PlayerWeaponGlock::SpawnParticles( FPSCamera & camera, GameMap & map ) {
        Vec3 forward = camera.GetForward();
        Vec3 right   = camera.GetRight();
        Vec3 up      = camera.GetUp();

        // Muzzle position: slightly forward and down-right from the camera
        Vec3 muzzlePos = camera.GetPosition()
                       + forward * 0.4f
                       + right   * 0.1f
                       + up      * -0.08f;

        ParticleSystem & ps = map.GetParticleSystem();

        // Muzzle smoke puff — expands and fades out
        ParticleParms smoke;
        smoke.position         = muzzlePos;
        smoke.positionVariance = Vec3( 0.02f );
        smoke.velocity         = forward * 1.5f + up * 0.4f;
        smoke.velocityVariance = Vec3( 0.3f, 0.3f, 0.3f );
        smoke.gravity          = Vec3( 0.0f, 0.2f, 0.0f );
        smoke.drag             = 3.0f;
        smoke.lifetime         = 0.4f;
        smoke.lifetimeVariance = 0.15f;
        smoke.startSize        = 0.04f;
        smoke.endSize          = 0.15f;
        smoke.startColor       = Color( 0.9f, 0.85f, 0.7f, 0.6f );
        smoke.endColor         = Color( 0.5f, 0.5f, 0.5f, 0.0f );
        smoke.texture          = particleTextureSmoke;
        smoke.count            = 6;
        ps.Emit( smoke );

        // Hot sparks — fast, small, bright
        ParticleParms sparks;
        sparks.position         = muzzlePos + forward * 0.05f;
        sparks.positionVariance = Vec3( 0.01f );
        sparks.velocity         = forward * 4.0f + up * 1.0f;
        sparks.velocityVariance = Vec3( 1.5f, 1.0f, 1.5f );
        sparks.gravity          = Vec3( 0.0f, -3.0f, 0.0f );
        sparks.drag             = 1.0f;
        sparks.lifetime         = 0.15f;
        sparks.lifetimeVariance = 0.08f;
        sparks.startSize        = 0.03f;
        sparks.endSize          = 0.005f;
        sparks.startColor       = Color( 1.0f, 0.9f, 0.4f, 1.0f );
        sparks.endColor         = Color( 1.0f, 0.3f, 0.0f, 0.0f );
        sparks.texture          = nullptr;
        sparks.count            = 4;
        ps.Emit( sparks );
    }
}
