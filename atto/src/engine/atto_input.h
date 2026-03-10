#pragma once

/*
    Atto Engine - Input System
    Handles keyboard, mouse input and state tracking
*/

#include "atto_core.h"
#include "atto_math.h"

struct GLFWwindow;

namespace atto {

    // Key codes (matching GLFW)
    enum class Key : i32 {
        Unknown = -1,

        // Printable keys
        Space = 32,
        Apostrophe = 39,
        Comma = 44,
        Minus = 45,
        Period = 46,
        Slash = 47,

        Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

        Semicolon = 59,
        Equal = 61,

        A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        LeftBracket = 91,
        Backslash = 92,
        RightBracket = 93,
        GraveAccent = 96,

        // Function keys
        Escape = 256,
        Enter = 257,
        Tab = 258,
        Backspace = 259,
        Insert = 260,
        Delete = 261,
        Right = 262,
        Left = 263,
        Down = 264,
        Up = 265,
        PageUp = 266,
        PageDown = 267,
        Home = 268,
        End = 269,
        CapsLock = 280,
        ScrollLock = 281,
        NumLock = 282,
        PrintScreen = 283,
        Pause = 284,

        F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

        // Keypad
        KP0 = 320, KP1, KP2, KP3, KP4, KP5, KP6, KP7, KP8, KP9,
        KPDecimal = 330,
        KPDivide = 331,
        KPMultiply = 332,
        KPSubtract = 333,
        KPAdd = 334,
        KPEnter = 335,
        KPEqual = 336,

        // Modifiers
        LeftShift = 340,
        LeftControl = 341,
        LeftAlt = 342,
        LeftSuper = 343,
        RightShift = 344,
        RightControl = 345,
        RightAlt = 346,
        RightSuper = 347,
        Menu = 348,

        MaxKeys = 349
    };

    // Mouse buttons
    enum class MouseButton : i32 {
        Left = 0,
        Right = 1,
        Middle = 2,
        Button4 = 3,
        Button5 = 4,
        Button6 = 5,
        Button7 = 6,
        Button8 = 7,

        MaxButtons = 8
    };

    class Input {
    public:
        void Initialize( GLFWwindow * win );
        void Update();

        // Keyboard
        bool IsKeyDown( Key key ) const;
        bool IsKeyUp( Key key ) const;
        bool IsKeyPressed( Key key ) const;   // Just pressed this frame
        bool IsKeyReleased( Key key ) const;  // Just released this frame

        // Mouse buttons
        bool IsMouseButtonDown( MouseButton button ) const;
        bool IsMouseButtonUp( MouseButton button ) const;
        bool IsMouseButtonPressed( MouseButton button ) const;
        bool IsMouseButtonReleased( MouseButton button ) const;

        // Mouse position (screen coordinates)
        Vec2 GetMousePosition() const { return mousePos; }
        Vec2 GetMouseDelta() const { return mouseDelta; }

        // Mouse scroll
        f32 GetScrollDelta() const { return scrollDelta; }

        // Cursor capture (hides and locks cursor for FPS-style look)
        void SetCursorCaptured( bool captured );
        bool IsCursorCaptured() const { return cursorCaptured; }

        // Callbacks (set by GLFW)
        void OnKey( i32 key, i32 action );
        void OnMouseButton( i32 button, i32 action );
        void OnMouseMove( f64 x, f64 y );
        void OnScroll( f64 yoffset );

    private:
        static constexpr i32 MAX_KEYS = static_cast<i32>(Key::MaxKeys);
        static constexpr i32 MAX_MOUSE_BUTTONS = static_cast<i32>(MouseButton::MaxButtons);

        GLFWwindow * window = nullptr;

        // Keyboard state
        bool keysDown[MAX_KEYS] = {};
        bool keysPressed[MAX_KEYS] = {};
        bool keysReleased[MAX_KEYS] = {};

        // Mouse state
        bool mouseButtonsDown[MAX_MOUSE_BUTTONS] = {};
        bool mouseButtonsPressed[MAX_MOUSE_BUTTONS] = {};
        bool mouseButtonsReleased[MAX_MOUSE_BUTTONS] = {};

        Vec2 mousePos = Vec2( 0.0f );
        Vec2 mousePrevPos = Vec2( 0.0f );
        Vec2 mouseDelta = Vec2( 0.0f );

        f32 scrollDelta = 0.0f;
        f32 scrollAccumulator = 0.0f;

        bool cursorCaptured = false;
    };

} // namespace atto
