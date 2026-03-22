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

        bool isSprinting = input.IsKeyDown( Key::LeftShift ) || input.IsKeyDown( Key::RightShift );
        f32  speed       = camera.GetMoveSpeed() * ( isSprinting ? SprintSpeedMultiplier : 1.0f ) * deltaTime;

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
            knife.OnUpdate( deltaTime, isMoving, isSprinting, camera, map );
        }
        else {
            glock.OnUpdate( deltaTime, isMoving, isSprinting, camera, map );
        }

        Vec3 playerPos = camera.GetPosition();
        playerPos.y    = 0.0f;
        playerCapsule  = Capsule::FromTips( playerPos, playerPos + Vec3( 0, PlayerHeight, 0 ), 0.3f );

        Vec3 correction = map.ResolvePlayerCollision( playerCapsule );
        correction.y    = 0.0f;
        if ( correction.x != 0.0f || correction.z != 0.0f ) {
            Vec3 camPos = camera.GetPosition();
            camPos     += correction;
            camera.SetPosition( camPos );

            playerPos     = camPos;
            playerPos.y   = 0.0f;
            playerCapsule = Capsule::FromTips( playerPos, playerPos + Vec3( 0, PlayerHeight, 0 ), 0.3f );
        }

        footstepInterval = isSprinting ? FootstepIntervalSprint : FootstepIntervalWalk;

        if ( isMoving ) {
            footstepTimer += deltaTime;
            if ( footstepTimer >= footstepInterval ) {
                footstepTimer -= footstepInterval;
                sndFootsteps.Play( 0.5f );
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
        LOG_INFO( "Player took %d damage, health: %d", damage, health );
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
