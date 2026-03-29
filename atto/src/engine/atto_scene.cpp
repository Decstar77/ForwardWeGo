#include "atto_scene.h"
#include "../editor/editor_scene.h"
#include "../game/game_scene_map.h"
#include "game/game_scene_pick_card.h"
#include "game/game_scene_main_menu.h"

namespace atto {
    void RegisterAllScenes() {
#if ATTO_EDITOR
        SceneRegistry::Register<EditorScene>();
#endif
        SceneRegistry::Register<GameMapScene>();
        SceneRegistry::Register<GameScenePickCard>();
        SceneRegistry::Register<GameSceneMainMenu>();
    }
}
