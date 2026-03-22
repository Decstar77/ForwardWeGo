#include "editor_scene.h"
#include "editor_property_serializer.h"

#include <algorithm>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuizmo.h>

namespace atto {

    void EditorScene::OnStart( const char * args ) {
        StartImgui();

        Vec2i windowSize = Engine::Get().GetWindowSize();

        flyCamera.SetViewportSize( windowSize.x, windowSize.y );
        flyCamera.SetPosition( Vec3( 0.0f, 0.0f, 3.0f ) );
        flyCamera.SetFOV( 60.0f );
        flyCamera.SetMoveSpeed( 5.0f );
        flyCamera.SetLookSensitivity( 0.1f );
        LoadEditorState();

        Renderer & renderer = Engine::Get().GetRenderer();
        renderer.LoadSkybox( "assets/textures/FS002_Day_Sunless.png" );

        Engine::Get().GetInput().SetCursorCaptured( false );

        assetBrowser.OnStart();
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
        {
            char title[256];
            const char * mapName = currentMapPath.empty() ? "Untitled" : currentMapPath.c_str();
            snprintf( title, sizeof( title ), "ATTO Editor - %s%s", mapName, unsavedChanges ? " *" : "" );
            Engine::Get().SetWindowTitle( title );
        }

        Input & input = Engine::Get().GetInput();

        if ( input.IsKeyPressed( Key::Escape ) ) {
            if ( selectionMode == EditorSelectionMode::Brush ) {
                selectedBrushIndex = -1;
            }
            else if ( selectionMode == EditorSelectionMode::Entity ) {
                selectedEntityIndex = -1;
                selectedEntityIndices.clear();
            }
            else if ( selectionMode == EditorSelectionMode::NavGraph ) {
                if ( navConnectMode ) {
                    navConnectMode = false;
                }
                else if ( !selectedNavNodeIndices.empty() ) {
                    selectedNavNodeIndices.clear();
                }
                else {
                    selectedNavNodeIndex = -1;
                }
            }
        }

        if ( Engine::Get().IsCloseRequested() ) {
            Engine::Get().CancelCloseRequest();
            TryExit();
        }

        bool ctrl = input.IsKeyDown( Key::LeftControl ) || input.IsKeyDown( Key::RightControl );
        bool shift = input.IsKeyDown( Key::LeftShift ) || input.IsKeyDown( Key::RightShift );

        if ( ctrl && shift && input.IsKeyPressed( Key::S ) ) { SaveMapAs(); }
        else if ( ctrl && input.IsKeyPressed( Key::S ) ) { SaveMap(); }
        if ( ctrl && shift && input.IsKeyPressed( Key::Z ) ) { Redo(); }
        else if ( ctrl && input.IsKeyPressed( Key::Z ) ) { Undo(); }
        if ( ctrl && input.IsKeyPressed( Key::C ) ) { CopySelected(); }
        if ( ctrl && input.IsKeyPressed( Key::V ) ) { PasteSelected(); }
        if ( ctrl && input.IsKeyPressed( Key::N ) ) { NewMap(); }
        if ( ctrl && input.IsKeyPressed( Key::O ) ) { OpenMap(); }
        if ( input.IsKeyPressed( Key::Delete ) ) { DeleteSelected(); }

        bool alt = input.IsKeyDown( Key::LeftAlt ) || input.IsKeyDown( Key::RightAlt );
        if ( alt && input.IsKeyPressed( Key::Num1 ) ) { viewMode = EditorViewMode::XZ;   input.SetCursorCaptured( false ); brushDrag.mode = BrushDragMode::None; renderMode = EditorRenderMode::Wireframe; }
        if ( alt && input.IsKeyPressed( Key::Num2 ) ) { viewMode = EditorViewMode::XY;   input.SetCursorCaptured( false ); brushDrag.mode = BrushDragMode::None; renderMode = EditorRenderMode::Wireframe; }
        if ( alt && input.IsKeyPressed( Key::Num3 ) ) { viewMode = EditorViewMode::ZY;   input.SetCursorCaptured( false ); brushDrag.mode = BrushDragMode::None; renderMode = EditorRenderMode::Wireframe; }
        if ( alt && input.IsKeyPressed( Key::Num4 ) ) { viewMode = EditorViewMode::Cam3D; brushDrag.mode = BrushDragMode::None; renderMode = EditorRenderMode::Lit; }

        if ( !input.IsCursorCaptured() ) {
            if ( input.IsKeyPressed( Key::Num1 ) && !alt ) { selectionMode = EditorSelectionMode::Brush; }
            if ( input.IsKeyPressed( Key::Num2 ) && !alt ) { selectionMode = EditorSelectionMode::Entity; }
            if ( input.IsKeyPressed( Key::Num3 ) && !alt ) { selectionMode = EditorSelectionMode::PlayerStart; }
            if ( input.IsKeyPressed( Key::Num4 ) && !alt ) { selectionMode = EditorSelectionMode::NavGraph; navConnectMode = false; selectedNavNodeIndices.clear(); }

            if ( input.IsKeyPressed( Key::W ) ) { gizmoMode = EditorGizmoMode::Translate; }
            if ( input.IsKeyPressed( Key::E ) ) { gizmoMode = EditorGizmoMode::Rotate; }

            if ( selectionMode == EditorSelectionMode::NavGraph && !ctrl && input.IsKeyPressed( Key::C ) ) {
                if ( selectedNavNodeIndices.size() >= 2 ) {
                    Snapshot();
                    WaypointGraph & graph = map.GetNavGraph();
                    for ( i32 i = 0; i < (i32)selectedNavNodeIndices.size(); i++ ) {
                        for ( i32 j = i + 1; j < (i32)selectedNavNodeIndices.size(); j++ ) {
                            i32 a = selectedNavNodeIndices[ i ];
                            i32 b = selectedNavNodeIndices[ j ];
                            const WaypointNode & nodeA = graph.GetWaypoint( a );
                            bool alreadyConnected = false;
                            for ( i32 e = 0; e < nodeA.edges.GetCount(); e++ ) {
                                if ( nodeA.edges[ e ].nodeBIndex == b ) { alreadyConnected = true; break; }
                            }
                            if ( !alreadyConnected ) {
                                graph.ConnectWaypoints( a, b );
                            }
                        }
                    }
                    unsavedChanges = true;
                }
            }
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

        bool imguiWantsMouse = ImGui::GetIO().WantCaptureMouse || ( ImGuizmo::IsOver() && ImGuizmo::IsUsing() );

        if ( selectionMode == EditorSelectionMode::NavGraph && !imguiWantsMouse && input.IsMouseButtonPressed( MouseButton::Left ) ) {
            Vec2 mousePos = input.GetMousePosition();
            i32 picked = NavNodePick3D( mousePos );

            if ( navConnectMode ) {
                // Connect/disconnect the picked node with the selected one
                if ( picked >= 0 && picked != selectedNavNodeIndex && selectedNavNodeIndex >= 0 ) {
                    Snapshot();
                    WaypointGraph & graph = map.GetNavGraph();
                    const WaypointNode & selNode = graph.GetWaypoint( selectedNavNodeIndex );
                    bool alreadyConnected = false;
                    for ( i32 e = 0; e < selNode.edges.GetCount(); e++ ) {
                        if ( selNode.edges[ e ].nodeBIndex == picked ) { alreadyConnected = true; break; }
                    }
                    if ( alreadyConnected ) {
                        graph.DisconnectWaypoints( selectedNavNodeIndex, picked );
                    }
                    else {
                        graph.ConnectWaypoints( selectedNavNodeIndex, picked );
                    }
                    unsavedChanges = true;
                }
                navConnectMode = false;
            }
            else if ( alt ) {
                // Alt+click: add a new node at the world hit point via ray
                Vec2i windowSize = Engine::Get().GetWindowSize();
                f32 ndcX = 2.0f * mousePos.x / static_cast<f32>( windowSize.x ) - 1.0f;
                f32 ndcY = 1.0f - 2.0f * mousePos.y / static_cast<f32>( windowSize.y );
                Mat4 vp = ( viewMode == EditorViewMode::Cam3D )
                    ? flyCamera.GetViewProjectionMatrix() : GetOrthoViewProjectionMatrix();
                Mat4 invVP = glm::inverse( vp );
                Vec4 nearNDC = invVP * Vec4( ndcX, ndcY, -1.0f, 1.0f );
                Vec4 farNDC  = invVP * Vec4( ndcX, ndcY,  1.0f, 1.0f );
                Vec3 rayOrigin = Vec3( nearNDC ) / nearNDC.w;
                Vec3 rayDir    = Normalize( Vec3( farNDC ) / farNDC.w - rayOrigin );

                Vec3 spawnPos;
                MapRaycastResult hitResult;
                if ( map.Raycast( rayOrigin, rayDir, hitResult ) && hitResult.distance < 1e20f ) {
                    spawnPos = rayOrigin + rayDir * hitResult.distance;
                }
                else {
                    spawnPos = ( viewMode == EditorViewMode::Cam3D )
                        ? flyCamera.GetPosition() + flyCamera.GetForward() * 5.0f
                        : ScreenToWorldOrtho( mousePos );
                }
                if ( snapEnabled ) {
                    spawnPos.x = SnapValue( spawnPos.x );
                    spawnPos.y = SnapValue( spawnPos.y );
                    spawnPos.z = SnapValue( spawnPos.z );
                }

                Snapshot();
                map.GetNavGraph().AddWaypoint( spawnPos );
                selectedNavNodeIndex = map.GetNavGraph().GetNodeCount() - 1;
                selectedNavNodeIndices.clear();
                unsavedChanges = true;
            }
            else if ( shift ) {
                // Shift+click: toggle node in multi-selection
                if ( picked >= 0 ) {
                    // Ensure the current primary is in the set before toggling
                    if ( selectedNavNodeIndex >= 0 ) {
                        auto pit = std::find( selectedNavNodeIndices.begin(), selectedNavNodeIndices.end(), selectedNavNodeIndex );
                        if ( pit == selectedNavNodeIndices.end() ) {
                            selectedNavNodeIndices.push_back( selectedNavNodeIndex );
                        }
                    }
                    auto it = std::find( selectedNavNodeIndices.begin(), selectedNavNodeIndices.end(), picked );
                    if ( it != selectedNavNodeIndices.end() ) {
                        selectedNavNodeIndices.erase( it );
                        if ( selectedNavNodeIndex == picked ) {
                            selectedNavNodeIndex = selectedNavNodeIndices.empty() ? -1 : selectedNavNodeIndices.back();
                        }
                    }
                    else {
                        selectedNavNodeIndices.push_back( picked );
                        selectedNavNodeIndex = picked;
                    }
                }
            }
            else {
                if ( picked >= 0 ) {
                    // Plain click on a node: select it, clear multi-selection
                    selectedNavNodeIndex = picked;
                    selectedNavNodeIndices.clear();
                }
                else if ( selectedNavNodeIndex >= 0 && ctrl ) {
                    // Plain click on empty space with a node selected: spawn adjacent and connect
                    WaypointGraph & graph = map.GetNavGraph();
                    Vec3 spawnPos = graph.GetWaypoint( selectedNavNodeIndex ).position + Vec3( snapSize, 0.0f, 0.0f );
                    if ( snapEnabled ) {
                        spawnPos.x = SnapValue( spawnPos.x );
                        spawnPos.y = SnapValue( spawnPos.y );
                        spawnPos.z = SnapValue( spawnPos.z );
                    }
                    Snapshot();
                    graph.AddWaypoint( spawnPos );
                    i32 newIdx = graph.GetNodeCount() - 1;
                    graph.ConnectWaypoints( selectedNavNodeIndex, newIdx );
                    selectedNavNodeIndex = newIdx;
                    selectedNavNodeIndices.clear();
                    unsavedChanges = true;
                }
                else {
                    selectedNavNodeIndex = -1;
                    selectedNavNodeIndices.clear();
                }
            }
        }

        if ( selectionMode == EditorSelectionMode::Entity && !imguiWantsMouse && input.IsMouseButtonPressed( MouseButton::Left ) ) {
            i32 picked = EntityPick3D( input.GetMousePosition() );
            bool shift  = input.IsKeyDown( Key::LeftShift ) || input.IsKeyDown( Key::RightShift );
            if ( shift ) {
                if ( picked >= 0 ) {
                    auto it = std::find( selectedEntityIndices.begin(), selectedEntityIndices.end(), picked );
                    if ( it != selectedEntityIndices.end() ) {
                        selectedEntityIndices.erase( it );
                        selectedEntityIndex = selectedEntityIndices.empty() ? -1 : selectedEntityIndices.back();
                    }
                    else {
                        selectedEntityIndices.push_back( picked );
                        selectedEntityIndex = picked;
                    }
                }
            }
            else {
                selectedEntityIndices.clear();
                if ( picked >= 0 ) {
                    selectedEntityIndices.push_back( picked );
                }
                selectedEntityIndex = picked;
            }
        }

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
            SaveEditorState();
            Engine::Get().TransitionToScene( "GameMapScene", currentMapPath.c_str() );
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
            renderer.UseUnlitShader();
        }

        if ( renderMode == EditorRenderMode::Unlit ) {
            renderer.UseUnlitShader();
        }

        map.Render( renderer, 0.0, selectedBrushIndex );

        if ( selectionMode == EditorSelectionMode::NavGraph ) {
            // Highlight the selected node: yellow normally, orange when in connect mode
            map.GetNavGraph().DebugDraw( renderer, selectedNavNodeIndex, navConnectMode ? Vec3( 1.0f, 0.5f, 0.0f ) : Vec3( 1.0f, 1.0f, 0.0f ) );

            const WaypointGraph & graph = map.GetNavGraph();

            // Re-draw edges of a node in a highlight color (drawn on top of the default blue)
            auto drawHighlightedEdges = [&]( i32 nodeIdx, Vec3 color ) {
                if ( nodeIdx < 0 || nodeIdx >= graph.GetNodeCount() ) { return; }
                const WaypointNode & node = graph.GetWaypoint( nodeIdx );
                for ( i32 e = 0; e < node.edges.GetCount(); e++ ) {
                    renderer.DebugLine( node.position, graph.GetWaypoint( node.edges[ e ].nodeBIndex ).position, color );
                }
            };

            // Primary selected node: yellow edges (orange in connect mode)
            Vec3 primaryColor = navConnectMode ? Vec3( 1.0f, 0.5f, 0.0f ) : Vec3( 1.0f, 1.0f, 0.0f );
            drawHighlightedEdges( selectedNavNodeIndex, primaryColor );

            // Multi-selected nodes: cyan spheres + cyan edges
            for ( i32 idx : selectedNavNodeIndices ) {
                if ( idx != selectedNavNodeIndex && idx < graph.GetNodeCount() ) {
                    Sphere s;
                    s.center = graph.GetWaypoint( idx ).position;
                    s.radius = 0.18f;
                    renderer.DebugSphere( s, Vec3( 0.0f, 1.0f, 1.0f ) );
                    drawHighlightedEdges( idx, Vec3( 0.0f, 1.0f, 1.0f ) );
                }
            }
        }

        if ( selectionMode == EditorSelectionMode::PlayerStart ) {
            const PlayerStart & playerStart = map.GetPlayerStart();
            Vec3 capsuleColor = map.IsPlayerStartColliding() ? Vec3( 1.0f, 0.0f, 0.0f ) : Vec3( 0.0f, 1.0f, 0.0f );
            renderer.DebugCapsule( playerStart.GetCapsule(), capsuleColor );
        }
        
        renderer.SetWireframe( false );
        renderer.UseLitShader();

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

            f32 translateSnap[3] = { snapSize, snapSize, snapSize };
            f32 rotateSnap[3] = { 15.0f, 15.0f, 15.0f };
            f32 * currentSnap = nullptr;

            ImGuizmo::OPERATION gizmoOp = (gizmoMode == EditorGizmoMode::Rotate)
                ? ImGuizmo::ROTATE
                : ImGuizmo::TRANSLATE;

            if ( snapEnabled ) {
                currentSnap = (gizmoMode == EditorGizmoMode::Rotate) ? rotateSnap : translateSnap;
            }

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
                    snapEnabled ? translateSnap : nullptr ) ) {
                    ps.spawnPos = Vec3( gizmoMatrix[3] );
                    unsavedChanges = true;
                }
            }
            else if ( selectionMode == EditorSelectionMode::NavGraph && selectedNavNodeIndex >= 0 && selectedNavNodeIndex < map.GetNavGraph().GetNodeCount() ) {
                WaypointNode & node = map.GetNavGraph().GetWaypoint( selectedNavNodeIndex );
                Mat4 gizmoMatrix = glm::translate( Mat4( 1.0f ), node.position );

                if ( ImGuizmo::Manipulate(
                    glm::value_ptr( view ),
                    glm::value_ptr( proj ),
                    ImGuizmo::TRANSLATE,
                    ImGuizmo::WORLD,
                    glm::value_ptr( gizmoMatrix ),
                    nullptr,
                    snapEnabled ? translateSnap : nullptr ) ) {
                    node.position = Vec3( gizmoMatrix[ 3 ] );
                    // Update edge distances for all edges incident to this node
                    WaypointGraph & graph = map.GetNavGraph();
                    for ( i32 e = 0; e < node.edges.GetCount(); e++ ) {
                        WaypointEdge & edge = node.edges[ e ];
                        const Vec3 & otherPos = graph.GetWaypoint( edge.nodeBIndex ).position;
                        f32 newDist = Distance( node.position, otherPos );
                        edge.distance = newDist;
                        edge.cost     = newDist;
                        // Mirror on the reverse edge
                        WaypointNode & other = graph.GetWaypoint( edge.nodeBIndex );
                        for ( i32 re = 0; re < other.edges.GetCount(); re++ ) {
                            if ( other.edges[ re ].nodeBIndex == selectedNavNodeIndex ) {
                                other.edges[ re ].distance = newDist;
                                other.edges[ re ].cost     = newDist;
                                break;
                            }
                        }
                    }
                    unsavedChanges = true;
                }
            }
            else if ( selectionMode == EditorSelectionMode::Entity && !selectedEntityIndices.empty() ) {
                // Compute average pivot across all selected entities
                Vec3 pivot = Vec3( 0.0f );
                for ( i32 idx : selectedEntityIndices ) {
                    pivot += map.GetEntity( idx )->GetPosition();
                }
                pivot /= static_cast<f32>( selectedEntityIndices.size() );

                // Gizmo lives at pivot with world-space axes (no rotation baked in)
                Mat4 gizmoMatrix = glm::translate( Mat4( 1.0f ), pivot );
                Mat4 deltaMatrix = Mat4( 1.0f );

                if ( ImGuizmo::Manipulate(
                    glm::value_ptr( view ),
                    glm::value_ptr( proj ),
                    gizmoOp,
                    ImGuizmo::WORLD,
                    glm::value_ptr( gizmoMatrix ),
                    glm::value_ptr( deltaMatrix ),
                    currentSnap ) ) {

                    if ( gizmoOp == ImGuizmo::TRANSLATE ) {
                        Vec3 delta = Vec3( deltaMatrix[3] );
                        for ( i32 idx : selectedEntityIndices ) {
                            Entity * ent = map.GetEntity( idx );
                            ent->SetPosition( ent->GetPosition() + delta );
                        }
                    }
                    else {
                        Mat3 rotDelta = Mat3( deltaMatrix );
                        for ( i32 idx : selectedEntityIndices ) {
                            Entity * ent = map.GetEntity( idx );
                            Vec3 relPos = ent->GetPosition() - pivot;
                            ent->SetPosition( pivot + rotDelta * relPos );
                            ent->SetOrientation( rotDelta * ent->GetOrientation() );
                        }
                    }
                    unsavedChanges = true;
                }
            }
        }

        // Snapshot when gizmo first becomes active
        bool gizmoNowUsing = ImGuizmo::IsUsing();
        if ( gizmoNowUsing && !gizmoWasUsing ) {
            Snapshot();
        }
        gizmoWasUsing = gizmoNowUsing;

        DrawMainMenuBar();
        DrawToolbar();
        DrawInspectorPanel();
        DrawViewOverlay();
        DrawUnsavedChangesDialog();
        assetBrowser.Draw();

        // Handle model drag-drop from asset browser into viewport
        {
            const ImGuiPayload * dragPayload = ImGui::GetDragDropPayload();
            if ( dragPayload && dragPayload->IsDataType( "MODEL_ASSET" ) && ImGui::IsMouseReleased( ImGuiMouseButton_Left ) ) {
                // Only accept if mouse is over the viewport (not over any ImGui window)
                if ( !ImGui::IsWindowHovered( ImGuiHoveredFlags_AnyWindow ) ) {
                    const char * modelPath = static_cast<const char *>( dragPayload->Data );

                    // Normalize path separators
                    std::string path = modelPath;
                    for ( char & ch : path ) {
                        if ( ch == '\\' ) ch = '/';
                    }

                    // Compute placement position via raycast from mouse into the scene
                    Vec2 mousePos = Engine::Get().GetInput().GetMousePosition();
                    Vec2i windowSize = Engine::Get().GetWindowSize();
                    f32 ndcX = 2.0f * mousePos.x / static_cast<f32>( windowSize.x ) - 1.0f;
                    f32 ndcY = 1.0f - 2.0f * mousePos.y / static_cast<f32>( windowSize.y );

                    Mat4 vp = ( viewMode == EditorViewMode::Cam3D )
                        ? flyCamera.GetViewProjectionMatrix()
                        : GetOrthoViewProjectionMatrix();
                    Mat4 invVP = glm::inverse( vp );

                    Vec4 nearNDC = invVP * Vec4( ndcX, ndcY, -1.0f, 1.0f );
                    Vec4 farNDC  = invVP * Vec4( ndcX, ndcY,  1.0f, 1.0f );
                    Vec3 rayOrigin = Vec3( nearNDC ) / nearNDC.w;
                    Vec3 rayDir    = Normalize( Vec3( farNDC ) / farNDC.w - rayOrigin );

                    Vec3 spawnPos;
                    MapRaycastResult hitResult;
                    if ( map.Raycast( rayOrigin, rayDir, hitResult ) && hitResult.distance < 1e20f ) {
                        spawnPos = rayOrigin + rayDir * hitResult.distance;
                    }
                    else {
                        // No intersection — place 5 meters ahead of the camera
                        if ( viewMode == EditorViewMode::Cam3D ) {
                            spawnPos = flyCamera.GetPosition() + flyCamera.GetForward() * 5.0f;
                        }
                        else {
                            spawnPos = ScreenToWorldOrtho( mousePos );
                        }
                    }

                    // Snap position
                    if ( snapEnabled ) {
                        spawnPos.x = SnapValue( spawnPos.x );
                        spawnPos.y = SnapValue( spawnPos.y );
                        spawnPos.z = SnapValue( spawnPos.z );
                    }

                    Snapshot();
                    Entity * ent = map.CreateEntity( EntityType::Prop );
                    if ( ent ) {
                        ent->SetPosition( spawnPos );
                        ent->OnSpawn();

                        const StaticModel * mdl = renderer.GetOrLoadStaticModel( path.c_str() );
                        static_cast<Entity_Prop *>( ent )->SetModel( mdl );

                        selectionMode = EditorSelectionMode::Entity;
                        selectedEntityIndex = map.GetEntityCount() - 1;
                        selectedEntityIndices = { selectedEntityIndex };
                        unsavedChanges = true;
                    }
                }
            }
        }

        // Handle texture selection from asset browser
        if ( assetBrowser.selectionMade ) {
            i32 brushIdx = assetBrowser.selectingForBrush;
            if ( brushIdx >= 0 && brushIdx < map.GetBrushCount() ) {
                Snapshot();
                // Normalize path separators to forward slashes
                std::string texPath = assetBrowser.selectedTexturePath;
                for ( char & ch : texPath ) {
                    if ( ch == '\\' ) ch = '/';
                }
                map.GetBrush( brushIdx ).texturePath = texPath;
                map.RebuildBrushModel( brushIdx );
                map.RebuildBrushTexture( brushIdx );
                unsavedChanges = true;
            }
            assetBrowser.selectionMade = false;
            assetBrowser.selectingForBrush = -1;
        }

        // Snapshot when the user first activates an inspector widget
        bool imguiNowActive = ImGui::IsAnyItemActive();
        if ( imguiNowActive && !imguiWasAnyItemActive ) {
            Snapshot();
        }
        imguiWasAnyItemActive = imguiNowActive;

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

    i32 EditorScene::EntityPick3D( Vec2 screenPos ) const {
        Vec2i windowSize = Engine::Get().GetWindowSize();
        f32 ndcX = 2.0f * screenPos.x / static_cast<f32>(windowSize.x) - 1.0f;
        f32 ndcY = 1.0f - 2.0f * screenPos.y / static_cast<f32>(windowSize.y);

        Mat4 vp = ( viewMode == EditorViewMode::Cam3D ) ? flyCamera.GetViewProjectionMatrix() : GetOrthoViewProjectionMatrix();
        Mat4 invVP = glm::inverse( vp );
        Vec4 nearNDC = invVP * Vec4( ndcX, ndcY, -1.0f, 1.0f );
        Vec4 farNDC  = invVP * Vec4( ndcX, ndcY,  1.0f, 1.0f );
        Vec3 rayOrigin = Vec3( nearNDC ) / nearNDC.w;
        Vec3 rayDir    = Normalize( Vec3( farNDC ) / farNDC.w - rayOrigin );

        i32 bestIndex = -1;
        f32 bestDist  = 1e30f;

        for ( i32 i = 0; i < map.GetEntityCount(); i++ ) {
            const Entity * ent = map.GetEntity( i );
            f32 dist = 0.0f;
            if ( ent->RayTest( rayOrigin, rayDir, dist ) && dist < bestDist ) {
                bestDist  = dist;
                bestIndex = i;
            }
        }

        return bestIndex;
    }

    i32 EditorScene::NavNodePick3D( Vec2 screenPos ) const {
        const WaypointGraph & graph = map.GetNavGraph();
        if ( graph.GetNodeCount() == 0 ) { return -1; }

        Vec2i windowSize = Engine::Get().GetWindowSize();
        f32 ndcX = 2.0f * screenPos.x / static_cast<f32>( windowSize.x ) - 1.0f;
        f32 ndcY = 1.0f - 2.0f * screenPos.y / static_cast<f32>( windowSize.y );

        Mat4 vp = ( viewMode == EditorViewMode::Cam3D ) ? flyCamera.GetViewProjectionMatrix() : GetOrthoViewProjectionMatrix();
        Mat4 invVP = glm::inverse( vp );
        Vec4 nearNDC = invVP * Vec4( ndcX, ndcY, -1.0f, 1.0f );
        Vec4 farNDC  = invVP * Vec4( ndcX, ndcY,  1.0f, 1.0f );
        Vec3 rayOrigin = Vec3( nearNDC ) / nearNDC.w;
        Vec3 rayDir    = Normalize( Vec3( farNDC ) / farNDC.w - rayOrigin );

        constexpr f32 pickRadius = 0.15f;
        i32 bestIndex = -1;
        f32 bestAlong = 1e30f;

        for ( i32 i = 0; i < graph.GetNodeCount(); i++ ) {
            const Vec3 toNode = graph.GetWaypoint( i ).position - rayOrigin;
            f32 along = Dot( toNode, rayDir );
            if ( along < 0.0f ) { continue; }
            Vec3 perp = toNode - rayDir * along;
            if ( Dot( perp, perp ) <= pickRadius * pickRadius && along < bestAlong ) {
                bestAlong = along;
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

        Snapshot();
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
        unsavedChanges = true;
    }

    void EditorScene::BrushStartMoveDrag( Vec3 worldClickPos ) {
        Snapshot();
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
        unsavedChanges = true;
    }

    f32 EditorScene::SnapValue( f32 value ) const {
        if ( !snapEnabled || snapSize <= 0.0f ) {
            return value;
        }
        return floorf( value / snapSize + 0.5f ) * snapSize;
    }

    void EditorScene::BrushStartCreateDrag( Vec3 worldClickPos ) {
        Snapshot();
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
        unsavedChanges = true;
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

    void EditorScene::DrawMainMenuBar() {
        if ( ImGui::BeginMainMenuBar() ) {
            if ( ImGui::BeginMenu( "File" ) ) {
                if ( ImGui::MenuItem( "New Map", "Ctrl+N" ) ) { NewMap(); }
                if ( ImGui::MenuItem( "Open Map...", "Ctrl+O" ) ) { OpenMap(); }
                ImGui::Separator();
                if ( ImGui::MenuItem( "Save", "Ctrl+S" ) ) { SaveMap(); }
                if ( ImGui::MenuItem( "Save As...", "Ctrl+Shift+S" ) ) { SaveMapAs(); }
                ImGui::Separator();
                if ( ImGui::MenuItem( "Play", "F5" ) ) {
                    SaveEditorState();
                    Engine::Get().TransitionToScene( "GameMapScene", currentMapPath.c_str() );
                }
                ImGui::Separator();
                if ( ImGui::MenuItem( "Exit" ) ) {
                    TryExit();
                }
                ImGui::EndMenu();
            }

            if ( ImGui::BeginMenu( "Edit" ) ) {
                if ( ImGui::MenuItem( "Undo", "Ctrl+Z", false, !undoStack.empty() ) ) { Undo(); }
                if ( ImGui::MenuItem( "Redo", "Ctrl+Shift+Z", false, !redoStack.empty() ) ) { Redo(); }
                ImGui::Separator();
                bool canCopy = (selectionMode == EditorSelectionMode::Brush  && selectedBrushIndex  >= 0) ||
                               (selectionMode == EditorSelectionMode::Entity && selectedEntityIndex >= 0);
                bool canPaste = (selectionMode == EditorSelectionMode::Brush  && hasBrushClipboard) ||
                                (selectionMode == EditorSelectionMode::Entity && hasEntityClipboard);
                if ( ImGui::MenuItem( "Copy",  "Ctrl+C", false, canCopy  ) ) { CopySelected(); }
                if ( ImGui::MenuItem( "Paste", "Ctrl+V", false, canPaste ) ) { PasteSelected(); }
                ImGui::Separator();
                if ( ImGui::MenuItem( "Delete Selected", "Del" ) ) { DeleteSelected(); }
                ImGui::Separator();
                ImGui::Checkbox( "Snap", &snapEnabled );
                ImGui::SetNextItemWidth( 80.0f );
                ImGui::DragFloat( "Snap Size", &snapSize, 0.05f, 0.01f, 100.0f, "%.2f m" );
                ImGui::EndMenu();
            }

            if ( ImGui::BeginMenu( "View" ) ) {
                if ( ImGui::MenuItem( "Top (XZ)", "Alt+1", viewMode == EditorViewMode::XZ ) ) {
                    viewMode = EditorViewMode::XZ;
                    Engine::Get().GetInput().SetCursorCaptured( false );
                    brushDrag.mode = BrushDragMode::None;
                    renderMode = EditorRenderMode::Wireframe;
                }
                if ( ImGui::MenuItem( "Front (XY)", "Alt+2", viewMode == EditorViewMode::XY ) ) {
                    viewMode = EditorViewMode::XY;
                    Engine::Get().GetInput().SetCursorCaptured( false );
                    brushDrag.mode = BrushDragMode::None;
                    renderMode = EditorRenderMode::Wireframe;
                }
                if ( ImGui::MenuItem( "Side (ZY)", "Alt+3", viewMode == EditorViewMode::ZY ) ) {
                    viewMode = EditorViewMode::ZY;
                    Engine::Get().GetInput().SetCursorCaptured( false );
                    brushDrag.mode = BrushDragMode::None;
                    renderMode = EditorRenderMode::Wireframe;
                }
                if ( ImGui::MenuItem( "3D Camera", "Alt+4", viewMode == EditorViewMode::Cam3D ) ) {
                    viewMode = EditorViewMode::Cam3D;
                    brushDrag.mode = BrushDragMode::None;
                    renderMode = EditorRenderMode::Lit;
                }
                ImGui::Separator();

                i32 rm = static_cast<i32>(renderMode);
                if ( ImGui::RadioButton( "Lit", &rm, 0 ) ) { renderMode = EditorRenderMode::Lit; }
                if ( ImGui::RadioButton( "Unlit", &rm, 1 ) ) { renderMode = EditorRenderMode::Unlit; }
                if ( ImGui::RadioButton( "Wireframe", &rm, 2 ) ) { renderMode = EditorRenderMode::Wireframe; }

                ImGui::EndMenu();
            }

            if ( ImGui::BeginMenu( "Window" ) ) {
                ImGui::MenuItem( "Asset Browser", nullptr, &assetBrowser.isOpen );
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void EditorScene::DrawToolbar() {
        ImGui::SetNextWindowPos( ImVec2( 10, 30 ), ImGuiCond_Always );
        ImGui::SetNextWindowBgAlpha( 0.75f );
        ImGui::Begin( "##Toolbar", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav );

        i32 sm = static_cast<i32>(selectionMode);
        ImGui::RadioButton( "Brush (1)", &sm, static_cast<i32>(EditorSelectionMode::Brush) ); ImGui::SameLine();
        ImGui::RadioButton( "Entity (2)", &sm, static_cast<i32>(EditorSelectionMode::Entity) ); ImGui::SameLine();
        ImGui::RadioButton( "Player (3)", &sm, static_cast<i32>(EditorSelectionMode::PlayerStart) ); ImGui::SameLine();
        ImGui::RadioButton( "NavGraph (4)", &sm, static_cast<i32>(EditorSelectionMode::NavGraph) );
        EditorSelectionMode newMode = static_cast<EditorSelectionMode>(sm);
        if ( newMode != selectionMode && newMode != EditorSelectionMode::NavGraph ) {
            navConnectMode = false;
        }
        selectionMode = newMode;

        ImGui::Separator();

        i32 gm = static_cast<i32>(gizmoMode);
        ImGui::RadioButton( "Translate (W)", &gm, static_cast<i32>(EditorGizmoMode::Translate) ); ImGui::SameLine();
        ImGui::RadioButton( "Rotate (E)", &gm, static_cast<i32>(EditorGizmoMode::Rotate) );
        gizmoMode = static_cast<EditorGizmoMode>(gm);

        ImGui::End();
    }

    void EditorScene::DrawViewOverlay() {
        static const char * modeLabels[] = { "XY (Front)", "ZY (Side)", "XZ (Top)", "3D Camera" };
        Vec2i windowSize = Engine::Get().GetWindowSize();
        ImGui::SetNextWindowPos( ImVec2( static_cast<f32>(windowSize.x) - 10.0f, 30.0f ), ImGuiCond_Always, ImVec2( 1.0f, 0.0f ) );
        ImGui::SetNextWindowBgAlpha( 0.5f );
        ImGui::Begin( "##ViewOverlay", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav );
        ImGui::Text( "View: %s", modeLabels[static_cast<i32>(viewMode)] );
        ImGui::End();
    }

    void EditorScene::DrawInspectorPanel() {
        ImGui::SetNextWindowSize( ImVec2( 300, 500 ), ImGuiCond_FirstUseEver );
        ImGui::Begin( "Inspector" );

        if ( selectionMode == EditorSelectionMode::Brush ) {
            if ( ImGui::Button( "+ Add Brush" ) ) {
                Snapshot();
                selectedBrushIndex = map.AddBrush();
                unsavedChanges = true;
            }

            ImGui::Separator();

            i32 brushCount = map.GetBrushCount();

            for ( i32 i = 0; i < brushCount; i++ ) {
                char label[64];
                snprintf( label, sizeof( label ), "Brush %d", i );

                bool isSelected = (selectedBrushIndex == i);
                if ( ImGui::Selectable( label, isSelected ) ) {
                    selectedBrushIndex = i;
                }
            }

            ImGui::Separator();

            if ( selectedBrushIndex >= 0 && selectedBrushIndex < brushCount ) {
                Brush & brush = map.GetBrush( selectedBrushIndex );

                ImGui::Text( "Brush %d Properties", selectedBrushIndex );
                ImGui::Spacing();

                ImguiPropertySerializer propSerializer;
                brush.Serialize( propSerializer );

                if ( propSerializer.HasChanges() ) {
                    map.RebuildBrushModel( selectedBrushIndex );
                    map.RebuildBrushCollision( selectedBrushIndex );
                    map.RebuildBrushTexture( selectedBrushIndex );
                    unsavedChanges = true;
                }

                ImGui::Separator();
                ImGui::Text( "Texture" );

                if ( !brush.texturePath.empty() ) {
                    const Texture * tex = Engine::Get().GetRenderer().GetOrLoadTexture( brush.texturePath.c_str() );
                    if ( tex && tex->IsValid() ) {
                        ImTextureID texID = (ImTextureID)(intptr_t)tex->GetHandle();
                        ImGui::Image( texID, ImVec2( 64, 64 ), ImVec2( 0, 1 ), ImVec2( 1, 0 ) );
                    }

                    // Extract just the filename for display
                    std::string displayName = brush.texturePath;
                    size_t lastSlash = displayName.find_last_of( "/\\" );
                    if ( lastSlash != std::string::npos ) {
                        displayName = displayName.substr( lastSlash + 1 );
                    }
                    ImGui::TextWrapped( "%s", displayName.c_str() );

                    if ( ImGui::Button( "Clear Texture" ) ) {
                        Snapshot();
                        brush.texturePath.clear();
                        map.RebuildBrushTexture( selectedBrushIndex );
                        unsavedChanges = true;
                    }
                }
                else {
                    ImGui::TextDisabled( "No texture" );
                }

                if ( ImGui::Button( "Browse Textures..." ) ) {
                    assetBrowser.isOpen = true;
                    assetBrowser.selectingForBrush = selectedBrushIndex;
                }

                ImGui::Spacing();

                if ( ImGui::Button( "Delete Selected" ) ) {
                    Snapshot();
                    map.RemoveBrush( selectedBrushIndex );
                    brushCount = map.GetBrushCount();
                    if ( selectedBrushIndex >= brushCount ) {
                        selectedBrushIndex = brushCount - 1;
                    }
                    unsavedChanges = true;
                }
            }
            else {
                selectedBrushIndex = -1;
                ImGui::TextDisabled( "No brush selected" );
            }
        }
        else if ( selectionMode == EditorSelectionMode::Entity ) {
            static i32 newEntityTypeIndex = 0;
            const char ** entityTypeNames = &EntityTypeNames[1];
            const EntityType * entityTypes = &EntityTypes[1];
            constexpr i32 entityTypeCount = EntityTypeCount - 1;

            ImGui::SetNextItemWidth( 150.0f );
            ImGui::Combo( "##EntityType", &newEntityTypeIndex, entityTypeNames, entityTypeCount );
            ImGui::SameLine();
            if ( ImGui::Button( "+ Add Entity" ) ) {
                Snapshot();
                Entity * ent = map.CreateEntity( entityTypes[newEntityTypeIndex] );
                if ( ent ) {
                    ent->OnSpawn();
                    selectedEntityIndex = map.GetEntityCount() - 1;
                    selectedEntityIndices = { selectedEntityIndex };
                    unsavedChanges = true;
                }
            }

            ImGui::Separator();

            i32 entityCount = map.GetEntityCount();

            for ( i32 i = 0; i < entityCount; i++ ) {
                const Entity * ent = map.GetEntity( i );
                char label[64];
                snprintf( label, sizeof( label ), "%s %d", EntityTypeToString( ent->GetType() ), i );

                bool isSelected = std::find( selectedEntityIndices.begin(), selectedEntityIndices.end(), i ) != selectedEntityIndices.end();
                if ( ImGui::Selectable( label, isSelected ) ) {
                    bool shift = ImGui::GetIO().KeyShift;
                    if ( shift ) {
                        auto it = std::find( selectedEntityIndices.begin(), selectedEntityIndices.end(), i );
                        if ( it != selectedEntityIndices.end() ) {
                            selectedEntityIndices.erase( it );
                            selectedEntityIndex = selectedEntityIndices.empty() ? -1 : selectedEntityIndices.back();
                        }
                        else {
                            selectedEntityIndices.push_back( i );
                            selectedEntityIndex = i;
                        }
                    }
                    else {
                        selectedEntityIndices = { i };
                        selectedEntityIndex = i;
                    }
                }
            }

            ImGui::Separator();

            if ( selectedEntityIndices.size() > 1 ) {
                ImGui::Text( "%d entities selected", static_cast<i32>( selectedEntityIndices.size() ) );
                ImGui::Spacing();

                if ( ImGui::Button( "Delete Selected" ) ) {
                    Snapshot();
                    std::vector<i32> toDelete = selectedEntityIndices;
                    std::sort( toDelete.begin(), toDelete.end(), std::greater<i32>() );
                    for ( i32 idx : toDelete ) {
                        if ( idx >= 0 && idx < map.GetEntityCount() ) {
                            map.DestroyEntityByIndex( idx );
                        }
                    }
                    map.FlushDestroyedEntities();
                    selectedEntityIndices.clear();
                    selectedEntityIndex = -1;
                    entityCount = map.GetEntityCount();
                    unsavedChanges = true;
                }
            }
            else if ( selectedEntityIndex >= 0 && selectedEntityIndex < entityCount ) {
                Entity * ent = map.GetEntity( selectedEntityIndex );

                ImGui::Text( "%s %d Properties", EntityTypeToString( ent->GetType() ), selectedEntityIndex );
                ImGui::Spacing();

                ImguiPropertySerializer propSerializer;
                propSerializer.SetAssetBrowser( &assetBrowser );
                ent->Serialize( propSerializer );

                if ( propSerializer.HasChanges() ) {
                    unsavedChanges = true;
                }

                ImGui::Spacing();

                if ( ImGui::Button( "Delete Selected" ) ) {
                    Snapshot();
                    map.DestroyEntityByIndex( selectedEntityIndex );
                    map.FlushDestroyedEntities();
                    entityCount = map.GetEntityCount();
                    selectedEntityIndices.clear();
                    selectedEntityIndex = -1;
                    unsavedChanges = true;
                }
            }
            else {
                selectedEntityIndex = -1;
                selectedEntityIndices.clear();
                ImGui::TextDisabled( "No entity selected" );
            }
        }
        else if ( selectionMode == EditorSelectionMode::PlayerStart ) {
            PlayerStart & ps = map.GetPlayerStart();

            ImGui::Text( "Player Start" );
            ImGui::Spacing();

            ImguiPropertySerializer propSerializer;
            ps.Serialize( propSerializer );

            if ( propSerializer.HasChanges() ) {
                unsavedChanges = true;
            }

            bool colliding = map.IsPlayerStartColliding();
            if ( colliding ) {
                ImGui::TextColored( ImVec4( 1, 0, 0, 1 ), "WARNING: Colliding with brush!" );
            }
        }
        else if ( selectionMode == EditorSelectionMode::NavGraph ) {
            WaypointGraph & graph = map.GetNavGraph();
            const i32 nodeCount = graph.GetNodeCount();

            ImGui::Text( "Nav Graph  (%d nodes)", nodeCount );
            ImGui::Spacing();

            // Add node button
            if ( ImGui::Button( "+ Add Node" ) ) {
                Vec3 spawnPos = ( viewMode == EditorViewMode::Cam3D )
                    ? flyCamera.GetPosition() + flyCamera.GetForward() * 5.0f
                    : orthoTarget;
                if ( snapEnabled ) {
                    spawnPos.x = SnapValue( spawnPos.x );
                    spawnPos.y = SnapValue( spawnPos.y );
                    spawnPos.z = SnapValue( spawnPos.z );
                }
                Snapshot();
                graph.AddWaypoint( spawnPos );
                selectedNavNodeIndex = graph.GetNodeCount() - 1;
                unsavedChanges = true;
            }

            ImGui::SameLine();
            ImGui::TextDisabled( "or Alt+Click in viewport" );

            ImGui::Separator();

            // Scrollable node list
            ImGui::BeginChild( "##nav_node_list", ImVec2( 0, 120 ), true );
            for ( i32 i = 0; i < nodeCount; i++ ) {
                char label[32];
                snprintf( label, sizeof( label ), "Node %d", i );
                bool inMultiSel = std::find( selectedNavNodeIndices.begin(), selectedNavNodeIndices.end(), i ) != selectedNavNodeIndices.end();
                if ( ImGui::Selectable( label, selectedNavNodeIndex == i || inMultiSel ) ) {
                    selectedNavNodeIndex = i;
                    selectedNavNodeIndices.clear();
                    navConnectMode = false;
                }
            }
            ImGui::EndChild();

            ImGui::Separator();

            if ( selectedNavNodeIndices.size() >= 2 ) {
                ImGui::TextColored( ImVec4( 0.0f, 1.0f, 1.0f, 1.0f ), "%d nodes selected", (i32)selectedNavNodeIndices.size() );
                ImGui::TextDisabled( "Press C to connect all to each other." );
                ImGui::Spacing();
            }

            if ( selectedNavNodeIndex >= 0 && selectedNavNodeIndex < nodeCount ) {
                WaypointNode & node = graph.GetWaypoint( selectedNavNodeIndex );

                ImGui::Text( "Node %d", selectedNavNodeIndex );
                ImGui::Spacing();

                if ( ImGui::DragFloat3( "Position", &node.position.x, 0.1f ) ) {
                    // Recompute edge distances after manual position edit
                    for ( i32 e = 0; e < node.edges.GetCount(); e++ ) {
                        WaypointEdge & edge = node.edges[ e ];
                        f32 newDist = Distance( node.position, graph.GetWaypoint( edge.nodeBIndex ).position );
                        edge.distance = newDist;
                        edge.cost     = newDist;
                        WaypointNode & other = graph.GetWaypoint( edge.nodeBIndex );
                        for ( i32 re = 0; re < other.edges.GetCount(); re++ ) {
                            if ( other.edges[ re ].nodeBIndex == selectedNavNodeIndex ) {
                                other.edges[ re ].distance = newDist;
                                other.edges[ re ].cost     = newDist;
                                break;
                            }
                        }
                    }
                    unsavedChanges = true;
                }

                ImGui::Spacing();

                // Connect mode
                if ( navConnectMode ) {
                    ImGui::TextColored( ImVec4( 1.0f, 0.5f, 0.0f, 1.0f ), "Click a node to connect/disconnect..." );
                    if ( ImGui::Button( "Cancel" ) ) {
                        navConnectMode = false;
                    }
                }
                else {
                    if ( ImGui::Button( "Connect to Node..." ) ) {
                        navConnectMode = true;
                    }
                }

                ImGui::Spacing();
                ImGui::Text( "Connections: %d", node.edges.GetCount() );
                ImGui::Separator();

                for ( i32 e = node.edges.GetCount() - 1; e >= 0; e-- ) {
                    WaypointEdge & edge = node.edges[ e ];
                    ImGui::PushID( e );

                    ImGui::Checkbox( "##en", &edge.enabled );
                    ImGui::SameLine();
                    ImGui::Text( "-> Node %d  (%.2f m)", edge.nodeBIndex, edge.distance );
                    ImGui::SameLine();
                    if ( ImGui::SmallButton( "X" ) ) {
                        Snapshot();
                        graph.DisconnectWaypoints( selectedNavNodeIndex, edge.nodeBIndex );
                        unsavedChanges = true;
                        ImGui::PopID();
                        break;
                    }

                    ImGui::PopID();
                }

                ImGui::Spacing();
                if ( ImGui::Button( "Delete Node" ) ) {
                    Snapshot();
                    graph.RemoveNode( selectedNavNodeIndex );
                    selectedNavNodeIndex = Min( selectedNavNodeIndex, graph.GetNodeCount() - 1 );
                    selectedNavNodeIndices.clear();
                    navConnectMode = false;
                    unsavedChanges = true;
                }
            }
            else {
                selectedNavNodeIndex = -1;
                ImGui::TextDisabled( "No node selected." );
                ImGui::TextDisabled( "Click a node to select it." );
                ImGui::TextDisabled( "Click empty space (with selection) to chain a node." );
                ImGui::TextDisabled( "Shift+Click to multi-select. C to connect all." );
                ImGui::TextDisabled( "Alt+Click to add a node at cursor." );
            }
        }

        ImGui::End();
    }

    void EditorScene::NewMap() {
        if ( unsavedChanges ) {
            pendingAction = UnsavedChangesAction::NewMap;
            showUnsavedChangesDialog = true;
            return;
        }
        map.Clear();
        map.Initialize();
        currentMapPath.clear();
        selectedBrushIndex = -1;
        selectedEntityIndex = -1;
        selectedEntityIndices.clear();
        selectedNavNodeIndex = -1;
        selectedNavNodeIndices.clear();
        navConnectMode = false;
        brushDrag.mode = BrushDragMode::None;
        unsavedChanges = false;
        undoStack.clear();
        redoStack.clear();
    }

    void EditorScene::OpenMap() {
        std::string path = Engine::Get().GetAssetManager().OpenFilePicker( "assets/maps" );
        if ( path.empty() ) {
            return;
        }
        if ( unsavedChanges ) {
            pendingOpenPath = path;
            pendingAction = UnsavedChangesAction::OpenMap;
            showUnsavedChangesDialog = true;
            return;
        }
        LoadMapFromFile( path );
    }

    void EditorScene::SaveMap() {
        if ( currentMapPath.empty() ) {
            SaveMapAs();
            return;
        }
        SaveMapToFile( currentMapPath );
    }

    void EditorScene::SaveMapAs() {
        std::string path = Engine::Get().GetAssetManager().SaveFilePicker( "assets/maps", "map" );
        if ( !path.empty() ) {
            currentMapPath = path;
            SaveMapToFile( currentMapPath );
        }
    }

    void EditorScene::LoadMapFromFile( const std::string & path ) {
        std::string content = Engine::Get().GetAssetManager().ReadTextFile( path );
        if ( content.empty() ) {
            return;
        }

        map.Clear();

        JsonSerializer serializer( false );
        serializer.FromString( content );
        map.Serialize( serializer );
        map.Initialize();

        currentMapPath = path;
        selectedBrushIndex = -1;
        selectedEntityIndex = -1;
        selectedEntityIndices.clear();
        selectedNavNodeIndex = -1;
        selectedNavNodeIndices.clear();
        navConnectMode = false;
        brushDrag.mode = BrushDragMode::None;
        unsavedChanges = false;
        undoStack.clear();
        redoStack.clear();
    }

    void EditorScene::SaveMapToFile( const std::string & path ) {
        JsonSerializer serializer( true );
        map.Serialize( serializer );
        Engine::Get().GetAssetManager().WriteTextFile( path, serializer.ToString() );
        unsavedChanges = false;
    }

    void EditorScene::SaveEditorState() {
        JsonSerializer s( true );
        s( "MapPath",     currentMapPath );
        s( "ViewMode",    reinterpret_cast<i32 &>( viewMode ) );
        s( "OrthoTarget", orthoTarget );
        s( "OrthoSize",   orthoSize );
        Vec3 camPos   = flyCamera.GetPosition();
        f32  camYaw   = flyCamera.GetYaw();
        f32  camPitch = flyCamera.GetPitch();
        s( "CamPos",   camPos );
        s( "CamYaw",   camYaw );
        s( "CamPitch", camPitch );
        Engine::Get().GetAssetManager().WriteTextFile( EditorStatePath, s.ToString() );
    }

    void EditorScene::LoadEditorState() {
        std::string content = Engine::Get().GetAssetManager().ReadTextFile( EditorStatePath );
        if ( content.empty() ) {
            LoadMapFromFile( "assets/maps/default.map" );
            return;
        }

        JsonSerializer serializer( false );
        serializer.FromString( content );

        std::string mapPath;
        serializer( "MapPath",     mapPath );
        serializer( "ViewMode",    reinterpret_cast<i32 &>( viewMode ) );
        serializer( "OrthoTarget", orthoTarget );
        serializer( "OrthoSize",   orthoSize );

        Vec3 camPos   = flyCamera.GetPosition();
        f32  camYaw   = flyCamera.GetYaw();
        f32  camPitch = flyCamera.GetPitch();
        serializer( "CamPos",   camPos );
        serializer( "CamYaw",   camYaw );
        serializer( "CamPitch", camPitch );
        flyCamera.SetPosition( camPos );
        flyCamera.SetYaw( camYaw );
        flyCamera.SetPitch( camPitch );

        if ( !mapPath.empty() ) {
            LoadMapFromFile( mapPath );
        }
        else {
            LoadMapFromFile( "assets/maps/default.map" );
        }
    }

    void EditorScene::CopySelected() {
        if ( selectionMode == EditorSelectionMode::Brush && selectedBrushIndex >= 0 && selectedBrushIndex < map.GetBrushCount() ) {
            brushClipboard    = map.GetBrush( selectedBrushIndex );
            hasBrushClipboard = true;
        }
        else if ( selectionMode == EditorSelectionMode::Entity && selectedEntityIndex >= 0 && selectedEntityIndex < map.GetEntityCount() ) {
            const Entity * ent = map.GetEntity( selectedEntityIndex );
            entityClipboardType = ent->GetType();
            JsonSerializer serializer( true );
            const_cast<Entity *>(ent)->Serialize( serializer );
            entityClipboardJson  = serializer.ToString();
            hasEntityClipboard   = true;
        }
    }

    void EditorScene::PasteSelected() {
        constexpr Vec3 PasteOffset = Vec3( 1.0f, 0.0f, 0.0f );

        if ( selectionMode == EditorSelectionMode::Brush && hasBrushClipboard ) {
            Snapshot();
            selectedBrushIndex = map.AddBrush();
            Brush & newBrush   = map.GetBrush( selectedBrushIndex );
            newBrush           = brushClipboard;
            newBrush.center   += PasteOffset;
            map.RebuildBrushModel( selectedBrushIndex );
            map.RebuildBrushCollision( selectedBrushIndex );
            unsavedChanges = true;
        }
        else if ( selectionMode == EditorSelectionMode::Entity && hasEntityClipboard ) {
            Snapshot();
            Entity * newEnt = map.CreateEntity( entityClipboardType );
            if ( newEnt ) {
                JsonSerializer deserializer( false );
                deserializer.FromString( entityClipboardJson );
                newEnt->Serialize( deserializer );
                newEnt->SetSpawnId( Engine::Get().GetRNG().Unsigned64() );
                newEnt->SetPosition( newEnt->GetPosition() + PasteOffset );
                newEnt->OnSpawn();
                selectedEntityIndex = map.GetEntityCount() - 1;
                selectedEntityIndices = { selectedEntityIndex };
                unsavedChanges = true;
            }
        }
    }

    void EditorScene::DeleteSelected() {
        if ( selectionMode == EditorSelectionMode::Brush && selectedBrushIndex >= 0 && selectedBrushIndex < map.GetBrushCount() ) {
            Snapshot();
            map.RemoveBrush( selectedBrushIndex );
            i32 brushCount = map.GetBrushCount();
            if ( selectedBrushIndex >= brushCount ) {
                selectedBrushIndex = brushCount - 1;
            }
            unsavedChanges = true;
        }
        else if ( selectionMode == EditorSelectionMode::Entity && !selectedEntityIndices.empty() ) {
            Snapshot();
            std::vector<i32> toDelete = selectedEntityIndices;
            std::sort( toDelete.begin(), toDelete.end(), std::greater<i32>() );
            for ( i32 idx : toDelete ) {
                if ( idx >= 0 && idx < map.GetEntityCount() ) {
                    map.DestroyEntityByIndex( idx );
                }
            }
            map.FlushDestroyedEntities();
            selectedEntityIndices.clear();
            selectedEntityIndex = -1;
            unsavedChanges = true;
        }
        else if ( selectionMode == EditorSelectionMode::NavGraph && selectedNavNodeIndex >= 0 ) {
            WaypointGraph & graph = map.GetNavGraph();
            if ( selectedNavNodeIndex < graph.GetNodeCount() ) {
                Snapshot();
                graph.RemoveNode( selectedNavNodeIndex );
                selectedNavNodeIndex = Min( selectedNavNodeIndex, graph.GetNodeCount() - 1 );
                selectedNavNodeIndices.clear();
                navConnectMode = false;
                unsavedChanges = true;
            }
        }
    }

    void EditorScene::Snapshot() {
        JsonSerializer serializer( true );
        map.Serialize( serializer );
        undoStack.push_back( serializer.ToString() );
        if ( (i32)undoStack.size() > MaxUndoSteps ) {
            undoStack.erase( undoStack.begin() );
        }
        redoStack.clear();
    }

    void EditorScene::Undo() {
        if ( undoStack.empty() ) { return; }

        JsonSerializer current( true );
        map.Serialize( current );
        redoStack.push_back( current.ToString() );

        std::string state = undoStack.back();
        undoStack.pop_back();

        map.Clear();
        JsonSerializer deserializer( false );
        deserializer.FromString( state );
        map.Serialize( deserializer );
        map.Initialize();

        selectedBrushIndex = -1;
        selectedEntityIndex = -1;
        selectedEntityIndices.clear();
        brushDrag.mode = BrushDragMode::None;
        unsavedChanges = true;
    }

    void EditorScene::Redo() {
        if ( redoStack.empty() ) { return; }

        JsonSerializer current( true );
        map.Serialize( current );
        undoStack.push_back( current.ToString() );

        std::string state = redoStack.back();
        redoStack.pop_back();

        map.Clear();
        JsonSerializer deserializer( false );
        deserializer.FromString( state );
        map.Serialize( deserializer );
        map.Initialize();

        selectedBrushIndex = -1;
        selectedEntityIndex = -1;
        selectedEntityIndices.clear();
        brushDrag.mode = BrushDragMode::None;
        unsavedChanges = true;
    }

    void EditorScene::TryExit() {
        if ( unsavedChanges ) {
            pendingAction = UnsavedChangesAction::Exit;
            showUnsavedChangesDialog = true;
        }
        else {
            Engine::Get().RequestQuit();
        }
    }

    void EditorScene::ConfirmUnsavedAction() {
        UnsavedChangesAction action = pendingAction;
        pendingAction = UnsavedChangesAction::None;

        switch ( action ) {
        case UnsavedChangesAction::Exit:
            Engine::Get().RequestQuit();
            break;
        case UnsavedChangesAction::NewMap:
            unsavedChanges = false;
            map.Clear();
            map.Initialize();
            currentMapPath.clear();
            selectedBrushIndex = -1;
            selectedEntityIndex = -1;
            selectedEntityIndices.clear();
            brushDrag.mode = BrushDragMode::None;
            break;
        case UnsavedChangesAction::OpenMap:
            unsavedChanges = false;
            LoadMapFromFile( pendingOpenPath );
            pendingOpenPath.clear();
            break;
        default: break;
        }
    }

    void EditorScene::DrawUnsavedChangesDialog() {
        if ( showUnsavedChangesDialog ) {
            ImGui::OpenPopup( "Unsaved Changes" );
            showUnsavedChangesDialog = false;
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos( center, ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
        if ( ImGui::BeginPopupModal( "Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) ) {
            ImGui::Text( "You have unsaved changes. Save before continuing?" );
            ImGui::Spacing();

            if ( ImGui::Button( "Save", ImVec2( 80, 0 ) ) ) {
                ImGui::CloseCurrentPopup();
                SaveMap();
                if ( !unsavedChanges ) {
                    ConfirmUnsavedAction();
                }
                else {
                    pendingAction = UnsavedChangesAction::None;
                }
            }
            ImGui::SameLine();
            if ( ImGui::Button( "Discard", ImVec2( 80, 0 ) ) ) {
                ImGui::CloseCurrentPopup();
                ConfirmUnsavedAction();
            }
            ImGui::SameLine();
            if ( ImGui::Button( "Cancel", ImVec2( 80, 0 ) ) ) {
                ImGui::CloseCurrentPopup();
                pendingAction = UnsavedChangesAction::None;
            }

            ImGui::EndPopup();
        }
    }

    void EditorScene::OnShutdown() {
        SaveEditorState();
    }

    void EditorScene::OnResize( i32 width, i32 height ) {
        flyCamera.SetViewportSize( width, height );
    }

} // namespace atto
