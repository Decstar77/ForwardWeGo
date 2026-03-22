/*
    Atto Engine - Main Engine Implementation
*/

#include "atto_engine.h"
#include "atto_audio.h"

#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <cstdio>
#include <filesystem>

#include <stb_image/std_image.h>

namespace atto {

    void Color::Serialize( Serializer & serializer ) {
        serializer( "r", r );
        serializer( "g", g );
        serializer( "b", b );
        serializer( "a", a );
    }

    f32 RandomFloat() {
        return Engine::Get().GetRNG().Float();
    }

    Engine & Engine::Get() {
        static Engine instance;
        return instance;
    }

    bool Engine::Initialize( const EngineConfig & config ) {
        if ( initialized ) {
            LOG_WARN( "Engine already initialized" );
            return false;
        }

        // Initialize logger first
        Logger::Get().Initialize();

        // Initialize GLFW
        glfwSetErrorCallback( OnErrorCallback );

        if ( !glfwInit() ) {
            LOG_FATAL( "Failed to initialize GLFW" );
            return false;
        }

        // Configure OpenGL context
        glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
        glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
        glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
        glfwWindowHint( GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE );

#ifdef __APPLE__
        glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
#endif

        // Store windowed size
        windowedWidth = config.windowWidth;
        windowedHeight = config.windowHeight;

        // Create window
        GLFWmonitor * monitor = config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
        isFullscreen = config.fullscreen;

        window = glfwCreateWindow( config.windowWidth, config.windowHeight, config.windowTitle, monitor, nullptr );

        if ( !window ) {
            LOG_FATAL( "Failed to create GLFW window" );
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent( window );
        glfwSwapInterval( config.vsync ? 1 : 0 );

        // Store engine pointer for callbacks
        glfwSetWindowUserPointer( window, this );

        // Set up callbacks
        glfwSetKeyCallback( window, OnKeyCallback );
        glfwSetMouseButtonCallback( window, OnMouseButtonCallback );
        glfwSetCursorPosCallback( window, OnMouseMoveCallback );
        glfwSetScrollCallback( window, OnScrollCallback );
        glfwSetFramebufferSizeCallback( window, OnResizeCallback );

#if EMSCRIPTEN
        // Nothing 
#else 
        // Initialize GLAD
        if ( !gladLoadGLLoader( (GLADloadproc)glfwGetProcAddress ) ) {
            LOG_FATAL( "Failed to initialize GLAD" );
            glfwDestroyWindow( window );
            glfwTerminate();
            return false;
        }

        LOG_INFO( "Old working directory: %s", std::filesystem::current_path().string().c_str() );
        // Change the current working directory to "assets"
        //std::filesystem::current_path( "../" );
        //LOG_INFO( "New working directory: %s", std::filesystem::current_path().string().c_str() );
#endif

        // Set window icon
        {
            int w, h, channels;
            unsigned char* pixels = stbi_load("assets/textures/ai-gen/gothgirl-square.png", &w, &h, &channels, 4);
            if (pixels) {
                GLFWimage icon;
                icon.width = w;
                icon.height = h;
                icon.pixels = pixels;
                glfwSetWindowIcon(window, 1, &icon);
                stbi_image_free(pixels);
            }
        }

        // Print OpenGL info
        LOG_INFO( "OpenGL Version: %s", glGetString( GL_VERSION ) );
        LOG_INFO( "OpenGL Renderer: %s", glGetString( GL_RENDERER ) );

        // Initialize subsystems
        rng.Initialize();
        input.Initialize( window );
        assetManager.Initialize();
        audioSystem.Initialize();

        if ( !renderer.Initialize() ) {
            LOG_FATAL( "Failed to initialize renderer" );
            system("PAUSE");
            glfwDestroyWindow( window );
            glfwTerminate();
            return false;
        }

        lastFrameTime = glfwGetTime();
        initialized = true;

        LOG_INFO( "Atto Engine initialized successfully" );

        return true;
    }

#ifdef EMSCRIPTEN
    static void WebMainLoop() {
        Engine & engine = Engine::Get();
        engine.MainLoop();

        if ( engine.IsRunning() == false ) {
            emscripten_cancel_main_loop();
        }
    }
#endif

    void Engine::MainLoop() {
        // Calculate delta time
        f64 currentTime = glfwGetTime();
        deltaTime = static_cast<f32>(currentTime - lastFrameTime);
        lastFrameTime = currentTime;
        time = static_cast<f32>(currentTime);

        // FPS calculation
        fpsAccumulator += deltaTime;
        frameCount++;
        if ( fpsAccumulator >= 1.0f ) {
            fps = static_cast<f32>(frameCount) / fpsAccumulator;
            fpsAccumulator = 0.0f;
            frameCount = 0;
        }

        // Don't explode on huge lag spike.
        deltaTime = Clamp( deltaTime, 0.0f, 0.2f );

        glfwPollEvents();

        // Handle fullscreen toggle
        if ( (input.IsKeyDown( Key::LeftShift ) || input.IsKeyDown( Key::RightShift )) && input.IsKeyPressed( Key::Enter ) ) {
            ToggleFullscreen();
        }

        audioSystem.Update();

        // Update game
        if ( currentScene ) {
            currentScene->OnUpdate( deltaTime );
        }

        renderer.BeginFrame();

        if ( currentScene ) {
            currentScene->OnRender( renderer );
        }

        renderer.EndFrame();

        // Swap buffers
        glfwSwapBuffers( window );

        // Defered input because of web stuff
        input.Update();

        if ( transitionSceneName.empty() == false ) {
            currentScene->OnShutdown();
            currentScene = SceneRegistry::CreateNew( transitionSceneName.c_str() );
            currentScene->OnStart( transitionSceneArgs.c_str() );
            transitionSceneName = "";
            transitionSceneArgs = "";
        }
    }

    void Engine::Run( const char * startingSceneName ) {
        if ( !initialized ) {
            LOG_ERROR( "Engine not initialized" );
            return;
        }

        currentScene = SceneRegistry::CreateNew( startingSceneName );
        running = true;

        // Call game start
        if ( currentScene ) {
            currentScene->OnStart( nullptr );
        }

        // Main loop
#ifdef EMSCRIPTEN
        emscripten_set_main_loop( WebMainLoop, 0, 1 );
#else
        while ( running && !glfwWindowShouldClose( window ) ) {
            MainLoop();
        }
#endif

        // Call game shutdown
        if ( currentScene ) {
            currentScene->OnShutdown();
        }

        currentScene = nullptr;
    }

    void Engine::SetFullscreen( bool fullscreen ) {
        if ( fullscreen == isFullscreen ) return;

        if ( fullscreen ) {
            // Save windowed position and size
            glfwGetWindowPos( window, &windowedPosX, &windowedPosY );
            glfwGetWindowSize( window, &windowedWidth, &windowedHeight );

            // Switch to fullscreen
            GLFWmonitor * monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode * mode = glfwGetVideoMode( monitor );
            glfwSetWindowMonitor( window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate );
        }
        else {
            // Switch back to windowed
            glfwSetWindowMonitor( window, nullptr, windowedPosX, windowedPosY, windowedWidth, windowedHeight, 0 );
        }

        isFullscreen = fullscreen;
    }

    void Engine::ToggleFullscreen() {
        SetFullscreen( !isFullscreen );
    }

    void Engine::Shutdown() {
        if ( !initialized ) return;

        renderer.Shutdown();

        if ( window ) {
            glfwDestroyWindow( window );
            window = nullptr;
        }

        glfwTerminate();

        initialized = false;
        LOG_INFO( "Atto Engine shutdown" );

        // Shutdown logger last
        Logger::Get().Shutdown();
    }

    void Engine::TransitionToScene( const char * sceneName, const char * args ) {
        transitionSceneName = sceneName;
        transitionSceneArgs = args;
    }

    void Engine::SetWindowTitle( const char * title ) {
        if ( window ) {
            glfwSetWindowTitle( window, title );
        }
    }

    Vec2i Engine::GetWindowSize() const {
        Vec2i size( 0, 0 );
        if ( window ) {
            glfwGetWindowSize( window, &size.x, &size.y );
        }
        return size;
    }

    bool Engine::IsWindowFocused() const {
        return window && glfwGetWindowAttrib( window, GLFW_FOCUSED );
    }

    void Engine::RequestQuit() {
        running = false;
    }

    bool Engine::IsCloseRequested() const {
        return window && glfwWindowShouldClose( window );
    }

    void Engine::CancelCloseRequest() {
        if ( window ) {
            glfwSetWindowShouldClose( window, 0 );
        }
    }

    // GLFW Callbacks
    void Engine::OnKeyCallback( GLFWwindow * win, i32 key, i32 scancode, i32 action, i32 mods ) {
        Engine * engine = static_cast<Engine *>(glfwGetWindowUserPointer( win ));
        if ( engine ) {
            engine->input.OnKey( key, action );
        }
    }

    void Engine::OnMouseButtonCallback( GLFWwindow * win, i32 button, i32 action, i32 mods ) {
        Engine * engine = static_cast<Engine *>(glfwGetWindowUserPointer( win ));
        if ( engine ) {
            engine->input.OnMouseButton( button, action );
        }
    }

    void Engine::OnMouseMoveCallback( GLFWwindow * win, f64 x, f64 y ) {
        Engine * engine = static_cast<Engine *>(glfwGetWindowUserPointer( win ));
        if ( engine ) {
            engine->input.OnMouseMove( x, y );
        }
    }

    void Engine::OnScrollCallback( GLFWwindow * win, f64 xoffset, f64 yoffset ) {
        Engine * engine = static_cast<Engine *>(glfwGetWindowUserPointer( win ));
        if ( engine ) {
            engine->input.OnScroll( yoffset );
        }
    }

    void Engine::OnResizeCallback( GLFWwindow * win, i32 width, i32 height ) {
        Engine * engine = static_cast<Engine *>(glfwGetWindowUserPointer( win ));
        if ( engine ) {
            if ( engine->currentScene ) {
                engine->currentScene->OnResize( width, height );
            }
        }
    }

    void Engine::OnErrorCallback( i32 error, const char * description ) {
        LOG_ERROR( "GLFW Error (%d): %s", error, description );
    }

} // namespace atto
