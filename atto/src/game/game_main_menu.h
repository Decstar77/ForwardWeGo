#include "engine/atto_engine.h"
#include "engine/renderer/atto_render_model.h"

namespace atto {

    class MainMenuScene : public Scene<MainMenuScene> {
    public:
        static const char * GetSceneNameStatic() { return "MainMenu"; }
        const char * GetSceneName() override { return GetSceneNameStatic(); }

        void OnStart() override;
        void OnUpdate( f32 deltaTime ) override;
        void OnRender( Renderer & renderer ) override;
        void OnShutdown() override;
        void OnResize( i32 width, i32 height ) override;

    private:
        enum class CameraMode { Fly, FPS };

        CameraMode cameraMode = CameraMode::Fly;
        FlyCamera  flyCamera;
        FPSCamera  fpsCamera;
        StaticModel model;
    };

} // namespace atto
