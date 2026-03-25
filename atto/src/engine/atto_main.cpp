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
- Player reload bug
- Trigger brushes
- Collision brushes

- Multiple waves
- Player dying

- Level timer ( unsure )
- Hands leak render models
- Baneling ( Friday )
- Level hazard ( unsure )
- Impact sounds:
    - Flesh bullet hit sound
    - Metal bullet hit sound
- Weapons:
    - Glock:
        - Spawn bullet shell
    - M416:
        -recoil
    - Sniper
    - Frag grenade
- Main menu:
    Option to choose run biome
    Button to weapons menu
    Button to player upgrades
    Display player coins ( top right )
    Options ( Top left gear icon ):
        Mouse Sensitivity ( Slider )
        Gameplay volume ( Slider )
        Music volume ( Slider )
- Player upgrades:
    - Health increase
    - Med kits do more healing
    - More grenade slots
    - Wider scoop radius
    - Faster sprint
    - Faster crouch
- Player cards:
    - Attack speed increased ( must )
    - Attack damage increased ( must )
    - Attack inacuracy increased ( must )
    - Magazine capacity increased ( must )
    - Reload speed increased ( must )
    - Health increased ( must )
    - Restore health ( must )
    - Extra coins ( must )
    - Slow enemies
    - Poison bullets
    - Explosive bullets
    - Reflection damage
    - Life steal

Editor:
- SpawnID  selection
- Triangle selection
- Better nav graph creation

Engine:
- Animation blending
- ReservationTable
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