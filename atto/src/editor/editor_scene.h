#include "engine/atto_engine.h"
#include "engine/renderer/atto_render_model.h"
#include "game/game_map.h"

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

    enum class EditorEditMode {
        None,
        Brush
    };

    struct BrushEdgeDrag {
        bool active = false;
        i32 brushIndex = -1;
        i32 axis = -1;
        i32 sign = 0;
        f32 fixedEdge = 0;
    };

    class EditorScene : public Scene<EditorScene> {
    public:
        static const char * GetSceneNameStatic() { return "Editor"; }
        const char * GetSceneName() override { return GetSceneNameStatic(); }

        void OnStart() override;
        void OnUpdate( f32 deltaTime ) override;
        void OnRender( Renderer & renderer ) override;
        void OnShutdown() override;
        void OnResize( i32 width, i32 height ) override;

    private:
        void StartImgui();
        void DrawBrushPanel();
        Mat4 GetOrthoViewProjectionMatrix() const;

        Vec3 ScreenToWorldOrtho( Vec2 screenPos ) const;
        void GetOrthoAxes( i32 & hAxis, i32 & vAxis ) const;
        i32  PickBrushOrtho( Vec3 worldPos ) const;
        i32  PickBrush3D( Vec2 screenPos ) const;
        bool TryStartEdgeDrag( Vec3 worldClickPos );
        void UpdateEdgeDrag( Vec3 worldMousePos );

        EditorViewMode   viewMode = EditorViewMode::Cam3D;
        EditorRenderMode renderMode = EditorRenderMode::Lit;
        EditorEditMode editMode = EditorEditMode::None;

        FlyCamera  flyCamera;

        Vec3 orthoTarget = Vec3( 0.0f );
        f32  orthoSize = 10.0f;

        GameMap map;

        i32 selectedBrushIndex = -1;
        BrushEdgeDrag edgeDrag;

    };

} // namespace atto
