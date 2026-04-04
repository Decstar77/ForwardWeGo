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

#if ATTO_SHIPPING
    config.packedAssets = true;
#else
    config.packedAssets = false;
#endif

    if ( !engine.Initialize( config ) ) {
        return -1;
    }

    RegisterAllScenes();
#if ATTO_SHIPPING
    engine.Run( "GameSceneMainMenu", "assets/maps/game/game-level-001.map" );
#else
    engine.Run( "GameSceneMainMenu", "assets/maps/game/game-level-001.map" );
    //engine.Run( "Editor", nullptr );
#endif

    engine.Shutdown();

    return 0;
}

/*
TODO:
- Procedural maps
- M4:
    - Animations:
        - Walk
        - Run
        - Shoot
        - Reload
    - Sounds:
        - Shooting
        - Reload
    - FVX:
        - Muzzle
        - Hit
- Sniper:
    - Animations:
            - Walk
            - Run
            - Shoot
            - Reload
    - Sounds:
        - Shooting
        - Reload
    - FVX:
        - Tracer
        - Muzzle
        - Hit

- Emscripten build

- Grenade:
    - Animations:
        - Throw
    - Sounds:
        - Pull
        - Explode
    - VFX:
        - Explode
- Shipping: Card text bug
- Shipping: Esc crash

Brain waves:
Gameplay:
- Multiple waves
- Level timer ( unsure )

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

- Player upgrades:
    - Health increase
    - Med kits do more healing
    - More grenade slots
    - Wider scoop radius
    - Faster sprint
    - Faster crouch

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
- ReservationTable

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