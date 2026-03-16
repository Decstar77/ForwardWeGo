#include "engine/atto_engine.h"
#include "engine/renderer/atto_render_model.h"
#include "engine/renderer/atto_render_material.h"
#include "game/game_map.h"

namespace atto {

    // Viewmodel tuning: offset in camera-local space (+X=right, +Y=up, -Z=forward), uniform scale
    //constexpr Vec3 ArmsLocalOffset = Vec3( 0.15f, -0.25f, -0.3f );
    constexpr Vec3 ArmsLocalOffset = Vec3( 0.0f, -0.15f, -0.06f );
    constexpr f32  ArmsScale       = 0.01f;

    class GameMapScene : public Scene<GameMapScene> {
    public:
        static const char * GetSceneNameStatic() { return "GameMapScene"; }
        const char * GetSceneName() override { return GetSceneNameStatic(); }

        void OnStart() override;
        void OnUpdate( f32 deltaTime ) override;
        void OnRender( Renderer & renderer ) override;
        void OnShutdown() override;
        void OnResize( i32 width, i32 height ) override;

    private:
        GameMap         map;
        
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
