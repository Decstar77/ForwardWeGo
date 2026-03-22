#pragma once

#include "engine/atto_engine.h"
#include "game_player_weapons.h"

namespace atto {

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
        i32                 GetHealth() const { return health; }
        bool                IsAlive() const { return health > 0; }

    private:
        FPSCamera           camera;
        Capsule             playerCapsule;
        PlayerWeaponKnife   knife;
        PlayerWeaponGlock   glock;
        WeaponSlot          activeWeapon = WeaponSlot::Knife;

        i32                 health           = 100;
        SoundCollection     sndFootsteps;
        f32                 footstepTimer    = 0.0f;
        f32                 footstepInterval = 0.6f;
    };

}
