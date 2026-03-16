#include "atto_shapes_3D.h"



namespace atto {

    void AlignedBox::Translate( const Vec3 & delta ) {
        min += delta;
        max += delta;
    }

    void AlignedBox::Rotate( const Mat3 & rotation ) {
        Vec3 center = GetCenter();
        Vec3 extents[] = {
            min - center,
            Vec3( min.x - center.x, min.y - center.y, max.z - center.z ),
            Vec3( min.x - center.x, max.y - center.y, min.z - center.z ),
            Vec3( max.x - center.x, min.y - center.y, min.z - center.z ),
            Vec3( max.x - center.x, max.y - center.y, min.z - center.z ),
            Vec3( min.x - center.x, max.y - center.y, max.z - center.z ),
            Vec3( max.x - center.x, min.y - center.y, max.z - center.z ),
            max - center
        };
        Vec3 newMin( FLT_MAX );
        Vec3 newMax( -FLT_MAX );
        for ( int i = 0; i < 8; ++i ) {
            Vec3 pt = center + rotation * extents[i];
            newMin = glm::min( newMin, pt );
            newMax = glm::max( newMax, pt );
        }
        min = newMin;
        max = newMax;
    }

    void AlignedBox::RotateAround( const Vec3 & pivot, const Mat3 & rotation ) {
        // Rotates the box around a pivot point by a given rotation matrix
        Vec3 center = GetCenter();
        // Translate box so that pivot becomes the origin
        Vec3 minOffset = min - pivot;
        Vec3 maxOffset = max - pivot;

        // Find all 8 corners of the box, offset from the pivot
        Vec3 corners[8];
        corners[0] = minOffset;
        corners[1] = Vec3( min.x - pivot.x, min.y - pivot.y, max.z - pivot.z );
        corners[2] = Vec3( min.x - pivot.x, max.y - pivot.y, min.z - pivot.z );
        corners[3] = Vec3( max.x - pivot.x, min.y - pivot.y, min.z - pivot.z );
        corners[4] = Vec3( max.x - pivot.x, max.y - pivot.y, min.z - pivot.z );
        corners[5] = Vec3( min.x - pivot.x, max.y - pivot.y, max.z - pivot.z );
        corners[6] = Vec3( max.x - pivot.x, min.y - pivot.y, max.z - pivot.z );
        corners[7] = maxOffset;

        Vec3 newMin( FLT_MAX );
        Vec3 newMax( -FLT_MAX );

        // Rotate all corners around the pivot and find new bounds
        for ( int i = 0; i < 8; ++i ) {
            Vec3 rotated = pivot + rotation * corners[i];
            newMin = glm::min( newMin, rotated );
            newMax = glm::max( newMax, rotated );
        }
        min = newMin;
        max = newMax;
    }

    bool Raycast::TestSphere( const Vec3 & rayOrigin, const Vec3 & rayDirection, const Sphere & sphere, f32 & dist ) {
        Vec3 oc = rayOrigin - sphere.center;
        f32 a = Dot( rayDirection, rayDirection );
        f32 b = 2.0f * Dot( oc, rayDirection );
        f32 c = Dot( oc, oc ) - sphere.radius * sphere.radius;
        f32 discriminant = b * b - 4.0f * a * c;

        if ( discriminant < 0.0f ) {
            return false;
        }

        f32 sqrtD = Sqrt( discriminant );
        f32 invA2 = 1.0f / (2.0f * a);

        f32 t0 = (-b - sqrtD) * invA2;
        f32 t1 = (-b + sqrtD) * invA2;

        if ( t0 >= 0.0f ) {
            dist = t0;
            return true;
        }

        if ( t1 >= 0.0f ) {
            dist = t1;
            return true;
        }

        return false;
    }

    bool Raycast::TestAlignedBox( const Vec3 & rayOrigin, const Vec3 & rayDirection, const AlignedBox & alignedBox, f32 & dist ) {
        f32 tMin = -FLT_MAX;
        f32 tMax = FLT_MAX;

        for ( i32 i = 0; i < 3; i++ ) {
            if ( Abs( rayDirection[i] ) < 0.000001f ) {
                if ( rayOrigin[i] < alignedBox.min[i] || rayOrigin[i] > alignedBox.max[i] ) {
                    return false;
                }
            }
            else {
                f32 invD = 1.0f / rayDirection[i];
                f32 t1 = (alignedBox.min[i] - rayOrigin[i]) * invD;
                f32 t2 = (alignedBox.max[i] - rayOrigin[i]) * invD;

                if ( t1 > t2 ) {
                    f32 tmp = t1;
                    t1 = t2;
                    t2 = tmp;
                }

                tMin = Max( tMin, t1 );
                tMax = Min( tMax, t2 );

                if ( tMin > tMax ) {
                    return false;
                }
            }
        }

        if ( tMin >= 0.0f ) {
            dist = tMin;
            return true;
        }

        if ( tMax >= 0.0f ) {
            dist = tMax;
            return true;
        }

        return false;
    }

    bool Raycast::TestCapsule( const Vec3 & rayOrigin, const Vec3 & rayDirection, const Capsule & capsule, f32 & dist ) {
        Vec3 top = capsule.base + Vec3( 0.0f, capsule.height, 0.0f );
        f32 r = capsule.radius;
        f32 bestT = FLT_MAX;
        bool hit = false;

        // Test the infinite cylinder in XZ (Y-aligned axis through capsule.base)
        f32 ocX = rayOrigin.x - capsule.base.x;
        f32 ocZ = rayOrigin.z - capsule.base.z;
        f32 a = rayDirection.x * rayDirection.x + rayDirection.z * rayDirection.z;
        f32 b = 2.0f * (ocX * rayDirection.x + ocZ * rayDirection.z);
        f32 c = ocX * ocX + ocZ * ocZ - r * r;

        f32 discriminant = b * b - 4.0f * a * c;
        if ( a > 0.000001f && discriminant >= 0.0f ) {
            f32 sqrtD = Sqrt( discriminant );
            f32 invA2 = 1.0f / (2.0f * a);
            f32 roots[2] = { (-b - sqrtD) * invA2, (-b + sqrtD) * invA2 };

            for ( i32 i = 0; i < 2; i++ ) {
                f32 t = roots[i];
                if ( t < 0.0f ) continue;
                f32 y = rayOrigin.y + t * rayDirection.y;
                if ( y >= capsule.base.y && y <= top.y ) {
                    if ( t < bestT ) {
                        bestT = t;
                        hit = true;
                    }
                }
            }
        }

        // Test both hemisphere caps
        Sphere caps[2] = {
            { capsule.base, r },
            { top, r },
        };

        for ( i32 i = 0; i < 2; i++ ) {
            f32 t = 0.0f;
            if ( TestSphere( rayOrigin, rayDirection, caps[i], t ) ) {
                Vec3 hitPt = rayOrigin + rayDirection * t;
                bool inHemisphere = (i == 0) ? (hitPt.y <= capsule.base.y) : (hitPt.y >= top.y);
                if ( inHemisphere && t < bestT ) {
                    bestT = t;
                    hit = true;
                }
            }
        }

        if ( hit ) {
            dist = bestT;
        }

        return hit;
    }

    bool IntersectionTest::Sphere2( const Sphere & sphereA, const Sphere & sphereB ) {
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
            if ( v < box.min[0] ) {
                sqDist += (box.min[0] - v) * (box.min[0] - v);
            }
            else if ( v > box.max[0] ) {
                sqDist += (v - box.max[0]) * (v - box.max[0]);
            }
        }
        // Y axis
        {
            f32 v = sphere.center[1];
            if ( v < box.min[1] ) {
                sqDist += (box.min[1] - v) * (box.min[1] - v);
            }
            else if ( v > box.max[1] ) {
                sqDist += (v - box.max[1]) * (v - box.max[1]);
            }
        }
        // Z axis
        {
            f32 v = sphere.center[2];
            if ( v < box.min[2] ) {
                sqDist += (box.min[2] - v) * (box.min[2] - v);
            }
            else if ( v > box.max[2] ) {
                sqDist += (v - box.max[2]) * (v - box.max[2]);
            }
        }
        return sqDist <= sphere.radius * sphere.radius;
    }

    bool IntersectionTest::CapsuleAlignedBox( const Capsule & capsule, const AlignedBox & box ) {
        // Capsule axis is vertical (Y-up): from base to base + (0, height, 0).
        // X and Z are constant along the segment, so only Y affects distance.
        f32 segMinY = capsule.base.y;
        f32 segMaxY = capsule.base.y + capsule.height;

        // Find the Y on the segment that minimizes squared distance to the box.
        f32 bestY = Clamp( Clamp( segMinY, box.min.y, box.max.y ), segMinY, segMaxY );

        Sphere s = { Vec3( capsule.base.x, bestY, capsule.base.z ), capsule.radius };
        return SphereAlignedBox( s, box );
    }

    bool CollisionSweep::CapsuleAlignedBox( const Capsule & capsule, const AlignedBox & box, SweepResult & result ) {
        f32 segMinY = capsule.base.y;
        f32 segMaxY = capsule.base.y + capsule.height;
        f32 bestY = Clamp( Clamp( segMinY, box.min.y, box.max.y ), segMinY, segMaxY );
        Vec3 axisPoint = Vec3( capsule.base.x, bestY, capsule.base.z );

        Vec3 boxPoint = Vec3(
            Clamp( axisPoint.x, box.min.x, box.max.x ),
            Clamp( axisPoint.y, box.min.y, box.max.y ),
            Clamp( axisPoint.z, box.min.z, box.max.z )
        );

        Vec3 delta = axisPoint - boxPoint;
        f32 distSq = Dot( delta, delta );
        f32 r = capsule.radius;

        if ( distSq > r * r ) {
            return false;
        }

        f32 dist = Sqrt( distSq );
        result.toi = 0.0f;

        if ( dist > 0.0001f ) {
            // Axis point is outside the box but sphere shell overlaps
            result.normal = delta / dist;
            result.pen = r - dist;
        }
        else {
            // Axis point is inside the box -- find the nearest face for minimum push-out
            f32 faces[6] = {
                axisPoint.x - box.min.x,
                box.max.x - axisPoint.x,
                axisPoint.y - box.min.y,
                box.max.y - axisPoint.y,
                axisPoint.z - box.min.z,
                box.max.z - axisPoint.z,
            };

            Vec3 normals[6] = {
                Vec3( -1, 0, 0 ), Vec3( 1, 0, 0 ),
                Vec3( 0, -1, 0 ), Vec3( 0, 1, 0 ),
                Vec3( 0, 0, -1 ), Vec3( 0, 0, 1 ),
            };

            i32 minIdx = 0;
            for ( i32 i = 1; i < 6; i++ ) {
                if ( faces[i] < faces[minIdx] ) {
                    minIdx = i;
                }
            }

            result.normal = normals[minIdx];
            result.pen = faces[minIdx] + r;
        }

        return true;
    }

}