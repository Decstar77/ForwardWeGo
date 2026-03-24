#include "atto_scene.h"
#include "atto_engine.h"

using namespace atto;

int main() {
    Engine & engine = Engine::Get();

    EngineConfig config;
    config.windowTitle = "Forward We Go";
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.vsync = true;
    config.resizable = true;

    if ( !engine.Initialize( config ) ) {
        return -1;
    }

    RegisterAllScenes();
    engine.Run( "Editor" );

    engine.Shutdown();

    return 0;
}


/*
TODO:
Gameplay:
- Entity id's
- Animation blending
- ReservationTable
- player reload bug
- Coins for next run
- Explosive barrel
- Chest
- Level hazard
- Portal effect

Editor:
- SpawnID  selection
- Triangle selection

Engine:
- Animation blending
- Packaged builds, packing and loading assets.
- Remove editor from build
- Remove loading raw asset files from build, remove assimp/ audio file / stb include etc.

Rendering:
- MSAA
- Cavity
- Shadows
- SSAO
- Image based lighting
- Light maps
- Light probes
- ? PBR ?

https://www.fab.com/listings/8580e72a-1ac7-42b0-b76e-0ebbe0a922a8
*/