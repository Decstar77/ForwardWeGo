#pragma once

#include "engine/atto_engine.h"

namespace atto {

    class GameSceneMainMenu : public SceneInterface {
    public:
        static const char * GetSceneNameStatic() { return "GameSceneMainMenu"; }
        const char * GetSceneName() override { return GetSceneNameStatic(); }

        void OnUpdate( f32 deltaTime ) override;
        void OnRender( Renderer &renderer ) override;
        void OnShutdown() override;
        void OnResize( i32 width, i32 height ) override;

    private:
    };

}


