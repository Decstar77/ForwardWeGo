#include "editor_scene.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace atto {

    void EditorScene::OnStart() {
        StartImgui();

        Vec2i windowSize = Engine::Get().GetWindowSize();

        flyCamera.SetViewportSize( windowSize.x, windowSize.y );
        flyCamera.SetPosition( Vec3( 0.0f, 0.0f, 3.0f ) );
        flyCamera.SetFOV( 60.0f );
        flyCamera.SetMoveSpeed( 5.0f );
        flyCamera.SetLookSensitivity( 0.1f );

        map.Initialize();
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

        bool alt = input.IsKeyDown( Key::LeftAlt ) || input.IsKeyDown( Key::RightAlt );
        if ( alt && input.IsKeyPressed( Key::Num1 ) ) { viewMode = EditorViewMode::XY;   input.SetCursorCaptured( false ); edgeDrag.active = false; }
        if ( alt && input.IsKeyPressed( Key::Num2 ) ) { viewMode = EditorViewMode::ZY;   input.SetCursorCaptured( false ); edgeDrag.active = false; }
        if ( alt && input.IsKeyPressed( Key::Num3 ) ) { viewMode = EditorViewMode::XZ;   input.SetCursorCaptured( false ); edgeDrag.active = false; }
        if ( alt && input.IsKeyPressed( Key::Num4 ) ) { viewMode = EditorViewMode::Cam3D; edgeDrag.active = false; }

        if ( edgeDrag.active ) {
            if ( input.IsMouseButtonDown( MouseButton::Left ) ) {
                Vec3 worldPos = ScreenToWorldOrtho( input.GetMousePosition() );
                UpdateEdgeDrag( worldPos );
            }
            if ( input.IsMouseButtonReleased( MouseButton::Left ) ) {
                edgeDrag.active = false;
            }
        }

        bool imguiWantsMouse = ImGui::GetIO().WantCaptureMouse;
        if ( !imguiWantsMouse && !edgeDrag.active && input.IsMouseButtonPressed( MouseButton::Left ) ) {
            Vec2 mousePos = input.GetMousePosition();

            if ( viewMode != EditorViewMode::Cam3D ) {
                Vec3 worldPos = ScreenToWorldOrtho( mousePos );
                if ( !TryStartEdgeDrag( worldPos ) ) {
                    selectedBrushIndex = PickBrushOrtho( worldPos );
                }
            } else {
                selectedBrushIndex = PickBrush3D( mousePos );
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
            }

            f32 speed = flyCamera.GetMoveSpeed() * deltaTime;

            if ( input.IsKeyDown( Key::W ) ) flyCamera.MoveForward( speed );
            if ( input.IsKeyDown( Key::S ) ) flyCamera.MoveForward( -speed );
            if ( input.IsKeyDown( Key::D ) ) flyCamera.MoveRight( speed );
            if ( input.IsKeyDown( Key::A ) ) flyCamera.MoveRight( -speed );
            if ( input.IsKeyDown( Key::Space ) ) flyCamera.MoveUp( speed );
            if ( input.IsKeyDown( Key::LeftControl ) ) flyCamera.MoveUp( -speed );
        } else {
            // Ortho pan with right or middle mouse drag
            if ( input.IsMouseButtonDown( MouseButton::Right ) || input.IsMouseButtonDown( MouseButton::Middle ) ) {
                Vec2 mouseDelta = input.GetMouseDelta();
                Vec2i windowSize = Engine::Get().GetWindowSize();
                f32 scale = ( 2.0f * orthoSize ) / static_cast<f32>( windowSize.y );

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
                orthoSize *= ( scroll > 0.0f ) ? 0.9f : 1.1f;
                orthoSize = Clamp( orthoSize, 0.1f, 1000.0f );
            }
        }
    }

    Mat4 EditorScene::GetOrthoViewProjectionMatrix() const {
        Vec2i windowSize = Engine::Get().GetWindowSize();
        f32 aspect = ( windowSize.y > 0 ) ? static_cast<f32>( windowSize.x ) / static_cast<f32>( windowSize.y ) : 1.0f;
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
        if ( viewMode == EditorViewMode::Cam3D ) {
            renderer.SetViewProjectionMatrix( flyCamera.GetViewProjectionMatrix() );
        } else {
            renderer.SetViewProjectionMatrix( GetOrthoViewProjectionMatrix() );

            Vec2i windowSize = Engine::Get().GetWindowSize();
            f32 aspect = ( windowSize.y > 0 ) ? static_cast<f32>( windowSize.x ) / static_cast<f32>( windowSize.y ) : 1.0f;
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

        renderer.RenderTestTriangle();

        map.Render( renderer, 0.0, renderMode == EditorRenderMode::Lit, selectedBrushIndex );

        renderer.SetWireframe( false );

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();

        static const char * modeLabels[] = { "XY (Front)", "ZY (Side)", "XZ (Top)", "3D Camera" };
        ImGui::SetNextWindowPos( ImVec2( 10, 10 ), ImGuiCond_Always );
        ImGui::SetNextWindowBgAlpha( 0.5f );
        ImGui::Begin( "##ViewMode", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav );
        ImGui::Text( "View: %s", modeLabels[static_cast<i32>( viewMode )] );
        ImGui::End();

        Vec2i windowSize = Engine::Get().GetWindowSize();
        ImGui::SetNextWindowPos( ImVec2( static_cast<f32>( windowSize.x ) - 10.0f, 10.0f ), ImGuiCond_Always, ImVec2( 1.0f, 0.0f ) );
        ImGui::SetNextWindowBgAlpha( 0.5f );
        ImGui::Begin( "##RenderMode", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav );
        i32 rm = static_cast<i32>( renderMode );
        ImGui::RadioButton( "Lit",       &rm, 0 ); ImGui::SameLine();
        ImGui::RadioButton( "Unlit",     &rm, 1 ); ImGui::SameLine();
        ImGui::RadioButton( "Wireframe", &rm, 2 );
        renderMode = static_cast<EditorRenderMode>( rm );
        ImGui::End();

        DrawBrushPanel();

        //ImGui::ShowDemoWindow();
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
    }

    Vec3 EditorScene::ScreenToWorldOrtho( Vec2 screenPos ) const {
        Vec2i windowSize = Engine::Get().GetWindowSize();
        f32 ndcX = 2.0f * screenPos.x / static_cast<f32>( windowSize.x ) - 1.0f;
        f32 ndcY = 1.0f - 2.0f * screenPos.y / static_cast<f32>( windowSize.y );
        Mat4 invVP = glm::inverse( GetOrthoViewProjectionMatrix() );
        Vec4 world = invVP * Vec4( ndcX, ndcY, 0.0f, 1.0f );
        return Vec3( world ) / world.w;
    }

    void EditorScene::GetOrthoAxes( i32 & hAxis, i32 & vAxis ) const {
        switch ( viewMode ) {
            case EditorViewMode::XY: hAxis = 0; vAxis = 1; break;
            case EditorViewMode::ZY: hAxis = 2; vAxis = 1; break;
            case EditorViewMode::XZ: hAxis = 0; vAxis = 2; break;
            default: hAxis = 0; vAxis = 1; break;
        }
    }

    i32 EditorScene::PickBrushOrtho( Vec3 worldPos ) const {
        i32 hAxis, vAxis;
        GetOrthoAxes( hAxis, vAxis );

        for ( i32 i = map.GetBrushCount() - 1; i >= 0; i-- ) {
            const Brush & brush = map.GetBrush( i );
            f32 minH = brush.center[hAxis] - brush.halfExtents[hAxis];
            f32 maxH = brush.center[hAxis] + brush.halfExtents[hAxis];
            f32 minV = brush.center[vAxis] - brush.halfExtents[vAxis];
            f32 maxV = brush.center[vAxis] + brush.halfExtents[vAxis];

            if ( worldPos[hAxis] >= minH && worldPos[hAxis] <= maxH &&
                 worldPos[vAxis] >= minV && worldPos[vAxis] <= maxV ) {
                return i;
            }
        }
        return -1;
    }

    i32 EditorScene::PickBrush3D( Vec2 screenPos ) const {
        Vec2i windowSize = Engine::Get().GetWindowSize();
        f32 ndcX = 2.0f * screenPos.x / static_cast<f32>( windowSize.x ) - 1.0f;
        f32 ndcY = 1.0f - 2.0f * screenPos.y / static_cast<f32>( windowSize.y );

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
                } else {
                    f32 t1 = ( aabbMin[a] - rayOrigin[a] ) / rayDir[a];
                    f32 t2 = ( aabbMax[a] - rayOrigin[a] ) / rayDir[a];
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

    bool EditorScene::TryStartEdgeDrag( Vec3 worldClickPos ) {
        if ( selectedBrushIndex < 0 || selectedBrushIndex >= map.GetBrushCount() ) {
            return false;
        }

        const Brush & brush = map.GetBrush( selectedBrushIndex );
        i32 hAxis, vAxis;
        GetOrthoAxes( hAxis, vAxis );

        f32 threshold = orthoSize * 0.03f;

        f32 hMin = brush.center[hAxis] - brush.halfExtents[hAxis];
        f32 hMax = brush.center[hAxis] + brush.halfExtents[hAxis];
        f32 vMin = brush.center[vAxis] - brush.halfExtents[vAxis];
        f32 vMax = brush.center[vAxis] + brush.halfExtents[vAxis];

        bool inVRange = worldClickPos[vAxis] >= vMin - threshold && worldClickPos[vAxis] <= vMax + threshold;
        bool inHRange = worldClickPos[hAxis] >= hMin - threshold && worldClickPos[hAxis] <= hMax + threshold;

        i32 bestAxis = -1;
        i32 bestSign = 0;
        f32 bestDist = threshold;

        if ( inVRange ) {
            f32 d = Abs( worldClickPos[hAxis] - hMin );
            if ( d < bestDist && ( worldClickPos[hAxis] - brush.center[hAxis] ) < 0.0f ) {
                bestAxis = hAxis; bestSign = -1; bestDist = d;
            }
            d = Abs( worldClickPos[hAxis] - hMax );
            if ( d < bestDist && ( worldClickPos[hAxis] - brush.center[hAxis] ) > 0.0f ) {
                bestAxis = hAxis; bestSign = 1; bestDist = d;
            }
        }

        if ( inHRange ) {
            f32 d = Abs( worldClickPos[vAxis] - vMin );
            if ( d < bestDist && ( worldClickPos[vAxis] - brush.center[vAxis] ) < 0.0f ) {
                bestAxis = vAxis; bestSign = -1; bestDist = d;
            }
            d = Abs( worldClickPos[vAxis] - vMax );
            if ( d < bestDist && ( worldClickPos[vAxis] - brush.center[vAxis] ) > 0.0f ) {
                bestAxis = vAxis; bestSign = 1; bestDist = d;
            }
        }

        if ( bestAxis < 0 ) {
            return false;
        }

        edgeDrag.active = true;
        edgeDrag.brushIndex = selectedBrushIndex;
        edgeDrag.axis = bestAxis;
        edgeDrag.sign = bestSign;
        edgeDrag.fixedEdge = brush.center[bestAxis] - bestSign * brush.halfExtents[bestAxis];
        return true;
    }

    void EditorScene::UpdateEdgeDrag( Vec3 worldMousePos ) {
        if ( !edgeDrag.active || edgeDrag.brushIndex < 0 || edgeDrag.brushIndex >= map.GetBrushCount() ) {
            edgeDrag.active = false;
            return;
        }

        Brush & brush = map.GetBrush( edgeDrag.brushIndex );
        f32 draggedPos = worldMousePos[edgeDrag.axis];

        constexpr f32 MIN_SIZE = 0.01f;

        if ( edgeDrag.sign > 0 ) {
            if ( draggedPos < edgeDrag.fixedEdge + MIN_SIZE ) {
                draggedPos = edgeDrag.fixedEdge + MIN_SIZE;
            }
            brush.center[edgeDrag.axis] = ( edgeDrag.fixedEdge + draggedPos ) * 0.5f;
            brush.halfExtents[edgeDrag.axis] = ( draggedPos - edgeDrag.fixedEdge ) * 0.5f;
        } else {
            if ( draggedPos > edgeDrag.fixedEdge - MIN_SIZE ) {
                draggedPos = edgeDrag.fixedEdge - MIN_SIZE;
            }
            brush.center[edgeDrag.axis] = ( draggedPos + edgeDrag.fixedEdge ) * 0.5f;
            brush.halfExtents[edgeDrag.axis] = ( edgeDrag.fixedEdge - draggedPos ) * 0.5f;
        }

        map.RebuildBrushModel( edgeDrag.brushIndex );
    }

    void EditorScene::DrawBrushPanel() {
        ImGui::SetNextWindowSize( ImVec2( 300, 400 ), ImGuiCond_FirstUseEver );
        ImGui::Begin( "Brushes" );

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

        ImGui::End();
    }

    void EditorScene::OnShutdown() {
    }

    void EditorScene::OnResize( i32 width, i32 height ) {
        flyCamera.SetViewportSize( width, height );
    }

} // namespace atto
