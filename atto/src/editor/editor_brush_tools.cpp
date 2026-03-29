#include "editor_brush_tools.h"

#if ATTO_EDITOR

namespace atto {

    void EditorBrushTools::GetOrthoAxes( EditorViewMode viewMode, i32 & hAxis, i32 & vAxis ) {
        switch ( viewMode ) {
        case EditorViewMode::XY: hAxis = 0; vAxis = 1; break;
        case EditorViewMode::ZY: hAxis = 2; vAxis = 1; break;
        case EditorViewMode::XZ: hAxis = 0; vAxis = 2; break;
        default: hAxis = 0; vAxis = 1; break;
        }
    }

    f32 EditorBrushTools::SnapValue( f32 value, bool snapEnabled, f32 snapSize ) {
        if ( !snapEnabled || snapSize <= 0.0f ) {
            return value;
        }
        return floorf( value / snapSize + 0.5f ) * snapSize;
    }

    i32 EditorBrushTools::PickOrtho( const GameMap & map, EditorViewMode viewMode, Vec3 worldPos ) const {
        i32 hAxis, vAxis;
        GetOrthoAxes( viewMode, hAxis, vAxis );
        i32 depthAxis = 3 - hAxis - vAxis;

        i32 bestIndex = -1;
        f32 bestDepth = -1e30f;
        for ( i32 i = 0; i < map.GetBrushCount(); i++ ) {
            const Brush & brush = map.GetBrush( i );
            if ( brush.IsPointInside( worldPos, hAxis, vAxis ) ) {
                f32 frontFace = brush.center[ depthAxis ] + brush.halfExtents[ depthAxis ];
                if ( frontFace > bestDepth ) {
                    bestDepth = frontFace;
                    bestIndex = i;
                }
            }
        }
        return bestIndex;
    }

    i32 EditorBrushTools::Pick3D( const GameMap & map, const FlyCamera & camera, Vec2 screenPos ) const {
        Vec2i windowSize = Engine::Get().GetWindowSize();
        f32 ndcX = 2.0f * screenPos.x / static_cast<f32>(windowSize.x) - 1.0f;
        f32 ndcY = 1.0f - 2.0f * screenPos.y / static_cast<f32>(windowSize.y);

        Mat4 invVP = glm::inverse( camera.GetViewProjectionMatrix() );
        Vec4 nearNDC = invVP * Vec4( ndcX, ndcY, -1.0f, 1.0f );
        Vec4 farNDC = invVP * Vec4( ndcX, ndcY, 1.0f, 1.0f );
        Vec3 nearWorld = Vec3( nearNDC ) / nearNDC.w;
        Vec3 farWorld = Vec3( farNDC ) / farNDC.w;

        Vec3 rayOrigin = nearWorld;
        Vec3 rayDir = Normalize( farWorld - nearWorld );

        i32 bestIndex = -1;
        f32 bestT = 1e30f;

        for ( i32 i = 0; i < map.GetBrushCount(); i++ ) {
            const Brush & brush = map.GetBrush( i );
            Vec3 aabbMin = brush.center - brush.halfExtents;
            Vec3 aabbMax = brush.center + brush.halfExtents;

            f32 tmin = -1e30f;
            f32 tmax = 1e30f;
            bool hit = true;

            for ( i32 a = 0; a < 3; a++ ) {
                if ( Abs( rayDir[a] ) < 1e-8f ) {
                    if ( rayOrigin[a] < aabbMin[a] || rayOrigin[a] > aabbMax[a] ) {
                        hit = false;
                        break;
                    }
                }
                else {
                    f32 t1 = (aabbMin[a] - rayOrigin[a]) / rayDir[a];
                    f32 t2 = (aabbMax[a] - rayOrigin[a]) / rayDir[a];
                    if ( t1 > t2 ) { f32 tmp = t1; t1 = t2; t2 = tmp; }
                    if ( t1 > tmin ) tmin = t1;
                    if ( t2 < tmax ) tmax = t2;
                    if ( tmin > tmax ) { hit = false; break; }
                }
            }

            if ( hit && tmin >= 0.0f && tmin < bestT ) {
                bestT = tmin;
                bestIndex = i;
            }
        }

        return bestIndex;
    }

    bool EditorBrushTools::TryStartEdgeDrag( const GameMap & map, EditorViewMode viewMode, f32 orthoSize, i32 selectedBrushIndex, Vec3 worldClickPos ) {
        if ( selectedBrushIndex < 0 || selectedBrushIndex >= map.GetBrushCount() ) {
            return false;
        }

        const Brush & brush = map.GetBrush( selectedBrushIndex );
        i32 hAxis, vAxis;
        GetOrthoAxes( viewMode, hAxis, vAxis );

        f32 hMin = brush.center[hAxis] - brush.halfExtents[hAxis];
        f32 hMax = brush.center[hAxis] + brush.halfExtents[hAxis];
        f32 vMin = brush.center[vAxis] - brush.halfExtents[vAxis];
        f32 vMax = brush.center[vAxis] + brush.halfExtents[vAxis];

        bool inVRange = worldClickPos[vAxis] >= vMin && worldClickPos[vAxis] <= vMax;
        bool inHRange = worldClickPos[hAxis] >= hMin && worldClickPos[hAxis] <= hMax;

        f32 threshold = orthoSize * 1.1f;

        i32 bestAxis = -1;
        i32 bestSign = 0;
        f32 bestDist = threshold;

        if ( inVRange ) {
            f32 d = Abs( worldClickPos[hAxis] - hMin );
            if ( d < bestDist && (worldClickPos[hAxis] - brush.center[hAxis]) < 0.0f ) {
                bestAxis = hAxis; bestSign = -1; bestDist = d;
            }
            d = Abs( worldClickPos[hAxis] - hMax );
            if ( d < bestDist && (worldClickPos[hAxis] - brush.center[hAxis]) > 0.0f ) {
                bestAxis = hAxis; bestSign = 1; bestDist = d;
            }
        }

        if ( inHRange ) {
            f32 d = Abs( worldClickPos[vAxis] - vMin );
            if ( d < bestDist && (worldClickPos[vAxis] - brush.center[vAxis]) < 0.0f ) {
                bestAxis = vAxis; bestSign = -1; bestDist = d;
            }
            d = Abs( worldClickPos[vAxis] - vMax );
            if ( d < bestDist && (worldClickPos[vAxis] - brush.center[vAxis]) > 0.0f ) {
                bestAxis = vAxis; bestSign = 1; bestDist = d;
            }
        }

        if ( bestAxis < 0 ) {
            return false;
        }

        f32 edgePos = brush.center[bestAxis] + bestSign * brush.halfExtents[bestAxis];

        drag.mode = BrushDragMode::Edge;
        drag.brushIndex = selectedBrushIndex;
        drag.axis = bestAxis;
        drag.sign = bestSign;
        drag.fixedEdge = brush.center[bestAxis] - bestSign * brush.halfExtents[bestAxis];
        drag.mouseOffset = worldClickPos[bestAxis] - edgePos;
        return true;
    }

    void EditorBrushTools::UpdateEdgeDrag( GameMap & map, Vec3 worldMousePos, bool snapEnabled, f32 snapSize ) {
        if ( drag.brushIndex < 0 || drag.brushIndex >= map.GetBrushCount() ) {
            drag.mode = BrushDragMode::None;
            return;
        }

        Brush & brush = map.GetBrush( drag.brushIndex );
        f32 draggedPos = worldMousePos[drag.axis] - drag.mouseOffset;
        draggedPos = SnapValue( draggedPos, snapEnabled, snapSize );

        constexpr f32 MIN_SIZE = 0.01f;

        if ( drag.sign > 0 ) {
            if ( draggedPos < drag.fixedEdge + MIN_SIZE ) {
                draggedPos = drag.fixedEdge + MIN_SIZE;
            }
            brush.center[drag.axis] = (drag.fixedEdge + draggedPos) * 0.5f;
            brush.halfExtents[drag.axis] = (draggedPos - drag.fixedEdge) * 0.5f;
        }
        else {
            if ( draggedPos > drag.fixedEdge - MIN_SIZE ) {
                draggedPos = drag.fixedEdge - MIN_SIZE;
            }
            brush.center[drag.axis] = (draggedPos + drag.fixedEdge) * 0.5f;
            brush.halfExtents[drag.axis] = (drag.fixedEdge - draggedPos) * 0.5f;
        }

        map.RebuildBrushModel( drag.brushIndex );
        map.RebuildBrushCollision( drag.brushIndex );
    }

    void EditorBrushTools::StartMoveDrag( const GameMap & map, EditorViewMode viewMode, i32 selectedBrushIndex, Vec3 worldClickPos ) {
        drag.mode = BrushDragMode::Move;
        drag.brushIndex = selectedBrushIndex;

        i32 hAxis, vAxis;
        GetOrthoAxes( viewMode, hAxis, vAxis );

        const Brush & brush = map.GetBrush( selectedBrushIndex );
        drag.moveOffset = Vec3( 0.0f );
        drag.moveOffset[hAxis] = (brush.center[hAxis] - brush.halfExtents[hAxis]) - worldClickPos[hAxis];
        drag.moveOffset[vAxis] = (brush.center[vAxis] - brush.halfExtents[vAxis]) - worldClickPos[vAxis];
    }

    void EditorBrushTools::UpdateMoveDrag( GameMap & map, EditorViewMode viewMode, Vec3 worldMousePos, bool snapEnabled, f32 snapSize ) {
        if ( drag.brushIndex < 0 || drag.brushIndex >= map.GetBrushCount() ) {
            drag.mode = BrushDragMode::None;
            return;
        }

        i32 hAxis, vAxis;
        GetOrthoAxes( viewMode, hAxis, vAxis );

        Brush & brush = map.GetBrush( drag.brushIndex );
        brush.center[hAxis] = SnapValue( worldMousePos[hAxis] + drag.moveOffset[hAxis], snapEnabled, snapSize ) + brush.halfExtents[hAxis];
        brush.center[vAxis] = SnapValue( worldMousePos[vAxis] + drag.moveOffset[vAxis], snapEnabled, snapSize ) + brush.halfExtents[vAxis];

        map.RebuildBrushModel( drag.brushIndex );
        map.RebuildBrushCollision( drag.brushIndex );
    }

    void EditorBrushTools::StartCreateDrag( GameMap & map, EditorViewMode viewMode, Vec3 worldClickPos, bool snapEnabled, f32 snapSize, i32 & outSelectedBrushIndex ) {
        i32 hAxis, vAxis;
        GetOrthoAxes( viewMode, hAxis, vAxis );
        i32 depthAxis = 3 - hAxis - vAxis;

        Vec3 snapped = worldClickPos;
        snapped[hAxis] = SnapValue( snapped[hAxis], snapEnabled, snapSize );
        snapped[vAxis] = SnapValue( snapped[vAxis], snapEnabled, snapSize );

        outSelectedBrushIndex = map.AddBrush();

        Brush & brush = map.GetBrush( outSelectedBrushIndex );
        brush.center = Vec3( 0.0f );
        brush.halfExtents = Vec3( 0.0f );
        brush.center[hAxis] = snapped[hAxis];
        brush.center[vAxis] = snapped[vAxis];
        brush.halfExtents[depthAxis] = 0.5f;

        drag.mode = BrushDragMode::Create;
        drag.brushIndex = outSelectedBrushIndex;
        drag.createStartPos = snapped;

        map.RebuildBrushModel( outSelectedBrushIndex );
        map.RebuildBrushCollision( outSelectedBrushIndex );
    }

    void EditorBrushTools::UpdateCreateDrag( GameMap & map, EditorViewMode viewMode, Vec3 worldMousePos, bool snapEnabled, f32 snapSize ) {
        if ( drag.brushIndex < 0 || drag.brushIndex >= map.GetBrushCount() ) {
            drag.mode = BrushDragMode::None;
            return;
        }

        i32 hAxis, vAxis;
        GetOrthoAxes( viewMode, hAxis, vAxis );

        f32 snappedH = SnapValue( worldMousePos[hAxis], snapEnabled, snapSize );
        f32 snappedV = SnapValue( worldMousePos[vAxis], snapEnabled, snapSize );

        f32 minH = Min( drag.createStartPos[hAxis], snappedH );
        f32 maxH = Max( drag.createStartPos[hAxis], snappedH );
        f32 minV = Min( drag.createStartPos[vAxis], snappedV );
        f32 maxV = Max( drag.createStartPos[vAxis], snappedV );

        Brush & brush = map.GetBrush( drag.brushIndex );
        brush.center[hAxis] = (minH + maxH) * 0.5f;
        brush.center[vAxis] = (minV + maxV) * 0.5f;
        brush.halfExtents[hAxis] = (maxH - minH) * 0.5f;
        brush.halfExtents[vAxis] = (maxV - minV) * 0.5f;

        map.RebuildBrushModel( drag.brushIndex );
        map.RebuildBrushCollision( drag.brushIndex );
    }

    void EditorBrushTools::FinishCreateDrag( GameMap & map, EditorViewMode viewMode, i32 & selectedBrushIndex ) {
        if ( drag.brushIndex >= 0 && drag.brushIndex < map.GetBrushCount() ) {
            i32 hAxis, vAxis;
            GetOrthoAxes( viewMode, hAxis, vAxis );

            const Brush & brush = map.GetBrush( drag.brushIndex );
            constexpr f32 MIN_CREATE_SIZE = 0.01f;

            if ( brush.halfExtents[hAxis] < MIN_CREATE_SIZE || brush.halfExtents[vAxis] < MIN_CREATE_SIZE ) {
                map.RemoveBrush( drag.brushIndex );
                selectedBrushIndex = -1;
            }
        }

        drag.mode = BrushDragMode::None;
    }

} // namespace atto

#endif