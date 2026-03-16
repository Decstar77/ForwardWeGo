#include "atto_scene.h"
#include "atto_engine.h"

using namespace atto;

int main() {
    Engine & engine = Engine::Get();

    EngineConfig config;
    config.windowTitle = "Foward We Go";
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.vsync = true;
    config.resizable = true;

    if ( !engine.Initialize( config ) ) {
        return -1;
    }

    engine.Run( "Editor" );

    engine.Shutdown();

    return 0;
}


/*
TODO:

Gameplay:
- Entity id's
- Entities should be marked for removed/destroyed and them cleaned up at the end of the frame

Editor:
- Need an undo

- Add textures on brushes and materials
- Shadows
 - MSAA
 - SSAO
 - Image based lighting
 - Light maps
 - Light probes
 - ? PBR ?
*/