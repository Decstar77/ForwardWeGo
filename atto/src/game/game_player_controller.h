#pragma once

#include "engine/atto_engine.h"
#include "game_player_weapons.h"

namespace atto {

    constexpr f32 PlayerStandingHeight   = 1.8f;
    constexpr f32 PlayerEyeHeight        = 1.7f;
    constexpr f32 PlayerCrouchHeight     = 1.0f;
    constexpr f32 PlayerCrouchEyeHeight  = 0.9f;

    struct PlayerStart {
        Vec3 spawnPos;
        Mat3 spawnOri;

        void Serialize( Serializer & serializer );
        Capsule GetCapsule() const;
    };

    class GameMap;

    enum class WeaponSlot { Knife, Glock };

    class PlayerController {
    public:
        void OnStart( const Vec3 & position );
        void OnUpdate( f32 deltaTime, GameMap & map );
        void OnRender( Renderer & renderer );
        void OnResize( i32 width, i32 height );

        FPSCamera &         GetCamera() { return camera; }
        const FPSCamera &   GetCamera() const { return camera; }

        WeaponSlot                  GetActiveWeapon() const { return activeWeapon; }
        const PlayerWeaponGlock &   GetGlock()        const { return glock; }

        void                TakeDamage( i32 damage );
        void                Heal( i32 amount );
        i32                 GetHealth() const { return health; }
        bool                IsAlive() const { return health > 0; }

        void                ShowHitMarker() { hitMarkerTimer = HitMarkerDuration; }
        f32                 GetHitMarkerAlpha() const { return hitMarkerTimer > 0.0f ? hitMarkerTimer / HitMarkerDuration : 0.0f; }

        f32                 GetDamageVignetteAlpha() const { return damageVignetteTimer > 0.0f ? damageVignetteTimer / DamageVignetteDuration : 0.0f; }

    private:
        FPSCamera           camera;
        Capsule             playerCapsule = {};
        PlayerWeaponKnife   knife;
        PlayerWeaponGlock   glock;
        PlayerWeaponM416    m416;
        WeaponSlot          activeWeapon = WeaponSlot::Knife;

        i32                 health           = 100;
        static constexpr f32 HitMarkerDuration = 0.2f;
        f32                 hitMarkerTimer   = 0.0f;
        static constexpr f32 DamageVignetteDuration = 0.6f;
        f32                 damageVignetteTimer = 0.0f;
        bool                isCrouching      = false;
        f32                 currentEyeHeight = PlayerEyeHeight;
        f32                 currentHeight    = PlayerStandingHeight;
        SoundCollection     sndFootsteps;
        f32                 footstepTimer    = 0.0f;
        f32                 footstepInterval = 0.6f;
    };

}
