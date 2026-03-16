#include "editor_scene.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuizmo.h>

namespace atto {

    void EditorScene::OnStart() {
        StartImgui();

        Vec2i windowSize = Engine::Get().GetWindowSize();

        flyCamera.SetViewportSize( windowSize.x, windowSize.y );
        flyCamera.SetPosition( Vec3( 0.0f, 0.0f, 3.0f ) );
        flyCamera.SetFOV( 60.0f );
        flyCamera.SetMoveSpeed( 5.0f );
        flyCamera.SetLookSensitivity( 0.1f );

        JsonSerializer serializer( false );
        serializer.FromString( Engine::Get().GetAssetManager().ReadTextFile( "assets/maps/game.map" ) );
        map.Serialize( serializer );

        map.Initialize();

        Renderer & renderer = Engine::Get().GetRenderer();
        renderer.LoadSkybox( "assets/FS002_Day_Sunless.png" );
    }

    void EditorScene::StartImgui() {
        static bool initialized = false;
        if ( initialized == true ) {
            return;
        }

        initialized = true;

        ImGui::CreateContext();
        ImGuiIO & io = ImGui::GetIO();

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Initialize ImGui GLFW and OpenGL3 bindings
        ImGui_ImplGlfw_InitForOpenGL( Engine::Get().GetWindowHandle(), true );
        ImGui_ImplOpenGL3_Init( "#version 330 core" );
    }

    void EditorScene::OnUpdate( f32 deltaTime ) {
        Input & input = Engine::Get().GetInput();

        if ( input.IsKeyPressed( Key::Escape ) ) {
            if ( selectionMode == EditorSelectionMode::Brush ) {
                selectedBrushIndex = -1;
            }
            else if ( selectionMode == EditorSelectionMode::Entity ) {
                selectedEntityIndex = -1;
            }
        }

        if ( input.IsKeyDown( Key::LeftControl ) && input.IsKeyPressed( Key::S ) ) {
            JsonSerializer serializer( true );
            map.Serialize( serializer );
            Engine::Get().GetAssetManager().WriteTextFile( "assets/maps/game.map", serializer.ToString() );
        }

        bool alt = input.IsKeyDown( Key::LeftAlt ) || input.IsKeyDown( Key::RightAlt );
        if ( alt && input.IsKeyPressed( Key::Num1 ) ) { viewMode = EditorViewMode::XZ;   input.SetCursorCaptured( false ); brushDrag.mode = BrushDragMode::None; renderMode = EditorRenderMode::Wireframe; }
        if ( alt && input.IsKeyPressed( Key::Num2 ) ) { viewMode = EditorViewMode::XY;   input.SetCursorCaptured( false ); brushDrag.mode = BrushDragMode::None; renderMode = EditorRenderMode::Wireframe; }
        if ( alt && input.IsKeyPressed( Key::Num3 ) ) { viewMode = EditorViewMode::ZY;   input.SetCursorCaptured( false ); brushDrag.mode = BrushDragMode::None; renderMode = EditorRenderMode::Wireframe; }
        if ( alt && input.IsKeyPressed( Key::Num4 ) ) { viewMode = EditorViewMode::Cam3D; brushDrag.mode = BrushDragMode::None; renderMode = EditorRenderMode::Lit;}

        if ( !input.IsCursorCaptured() ) {
            if ( input.IsKeyPressed( Key::B ) ) { selectionMode = EditorSelectionMode::Brush; }
            if ( input.IsKeyPressed( Key::E ) ) { selectionMode = EditorSelectionMode::Entity; }
            if ( input.IsKeyPressed( Key::P ) ) { selectionMode = EditorSelectionMode::PlayerStart; }
        }

        if ( brushDrag.mode != BrushDragMode::None ) {
            if ( input.IsMouseButtonDown( MouseButton::Left ) ) {
                Vec3 worldPos = ScreenToWorldOrtho( input.GetMousePosition() );
                if ( brushDrag.mode == BrushDragMode::Edge ) {
                    BrushUpdateEdgeDrag( worldPos );
                }
                else if ( brushDrag.mode == BrushDragMode::Move ) {
                    BrushUpdateMoveDrag( worldPos );
                }
                else if ( brushDrag.mode == BrushDragMode::Create ) {
                    BrushUpdateCreateDrag( worldPos );
                }
            }
            if ( input.IsMouseButtonReleased( MouseButton::Left ) ) {
                if ( brushDrag.mode == BrushDragMode::Create ) {
                    BrushFinishCreateDrag();
                }
                else {
                    brushDrag.mode = BrushDragMode::None;
                }
            }
        }

        bool imguiWantsMouse = ImGui::GetIO().WantCaptureMouse || ImGuizmo::IsOver() || ImGuizmo::IsUsing();
        if ( selectionMode == EditorSelectionMode::Brush && !imguiWantsMouse && brushDrag.mode == BrushDragMode::None && input.IsMouseButtonPressed( MouseButton::Left ) ) {
            Vec2 mousePos = input.GetMousePosition();

            if ( viewMode != EditorViewMode::Cam3D ) {
                Vec3 worldPos = ScreenToWorldOrtho( mousePos );

                i32 picked = BrushPickOrtho( worldPos );
                if ( picked >= 0 ) {
                    selectedBrushIndex = picked;
                    BrushStartMoveDrag( worldPos );
                }
                else if ( selectedBrushIndex >= 0 && BrushTryStartEdgeDrag( worldPos ) ) {
                    // Edge drag started on selected brush
                }
                else if ( selectedBrushIndex >= 0 ) {
                    selectedBrushIndex = -1;
                }
                else {
                    BrushStartCreateDrag( worldPos );
                }
            }
            else {
                selectedBrushIndex = BrushPick3D( mousePos );
            }
        }

        if ( viewMode == EditorViewMode::Cam3D ) {
            if ( input.IsMouseButtonPressed( MouseButton::Right ) ) {
                input.SetCursorCaptured( true );
            }
            if ( input.IsMouseButtonReleased( MouseButton::Right ) ) {
                input.SetCursorCaptured( false );
            }

            if ( input.IsCursorCaptured() ) {
                Vec2 mouseDelta = input.GetMouseDelta();
                flyCamera.Rotate(
                    mouseDelta.x * flyCamera.GetLookSensitivity() * DEG_TO_RAD,
                    -mouseDelta.y * flyCamera.GetLookSensitivity() * DEG_TO_RAD
                );

                f32 speed = flyCamera.GetMoveSpeed() * deltaTime;

                if ( input.IsKeyDown( Key::W ) ) flyCamera.MoveForward( speed );
                if ( input.IsKeyDown( Key::S ) ) flyCamera.MoveForward( -speed );
                if ( input.IsKeyDown( Key::D ) ) flyCamera.MoveRight( speed );
                if ( input.IsKeyDown( Key::A ) ) flyCamera.MoveRight( -speed );
                if ( input.IsKeyDown( Key::Space ) ) flyCamera.MoveUp( speed );
                if ( input.IsKeyDown( Key::LeftControl ) ) flyCamera.MoveUp( -speed );
            }
        }
        else {
            // Ortho pan with right or middle mouse drag
            if ( input.IsMouseButtonDown( MouseButton::Right ) || input.IsMouseButtonDown( MouseButton::Middle ) ) {
                Vec2 mouseDelta = input.GetMouseDelta();
                Vec2i windowSize = Engine::Get().GetWindowSize();
                f32 scale = (2.0f * orthoSize) / static_cast<f32>(windowSize.y);

                switch ( viewMode ) {
                case EditorViewMode::XY:
                    orthoTarget.x -= mouseDelta.x * scale;
                    orthoTarget.y += mouseDelta.y * scale;
                    break;
                case EditorViewMode::ZY:
                    orthoTarget.z -= mouseDelta.x * scale;
                    orthoTarget.y += mouseDelta.y * scale;
                    break;
                case EditorViewMode::XZ:
                    orthoTarget.x -= mouseDelta.x * scale;
                    orthoTarget.z -= mouseDelta.y * scale;
                    break;
                default: break;
                }
            }

            f32 scroll = input.GetScrollDelta();
            if ( scroll != 0.0f ) {
                orthoSize *= (scroll > 0.0f) ? 0.9f : 1.1f;
                orthoSize = Clamp( orthoSize, 0.1f, 1000.0f );
            }
        }

        if ( input.IsKeyPressed( Key::F5 ) ) {
            Engine::Get().TransitionToScene( "GameMapScene" );
        }
    }

    Mat4 EditorScene::GetOrthoViewProjectionMatrix() const {
        Vec2i windowSize = Engine::Get().GetWindowSize();
        f32 aspect = (windowSize.y > 0) ? static_cast<f32>(windowSize.x) / static_cast<f32>(windowSize.y) : 1.0f;
        f32 halfH = orthoSize;
        f32 halfW = orthoSize * aspect;

        Mat4 proj = glm::ortho( -halfW, halfW, -halfH, halfH, -1000.0f, 1000.0f );

        constexpr f32 d = 500.0f;
        Mat4 view( 1.0f );
        switch ( viewMode ) {
        case EditorViewMode::XY:
            view = glm::lookAt( orthoTarget + Vec3( 0, 0, d ), orthoTarget, Vec3( 0, 1, 0 ) );
            break;
        case EditorViewMode::ZY:
            view = glm::lookAt( orthoTarget + Vec3( -d, 0, 0 ), orthoTarget, Vec3( 0, 1, 0 ) );
            break;
        case EditorViewMode::XZ:
            view = glm::lookAt( orthoTarget + Vec3( 0, d, 0 ), orthoTarget, Vec3( 0, 0, -1 ) );
            break;
        default: break;
        }

        return proj * view;
    }

    void EditorScene::OnRender( Renderer & renderer ) {
        renderer.SetViewport( 0, 0, flyCamera.GetViewportWidth(), flyCamera.GetViewportHeight() );

        if ( viewMode == EditorViewMode::Cam3D ) {
            renderer.SetViewProjectionMatrix( flyCamera.GetViewProjectionMatrix() );
        }
        else {
            renderer.SetViewProjectionMatrix( GetOrthoViewProjectionMatrix() );

            Vec2i windowSize = Engine::Get().GetWindowSize();
            f32 aspect = (windowSize.y > 0) ? static_cast<f32>(windowSize.x) / static_cast<f32>(windowSize.y) : 1.0f;
            f32 halfH = orthoSize;
            f32 halfW = orthoSize * aspect;

            f32 spacing = powf( 10.0f, floorf( log10f( orthoSize * 0.2f ) ) );

            switch ( viewMode ) {
            case EditorViewMode::XY:
                renderer.RenderGrid( Vec3( 1, 0, 0 ), Vec3( 0, 1, 0 ), orthoTarget, spacing, halfW, halfH );
                break;
            case EditorViewMode::ZY:
                renderer.RenderGrid( Vec3( 0, 0, 1 ), Vec3( 0, 1, 0 ), orthoTarget, spacing, halfW, halfH );
                break;
            case EditorViewMode::XZ:
                renderer.RenderGrid( Vec3( 1, 0, 0 ), Vec3( 0, 0, 1 ), orthoTarget, spacing, halfW, halfH );
                break;
            default: break;
            }
        }

        if ( renderMode == EditorRenderMode::Wireframe ) {
            renderer.SetWireframe( true );
        }

        map.Render( renderer, 0.0, renderMode == EditorRenderMode::Lit, selectedBrushIndex );

        const PlayerStart & playerStart = map.GetPlayerStart();
        Vec3 capsuleColor = map.IsPlayerStartColliding() ? Vec3( 1.0f, 0.0f, 0.0f ) : Vec3( 0.0f, 1.0f, 0.0f );
        renderer.DebugCapsule( playerStart.GetCapsule(), capsuleColor );

        renderer.SetWireframe( false );

        if ( viewMode == EditorViewMode::Cam3D ) {
            renderer.RenderSkybox( flyCamera.GetViewMatrix(), flyCamera.GetProjectionMatrix() );
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        {
            ImGuiIO & io = ImGui::GetIO();
            ImGuizmo::SetRect( 0, 0, io.DisplaySize.x, io.DisplaySize.y );
            ImGuizmo::SetOrthographic( viewMode != EditorViewMode::Cam3D );

            Mat4 view;
            Mat4 proj;
            if ( viewMode == EditorViewMode::Cam3D ) {
                view = flyCamera.GetViewMatrix();
                proj = flyCamera.GetProjectionMatrix();
            }
            else {
                Vec2i ws = Engine::Get().GetWindowSize();
                f32 aspect = (ws.y > 0) ? static_cast<f32>(ws.x) / static_cast<f32>(ws.y) : 1.0f;
                f32 halfH = orthoSize;
                f32 halfW = orthoSize * aspect;
                proj = glm::ortho( -halfW, halfW, -halfH, halfH, -1000.0f, 1000.0f );

                constexpr f32 d = 500.0f;
                switch ( viewMode ) {
                case EditorViewMode::XY:
                    view = glm::lookAt( orthoTarget + Vec3( 0, 0, d ), orthoTarget, Vec3( 0, 1, 0 ) );
                    break;
                case EditorViewMode::ZY:
                    view = glm::lookAt( orthoTarget + Vec3( -d, 0, 0 ), orthoTarget, Vec3( 0, 1, 0 ) );
                    break;
                case EditorViewMode::XZ:
                    view = glm::lookAt( orthoTarget + Vec3( 0, d, 0 ), orthoTarget, Vec3( 0, 0, -1 ) );
                    break;
                default: break;
                }
            }

            f32 snap3[3] = { snapSize, snapSize, snapSize };

            if ( selectionMode == EditorSelectionMode::PlayerStart ) {
                PlayerStart & ps = map.GetPlayerStart();
                Mat4 gizmoMatrix = glm::translate( Mat4( 1.0f ), ps.spawnPos );

                if ( ImGuizmo::Manipulate(
                    glm::value_ptr( view ),
                    glm::value_ptr( proj ),
                    ImGuizmo::TRANSLATE,
                    ImGuizmo::WORLD,
                    glm::value_ptr( gizmoMatrix ),
                    nullptr,
                    snapEnabled ? snap3 : nullptr ) ) {
                    ps.spawnPos = Vec3( gizmoMatrix[3] );
                }
            }
            else if ( selectionMode == EditorSelectionMode::Entity && selectedEntityIndex >= 0 && selectedEntityIndex < map.GetEntityCount() ) {
                Entity * ent = map.GetEntity( selectedEntityIndex );
                Vec3 entPos = ent->GetPosition();
                Mat4 gizmoMatrix = glm::translate( Mat4( 1.0f ), entPos );

                if ( ImGuizmo::Manipulate(
                    glm::value_ptr( view ),
                    glm::value_ptr( proj ),
                    ImGuizmo::TRANSLATE,
                    ImGuizmo::WORLD,
                    glm::value_ptr( gizmoMatrix ),
                    nullptr,
                    snapEnabled ? snap3 : nullptr ) ) {
                    ent->SetPosition( Vec3( gizmoMatrix[3] ) );
                }
            }
        }

        static const char * modeLabels[] = { "XY (Front)", "ZY (Side)", "XZ (Top)", "3D Camera" };
        ImGui::SetNextWindowPos( ImVec2( 10, 10 ), ImGuiCond_Always );
        ImGui::SetNextWindowBgAlpha( 0.5f );
        ImGui::Begin( "##ViewMode", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav );
        ImGui::Text( "View: %s", modeLabels[static_cast<i32>(viewMode)] );
        ImGui::End();

        Vec2i windowSize = Engine::Get().GetWindowSize();
        ImGui::SetNextWindowPos( ImVec2( static_cast<f32>(windowSize.x) - 10.0f, 10.0f ), ImGuiCond_Always, ImVec2( 1.0f, 0.0f ) );
        ImGui::SetNextWindowBgAlpha( 0.5f );
        ImGui::Begin( "##RenderMode", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav );
        i32 rm = static_cast<i32>(renderMode);
        ImGui::RadioButton( "Lit", &rm, 0 ); ImGui::SameLine();
        ImGui::RadioButton( "Unlit", &rm, 1 ); ImGui::SameLine();
        ImGui::RadioButton( "Wireframe", &rm, 2 );
        renderMode = static_cast<EditorRenderMode>(rm);
        ImGui::End();

        DrawEditorPanel();

        //ImGui::ShowDemoWindow();
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
    }

    Vec3 EditorScene::ScreenToWorldOrtho( Vec2 screenPos ) const {
        Vec2i windowSize = Engine::Get().GetWindowSize();
        f32 ndcX = 2.0f * screenPos.x / static_cast<f32>(windowSize.x) - 1.0f;
        f32 ndcY = 1.0f - 2.0f * screenPos.y / static_cast<f32>(windowSize.y);
        Mat4 invVP = glm::inverse( GetOrthoViewProjectionMatrix() );
        Vec4 world = invVP * Vec4( ndcX, ndcY, 0.0f, 1.0f );
        return Vec3( world ) / world.w;
    }

    void EditorScene::BrushGetOrthoAxes( i32 & hAxis, i32 & vAxis ) const {
        switch ( viewMode ) {
        case EditorViewMode::XY: hAxis = 0; vAxis = 1; break;
        case EditorViewMode::ZY: hAxis = 2; vAxis = 1; break;
        case EditorViewMode::XZ: hAxis = 0; vAxis = 2; break;
        default: hAxis = 0; vAxis = 1; break;
        }
    }

    i32 EditorScene::BrushPickOrtho( Vec3 worldPos ) const {
        i32 hAxis, vAxis;
        BrushGetOrthoAxes( hAxis, vAxis );

        for ( i32 i = map.GetBrushCount() - 1; i >= 0; i-- ) {
            const Brush & brush = map.GetBrush( i );
            if ( brush.IsPointInside( worldPos, hAxis, vAxis ) ) {
                return i;
            }
        }
        return -1;
    }

    i32 EditorScene::BrushPick3D( Vec2 screenPos ) const {
        Vec2i windowSize = Engine::Get().GetWindowSize();
        f32 ndcX = 2.0f * screenPos.x / static_cast<f32>(windowSize.x) - 1.0f;
        f32 ndcY = 1.0f - 2.0f * screenPos.y / static_cast<f32>(windowSize.y);

        Mat4 invVP = glm::inverse( flyCamera.GetViewProjectionMatrix() );
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

    bool EditorScene::BrushTryStartEdgeDrag( Vec3 worldClickPos ) {
        if ( selectedBrushIndex < 0 || selectedBrushIndex >= map.GetBrushCount() ) {
            return false;
        }

        const Brush & brush = map.GetBrush( selectedBrushIndex );
        i32 hAxis, vAxis;
        BrushGetOrthoAxes( hAxis, vAxis );


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

        brushDrag.mode = BrushDragMode::Edge;
        brushDrag.brushIndex = selectedBrushIndex;
        brushDrag.axis = bestAxis;
        brushDrag.sign = bestSign;
        brushDrag.fixedEdge = brush.center[bestAxis] - bestSign * brush.halfExtents[bestAxis];
        brushDrag.mouseOffset = worldClickPos[bestAxis] - edgePos;
        return true;
    }

    void EditorScene::BrushUpdateEdgeDrag( Vec3 worldMousePos ) {
        if ( brushDrag.brushIndex < 0 || brushDrag.brushIndex >= map.GetBrushCount() ) {
            brushDrag.mode = BrushDragMode::None;
            return;
        }

        Brush & brush = map.GetBrush( brushDrag.brushIndex );
        f32 draggedPos = worldMousePos[brushDrag.axis] - brushDrag.mouseOffset;
        draggedPos = SnapValue( draggedPos );

        constexpr f32 MIN_SIZE = 0.01f;

        if ( brushDrag.sign > 0 ) {
            if ( draggedPos < brushDrag.fixedEdge + MIN_SIZE ) {
                draggedPos = brushDrag.fixedEdge + MIN_SIZE;
            }
            brush.center[brushDrag.axis] = (brushDrag.fixedEdge + draggedPos) * 0.5f;
            brush.halfExtents[brushDrag.axis] = (draggedPos - brushDrag.fixedEdge) * 0.5f;
        }
        else {
            if ( draggedPos > brushDrag.fixedEdge - MIN_SIZE ) {
                draggedPos = brushDrag.fixedEdge - MIN_SIZE;
            }
            brush.center[brushDrag.axis] = (draggedPos + brushDrag.fixedEdge) * 0.5f;
            brush.halfExtents[brushDrag.axis] = (brushDrag.fixedEdge - draggedPos) * 0.5f;
        }

        map.RebuildBrushModel( brushDrag.brushIndex );
        map.RebuildBrushCollision( brushDrag.brushIndex );
    }

    void EditorScene::BrushStartMoveDrag( Vec3 worldClickPos ) {
        brushDrag.mode = BrushDragMode::Move;
        brushDrag.brushIndex = selectedBrushIndex;

        i32 hAxis, vAxis;
        BrushGetOrthoAxes( hAxis, vAxis );

        const Brush & brush = map.GetBrush( selectedBrushIndex );
        brushDrag.moveOffset = Vec3( 0.0f );
        brushDrag.moveOffset[hAxis] = (brush.center[hAxis] - brush.halfExtents[hAxis]) - worldClickPos[hAxis];
        brushDrag.moveOffset[vAxis] = (brush.center[vAxis] - brush.halfExtents[vAxis]) - worldClickPos[vAxis];
    }

    void EditorScene::BrushUpdateMoveDrag( Vec3 worldMousePos ) {
        if ( brushDrag.brushIndex < 0 || brushDrag.brushIndex >= map.GetBrushCount() ) {
            brushDrag.mode = BrushDragMode::None;
            return;
        }

        i32 hAxis, vAxis;
        BrushGetOrthoAxes( hAxis, vAxis );

        Brush & brush = map.GetBrush( brushDrag.brushIndex );
        brush.center[hAxis] = SnapValue( worldMousePos[hAxis] + brushDrag.moveOffset[hAxis] ) + brush.halfExtents[hAxis];
        brush.center[vAxis] = SnapValue( worldMousePos[vAxis] + brushDrag.moveOffset[vAxis] ) + brush.halfExtents[vAxis];

        map.RebuildBrushModel( brushDrag.brushIndex );
        map.RebuildBrushCollision( brushDrag.brushIndex );
    }

    f32 EditorScene::SnapValue( f32 value ) const {
        if ( !snapEnabled || snapSize <= 0.0f ) {
            return value;
        }
        return floorf( value / snapSize + 0.5f ) * snapSize;
    }

    void EditorScene::BrushStartCreateDrag( Vec3 worldClickPos ) {
        i32 hAxis, vAxis;
        BrushGetOrthoAxes( hAxis, vAxis );
        i32 depthAxis = 3 - hAxis - vAxis;

        Vec3 snapped = worldClickPos;
        snapped[hAxis] = SnapValue( snapped[hAxis] );
        snapped[vAxis] = SnapValue( snapped[vAxis] );

        selectedBrushIndex = map.AddBrush();

        Brush & brush = map.GetBrush( selectedBrushIndex );
        brush.center = Vec3( 0.0f );
        brush.halfExtents = Vec3( 0.0f );
        brush.center[hAxis] = snapped[hAxis];
        brush.center[vAxis] = snapped[vAxis];
        brush.halfExtents[depthAxis] = 0.5f;

        brushDrag.mode = BrushDragMode::Create;
        brushDrag.brushIndex = selectedBrushIndex;
        brushDrag.createStartPos = snapped;

        map.RebuildBrushModel( selectedBrushIndex );
        map.RebuildBrushCollision( selectedBrushIndex );
    }

    void EditorScene::BrushUpdateCreateDrag( Vec3 worldMousePos ) {
        if ( brushDrag.brushIndex < 0 || brushDrag.brushIndex >= map.GetBrushCount() ) {
            brushDrag.mode = BrushDragMode::None;
            return;
        }

        i32 hAxis, vAxis;
        BrushGetOrthoAxes( hAxis, vAxis );

        f32 snappedH = SnapValue( worldMousePos[hAxis] );
        f32 snappedV = SnapValue( worldMousePos[vAxis] );

        f32 minH = Min( brushDrag.createStartPos[hAxis], snappedH );
        f32 maxH = Max( brushDrag.createStartPos[hAxis], snappedH );
        f32 minV = Min( brushDrag.createStartPos[vAxis], snappedV );
        f32 maxV = Max( brushDrag.createStartPos[vAxis], snappedV );

        Brush & brush = map.GetBrush( brushDrag.brushIndex );
        brush.center[hAxis] = (minH + maxH) * 0.5f;
        brush.center[vAxis] = (minV + maxV) * 0.5f;
        brush.halfExtents[hAxis] = (maxH - minH) * 0.5f;
        brush.halfExtents[vAxis] = (maxV - minV) * 0.5f;

        map.RebuildBrushModel( brushDrag.brushIndex );
        map.RebuildBrushCollision( brushDrag.brushIndex );
    }

    void EditorScene::BrushFinishCreateDrag() {
        if ( brushDrag.brushIndex >= 0 && brushDrag.brushIndex < map.GetBrushCount() ) {
            i32 hAxis, vAxis;
            BrushGetOrthoAxes( hAxis, vAxis );

            const Brush & brush = map.GetBrush( brushDrag.brushIndex );
            constexpr f32 MIN_CREATE_SIZE = 0.01f;

            if ( brush.halfExtents[hAxis] < MIN_CREATE_SIZE || brush.halfExtents[vAxis] < MIN_CREATE_SIZE ) {
                map.RemoveBrush( brushDrag.brushIndex );
                selectedBrushIndex = -1;
            }
        }

        brushDrag.mode = BrushDragMode::None;
    }

    void EditorScene::DrawEditorPanel() {
        ImGui::SetNextWindowSize( ImVec2( 300, 500 ), ImGuiCond_FirstUseEver );
        ImGui::Begin( "Editor" );

        i32 sm = static_cast<i32>( selectionMode );
        ImGui::RadioButton( "Brush (B)", &sm, static_cast<i32>( EditorSelectionMode::Brush ) ); ImGui::SameLine();
        ImGui::RadioButton( "Entity (E)", &sm, static_cast<i32>( EditorSelectionMode::Entity ) ); ImGui::SameLine();
        ImGui::RadioButton( "Player (P)", &sm, static_cast<i32>( EditorSelectionMode::PlayerStart ) );
        selectionMode = static_cast<EditorSelectionMode>( sm );

        ImGui::Separator();

        ImGui::Checkbox( "Snap", &snapEnabled );
        ImGui::SameLine();
        ImGui::SetNextItemWidth( 80.0f );
        ImGui::DragFloat( "##SnapSize", &snapSize, 0.05f, 0.01f, 100.0f, "%.2f m" );

        ImGui::Separator();

        if ( selectionMode == EditorSelectionMode::Brush ) {
            if ( ImGui::Button( "+ Add Brush" ) ) {
                selectedBrushIndex = map.AddBrush();
            }

            ImGui::Separator();

            i32 brushCount = map.GetBrushCount();

            for ( i32 i = 0; i < brushCount; i++ ) {
                char label[64];
                snprintf( label, sizeof( label ), "Brush %d", i );

                bool isSelected = ( selectedBrushIndex == i );
                if ( ImGui::Selectable( label, isSelected ) ) {
                    selectedBrushIndex = i;
                }
            }

            ImGui::Separator();

            if ( selectedBrushIndex >= 0 && selectedBrushIndex < brushCount ) {
                Brush & brush = map.GetBrush( selectedBrushIndex );

                ImGui::Text( "Brush %d Properties", selectedBrushIndex );
                ImGui::Spacing();

                bool changed = false;

                Vec3 pos = brush.center;
                if ( ImGui::DragFloat3( "Position", &pos.x, 0.1f ) ) {
                    brush.center = pos;
                    changed = true;
                }

                Vec3 size = brush.halfExtents * 2.0f;
                if ( ImGui::DragFloat3( "Size", &size.x, 0.1f, 0.01f, 1000.0f ) ) {
                    brush.halfExtents = size * 0.5f;
                    changed = true;
                }

                if ( changed ) {
                    map.RebuildBrushModel( selectedBrushIndex );
                    map.RebuildBrushCollision( selectedBrushIndex );
                }

                ImGui::Spacing();

                if ( ImGui::Button( "Delete Selected" ) ) {
                    map.RemoveBrush( selectedBrushIndex );
                    brushCount = map.GetBrushCount();
                    if ( selectedBrushIndex >= brushCount ) {
                        selectedBrushIndex = brushCount - 1;
                    }
                }
            }
            else {
                selectedBrushIndex = -1;
                ImGui::TextDisabled( "No brush selected" );
            }
        }
        else if ( selectionMode == EditorSelectionMode::Entity ) {
            static i32 newEntityTypeIndex = 0;
            const char * entityTypeNames[] = { "Barrel" };
            const EntityType entityTypes[] = { EntityType::Barrel };
            constexpr i32 entityTypeCount = sizeof( entityTypes ) / sizeof( entityTypes[0] );

            ImGui::SetNextItemWidth( 150.0f );
            ImGui::Combo( "##EntityType", &newEntityTypeIndex, entityTypeNames, entityTypeCount );
            ImGui::SameLine();
            if ( ImGui::Button( "+ Add Entity" ) ) {
                Entity * ent = map.CreateEntity( entityTypes[newEntityTypeIndex] );
                if ( ent ) {
                    ent->OnSpawn();
                    selectedEntityIndex = map.GetEntityCount() - 1;
                }
            }

            ImGui::Separator();

            i32 entityCount = map.GetEntityCount();

            for ( i32 i = 0; i < entityCount; i++ ) {
                const Entity * ent = map.GetEntity( i );
                char label[64];
                snprintf( label, sizeof( label ), "%s %d", EntityTypeToString( ent->GetType() ), i );

                bool isSelected = ( selectedEntityIndex == i );
                if ( ImGui::Selectable( label, isSelected ) ) {
                    selectedEntityIndex = i;
                }
            }

            ImGui::Separator();

            if ( selectedEntityIndex >= 0 && selectedEntityIndex < entityCount ) {
                Entity * ent = map.GetEntity( selectedEntityIndex );

                ImGui::Text( "%s %d Properties", EntityTypeToString( ent->GetType() ), selectedEntityIndex );
                ImGui::Spacing();

                Vec3 pos = ent->GetPosition();
                if ( ImGui::DragFloat3( "Position", &pos.x, 0.1f ) ) {
                    ent->SetPosition( pos );
                }

                ImGui::Spacing();

                if ( ImGui::Button( "Delete Selected" ) ) {
                    map.DestroyEntityByIndex( selectedEntityIndex );
                    entityCount = map.GetEntityCount();
                    if ( selectedEntityIndex >= entityCount ) {
                        selectedEntityIndex = entityCount - 1;
                    }
                }
            }
            else {
                selectedEntityIndex = -1;
                ImGui::TextDisabled( "No entity selected" );
            }
        }
        else if ( selectionMode == EditorSelectionMode::PlayerStart ) {
            PlayerStart & ps = map.GetPlayerStart();

            ImGui::Text( "Player Start" );
            ImGui::Spacing();

            ImGui::DragFloat3( "Spawn Position", &ps.spawnPos.x, 0.1f );

            bool colliding = map.IsPlayerStartColliding();
            if ( colliding ) {
                ImGui::TextColored( ImVec4( 1, 0, 0, 1 ), "WARNING: Colliding with brush!" );
            }
        }

        ImGui::End();
    }

    void EditorScene::OnShutdown() {
    }

    void EditorScene::OnResize( i32 width, i32 height ) {
        flyCamera.SetViewportSize( width, height );
    }

} // namespace atto
