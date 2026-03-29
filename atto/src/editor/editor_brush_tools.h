#pragma once

#include "engine/atto_core.h"
#if ATTO_EDITOR

#include "engine/atto_camera.h"
#include "engine/renderer/atto_render_model.h"
#include "game/game_map.h"

namespace atto {

    enum class EditorViewMode {
        XY,     // Front  - orthographic (Alt-1)
        ZY,     // Side   - orthographic (Alt-2)
        XZ,     // Top    - orthographic (Alt-3)
        Cam3D   // Free 3D perspective   (Alt-4)
    };

    enum class BrushDragMode {
        None,
        Edge,
        Move,
        Create
    };

    struct BrushDragState {
        BrushDragMode mode = BrushDragMode::None;
        i32 brushIndex = -1;
        i32 axis = -1;
        i32 sign = 0;
        f32 fixedEdge = 0;
        f32 mouseOffset = 0;
        Vec3 lastWorldPos = Vec3( 0.0f );
        Vec3 createStartPos = Vec3( 0.0f );
        Vec3 moveOffset = Vec3( 0.0f );
    };

    class EditorBrushTools {
    public:
        BrushDragState drag;

        static void GetOrthoAxes( EditorViewMode viewMode, i32 & hAxis, i32 & vAxis );
        static f32  SnapValue( f32 value, bool snapEnabled, f32 snapSize );

        i32  PickOrtho( const GameMap & map, EditorViewMode viewMode, Vec3 worldPos ) const;
        i32  Pick3D( const GameMap & map, const FlyCamera & camera, Vec2 screenPos ) const;

        bool TryStartEdgeDrag( const GameMap & map, EditorViewMode viewMode, f32 orthoSize, i32 selectedBrushIndex, Vec3 worldClickPos );
        void UpdateEdgeDrag( GameMap & map, Vec3 worldMousePos, bool snapEnabled, f32 snapSize );
        void StartMoveDrag( const GameMap & map, EditorViewMode viewMode, i32 selectedBrushIndex, Vec3 worldClickPos );
        void UpdateMoveDrag( GameMap & map, EditorViewMode viewMode, Vec3 worldMousePos, bool snapEnabled, f32 snapSize );
        void StartCreateDrag( GameMap & map, EditorViewMode viewMode, Vec3 worldClickPos, bool snapEnabled, f32 snapSize, i32 & outSelectedBrushIndex );
        void UpdateCreateDrag( GameMap & map, EditorViewMode viewMode, Vec3 worldMousePos, bool snapEnabled, f32 snapSize );
        void FinishCreateDrag( GameMap & map, EditorViewMode viewMode, i32 & selectedBrushIndex );
    };

} // namespace atto

#endif