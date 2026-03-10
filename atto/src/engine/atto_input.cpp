/*
    Atto Engine - Input System Implementation
*/

#include "atto_input.h"
#include "atto_log.h"
#include <GLFW/glfw3.h>

namespace atto {

    void Input::Initialize( GLFWwindow * win ) {
        window = win;
        // Initialize all states to false

        memset( keysDown, 0, sizeof( keysDown ) );
        memset( keysPressed, 0, sizeof( keysPressed ) );
        memset( keysReleased, 0, sizeof( keysReleased ) );
        memset( mouseButtonsDown, 0, sizeof( mouseButtonsDown ) );
        memset( mouseButtonsPressed, 0, sizeof( mouseButtonsPressed ) );
        memset( mouseButtonsReleased, 0, sizeof( mouseButtonsReleased ) );

        // Get initial mouse position
        f64 mx, my;
        glfwGetCursorPos( window, &mx, &my );
        mousePos = Vec2( static_cast<f32>(mx), static_cast<f32>(my) );
        mousePrevPos = mousePos;
    }

    void Input::Update() {
        // Calculate mouse delta
        mouseDelta = mousePos - mousePrevPos;
        mousePrevPos = mousePos;

        // Transfer scroll accumulator and reset
        scrollDelta = scrollAccumulator;
        scrollAccumulator = 0.0f;

        memset( keysPressed, 0, sizeof( keysPressed ) );
        memset( keysReleased, 0, sizeof( keysReleased ) );
        memset( mouseButtonsPressed, 0, sizeof( mouseButtonsPressed ) );
        memset( mouseButtonsReleased, 0, sizeof( mouseButtonsReleased ) );
    }

    bool Input::IsKeyDown( Key key ) const {
        i32 k = static_cast<i32>(key);
        if ( k < 0 || k >= MAX_KEYS ) return false;
        return keysDown[k];
    }

    bool Input::IsKeyUp( Key key ) const {
        return !IsKeyDown( key );
    }

    bool Input::IsKeyPressed( Key key ) const {
        i32 k = static_cast<i32>( key );
        if ( k < 0 || k >= MAX_KEYS ) return false;
        return keysPressed[k];
    }

    bool Input::IsKeyReleased( Key key ) const {
        i32 k = static_cast<i32>( key );
        if ( k < 0 || k >= MAX_KEYS ) return false;
        return keysReleased[k];
    }

    bool Input::IsMouseButtonDown( MouseButton button ) const {
        i32 b = static_cast<i32>( button );
        if ( b < 0 || b >= MAX_MOUSE_BUTTONS ) return false;
        return mouseButtonsDown[b];
    }

    bool Input::IsMouseButtonUp( MouseButton button ) const {
        return !IsMouseButtonDown( button );
    }

    bool Input::IsMouseButtonPressed( MouseButton button ) const {
        i32 b = static_cast<i32>( button );
        if ( b < 0 || b >= MAX_MOUSE_BUTTONS ) return false;
        return mouseButtonsPressed[b];
    }

    bool Input::IsMouseButtonReleased( MouseButton button ) const {
        i32 b = static_cast<i32>( button );
        if ( b < 0 || b >= MAX_MOUSE_BUTTONS ) return false;
        return mouseButtonsReleased[b];
    }

    void Input::OnKey( i32 key, i32 action ) {
        if ( key < 0 || key >= MAX_KEYS ) {
            LOG_ERROR( "Invalid key %d", key );
            return;
        }

        if ( action == GLFW_PRESS ) {
            keysDown[key] = true;
            keysPressed[key] = true;
        }
        if ( action == GLFW_RELEASE ) {
            keysDown[key] = false;
            keysReleased[key] = true;
        }
    }

    void Input::OnMouseButton( i32 button, i32 action ) {
        if ( button < 0 || button >= MAX_MOUSE_BUTTONS ) {
            LOG_ERROR( "Invalid mouse button %d", button );
            return;
        }

        if ( action == GLFW_PRESS ) {
            mouseButtonsDown[button] = true;
            mouseButtonsPressed[button] = true;
        }
        else if ( action == GLFW_RELEASE ) {
            mouseButtonsDown[button] = false;
            mouseButtonsReleased[button] = true;
        }
    }

    void Input::OnMouseMove( f64 x, f64 y ) {
        mousePos = Vec2( static_cast<f32>(x), static_cast<f32>(y) );
    }

    void Input::OnScroll( f64 yoffset ) {
        scrollAccumulator += static_cast<f32>(yoffset);
    }

    void Input::SetCursorCaptured( bool captured ) {
        if ( cursorCaptured == captured ) return;
        cursorCaptured = captured;
        if ( window ) {
            glfwSetInputMode( window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL );
        }
    }

} // namespace atto
