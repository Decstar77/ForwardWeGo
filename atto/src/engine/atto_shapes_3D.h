#pragma once

#include "atto_math.h"

namespace atto {
    struct AlignedBox {
        Vec3 min;
        Vec3 max;

        inline static AlignedBox FromMinMax( Vec3 min, Vec3 max ) { return { min, max }; }
        inline static AlignedBox FromCenterSize( Vec3 center, Vec3 size ) { return { center - size * 0.5f, center + size * 0.5f }; }
        inline Vec3 GetCenter() const { return (min + max) * 0.5f; }
        inline Vec3 GetSize() const { return max - min; }

        void Translate( const Vec3 & delta );
        void Rotate( const Mat3 & rotation );
    };

    struct Sphere {
        Vec3 center;
        f32 radius;
    };

    struct Capsule {
        Vec3    base;    // centre of the BOTTOM hemisphere
        f32     height;  // distance between the two hemisphere centres
        f32     radius;

        
        inline Vec3 GetTopTip() const { return base + Vec3( 0.0f, height, 0.0f ); }
        inline Vec3 GetBottomTip() const { return base - Vec3( 0.0f, height, 0.0f ); }

        inline static Capsule FromBaseTopRadius( const Vec3 & base, const Vec3 & top, f32 radius ) { return { base, glm::distance( base, top ), radius }; }
        inline static Capsule FromEndpoints( const Vec3 & p0, const Vec3 & p1, f32 radius ) { return { p0, glm::distance( p0, p1 ), radius }; }
        inline static Capsule FromTips( const Vec3 & bottomTip, const Vec3 & topTip, f32 radius ) {
            Vec3 axis = topTip - bottomTip;
            f32 axisLen = glm::length( axis );
            if ( axisLen < 2.0f * radius ) {
                // Degenerate, collapse to a sphere
                return { bottomTip + axis * 0.5f, 0.0f, radius };
            }
            Vec3 dir = axis / axisLen;
            Vec3 base = bottomTip + dir * radius;
            f32 height = axisLen - 2.0f * radius;
            return { base, height, radius };
        }
    };

    class Raycast {
    public:
        static bool TestSphere( const Vec3 & rayOrigin, const Vec3 & rayDirection, const Sphere & sphere, f32 & dist );
        static bool TestAlignedBox( const Vec3 & rayOrigin, const Vec3 & rayDirection, const AlignedBox & alignedBox, f32 & dist );
        static bool TestCapsule( const Vec3 & rayOrigin, const Vec3 & rayDirection, const Capsule & capsule, f32 & dist );
    };

    class IntersectionTest {
    public:
        static bool Sphere2( const Sphere & sphereA, const Sphere & sphereB );
        static bool AlignedBox2( const AlignedBox & boxA, const AlignedBox & boxB );
        static bool SphereAlignedBox( const Sphere & sphere, const AlignedBox & box );
        static bool CapsuleAlignedBox( const Capsule & capsule, const AlignedBox & box );
    };

    struct SweepResult {
        f32  toi;           // Time till impact
        Vec3 normal;        // Normal of said impact
        f32  pen;           // The penetrating amout ( if they are at all )
    };

    class CollisionSweep {
    public:
        static bool CapsuleAlignedBox( const Capsule & capsule, const AlignedBox & box, SweepResult & result );
    };
}