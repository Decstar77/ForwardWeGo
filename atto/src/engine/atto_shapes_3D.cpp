#include "atto_shapes_3D.h"


namespace atto {
    AlignedBox AlignedBox::FromMinMax( Vec3 min, Vec3 max ) {
        return { min, max };
    }

    AlignedBox AlignedBox::FromCenterSize( Vec3 center, Vec3 size ) {
        return { center - size * 0.5f, center + size * 0.5f };
    }

    Vec3 AlignedBox::GetCenter() const {
        return ( min + max ) * 0.5f;
    }

    Vec3 AlignedBox::GetSize() const {
        return max - min;
    }

    void AlignedBox::Translate( const Vec3 &delta ) {
        min += delta;
        max += delta;
    }

    void AlignedBox::Rotate( const Mat3 &rotation ) {
        Vec3 center = GetCenter();
        Vec3 extents[ ] = {
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
            Vec3 pt = center + rotation * extents[ i ];
            newMin = glm::min( newMin, pt );
            newMax = glm::max( newMax, pt );
        }
        min = newMin;
        max = newMax;
    }

    void AlignedBox::RotateAround( const Vec3 &pivot, const Mat3 &rotation ) {
        // Translate box so that pivot becomes the origin
        Vec3 minOffset = min - pivot;
        Vec3 maxOffset = max - pivot;

        // Find all 8 corners of the box, offset from the pivot
        Vec3 corners[ 8 ];
        corners[ 0 ] = minOffset;
        corners[ 1 ] = Vec3( min.x - pivot.x, min.y - pivot.y, max.z - pivot.z );
        corners[ 2 ] = Vec3( min.x - pivot.x, max.y - pivot.y, min.z - pivot.z );
        corners[ 3 ] = Vec3( max.x - pivot.x, min.y - pivot.y, min.z - pivot.z );
        corners[ 4 ] = Vec3( max.x - pivot.x, max.y - pivot.y, min.z - pivot.z );
        corners[ 5 ] = Vec3( min.x - pivot.x, max.y - pivot.y, max.z - pivot.z );
        corners[ 6 ] = Vec3( max.x - pivot.x, min.y - pivot.y, max.z - pivot.z );
        corners[ 7 ] = maxOffset;

        Vec3 newMin( FLT_MAX );
        Vec3 newMax( -FLT_MAX );

        // Rotate all corners around the pivot and find new bounds
        for ( int i = 0; i < 8; ++i ) {
            Vec3 rotated = pivot + rotation * corners[ i ];
            newMin = glm::min( newMin, rotated );
            newMax = glm::max( newMax, rotated );
        }
        min = newMin;
        max = newMax;
    }

    Vec3 AlignedBox::ClosestPoint( const Vec3 &point ) const {
        return Vec3(
            Clamp( point.x, min.x, max.x ),
            Clamp( point.y, min.y, max.y ),
            Clamp( point.z, min.z, max.z )
        );
    }

    bool Raycast::TestSphere( const Vec3 &rayOrigin, const Vec3 &rayDirection, const Sphere &sphere, f32 &dist ) {
        Vec3 oc = rayOrigin - sphere.center;
        f32 a = Dot( rayDirection, rayDirection );
        f32 b = 2.0f * Dot( oc, rayDirection );
        f32 c = Dot( oc, oc ) - sphere.radius * sphere.radius;
        f32 discriminant = b * b - 4.0f * a * c;

        if ( discriminant < 0.0f ) {
            return false;
        }

        f32 sqrtD = Sqrt( discriminant );
        f32 invA2 = 1.0f / ( 2.0f * a );

        f32 t0 = ( -b - sqrtD ) * invA2;
        f32 t1 = ( -b + sqrtD ) * invA2;

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

    bool Raycast::TestAlignedBox( const Vec3 &rayOrigin, const Vec3 &rayDirection, const AlignedBox &alignedBox,
                                  f32 &dist ) {
        f32 tMin = -FLT_MAX;
        f32 tMax = FLT_MAX;

        for ( i32 i = 0; i < 3; i++ ) {
            if ( Abs( rayDirection[ i ] ) < 0.000001f ) {
                if ( rayOrigin[ i ] < alignedBox.min[ i ] || rayOrigin[ i ] > alignedBox.max[ i ] ) {
                    return false;
                }
            } else {
                f32 invD = 1.0f / rayDirection[ i ];
                f32 t1 = ( alignedBox.min[ i ] - rayOrigin[ i ] ) * invD;
                f32 t2 = ( alignedBox.max[ i ] - rayOrigin[ i ] ) * invD;

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

    // ===============================================================
    //  Box (OBB)
    // ===============================================================

    Box Box::FromMinMax( const Vec3 min, const Vec3 max ) {
        Vec3 c = ( min + max ) * 0.5f;
        Vec3 he = ( max - min ) * 0.5f;
        return { c, Mat3( 1 ), he };
    }

    Box Box::FromCenterSize( const Vec3 center, const Vec3 halfExtent ) {
        return { center, Mat3( 1 ), halfExtent };
    }

    Box Box::FromCenterSizeOrientation( const Vec3 center, const Vec3 halfExtent, const Mat3 & orientation ) {
        return { center, orientation, halfExtent };
    }

    Box Box::FromAlignedBox( const AlignedBox & box ) {
        Box result = {};
        result.center = box.GetCenter();
        result.halfExtent = box.GetSize() / 2.0f;
        return result;
    }

    void Box::Translate( const Vec3 & delta ) {
        center += delta;
    }

    void Box::Rotate( const Mat3 & rotation ) {
        orientation = rotation * orientation;
    }

    void Box::RotateAround( const Vec3 & pivot, const Mat3 & rotation ) {
        center = pivot + rotation * ( center - pivot );
        orientation = rotation * orientation;
    }

    void Box::RotateXLocal( const f32 amount ) {
        const Mat3 r = Mat3( glm::rotate( Mat4( 1 ), amount, Vec3( orientation[0] ) ) );
        orientation = r * orientation;
    }

    void Box::RotateYLocal( const f32 amount ) {
        const Mat3 r = Mat3( glm::rotate( Mat4( 1 ), amount, Vec3( orientation[1] ) ) );
        orientation = r * orientation;
    }

    void Box::RotateZLocal( const f32 amount ) {
        const Mat3 r = Mat3( glm::rotate( Mat4( 1 ), amount, Vec3( orientation[2] ) ) );
        orientation = r * orientation;
    }

    void Box::RotateXGlobal( const f32 amount ) {
        const Mat3 r = Mat3( glm::rotate( Mat4( 1 ), amount, Vec3( 1, 0, 0 ) ) );
        orientation = r * orientation;
        center = r * center;
    }

    void Box::RotateYGlobal( const f32 amount ) {
        const Mat3 r = Mat3( glm::rotate( Mat4( 1 ), amount, Vec3( 0, 1, 0 ) ) );
        orientation = r * orientation;
        center = r * center;
    }

    void Box::RotateZGlobal( const f32 amount ) {
        const Mat3 r = Mat3( glm::rotate( Mat4( 1 ), amount, Vec3( 0, 0, 1 ) ) );
        orientation = r * orientation;
        center = r * center;
    }

    void Box::GetCorners( Vec3 * cornerArray ) const {
        const Vec3 ax = orientation[0] * halfExtent.x;
        const Vec3 ay = orientation[1] * halfExtent.y;
        const Vec3 az = orientation[2] * halfExtent.z;
        cornerArray[0] = center - ax - ay - az;
        cornerArray[1] = center + ax - ay - az;
        cornerArray[2] = center - ax + ay - az;
        cornerArray[3] = center + ax + ay - az;
        cornerArray[4] = center - ax - ay + az;
        cornerArray[5] = center + ax - ay + az;
        cornerArray[6] = center - ax + ay + az;
        cornerArray[7] = center + ax + ay + az;
    }

    // ===============================================================
    //  Raycast::TestBox  (OBB slab test)
    // ===============================================================
    bool Raycast::TestBox( const Vec3 & rayOrigin, const Vec3 & rayDirection, const Box & box, f32 & dist ) {
        Vec3 d = rayOrigin - box.center;
        f32 tMin = -FLT_MAX;
        f32 tMax = FLT_MAX;

        for ( i32 i = 0; i < 3; i++ ) {
            Vec3 axis = box.orientation[i];
            f32 e = Dot( axis, d );
            f32 f = Dot( axis, rayDirection );

            if ( Abs( f ) > 0.000001f ) {
                f32 invF = 1.0f / f;
                f32 t1 = ( -e - box.halfExtent[i] ) * invF;
                f32 t2 = ( -e + box.halfExtent[i] ) * invF;
                if ( t1 > t2 ) { f32 tmp = t1; t1 = t2; t2 = tmp; }
                tMin = Max( tMin, t1 );
                tMax = Min( tMax, t2 );
                if ( tMin > tMax ) { return false; }
            } else {
                if ( -e - box.halfExtent[i] > 0.0f || -e + box.halfExtent[i] < 0.0f ) {
                    return false;
                }
            }
        }

        if ( tMin >= 0.0f ) { dist = tMin; return true; }
        if ( tMax >= 0.0f ) { dist = tMax; return true; }
        return false;
    }

    bool Raycast::TestCapsule( const Vec3 &rayOrigin, const Vec3 &rayDirection, const Capsule &capsule, f32 &dist ) {
        Vec3 top = capsule.base + Vec3( 0.0f, capsule.height, 0.0f );
        f32 r = capsule.radius;
        f32 bestT = FLT_MAX;
        bool hit = false;

        // Test the infinite cylinder in XZ (Y-aligned axis through capsule.base)
        f32 ocX = rayOrigin.x - capsule.base.x;
        f32 ocZ = rayOrigin.z - capsule.base.z;
        f32 a = rayDirection.x * rayDirection.x + rayDirection.z * rayDirection.z;
        f32 b = 2.0f * ( ocX * rayDirection.x + ocZ * rayDirection.z );
        f32 c = ocX * ocX + ocZ * ocZ - r * r;

        f32 discriminant = b * b - 4.0f * a * c;
        if ( a > 0.000001f && discriminant >= 0.0f ) {
            f32 sqrtD = Sqrt( discriminant );
            f32 invA2 = 1.0f / ( 2.0f * a );
            f32 roots[ 2 ] = { ( -b - sqrtD ) * invA2, ( -b + sqrtD ) * invA2 };

            for ( i32 i = 0; i < 2; i++ ) {
                f32 t = roots[ i ];
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
        Sphere caps[ 2 ] = {
            { capsule.base, r },
            { top, r },
        };

        for ( i32 i = 0; i < 2; i++ ) {
            f32 t = 0.0f;
            if ( TestSphere( rayOrigin, rayDirection, caps[ i ], t ) ) {
                Vec3 hitPt = rayOrigin + rayDirection * t;
                bool inHemisphere = ( i == 0 ) ? ( hitPt.y <= capsule.base.y ) : ( hitPt.y >= top.y );
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

    bool IntersectionTest::Sphere2( const Sphere &sphereA, const Sphere &sphereB ) {
        // Check if the distance between centers is less than sum of radii
        Vec3 delta = sphereB.center - sphereA.center;
        f32 distSq = Dot( delta, delta );
        f32 r = sphereA.radius + sphereB.radius;
        return distSq <= r * r;
    }

    bool IntersectionTest::AlignedBox2( const AlignedBox &boxA, const AlignedBox &boxB ) {
        return ( boxA.min.x <= boxB.max.x && boxA.max.x >= boxB.min.x ) &&
               ( boxA.min.y <= boxB.max.y && boxA.max.y >= boxB.min.y ) &&
               ( boxA.min.z <= boxB.max.z && boxA.max.z >= boxB.min.z );
    }

    bool IntersectionTest::SphereAlignedBox( const Sphere &sphere, const AlignedBox &box ) {
        // Compute the squared distance from the sphere center to the box
        f32 sqDist = 0.0f;
        // Unrolled version of the loop for 3 dimensions (x, y, z)
        // X axis
        {
            f32 v = sphere.center[ 0 ];
            if ( v < box.min[ 0 ] ) {
                sqDist += ( box.min[ 0 ] - v ) * ( box.min[ 0 ] - v );
            } else if ( v > box.max[ 0 ] ) {
                sqDist += ( v - box.max[ 0 ] ) * ( v - box.max[ 0 ] );
            }
        }
        // Y axis
        {
            f32 v = sphere.center[ 1 ];
            if ( v < box.min[ 1 ] ) {
                sqDist += ( box.min[ 1 ] - v ) * ( box.min[ 1 ] - v );
            } else if ( v > box.max[ 1 ] ) {
                sqDist += ( v - box.max[ 1 ] ) * ( v - box.max[ 1 ] );
            }
        }
        // Z axis
        {
            f32 v = sphere.center[ 2 ];
            if ( v < box.min[ 2 ] ) {
                sqDist += ( box.min[ 2 ] - v ) * ( box.min[ 2 ] - v );
            } else if ( v > box.max[ 2 ] ) {
                sqDist += ( v - box.max[ 2 ] ) * ( v - box.max[ 2 ] );
            }
        }
        return sqDist <= sphere.radius * sphere.radius;
    }

    bool IntersectionTest::CapsuleAlignedBox( const Capsule &capsule, const AlignedBox &box ) {
        // Capsule axis is vertical (Y-up): from base to base + (0, height, 0).
        // X and Z are constant along the segment, so only Y affects distance.
        f32 segMinY = capsule.base.y;
        f32 segMaxY = capsule.base.y + capsule.height;

        // Find the Y on the segment that minimizes squared distance to the box.
        f32 bestY = Clamp( Clamp( segMinY, box.min.y, box.max.y ), segMinY, segMaxY );

        Sphere s = { Vec3( capsule.base.x, bestY, capsule.base.z ), capsule.radius };
        return SphereAlignedBox( s, box );
    }

    bool CollisionSweep::CapsuleAlignedBox( const Capsule &capsule, const AlignedBox &box, SweepResult &result ) {
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
        } else {
            // Axis point is inside the box -- find the nearest face for minimum push-out
            f32 faces[ 6 ] = {
                axisPoint.x - box.min.x,
                box.max.x - axisPoint.x,
                axisPoint.y - box.min.y,
                box.max.y - axisPoint.y,
                axisPoint.z - box.min.z,
                box.max.z - axisPoint.z,
            };

            Vec3 normals[ 6 ] = {
                Vec3( -1, 0, 0 ), Vec3( 1, 0, 0 ),
                Vec3( 0, -1, 0 ), Vec3( 0, 1, 0 ),
                Vec3( 0, 0, -1 ), Vec3( 0, 0, 1 ),
            };

            i32 minIdx = 0;
            for ( i32 i = 1; i < 6; i++ ) {
                if ( faces[ i ] < faces[ minIdx ] ) {
                    minIdx = i;
                }
            }

            result.normal = normals[ minIdx ];
            result.pen = faces[ minIdx ] + r;
        }

        return true;
    }

    // ===============================================================
    //  OBB helpers (file-local)
    // ===============================================================

    static Vec3 ClosestPointOnBox( const Box & box, const Vec3 & point ) {
        Vec3 d = point - box.center;
        Vec3 result = box.center;
        for ( i32 i = 0; i < 3; i++ ) {
            f32 proj = Clamp( Dot( d, box.orientation[i] ), -box.halfExtent[i], box.halfExtent[i] );
            result += box.orientation[i] * proj;
        }
        return result;
    }

    // ===============================================================
    //  IntersectionTest::SphereBox
    // ===============================================================

    bool IntersectionTest::SphereBox( const Sphere & sphere, const Box & box ) {
        Vec3 closest = ClosestPointOnBox( box, sphere.center );
        Vec3 diff = sphere.center - closest;
        return Dot( diff, diff ) <= sphere.radius * sphere.radius;
    }

    // ===============================================================
    //  OBB vs OBB  –  SAT (Gottschalk / Ericson)
    //  Used by BoxBox2 and AlignedBoxBox
    // ===============================================================

    static bool OBBOverlapSAT( const Vec3 & centerA, const Mat3 & oriA, const Vec3 & heA,
                               const Vec3 & centerB, const Mat3 & oriB, const Vec3 & heB ) {
        Vec3 d = centerB - centerA;

        f32 R[3][3], AbsR[3][3];
        for ( i32 i = 0; i < 3; i++ ) {
            for ( i32 j = 0; j < 3; j++ ) {
                R[i][j] = Dot( oriA[i], oriB[j] );
                AbsR[i][j] = Abs( R[i][j] ) + 0.000001f;
            }
        }

        Vec3 t = Vec3( Dot( d, oriA[0] ), Dot( d, oriA[1] ), Dot( d, oriA[2] ) );

        f32 ra, rb;

        // A's 3 axes
        for ( i32 i = 0; i < 3; i++ ) {
            ra = heA[i];
            rb = heB[0] * AbsR[i][0] + heB[1] * AbsR[i][1] + heB[2] * AbsR[i][2];
            if ( Abs( t[i] ) > ra + rb ) return false;
        }

        // B's 3 axes
        for ( i32 i = 0; i < 3; i++ ) {
            ra = heA[0] * AbsR[0][i] + heA[1] * AbsR[1][i] + heA[2] * AbsR[2][i];
            rb = heB[i];
            if ( Abs( t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i] ) > ra + rb ) return false;
        }

        // 9 cross-product axes
        // A0 x B0
        ra = heA[1] * AbsR[2][0] + heA[2] * AbsR[1][0];
        rb = heB[1] * AbsR[0][2] + heB[2] * AbsR[0][1];
        if ( Abs( t[2] * R[1][0] - t[1] * R[2][0] ) > ra + rb ) return false;

        // A0 x B1
        ra = heA[1] * AbsR[2][1] + heA[2] * AbsR[1][1];
        rb = heB[0] * AbsR[0][2] + heB[2] * AbsR[0][0];
        if ( Abs( t[2] * R[1][1] - t[1] * R[2][1] ) > ra + rb ) return false;

        // A0 x B2
        ra = heA[1] * AbsR[2][2] + heA[2] * AbsR[1][2];
        rb = heB[0] * AbsR[0][1] + heB[1] * AbsR[0][0];
        if ( Abs( t[2] * R[1][2] - t[1] * R[2][2] ) > ra + rb ) return false;

        // A1 x B0
        ra = heA[0] * AbsR[2][0] + heA[2] * AbsR[0][0];
        rb = heB[1] * AbsR[1][2] + heB[2] * AbsR[1][1];
        if ( Abs( t[0] * R[2][0] - t[2] * R[0][0] ) > ra + rb ) return false;

        // A1 x B1
        ra = heA[0] * AbsR[2][1] + heA[2] * AbsR[0][1];
        rb = heB[0] * AbsR[1][2] + heB[2] * AbsR[1][0];
        if ( Abs( t[0] * R[2][1] - t[2] * R[0][1] ) > ra + rb ) return false;

        // A1 x B2
        ra = heA[0] * AbsR[2][2] + heA[2] * AbsR[0][2];
        rb = heB[0] * AbsR[1][1] + heB[1] * AbsR[1][0];
        if ( Abs( t[0] * R[2][2] - t[2] * R[0][2] ) > ra + rb ) return false;

        // A2 x B0
        ra = heA[0] * AbsR[1][0] + heA[1] * AbsR[0][0];
        rb = heB[1] * AbsR[2][2] + heB[2] * AbsR[2][1];
        if ( Abs( t[1] * R[0][0] - t[0] * R[1][0] ) > ra + rb ) return false;

        // A2 x B1
        ra = heA[0] * AbsR[1][1] + heA[1] * AbsR[0][1];
        rb = heB[0] * AbsR[2][2] + heB[2] * AbsR[2][0];
        if ( Abs( t[1] * R[0][1] - t[0] * R[1][1] ) > ra + rb ) return false;

        // A2 x B2
        ra = heA[0] * AbsR[1][2] + heA[1] * AbsR[0][2];
        rb = heB[0] * AbsR[2][1] + heB[1] * AbsR[2][0];
        if ( Abs( t[1] * R[0][2] - t[0] * R[1][2] ) > ra + rb ) return false;

        return true;
    }

    bool IntersectionTest::BoxBox2( const Box & boxA, AlignedBox & boxB ) {
        Vec3 bCenter = boxB.GetCenter();
        Vec3 bHalf   = boxB.GetSize() * 0.5f;
        return OBBOverlapSAT( boxA.center, boxA.orientation, boxA.halfExtent,
                              bCenter, Mat3( 1 ), bHalf );
    }

    bool IntersectionTest::AlignedBoxBox( const AlignedBox & alignedBox, const Box & box ) {
        Vec3 aCenter = alignedBox.GetCenter();
        Vec3 aHalf   = alignedBox.GetSize() * 0.5f;
        return OBBOverlapSAT( aCenter, Mat3( 1 ), aHalf,
                              box.center, box.orientation, box.halfExtent );
    }

    // ===============================================================
    //  IntersectionTest::CapsuleBox
    //  Transform capsule into OBB local space, then segment-vs-AABB
    // ===============================================================

    bool IntersectionTest::CapsuleBox( const Capsule & capsule, const Box & box ) {
        Mat3 invOri = glm::transpose( box.orientation );
        Vec3 localBase = invOri * ( capsule.base - box.center );
        Vec3 localDir  = invOri * Vec3( 0.0f, capsule.height, 0.0f );
        Vec3 localTop  = localBase + localDir;
        Vec3 he = box.halfExtent;

        // Iterative closest-point between segment and local AABB (2 iterations)
        Vec3 segDir = localTop - localBase;
        f32  segLenSq = Dot( segDir, segDir );

        // Initial guess: project box center (origin) onto segment
        f32 t = ( segLenSq > 0.000001f ) ? Clamp( -Dot( localBase, segDir ) / segLenSq, 0.0f, 1.0f ) : 0.0f;
        Vec3 segPt = localBase + segDir * t;

        // Closest point on AABB to segPt
        Vec3 boxPt;
        boxPt.x = Clamp( segPt.x, -he.x, he.x );
        boxPt.y = Clamp( segPt.y, -he.y, he.y );
        boxPt.z = Clamp( segPt.z, -he.z, he.z );

        // Refine: closest point on segment to boxPt
        t = ( segLenSq > 0.000001f ) ? Clamp( Dot( boxPt - localBase, segDir ) / segLenSq, 0.0f, 1.0f ) : 0.0f;
        segPt = localBase + segDir * t;

        // Final closest point on AABB
        boxPt.x = Clamp( segPt.x, -he.x, he.x );
        boxPt.y = Clamp( segPt.y, -he.y, he.y );
        boxPt.z = Clamp( segPt.z, -he.z, he.z );

        Vec3 diff = segPt - boxPt;
        return Dot( diff, diff ) <= capsule.radius * capsule.radius;
    }

    // ===============================================================
    //  CollisionSweep::CapsuleBox
    //  Same local-space approach with penetration depth + normal
    // ===============================================================

    bool CollisionSweep::CapsuleBox( const Capsule & capsule, const Box & box, SweepResult & result ) {
        Mat3 invOri = glm::transpose( box.orientation );
        Vec3 localBase = invOri * ( capsule.base - box.center );
        Vec3 localDir  = invOri * Vec3( 0.0f, capsule.height, 0.0f );
        Vec3 localTop  = localBase + localDir;
        Vec3 he = box.halfExtent;
        f32  r  = capsule.radius;

        Vec3 segDir = localTop - localBase;
        f32  segLenSq = Dot( segDir, segDir );

        f32 t = ( segLenSq > 0.000001f ) ? Clamp( -Dot( localBase, segDir ) / segLenSq, 0.0f, 1.0f ) : 0.0f;
        Vec3 segPt = localBase + segDir * t;

        Vec3 boxPt;
        boxPt.x = Clamp( segPt.x, -he.x, he.x );
        boxPt.y = Clamp( segPt.y, -he.y, he.y );
        boxPt.z = Clamp( segPt.z, -he.z, he.z );

        t = ( segLenSq > 0.000001f ) ? Clamp( Dot( boxPt - localBase, segDir ) / segLenSq, 0.0f, 1.0f ) : 0.0f;
        segPt = localBase + segDir * t;

        boxPt.x = Clamp( segPt.x, -he.x, he.x );
        boxPt.y = Clamp( segPt.y, -he.y, he.y );
        boxPt.z = Clamp( segPt.z, -he.z, he.z );

        Vec3 delta = segPt - boxPt;
        f32 distSq = Dot( delta, delta );

        if ( distSq > r * r ) {
            return false;
        }

        f32 dist = Sqrt( distSq );
        result.toi = 0.0f;

        if ( dist > 0.0001f ) {
            Vec3 localNormal = delta / dist;
            result.normal = box.orientation * localNormal;
            result.pen = r - dist;
        } else {
            // Segment point is inside the local AABB — find nearest face
            f32 faces[6] = {
                segPt.x - ( -he.x ),
                he.x - segPt.x,
                segPt.y - ( -he.y ),
                he.y - segPt.y,
                segPt.z - ( -he.z ),
                he.z - segPt.z,
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

            result.normal = box.orientation * normals[minIdx];
            result.pen = faces[minIdx] + r;
        }

        return true;
    }
}
