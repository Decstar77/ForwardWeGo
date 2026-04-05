#pragma once

#include "engine/atto_core.h"
#if ATTO_EDITOR

#include "engine/atto_engine.h"
#include "engine/renderer/atto_render_model.h"
#include "game/game_map.h"
#include "editor_asset_browser.h"
#include "editor_asset_packer.h"
#include "editor_brush_tools.h"

namespace atto {

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
        void GenerateMapFromTextFile();

        // Unsaved changes dialog
        enum class UnsavedChangesAction { None, Exit, NewMap, OpenMap, Play };
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

        // Picking
        i32  EntityPick3D( Vec2 screenPos ) const;
        i32  NavNodePick3D( Vec2 screenPos ) const;

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

        // Texture eyedropper (Alt+Middle click picks, Middle click applies)
        std::string      pickedBrushTexturePath;

        // Selection state
        i32              selectedBrushIndex = -1;
        i32              selectedEntityIndex = -1;   // primary (last clicked), shown in inspector
        std::vector<i32> selectedEntityIndices;      // full multi-selection set
        EditorBrushTools brushTools;

        // NavGraph editing state
        i32              selectedNavNodeIndex = -1;
        bool             navConnectMode = false;    // when true, next viewport click connects to selected node
        std::vector<i32> selectedNavNodeIndices;    // multi-selection set (shift+click)
        bool             navGizmoEnabled = true;    // toggle with W in NavGraph mode
        bool             navBoxSelecting = false;   // ortho box-select in progress
        Vec2             navBoxStart     = Vec2( 0.0f );
        Vec2             navBoxCurrent   = Vec2( 0.0f );

        // Snap
        bool snapEnabled = true;
        f32  snapSize = 1.0f;

        EditorAssetBrowser assetBrowser;
        EditorAssetPacker  assetPacker;

        // Nav generation settings
        f32 navGenSpacing = 0.5f;
        f32 navGenPadding = 0.25f;
    };

} // namespace atto

#endif