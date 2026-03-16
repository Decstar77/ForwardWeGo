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
 - Fix materials and colors
 - Add textures to brushes
 - Entity id's
 - Entities should be marked for removed/destroyed and them cleaned up at the end of the frame

 - Need an undo

 - Shadows
 - MSAA
 - SSAO
 - Image based lighting
 - Light maps
 - Light probes
 - ? PBR ?
*/