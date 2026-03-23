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
- remove player ADS
- Coins for next run
- Explosive barrel
- Chest
- Level hazard
- Roach attack sound

Editor:

Engine:
- Animation blending

Renderering:
- Shadows
- MSAA
- SSAO
- Image based lighting
- Light maps
- Light probes
- ? PBR ?

https://www.fab.com/listings/8580e72a-1ac7-42b0-b76e-0ebbe0a922a8
*/