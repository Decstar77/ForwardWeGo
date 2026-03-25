#include "engine/atto_engine.h"
#include "engine/renderer/atto_render_model.h"
#include "engine/renderer/atto_render_material.h"

namespace atto {
    class GameScenePickCard : public SceneInterface {
    public:
        static const char * GetSceneNameStatic() { return "GameScenePickCard"; }
        const char * GetSceneName() override { return GetSceneNameStatic(); }

        void OnStart( const char * args ) override;
        void OnUpdate( f32 deltaTime ) override;
        void OnRender( Renderer &renderer ) override;
        void OnShutdown() override;
        void OnResize( i32 width, i32 height ) override;

    private:
        std::string nextMap = "";
    };
}
