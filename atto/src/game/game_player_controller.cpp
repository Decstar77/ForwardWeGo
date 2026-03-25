#include "game_player_controller.h"
#include "game_map.h"

namespace atto {
    void PlayerStart::Serialize( Serializer & serializer ) {
        serializer( "spawnPos", spawnPos );
        serializer( "spawnOri", spawnOri );
    }

    Capsule PlayerStart::GetCapsule() const {
        return Capsule::FromTips( spawnPos, Vec3( spawnPos.x, spawnPos.y + PlayerStandingHeight, spawnPos.z ), 0.4f );
    }

    void PlayerController::OnStart( const Vec3 & position ) {
        Vec2i windowSize = Engine::Get().GetWindowSize();

        camera.SetViewportSize( windowSize.x, windowSize.y );
        camera.SetPosition( Vec3( position.x, position.y + PlayerEyeHeight, position.z ) );
        camera.SetFOV( 60.0f );
        camera.SetMoveSpeed( 5.0f );
        camera.SetLookSensitivity( 0.1f );

        knife.OnStart();
        glock.OnStart();

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
    }

    void PlayerController::OnUpdate( f32 deltaTime, GameMap & map ) {
        if ( hitMarkerTimer > 0.0f ) {
            hitMarkerTimer = Max( hitMarkerTimer - deltaTime, 0.0f );
        }
        if ( damageVignetteTimer > 0.0f ) {
            damageVignetteTimer = Max( damageVignetteTimer - deltaTime, 0.0f );
        }

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

        constexpr f32 SprintSpeedMultiplier  = 1.8f;
        constexpr f32 FootstepIntervalWalk   = 0.6f;
        constexpr f32 FootstepIntervalSprint = 0.35f;

        // Crouch toggle
        if ( input.IsKeyDown( Key::LeftControl ) ) {
            isCrouching = true;
        } else {
            isCrouching = false;
        }


        bool isSprinting = input.IsKeyDown( Key::LeftShift ) || input.IsKeyDown( Key::RightShift );

        // Stand up when sprinting
        if ( isSprinting && isCrouching ) {
            isCrouching = false;
        }

        // Smoothly interpolate eye height and capsule height
        constexpr f32 CrouchSpeed        = 10.0f;
        constexpr f32 CrouchSpeedMult    = 0.5f;
        f32 targetEyeHeight = isCrouching ? PlayerCrouchEyeHeight : PlayerEyeHeight;
        f32 targetHeight    = isCrouching ? PlayerCrouchHeight    : PlayerStandingHeight;
        currentEyeHeight    = currentEyeHeight + (targetEyeHeight - currentEyeHeight) * Min( CrouchSpeed * deltaTime, 1.0f );
        currentHeight       = currentHeight    + (targetHeight    - currentHeight)    * Min( CrouchSpeed * deltaTime, 1.0f );

        f32 speedMult = isSprinting ? SprintSpeedMultiplier : ( isCrouching ? CrouchSpeedMult : 1.0f );
        f32  speed    = camera.GetMoveSpeed() * speedMult * deltaTime;

        bool isMoving = false;
        if ( input.IsKeyDown( Key::W ) ) { camera.MoveForward( speed );  isMoving = true; }
        if ( input.IsKeyDown( Key::S ) ) { camera.MoveForward( -speed ); isMoving = true; }
        if ( input.IsKeyDown( Key::D ) ) { camera.MoveRight( speed );    isMoving = true; }
        if ( input.IsKeyDown( Key::A ) ) { camera.MoveRight( -speed );   isMoving = true; }

        // Weapon switching
        if ( ( input.IsKeyPressed( Key::Num1 ) || input.IsKeyPressed( Key::Num3 ) ) && activeWeapon != WeaponSlot::Knife ) {
            activeWeapon = WeaponSlot::Knife;
            knife.OnEquip();
        }
        if ( input.IsKeyPressed( Key::Num2 )  && activeWeapon != WeaponSlot::Glock ) {
            activeWeapon = WeaponSlot::Glock;
            glock.OnEquip();
        }

        if ( activeWeapon == WeaponSlot::Knife ) {
            knife.OnUpdate( deltaTime, isMoving, isSprinting, isCrouching, camera, map );
            if ( knife.ConsumeHit() ) { ShowHitMarker(); }
        }
        else {
            glock.OnUpdate( deltaTime, isMoving, isSprinting, isCrouching, camera, map );
            if ( glock.ConsumeHit() ) { ShowHitMarker(); }
        }

        // Apply eye height (handles crouch smoothly)
        Vec3 camPos = camera.GetPosition();
        camPos.y = currentEyeHeight;
        camera.SetPosition( camPos );

        Vec3 playerPos = camera.GetPosition();
        playerPos.y    = 0.0f;
        playerCapsule  = Capsule::FromTips( playerPos, playerPos + Vec3( 0, currentHeight, 0 ), 0.3f );

        Vec3 correction = map.ResolvePlayerCollision( playerCapsule );
        correction.y    = 0.0f;
        if ( correction.x != 0.0f || correction.z != 0.0f ) {
            camPos      = camera.GetPosition();
            camPos     += correction;
            camera.SetPosition( camPos );

            playerPos     = camPos;
            playerPos.y   = 0.0f;
            playerCapsule = Capsule::FromTips( playerPos, playerPos + Vec3( 0, currentHeight, 0 ), 0.3f );
        }

        footstepInterval = isSprinting ? FootstepIntervalSprint : FootstepIntervalWalk;

        if ( isMoving ) {
            footstepTimer += deltaTime;
            if ( footstepTimer >= footstepInterval ) {
                footstepTimer -= footstepInterval;
                sndFootsteps.Play( 1.0f );
            }
        }
        else {
            footstepTimer = 0.0f;
        }
    }

    void PlayerController::TakeDamage( i32 damage ) {
        health -= damage;
        if ( health < 0 ) {
            health = 0;
        }
        damageVignetteTimer = DamageVignetteDuration;
        LOG_INFO( "Player took %d damage, health: %d", damage, health );
    }

    void PlayerController::Heal( i32 amount ) {
        health += amount;
        if ( health > 100 ) {
            health = 100;
        }
        LOG_INFO( "Player healed %d, health: %d", amount, health );
    }

    void PlayerController::OnRender( Renderer & renderer ) {
        renderer.ClearDepthBuffer();
        if ( activeWeapon == WeaponSlot::Knife ) {
            knife.OnRender( renderer, camera );
        }
        else {
            glock.OnRender( renderer, camera );
        }
    }

    void PlayerController::OnResize( i32 width, i32 height ) {
        camera.SetViewportSize( width, height );
    }

}
