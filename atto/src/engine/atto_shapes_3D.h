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
    };

    struct Sphere {
        Vec3 center;
        f32 radius;
    };

    struct Capsule {
        Vec3    base;    // centre of the BOTTOM hemisphere
        f32     height;  // distance between the two hemisphere centres
        f32     radius;

        inline static Capsule FromBaseTopRadius( const Vec3 & base, const Vec3 & top, f32 radius ) { return { base, glm::distance( base, top ), radius }; }
        inline static Capsule FromEndpoints( const Vec3 & p0, const Vec3 & p1, f32 radius ) { return { p0, glm::distance( p0, p1 ), radius }; }
        inline static Capsule FromCenterHeightAxis( const Vec3 & center, f32 height, const Vec3 & axisDir, f32 radius ) {
            Vec3 halfAxis = glm::normalize( axisDir ) * (height * 0.5f);
            Vec3 base = center - halfAxis;
            return { base, height, radius };
        }
    };

    class IntersectionTest {
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
        static bool CapsuleAlignedBox( const Capsule & capsule, const AlignedBox & box, SweepResult & result );
    };
}