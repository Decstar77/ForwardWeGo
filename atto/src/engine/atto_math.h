#pragma once

/*
    Atto Engine - Math Utilities
    Wrapper and extensions around GLM
*/

#include "atto_core.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

namespace atto {

    // Vector aliases
    using Vec2 = glm::vec2;
    using Vec3 = glm::vec3;
    using Vec4 = glm::vec4;

    using Vec2i = glm::ivec2;
    using Vec3i = glm::ivec3;
    using Vec4i = glm::ivec4;

    // Matrix aliases
    using Mat2 = glm::mat2;
    using Mat3 = glm::mat3;
    using Mat4 = glm::mat4;

    // Quaternion alias
    using Quat = glm::quat;

    // Constants
    constexpr f32 PI = 3.14159265358979323846f;
    constexpr f32 TWO_PI = 2.0f * PI;
    constexpr f32 HALF_PI = PI / 2.0f;
    constexpr f32 DEG_TO_RAD = PI / 180.0f;
    constexpr f32 RAD_TO_DEG = 180.0f / PI;

    // Math utilities
    inline f32 ToRadians( f32 degrees ) { return degrees * DEG_TO_RAD; }
    inline f32 ToDegrees( f32 radians ) { return radians * RAD_TO_DEG; }

    inline f32 Lerp( f32 a, f32 b, f32 t ) { return a + (b - a) * t; }
    inline Vec2 Lerp( const Vec2 & a, const Vec2 & b, f32 t ) { return a + (b - a) * t; }
    inline Vec3 Lerp( const Vec3 & a, const Vec3 & b, f32 t ) { return a + (b - a) * t; }
    inline Vec4 Lerp( const Vec4 & a, const Vec4 & b, f32 t ) { return a + (b - a) * t; }

    inline f32 Clamp( f32 value, f32 min, f32 max ) {
        if ( value < min ) return min;
        if ( value > max ) return max;
        return value;
    }

    inline i32 Clamp( i32 value, i32 min, i32 max ) {
        if ( value < min ) return min;
        if ( value > max ) return max;
        return value;
    }

    inline f32 Saturate( f32 value ) { return Clamp( value, 0.0f, 1.0f ); }

    inline f32 SmoothStep( f32 edge0, f32 edge1, f32 x ) {
        f32 t = Saturate( (x - edge0) / (edge1 - edge0) );
        return t * t * (3.0f - 2.0f * t);
    }

    inline f32 Min( f32 a, f32 b ) { return a < b ? a : b; }
    inline f32 Max( f32 a, f32 b ) { return a > b ? a : b; }
    inline i32 Min( i32 a, i32 b ) { return a < b ? a : b; }
    inline i32 Max( i32 a, i32 b ) { return a > b ? a : b; }

    inline f32 Abs( f32 x ) { return x < 0 ? -x : x; }
    inline i32 Abs( i32 x ) { return x < 0 ? -x : x; }
    inline f64 Abs( f64 x ) { return x < 0 ? -x : x; }

    inline f32 Sign( f32 x ) { return x < 0 ? -1.0f : (x > 0 ? 1.0f : 0.0f); }

    // Vector utilities
    inline f32 Length( const Vec2 & v ) { return glm::length( v ); }
    inline f32 Length( const Vec3 & v ) { return glm::length( v ); }
    inline f32 LengthSquared( const Vec2 & v ) { return glm::dot( v, v ); }
    inline f32 LengthSquared( const Vec3 & v ) { return glm::dot( v, v ); }

    inline Vec2 Normalize( const Vec2 & v ) {
        f32 len = Length( v );
        return len > 0.0001f ? v / len : Vec2( 0.0f );
    }
    inline Vec3 Normalize( const Vec3 & v ) {
        f32 len = Length( v );
        return len > 0.0001f ? v / len : Vec3( 0.0f );
    }

    inline f32 Dot( const Vec2 & a, const Vec2 & b ) { return glm::dot( a, b ); }
    inline f32 Dot( const Vec3 & a, const Vec3 & b ) { return glm::dot( a, b ); }
    inline Vec3 Cross( const Vec3 & a, const Vec3 & b ) { return glm::cross( a, b ); }

    inline f32 Distance( const Vec2 & a, const Vec2 & b ) { return Length( b - a ); }
    inline f32 Distance( const Vec3 & a, const Vec3 & b ) { return Length( b - a ); }
    inline f32 DistanceSquared( const Vec2 & a, const Vec2 & b ) { return LengthSquared( b - a ); }
    inline f32 DistanceSquared( const Vec3 & a, const Vec3 & b ) { return LengthSquared( b - a ); }

    inline f32 Sin( f32 x ) { return sinf( x ); }
    inline f32 Cos( f32 x ) { return cosf( x ); }

    inline f32 Sqrt( f32 x ) { return sqrt( x ); }

    // Angle between two 2D vectors (in radians)
    inline f32 Angle( const Vec2 & a, const Vec2 & b ) {
        f32 dot = Clamp( Dot( Normalize( a ), Normalize( b ) ), -1.0f, 1.0f );
        return acosf( dot );
    }

    // Rotate a 2D point around origin
    inline Vec2 Rotate( const Vec2 & point, f32 angle ) {
        f32 c = cosf( angle );
        f32 s = sinf( angle );
        return Vec2( point.x * c - point.y * s, point.x * s + point.y * c );
    }

    // Rotate a 2D point around a center
    inline Vec2 RotateAround( const Vec2 & point, const Vec2 & center, f32 angle ) {
        return center + Rotate( point - center, angle );
    }

    // Direction from angle (0 = right, PI/2 = up)
    inline Vec2 AngleToDirection( f32 angle ) {
        return Vec2( cosf( angle ), sinf( angle ) );
    }

    // Angle from direction
    inline f32 DirectionToAngle( const Vec2 & dir ) {
        return atan2f( dir.y, dir.x );
    }

    inline f32 NormalizeAngle( f32 angle ) {
        while ( angle > PI ) { angle -= TWO_PI; }
        while ( angle < -PI ) { angle += TWO_PI; }
        return angle;
    }
} // namespace atto
