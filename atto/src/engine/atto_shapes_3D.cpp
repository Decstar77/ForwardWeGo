#include "atto_shapes_3D.h"



namespace atto {
    bool  IntersectionTest::Sphere2( const Sphere & sphereA, const Sphere & sphereB ) {
        // Check if the distance between centers is less than sum of radii
        Vec3 delta = sphereB.center - sphereA.center;
        f32 distSq = Dot( delta, delta );
        f32 r = sphereA.radius + sphereB.radius;
        return distSq <= r * r;

    }

    bool IntersectionTest::AlignedBox2( const AlignedBox & boxA, const AlignedBox & boxB ) {
        return (boxA.min.x <= boxB.max.x && boxA.max.x >= boxB.min.x) &&
            (boxA.min.y <= boxB.max.y && boxA.max.y >= boxB.min.y) &&
            (boxA.min.z <= boxB.max.z && boxA.max.z >= boxB.min.z);
    }

    bool IntersectionTest::SphereAlignedBox( const Sphere & sphere, const AlignedBox & box ) {
        // Compute the squared distance from the sphere center to the box
        f32 sqDist = 0.0f;
        // Unrolled version of the loop for 3 dimensions (x, y, z)
        // X axis
        {
            f32 v = sphere.center[0];
            if (v < box.min[0]) {
                sqDist += (box.min[0] - v) * (box.min[0] - v);
            }
            else if (v > box.max[0]) {
                sqDist += (v - box.max[0]) * (v - box.max[0]);
            }
        }
        // Y axis
        {
            f32 v = sphere.center[1];
            if (v < box.min[1]) {
                sqDist += (box.min[1] - v) * (box.min[1] - v);
            }
            else if (v > box.max[1]) {
                sqDist += (v - box.max[1]) * (v - box.max[1]);
            }
        }
        // Z axis
        {
            f32 v = sphere.center[2];
            if (v < box.min[2]) {
                sqDist += (box.min[2] - v) * (box.min[2] - v);
            }
            else if (v > box.max[2]) {
                sqDist += (v - box.max[2]) * (v - box.max[2]);
            }
        }
        return sqDist <= sphere.radius * sphere.radius;
    }

    bool CollisionTest::SphereAlignedBox( const Sphere & sphere, const AlignedBox & box, Manifold & manifold ) {

    }
}