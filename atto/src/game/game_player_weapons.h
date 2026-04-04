#pragma once

#include "engine/atto_engine.h"
#include "engine/renderer/atto_render_model.h"

namespace atto {

    class GameMap;

    class PlayerWeaponKnife {
    public:
        void OnStart();
        void OnEquip();
        void OnUpdate( f32 dt, bool isMoving, bool isSprinting, bool isCrouching, FPSCamera & camera, GameMap & map );
        void OnRender( Renderer & renderer, const FPSCamera & camera );

        bool IsAttacking() const { return isAttacking; }
        bool ConsumeHit() { bool h = didHitEntity; didHitEntity = false; return h; }

    private:
        const AnimatedModel *   model = nullptr;
        Animator                animator;
        bool                    isAttacking  = false;
        bool                    isEquipped   = true;
        bool                    didHitEntity = false;

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
        void OnUpdate( f32 dt, bool isMoving, bool isSprinting, bool isCrouching, FPSCamera & camera, GameMap & map );
        void OnRender( Renderer & renderer, const FPSCamera & camera );

        bool IsAttacking() const { return isAttacking; }
        i32  GetAmmo()     const { return ammo; }
        i32  GetMaxAmmo()  const;
        bool ConsumeHit() { bool h = didHitEntity; didHitEntity = false; return h; }

        void SpawnParticles( FPSCamera & camera, GameMap & map );

    private:
        const AnimatedModel   * model = nullptr;
        Animator                animator;
        const Texture *         particleTextureSmoke = nullptr;
        const Texture *         particleTextureTrace1 = nullptr;
        const Texture *         particleTextureTrace2 = nullptr;

        static constexpr i32 MaxAmmo = 12;

        bool            isAttacking      = false;
        bool            isEquipped       = false;
        bool            isReloading      = false;
        bool            didHitEntity     = false;
        bool            reloadQueued     = false;
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
        SoundCollection sndHit;
    };

}
