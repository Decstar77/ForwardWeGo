#pragma once

#include "engine/atto_engine.h"
#include "engine/renderer/atto_render_model.h"

namespace atto {

    class GameMap;

    class PlayerWeaponKnife {
    public:
        void OnStart();
        void OnEquip();
        void OnUpdate( f32 dt, bool isMoving, bool isSprinting, FPSCamera & camera, GameMap & map );
        void OnRender( Renderer & renderer, const FPSCamera & camera );

        bool IsAttacking() const { return isAttacking; }

    private:
        AnimatedModel   model;
        Animator        animator;
        bool            isAttacking = false;
        bool            isEquipped  = true;

        SoundCollection sndEquip;
        SoundCollection sndSwing1;
        SoundCollection sndSwing2;
        SoundCollection sndHitMetal1;
        SoundCollection sndHitMetal2;
    };

    class PlayerWeaponGlock {
    public:
        void OnStart();
        void OnEquip();
        void OnUpdate( f32 dt, bool isMoving, bool isSprinting, FPSCamera & camera, GameMap & map );
        void OnRender( Renderer & renderer, const FPSCamera & camera );

        bool IsAttacking() const { return isAttacking; }

    private:
        AnimatedModel   model;
        Animator        animator;
        static constexpr i32 MaxAmmo = 12;

        bool            isAttacking      = false;
        bool            isEquipped       = false;
        bool            isReloading      = false;
        bool            isADS            = false;
        bool            reloadSnd1Played = false;
        bool            reloadSnd2Played = false;
        bool            reloadSnd3Played = false;
        i32             ammo             = MaxAmmo;

        SoundCollection sndEquip;
        SoundCollection sndShoot;
        SoundCollection sndDry;
        SoundCollection sndCock;
        SoundCollection sndRemoveMag;
        SoundCollection sndInsertMag;
    };

}
