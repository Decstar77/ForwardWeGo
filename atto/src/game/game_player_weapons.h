#pragma once

#include "engine/atto_engine.h"
#include "engine/renderer/atto_render_model.h"

namespace atto {

    class GameMap;

    class PlayerWeaponKnife {
    public:
        void OnStart();
        void OnUpdate( f32 dt, bool isMoving, bool isSprinting, FPSCamera & camera, GameMap & map );
        void OnRender( Renderer & renderer, const FPSCamera & camera );

        bool IsAttacking() const { return isAttacking; }

    private:
        AnimatedModel   model;
        Animator        animator;
        bool            isAttacking = false;

        SoundCollection sndSwing1;
        SoundCollection sndSwing2;
        SoundCollection sndHitMetal1;
        SoundCollection sndHitMetal2;
    };

    class PlayerWeaponGlock {
    public:
        void OnStart();
        void OnDraw();
        void OnUpdate( f32 dt, bool isMoving, bool isSprinting, FPSCamera & camera, GameMap & map );
        void OnRender( Renderer & renderer, const FPSCamera & camera );

        bool IsAttacking() const { return isAttacking; }

    private:
        AnimatedModel   model;
        Animator        animator;
        bool            isAttacking = false;
        bool            isDrawn     = false;
    };

}
