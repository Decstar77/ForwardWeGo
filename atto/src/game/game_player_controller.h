#pragma once

#include "engine/atto_engine.h"
#include "engine/renderer/atto_render_model.h"

namespace atto {

    constexpr Vec3 ArmsLocalOffset = Vec3( 0.0f, -0.15f, -0.06f );
    constexpr f32  ArmsScale       = 0.01f;

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
        FPSCamera       camera;
        Animator        animator;
        AnimatedModel   playerHands;
        Capsule         playerCapsule;
        bool            playerIsAttacking = false;

        SoundCollection sndFootsteps;
        SoundCollection sndKnifeSwing1;
        SoundCollection sndKnifeSwing2;
        SoundCollection sndKnifeHitMetal1;
        SoundCollection sndKnifeHitMetal2;
        f32             footstepTimer    = 0.0f;
        f32             footstepInterval = 0.6f;
    };

}
