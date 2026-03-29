#pragma once

/*
    Atto Engine - Core Types and Definitions
    A lightweight 2D game engine for RTS games
*/

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cassert>
#include <vector>
#include <array>
#include <memory>
#include <string>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define ATTO_PLATFORM_WINDOWS 1
#elif defined(__linux__)
#define ATTO_PLATFORM_LINUX 1
#elif defined(__APPLE__)
#define ATTO_PLATFORM_MACOS 1
#endif

// Debug/Release detection
#if defined(_DEBUG) || defined(DEBUG)
#define ATTO_DEBUG 1
#else
#define ATTO_RELEASE 1
#endif

// Assertions
#if ATTO_DEBUG
#define ATTO_ASSERT(condition, message) \
    do { if (!(condition)) { \
        fprintf(stderr, "Assertion failed: %s\n  Message: %s\n  File: %s\n  Line: %d\n", #condition, message, __FILE__, __LINE__); \
        __debugbreak(); \
    } } while(0)
#define INVALID_CODE_PATH ATTO_ASSERT(false, "Invalid code path")
#else
#define ATTO_ASSERT(condition, message) ((void)0)
#define INVALID_CODE_PATH
#endif

#define Stringify( x ) #x

// Fixed-size types
namespace atto {
    using b8 = int8_t;
    using i8 = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;

    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    using f32 = float;
    using f64 = double;

    using byte = uint8_t;
    using usize = size_t;

    #define U32_MAX 0xFFFFFFFF
    #define U64_MAX 0xFFFFFFFFFFFFFFFF
    #define I32_MAX 0x7FFFFFFF
    #define I64_MAX 0x7FFFFFFFFFFFFFFF
    #define I32_MIN 0x80000000
    #define I64_MIN 0x8000000000000000

    f32 RandomFloat();

    // Color structure (RGBA, 0-1 range)
    struct Color {
        f32 r, g, b, a;

        Color() : r( 1.0f ), g( 1.0f ), b( 1.0f ), a( 1.0f ) {}
        Color( f32 r, f32 g, f32 b, f32 a = 1.0f ) : r( r ), g( g ), b( b ), a( a ) {}

        // Common colors
        static Color White() { return Color( 1.0f, 1.0f, 1.0f, 1.0f ); }
        static Color Black() { return Color( 0.0f, 0.0f, 0.0f, 1.0f ); }
        static Color Red() { return Color( 1.0f, 0.0f, 0.0f, 1.0f ); }
        static Color Green() { return Color( 0.0f, 1.0f, 0.0f, 1.0f ); }
        static Color Blue() { return Color( 0.0f, 0.0f, 1.0f, 1.0f ); }
        static Color Yellow() { return Color( 1.0f, 1.0f, 0.0f, 1.0f ); }
        static Color Cyan() { return Color( 0.0f, 1.0f, 1.0f, 1.0f ); }
        static Color Magenta() { return Color( 1.0f, 0.0f, 1.0f, 1.0f ); }
        static Color Orange() { return Color( 1.0f, 0.5f, 0.0f, 1.0f ); }
        static Color Gray() { return Color( 0.5f, 0.5f, 0.5f, 1.0f ); }
        static Color Random() { return Color( RandomFloat(), RandomFloat(), RandomFloat(), 1.0f ); }

        static Color FromHex( u32 hex ) {
            return Color(
                ((hex >> 24) & 0xFF) / 255.0f,
                ((hex >> 16) & 0xFF) / 255.0f,
                ((hex >> 8) & 0xFF) / 255.0f,
                (hex & 0xFF) / 255.0f
            );
        }

        // Interpolate between two colors
        static Color Lerp( const Color & a, const Color & b, f32 t ) {
            return Color(
                a.r + (b.r - a.r) * t,
                a.g + (b.g - a.g) * t,
                a.b + (b.b - a.b) * t,
                a.a + (b.a - a.a) * t
            );
        }

        void Serialize( class Serializer & serializer );
    };

    // Handle type for resources
    template<typename T>
    struct Handle {
        u32 index = 0;
        u32 generation = 0;

        bool IsValid() const { return generation != 0; }
        bool operator==( const Handle & other ) const {
            return index == other.index && generation == other.generation;
        }
        bool operator!=( const Handle & other ) const { return !(*this == other); }
    };

    inline std::string StripFilePathAndExtension( const std::string & filePath ) {
        // Removes directory and extension, returns the base filename.
        // Example: "assets/images/picture.png" -> "picture"
        size_t lastSlash = filePath.find_last_of("/\\");
        size_t start = (lastSlash == std::string::npos) ? 0 : lastSlash + 1;
        size_t lastDot = filePath.find_last_of('.');
        // lastDot must be after start to be a real extension (not in folder name)
        if (lastDot != std::string::npos && lastDot > start) {
            return filePath.substr(start, lastDot - start);
        }
        return filePath.substr(start);
    }

} // namespace atto
