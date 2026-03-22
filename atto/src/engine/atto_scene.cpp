#include "atto_scene.h"
#include "../editor/editor_scene.h"
#include "../game/game_scene_map.h"

namespace atto {
    void RegisterAllScenes() {
        SceneRegistry::Register<EditorScene>();
        SceneRegistry::Register<GameMapScene>();
    }
}
