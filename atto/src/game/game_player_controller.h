#pragma once

#include "engine/atto_engine.h"
#include "game_player_weapons.h"

namespace atto {

    class GameMap;

    class PlayerController {
    public:
        void OnStart( const Vec3 & position );
        void OnUpdate( f32 deltaTime, GameMap & map );
        void OnRender( Renderer & renderer );
        void OnResize( i32 width, i32 height );

        FPSCamera &         GetCamera() { return camera; }
        const FPSCamera &   GetCamera() const { return camera; }

    private:
        FPSCamera           camera;
        Capsule             playerCapsule;
        PlayerWeaponKnife   knife;

        SoundCollection     sndFootsteps;
        f32                 footstepTimer    = 0.0f;
        f32                 footstepInterval = 0.6f;
    };

}
