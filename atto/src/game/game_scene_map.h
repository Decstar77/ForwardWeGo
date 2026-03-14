#include "engine/atto_engine.h"
#include "engine/renderer/atto_render_model.h"
#include "engine/renderer/atto_render_material.h"
#include "game/game_map.h"

namespace atto {

    constexpr f32 PlayerHeight = 2.0f;
    constexpr f32 PlayerEyeHeight = 1.8f;

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
        Animator animator;
        AnimatedModel   playerHands;
        FPSCamera       camera;
    };

}
