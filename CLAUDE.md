# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ForwardWeGo is a C++20 3D game engine with an integrated editor for creating a 3D FPS roguelike.
Uses OpenGL 3.3+ for rendering, ImGui for the editor UI, and a brush-based level design system.

## Build Commands

```bash
# Generate Visual Studio 2022 solution (or run generate_vs2022.bat)
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build from command line
cmake --build build --config Debug
cmake --build build --config Release

# Executable output: build/bin/Debug/atto.exe or build/bin/Release/atto.exe
```

OpenAL32.dll is automatically copied to the output directory post-build.

## Compiler Settings

- C++20, exceptions off (`/EHsc-`), RTTI off (`/GR-`) — do not use `try/catch`, `dynamic_cast`, or `typeid`
- MSVC: `/MP /W3`, suppressed: C4100, C4201, C4702
- Debug defines `_DEBUG`, Release defines `_RELEASE`

## Architecture

### Engine Singleton (`Engine`)

`atto/src/engine/atto_engine.h` — central singleton via `Engine::Get()`. Owns all subsystems: Renderer, Input, AudioSystem, AssetManager, RNG, and the current Scene. Entry point is `atto_main.cpp` which calls `engine.Run("Editor")`.

The main loop lives in `Engine::Run()`: calculates deltaTime, calls `scene->OnUpdate(dt)` then `scene->OnRender(renderer)`. Scene transitions are deferred to frame boundaries via `TransitionToScene(name, args)`.

### Scene System (Factory Pattern)

`atto/src/engine/atto_scene.h` — scenes register via `SceneRegistry::Register<T>()` and are created by name string. Interface: `OnStart`, `OnUpdate`, `OnRender`, `OnShutdown`, optional `OnResize`.

Two scenes exist:
- **EditorScene** (`editor/editor_scene.h`) — brush/entity editor with 4 view modes (XY/ZY/XZ orthographic + free 3D camera, switched via Alt+1/2/3/4), undo/redo (50 snapshots), ImGui property inspector
- **GameMapScene** (`game/game_scene_map.h`) — playable FPS gameplay

### Entity System

`game/entities/game_entity.h` — polymorphic base with virtual `OnSpawn`, `OnUpdate`, `OnRender`, `OnDespawn`, `TakeDamage`, `Serialize`. Entity types are registered via ClassFactory and created by `EntityType` enum:

| Type | Behavior |
|------|----------|
| Barrel | Destructible prop (100 HP) |
| Drone_QUAD | Flying AI, waypoint pathfinding, wander/chase/attack states |
| Roach | Ground melee enemy, A* pathfinding, walk/attack sounds |
| ExitDoor | Level completion trigger |
| Prop | Static decorative |
| GameMode_KillAllEntities | Win condition: kill all enemies |

### Map System

`game/game_map.h` — a map contains Brushes (AABB geometry with textures), Entities (polymorphic), a WaypointGraph (AI navigation), ParticleSystem, and PlayerStart (position + orientation). Key operations: `Raycast()`, `AddBrush/RemoveBrush`, `CreateEntity/DestroyEntity` (deferred cleanup).

Map files in `assets/maps/*.map` are JSON: root keys `brushes`, `entities`, `playerStart`, `navGraph`.

### Renderer

`engine/atto_renderer.h` — OpenGL renderer with lit (Phong), unlit, skeletal animation, sprite, particle, and damage vignette shaders. Shader sources are embedded as string constants. Resource caches use `FixedList<T, N>` with GetOrLoad pattern to prevent duplicate loading.

### Player Controller

`game/game_player_controller.h` — FPS controller with capsule collision, crouch (1.8m standing / 1.0m crouched), two weapon slots:
- **Knife**: melee swing with hit detection
- **Glock**: semi-auto, 12-round magazine, reload, ADS, recoil animation, raycast hit detection

HUD: crosshair, hitmarker (0.2s fade), damage vignette (0.6s fade), ammo counter.

### Waypoint Graph & AI

`engine/atto_waypoint_graph.h` — node/edge graph for AI pathfinding. Supports A* via `FindPath()`, spatial lookup via `FindNearestNode()`. Editable in the editor (NavGraph selection mode). Each node stores up to 16 edges (FixedList).

### Serialization

Two serialization paths:
1. **JSON file serialization** (`editor/editor_property_serializer.h`) — save/load maps using nlohmann::json
2. **ImGuiPropertySerializer** — same serializer interface but renders editable ImGui widgets in the editor

Polymorphic entity deserialization uses a factory lambda pattern to reconstruct the correct derived type.

## Core Types & Conventions

All code is in the `atto` namespace. Key types from `atto_core.h`:

```cpp
i8/i16/i32/i64, u8/u16/u32/u64, f32/f64, b8, byte, usize
```

**Custom containers** (`atto_containers.h`) — heap-free, fixed-capacity:
- `FixedList<T, Cap>` — stack-allocated array with count
- `SmallString` / `LargeString` — `FixedStringBase<64>` / `FixedStringBase<256>`
- `FixedQueue<T, Cap>` — circular buffer
- `Span<T>` — non-owning view
- `Handle<T>` — index + generation for validity checking

**Math** (`atto_math.h`): GLM aliases (Vec2/3/4, Mat2/3/4, Quat). Fixed-point: `fp = fpm::fixed_24_8`.

**Collision** (`atto_shapes_3D.h`): AlignedBox, Box, Sphere, Capsule, Raycast tests, CapsuleSweep.

**Logging** (`atto_log.h`): `LOG_TRACE/DEBUG/INFO/WARN/ERROR/FATAL` macros with compile-time level filtering.

## Vendor Dependencies

All vendored in `vendor/` and built from source (no package manager):
- **GLFW** — windowing/input
- **GLAD** — OpenGL loader
- **Assimp** — model import (FBX/GLTF/OBJ), built as static lib with no export/tests
- **Clipper2** — polygon clipping (C++17)
- **ImGui + ImGuizmo** — editor UI and 3D transform gizmos
- **STB** — image loading, vorbis audio, truetype fonts
- **OpenAL** — 3D audio (prebuilt Windows DLL)
- **GLM** — vector/matrix math
- **FPM** — fixed-point math
- **ENet** — networking (vendored, not actively used yet)
