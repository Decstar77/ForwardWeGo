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

    class IntersectionTest {
        static bool Sphere2( const Sphere & sphereA, const Sphere & sphereB );
        static bool AlignedBox2(  const AlignedBox & boxA, const AlignedBox & boxB );
        static bool SphereAlignedBox( const Sphere & sphere, const AlignedBox & box );
    };

    struct Manifold {
        Vec3 pointOnA;
        Vec3 pointOnB;
        f32 pen;
        Vec3 normal;
    };

    class CollisionTest {
        static bool SphereAlignedBox( const Sphere & sphere, const AlignedBox & box, Manifold & manifold );
    };
}