#include "engine/atto_engine.h"
#include "engine/renderer/atto_render_model.h"

namespace atto {
    class EditorScene : public Scene<EditorScene> {
    public:
        static const char * GetSceneNameStatic() { return "Editor"; }
        const char * GetSceneName() override { return GetSceneNameStatic(); }

        void OnStart() override;
        void OnUpdate( f32 deltaTime ) override;
        void OnRender( Renderer & renderer ) override;
        void OnShutdown() override;
        void OnResize( i32 width, i32 height ) override;

    private:
        void StartImgui();

        FlyCamera  flyCamera;
        StaticModel model;
        Texture texture;
    };

} // namespace atto
