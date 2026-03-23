#pragma once

#include "engine/atto_engine.h"
#include "engine/renderer/atto_render_model.h"
#include "game/game_map.h"
#include "editor_asset_browser.h"

namespace atto {

    enum class EditorViewMode {
        XY,     // Front  - orthographic (Alt-1)
        ZY,     // Side   - orthographic (Alt-2)
        XZ,     // Top    - orthographic (Alt-3)
        Cam3D   // Free 3D perspective   (Alt-4)
    };

    enum class EditorRenderMode {
        Lit,
        Unlit,
        Wireframe
    };

    enum class EditorSelectionMode {
        None,
        Brush,
        Entity,
        PlayerStart,
        NavGraph
    };

    enum class EditorGizmoMode {
        Translate,
        Rotate
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

    class EditorScene : public SceneInterface {
    public:
        static const char * GetSceneNameStatic() { return "Editor"; }
        const char * GetSceneName() override { return GetSceneNameStatic(); }

        void OnStart( const char * args ) override;
        void OnUpdate( f32 deltaTime ) override;
        void OnRender( Renderer & renderer ) override;
        void OnShutdown() override;
        void OnResize( i32 width, i32 height ) override;

    private:
        void StartImgui();

        // UI
        void DrawMainMenuBar();
        void DrawToolbar();
        void DrawInspectorPanel();
        void DrawViewOverlay();

        // Editor session state
        static constexpr const char * EditorStatePath = "assets/editor_state.json";
        void SaveEditorState();
        void LoadEditorState();

        // File operations
        void NewMap();
        void OpenMap();
        void SaveMap();
        void SaveMapAs();
        void LoadMapFromFile( const std::string & path );
        void SaveMapToFile( const std::string & path );

        // Unsaved changes dialog
        enum class UnsavedChangesAction { None, Exit, NewMap, OpenMap };
        void DrawUnsavedChangesDialog();
        void ConfirmUnsavedAction();
        void TryExit();

        // Undo/redo
        static constexpr i32 MaxUndoSteps = 50;
        void Snapshot();
        void Undo();
        void Redo();

        void DeleteSelected();
        void CopySelected();
        void PasteSelected();

        Mat4 GetOrthoViewProjectionMatrix() const;
        Vec3 ScreenToWorldOrtho( Vec2 screenPos ) const;

        // Brush operations
        void BrushGetOrthoAxes( i32 & hAxis, i32 & vAxis ) const;
        i32  BrushPickOrtho( Vec3 worldPos ) const;
        i32  BrushPick3D( Vec2 screenPos ) const;
        i32  EntityPick3D( Vec2 screenPos ) const;
        i32  NavNodePick3D( Vec2 screenPos ) const;
        bool BrushTryStartEdgeDrag( Vec3 worldClickPos );
        void BrushUpdateEdgeDrag( Vec3 worldMousePos );
        void BrushStartMoveDrag( Vec3 worldClickPos );
        void BrushUpdateMoveDrag( Vec3 worldMousePos );
        void BrushStartCreateDrag( Vec3 worldClickPos );
        void BrushUpdateCreateDrag( Vec3 worldMousePos );
        void BrushFinishCreateDrag();
        f32  SnapValue( f32 value ) const;

        // View state
        EditorViewMode      viewMode = EditorViewMode::Cam3D;
        EditorRenderMode    renderMode = EditorRenderMode::Unlit;
        EditorSelectionMode selectionMode = EditorSelectionMode::Brush;
        EditorGizmoMode     gizmoMode = EditorGizmoMode::Translate;

        FlyCamera  flyCamera;
        Vec3 orthoTarget = Vec3( 0.0f );
        f32  orthoSize = 10.0f;

        // Map state
        GameMap      map;
        std::string  currentMapPath;
        bool         unsavedChanges = false;

        // Unsaved changes dialog state
        bool                 showUnsavedChangesDialog = false;
        UnsavedChangesAction pendingAction = UnsavedChangesAction::None;
        std::string          pendingOpenPath;

        // Undo/redo state
        std::vector<std::string> undoStack;
        std::vector<std::string> redoStack;
        bool imguiWasAnyItemActive = false;
        bool gizmoWasUsing = false;

        // Clipboard
        bool        hasBrushClipboard  = false;
        Brush       brushClipboard;
        bool        hasEntityClipboard = false;
        EntityType  entityClipboardType = EntityType::None;
        std::string entityClipboardJson;
        bool                           hasNavClipboard = false;
        std::vector<Vec3>              navClipboardPositions;
        std::vector<std::pair<i32,i32>> navClipboardEdges; // local index pairs within clipboard

        // Selection state
        i32              selectedBrushIndex = -1;
        i32              selectedEntityIndex = -1;   // primary (last clicked), shown in inspector
        std::vector<i32> selectedEntityIndices;      // full multi-selection set
        BrushDragState   brushDrag;

        // NavGraph editing state
        i32              selectedNavNodeIndex = -1;
        bool             navConnectMode = false;    // when true, next viewport click connects to selected node
        std::vector<i32> selectedNavNodeIndices;    // multi-selection set (shift+click)

        // Snap
        bool snapEnabled = true;
        f32  snapSize = 1.0f;

        EditorAssetBrowser assetBrowser;
    };

} // namespace atto
