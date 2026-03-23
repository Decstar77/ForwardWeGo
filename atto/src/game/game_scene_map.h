#include "engine/atto_engine.h"
#include "engine/renderer/atto_render_model.h"
#include "engine/renderer/atto_render_material.h"
#include "game/game_map.h"
#include "game/game_player_controller.h"

namespace atto {

    class GameMapScene : public SceneInterface {
    public:
        static const char * GetSceneNameStatic() { return "GameMapScene"; }
        const char * GetSceneName() override { return GetSceneNameStatic(); }

        void OnStart( const char * args ) override;
        void OnUpdate( f32 deltaTime ) override;
        void OnRender( Renderer & renderer ) override;
        void OnShutdown() override;
        void OnResize( i32 width, i32 height ) override;

    private:
        GameMap             map;
        PlayerController    player;
        UICanvas            ui;
        const Texture *     coinTexture      = nullptr;
        const Texture *     crosshairTexture = nullptr;
        const Texture *     hitMarkerTexture = nullptr;
        const Font *        hudFont          = nullptr;
        const Font *        hudFontSmall     = nullptr;
        f32                 fps              = 0.0f;
    };

}
