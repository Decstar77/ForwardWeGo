#pragma once

/*
    Atto Engine - Main Engine Class
    Manages window, rendering, input, and game loop
*/

#include "atto_core.h"
#include "atto_math.h"
#include "atto_log.h"
#include "atto_input.h"
#include "atto_camera.h"
#include "atto_assets.h"
#include "atto_rng.h"
#include "atto_scene.h"
#include "atto_audio.h"
#include "atto_containers.h"
#include "atto_renderer.h"

struct GLFWwindow;

namespace atto {

    // Engine configuration
    struct EngineConfig {
        const char * windowTitle = "Atto Engine";
        i32 windowWidth = 1280;
        i32 windowHeight = 720;
        bool vsync = true;
        bool resizable = true;
        bool fullscreen = false;
    };

    // Main engine class
    class Engine {
    public:
        static Engine & Get();

        bool Initialize( const EngineConfig & config );
        void Run( const char * startingSceneName );
        void MainLoop();
        void Shutdown();

        // Scenes
        void TransitionToScene( const char * sceneName, const char * args );

        // Window
        void SetWindowTitle( const char * title );
        Vec2i GetWindowSize() const;
        bool IsWindowFocused() const;
        void RequestQuit();

        // Fullscreen
        void SetFullscreen( bool fullscreen );
        void ToggleFullscreen();
        bool IsFullscreen() const { return isFullscreen; }

        // Time
        f32 GetDeltaTime() const { return deltaTime; }
        f32 GetTime() const { return time; }
        f32 GetFPS() const { return fps; }

        // Subsystems
        RNG & GetRNG() { return rng; }
        Input & GetInput() { return input; }
        AssetManager & GetAssetManager() { return assetManager; }
        AudioSystem & GetAudioSystem() { return audioSystem; }
        Renderer & GetRenderer() { return renderer; }
        bool IsRunning() const { return running; }

        GLFWwindow * GetWindowHandle() const { return window; }

    private:
        Engine() = default;
        ~Engine() = default;
        Engine( const Engine & ) = delete;
        Engine & operator=( const Engine & ) = delete;

        // GLFW callbacks
        static void OnKeyCallback( GLFWwindow * window, i32 key, i32 scancode, i32 action, i32 mods );
        static void OnMouseButtonCallback( GLFWwindow * window, i32 button, i32 action, i32 mods );
        static void OnMouseMoveCallback( GLFWwindow * window, f64 x, f64 y );
        static void OnScrollCallback( GLFWwindow * window, f64 xoffset, f64 yoffset );
        static void OnResizeCallback( GLFWwindow * window, i32 width, i32 height );
        static void OnErrorCallback( i32 error, const char * description );

        GLFWwindow * window = nullptr;
        bool running = false;
        bool initialized = false;

        // Fullscreen state
        bool isFullscreen = false;
        i32 windowedPosX = 100;
        i32 windowedPosY = 100;
        i32 windowedWidth = 1280;
        i32 windowedHeight = 720;

        // Timing
        f32 deltaTime = 0.0f;
        f32 time = 0.0f;
        f32 fps = 0.0f;
        f64 lastFrameTime = 0.0;

        // FPS calculation
        f32 fpsAccumulator = 0.0f;
        i32 frameCount = 0;

        // Subsystems
        RNG             rng;
        Input           input;
        AssetManager    assetManager; 
        AudioSystem     audioSystem;
        Renderer        renderer;

        // Current game
        std::unique_ptr<SceneInterface> currentScene = nullptr;
        std::string                     transitionSceneName;
        std::string                     transitionSceneArgs = "";
    };
} // namespace atto
