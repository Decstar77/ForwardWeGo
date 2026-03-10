#include "atto_log.h"
#include "atto_shapes.h"
#include "atto_assets.h"
#include "clipper2/clipper.h"

#include <algorithm>

namespace atto {

    /*
    =============================
    =============================
    */
    Circle::Circle() : position( 0, 0 ), radius( 0 ) {

    }

    /*
    =============================
    =============================
    */
    Circle::Circle( f32 x, f32 y, f32 r ) : position( x, y ), radius( r ) {

    }

    /*
    =============================
    =============================
    */
    bool Circle::Intersects( const Circle & other ) const {
        return DistanceSquared( position, other.position ) <= (radius + other.radius) * (radius + other.radius);
    }

    /*
    =============================
    =============================
    */
    void Circle::Serialize( Serializer & serializer ) {
        serializer( "position", position );
        serializer( "radius", radius );
    }

    /*
    =============================
    =============================
    */
    Rect::Rect() : x( 0 ), y( 0 ), width( 0 ), height( 0 ) {

    }

    /*
    =============================
    =============================
    */
    Rect::Rect( f32 x, f32 y, f32 w, f32 h ) : x( x ), y( y ), width( w ), height( h ) {

    }

    /*
    =============================
    =============================
    */
    Rect Rect::FromCenter( const Vec2 & center, f32 width, f32 height ) {
        return Rect( center.x - width * 0.5f, center.y - height * 0.5f, width, height );
    }

    /*
    =============================
    =============================
    */
    f32 Rect::Left() const {
        return x;
    }

    /*
    =============================
    =============================
    */
    f32 Rect::Right() const {
        return x + width;
    }

    /*
    =============================
    =============================
    */
    f32 Rect::Top() const {
        return y;
    }

    /*
    =============================
    =============================
    */
    f32 Rect::Bottom() const {
        return y + height;
    }

    /*
    =============================
    =============================
    */
    Vec2 Rect::Size() const {
        return Vec2( width, height );
    }

    /*
    =============================
    =============================
    */
    Vec2 Rect::GetCenter() const {
        return Vec2( x + width * 0.5f, y + height * 0.5f );
    }

    /*
    =============================
    =============================
    */
    bool Rect::Contains( f32 px, f32 py ) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }

    /*
    =============================
    =============================
    */
    bool Rect::Contains( const Vec2 & point ) const {
        return Contains( point.x, point.y );
    }

    /*
    =============================
    =============================
    */
    void Rect::Translate( const Vec2 & delta ) {
        x += delta.x;
        y += delta.y;
    }

    /*
    =============================
    =============================
    */
    bool Rect::Intersects( const Rect & other ) const {
        return !(other.x > Right() || other.Right() < x ||
            other.y > Bottom() || other.Bottom() < y);
    }

    /*
    =============================
    =============================
    */
    bool Rect::Intersects( const Vec2 & circlePos, f32 circleRadius ) const {
        f32 distSq = DistanceSquared( circlePos, GetCenter() );
        return distSq <= circleRadius * circleRadius;
    }

    /*
    =============================
    =============================
    */
    void Rect::InvertY() {
        y = -y;
        height = -height;
    }

    /*
    =============================
    =============================
    */
    void Rect::InvertX() {
        x = -x;
        width = -width;
    }

    /*
    =============================
    =============================
    */
    bool Rect::Intersects( const Vec2 & circlePos, f32 circleRadius, f32 & penetration, Vec2 & normal ) const {
        // Clamp circle center to rectangle bounds
        f32 closestX = Max( x, Min( circlePos.x, x + width ) );
        f32 closestY = Max( y, Min( circlePos.y, y + height ) );

        Vec2 closestPoint = Vec2( closestX, closestY );
        Vec2 delta = circlePos - closestPoint;

        f32 distSq = LengthSquared( delta );
        f32 radiusSq = circleRadius * circleRadius;

        if ( distSq > radiusSq )
            return false;

        f32 dist = std::sqrt( distSq );

        // Circle center is outside rectangle
        if ( dist > 0.0001f ) {
            normal = delta / dist;
            penetration = circleRadius - dist;
        }
        // Circle center is inside rectangle (special case)
        else {
            // Find closest edge
            f32 left = circlePos.x - x;
            f32 right = (x + width) - circlePos.x;
            f32 top = circlePos.y - y;
            f32 bottom = (y + height) - circlePos.y;

            f32 minDist = left;
            normal = { -1, 0 };

            if ( right < minDist ) { minDist = right;  normal = { 1, 0 }; }
            if ( top < minDist ) { minDist = top;    normal = { 0, -1 }; }
            if ( bottom < minDist ) { minDist = bottom; normal = { 0, 1 }; }

            penetration = circleRadius + minDist;
        }

        return true;
    }

    /*
    =============================
    =============================
    */
    Vec2 Rect::ClosestPoint( const Vec2 & p ) const {
        return Vec2( Max( x, Min( p.x, x + width ) ), Max( y, Min( p.y, y + height ) ) );
    }

    /*
    =============================
    =============================
    */
    void Rect::Serialize( Serializer & serializer ) {
        serializer( "x", x );
        serializer( "y", y );
        serializer( "width", width );
        serializer( "height", height );
    }

    /*
    =============================
    =============================
    */
    Triangle::Triangle() : a( 0, 0 ), b( 0, 0 ), c( 0, 0 ) {

    }

    /*
    =============================
    =============================
    */
    Triangle::Triangle( const Vec2 & a, const Vec2 & b, const Vec2 & c ) : a( a ), b( b ), c( c ) {

    }

    /*
    =============================
    =============================
    */
    Triangle::Triangle( f32 ax, f32 ay, f32 bx, f32 by, f32 cx, f32 cy )
        : a( ax, ay ), b( bx, by ), c( cx, cy ) {
    }

    /*
    =============================
    =============================
    */
    void Triangle::Serialize( Serializer & serializer ) {
        serializer( "a", a );
        serializer( "b", b );
        serializer( "c", c );
    }

    using namespace Clipper2Lib;

    // Helper functions to convert between atto and Clipper2 types
    static PointD ToClipperPoint( const Vec2 & v ) {
        return PointD( static_cast<double>(v.x), static_cast<double>(v.y) );
    }

    static Vec2 FromClipperPoint( const PointD & p ) {
        return Vec2( static_cast<f32>(p.x), static_cast<f32>(p.y) );
    }

    // Polygon implementation
    Polygon::Polygon() : paths( new PathsD() ) {}

    Polygon::~Polygon() {
        delete paths;
    }

    Polygon::Polygon( const Polygon & other ) : paths( new PathsD( *other.paths ) ) {}

    Polygon::Polygon( Polygon && other ) noexcept : paths( other.paths ) {
        other.paths = new PathsD();
    }

    Polygon & Polygon::operator=( const Polygon & other ) {
        if ( this != &other ) {
            *paths = *other.paths;
        }
        return *this;
    }

    Polygon & Polygon::operator=( Polygon && other ) noexcept {
        if ( this != &other ) {
            delete paths;
            paths = other.paths;
            other.paths = new PathsD();
        }
        return *this;
    }

    Polygon::Polygon( const PathsD & clipperPaths ) : paths( new PathsD( clipperPaths ) ) {}

    // Factory methods
    Polygon Polygon::CreateRect( f32 x, f32 y, f32 width, f32 height ) {
        Polygon poly;
        PathD path;
        path.reserve( 4 );
        path.push_back( PointD( x, y ) );
        path.push_back( PointD( x + width, y ) );
        path.push_back( PointD( x + width, y + height ) );
        path.push_back( PointD( x, y + height ) );
        poly.paths->push_back( std::move( path ) );
        return poly;
    }

    Polygon Polygon::CreateRect( const Rect & rect ) {
        return CreateRect( rect.x, rect.y, rect.width, rect.height );
    }

    Polygon Polygon::CreateCircle( const Vec2 & center, f32 radius, i32 segments ) {
        Polygon poly;
        if ( radius <= 0 || segments < 3 ) {
            return poly;
        }

        PathD path;
        path.reserve( segments );

        for ( i32 i = 0; i < segments; ++i ) {
            f32 angle = TWO_PI * static_cast<f32>( i ) / static_cast<f32>( segments );
            f32 px = center.x + radius * std::cos( angle );
            f32 py = center.y + radius * std::sin( angle );
            path.push_back( PointD( px, py ) );
        }

        poly.paths->push_back( std::move( path ) );
        return poly;
    }

    Polygon Polygon::CreateCircle( const Circle & circle, i32 segments ) {
        return CreateCircle( circle.position, circle.radius, segments );
    }

    Polygon Polygon::CreateTriangle( const Vec2 & a, const Vec2 & b, const Vec2 & c ) {
        Polygon poly;
        PathD path;
        path.reserve( 3 );
        path.push_back( ToClipperPoint( a ) );
        path.push_back( ToClipperPoint( b ) );
        path.push_back( ToClipperPoint( c ) );
        poly.paths->push_back( std::move( path ) );
        return poly;
    }

    Polygon Polygon::CreateTriangle( const Triangle & triangle ) {
        return CreateTriangle( triangle.a, triangle.b, triangle.c );
    }

    // Boolean operations - return new polygon
    Polygon Polygon::Union( const Polygon & other ) const {
        PathsD result = Clipper2Lib::Union( *paths, *other.paths, FillRule::NonZero );
        return Polygon( result );
    }

    Polygon Polygon::Difference( const Polygon & other ) const {
        PathsD result = Clipper2Lib::Difference( *paths, *other.paths, FillRule::NonZero );
        return Polygon( result );
    }

    Polygon Polygon::Intersect( const Polygon & other ) const {
        PathsD result = Clipper2Lib::Intersect( *paths, *other.paths, FillRule::NonZero );
        return Polygon( result );
    }

    // In-place boolean operations
    Polygon & Polygon::UnionWith( const Polygon & other ) {
        *paths = Clipper2Lib::Union( *paths, *other.paths, FillRule::NonZero );
        return *this;
    }

    Polygon & Polygon::DifferenceWith( const Polygon & other ) {
        *paths = Clipper2Lib::Difference( *paths, *other.paths, FillRule::NonZero );
        return *this;
    }

    Polygon & Polygon::IntersectWith( const Polygon & other ) {
        *paths = Clipper2Lib::Intersect( *paths, *other.paths, FillRule::NonZero );
        return *this;
    }

    // Transform operations
    Polygon & Polygon::Translate( const Vec2 & offset ) {
        return Translate( offset.x, offset.y );
    }

    Polygon & Polygon::Translate( f32 dx, f32 dy ) {
        *paths = TranslatePaths( *paths, static_cast<double>(dx), static_cast<double>(dy) );
        return *this;
    }

    Polygon Polygon::Translated( const Vec2 & offset ) const {
        return Translated( offset.x, offset.y );
    }

    Polygon Polygon::Translated( f32 dx, f32 dy ) const {
        PathsD result = TranslatePaths( *paths, static_cast<double>(dx), static_cast<double>(dy) );
        return Polygon( result );
    }

    // Rotation operations
    Polygon & Polygon::Rotate( f32 angle, const Vec2 & pivot ) {
        f32 cosA = std::cos( angle );
        f32 sinA = std::sin( angle );

        for ( auto & path : *paths ) {
            for ( auto & pt : path ) {
                f32 x = static_cast<f32>(pt.x) - pivot.x;
                f32 y = static_cast<f32>(pt.y) - pivot.y;

                f32 newX = x * cosA - y * sinA + pivot.x;
                f32 newY = x * sinA + y * cosA + pivot.y;

                pt.x = static_cast<double>(newX);
                pt.y = static_cast<double>(newY);
            }
        }
        return *this;
    }

    Polygon Polygon::Rotated( f32 angle, const Vec2 & pivot ) const {
        Polygon result( *this );
        result.Rotate( angle, pivot );
        return result;
    }

    Polygon & Polygon::RotateAroundCenter( f32 angle ) {
        Rect bounds = GetBounds();
        Vec2 center = bounds.GetCenter();
        return Rotate( angle, center );
    }

    Polygon Polygon::RotatedAroundCenter( f32 angle ) const {
        Polygon result( *this );
        result.RotateAroundCenter( angle );
        return result;
    }

    // Scale operations
    Polygon & Polygon::Scale( f32 factor, const Vec2 & pivot ) {
        for ( auto & path : *paths ) {
            for ( auto & pt : path ) {
                f32 x = static_cast<f32>(pt.x) - pivot.x;
                f32 y = static_cast<f32>(pt.y) - pivot.y;

                pt.x = static_cast<double>(x * factor + pivot.x);
                pt.y = static_cast<double>(y * factor + pivot.y);
            }
        }
        return *this;
    }

    Polygon Polygon::Scaled( f32 factor, const Vec2 & pivot ) const {
        Polygon result( *this );
        result.Scale( factor, pivot );
        return result;
    }

    Polygon & Polygon::ScaleAroundCenter( f32 factor ) {
        Rect bounds = GetBounds();
        Vec2 center = bounds.GetCenter();
        return Scale( factor, center );
    }

    Polygon & Polygon::CenterMirrorX() {
        // Mirrors the polygon along the X axis (across the horizontal axis passing through the center Y)
        Rect bounds = GetBounds();
        f32 centerY = bounds.GetCenter().y;
        for ( auto & path : *paths ) {
            for ( auto & pt : path ) {
                f32 y = static_cast<f32>(pt.y);
                pt.y = static_cast<double>(2.0f * centerY - y);
            }
            // Reverse the path to maintain correct winding order after mirroring
            std::reverse( path.begin(), path.end() );
        }
        return *this;
    }

    Polygon & Polygon::CenterMirrorY() {
        // Mirrors the polygon along the Y axis (across the vertical axis passing through the center X)
        Rect bounds = GetBounds();
        f32 centerX = bounds.GetCenter().x;
        for ( auto & path : *paths ) {
            for ( auto & pt : path ) {
                f32 x = static_cast<f32>(pt.x);
                pt.x = static_cast<double>(2.0f * centerX - x);
            }
            // Reverse the path to maintain correct winding order after mirroring
            std::reverse( path.begin(), path.end() );
        }

        return *this;
    }

    Polygon & Polygon::GlobalMirrorX() {
        // Mirrors the polygon along the X axis (across the global (Y=0) axis)
        for ( auto & path : *paths ) {
            for ( auto & pt : path ) {
                f32 y = static_cast<f32>(pt.y);
                pt.y = static_cast<double>(-y);
            }
            // Reverse the path to maintain correct winding order after mirroring
            std::reverse( path.begin(), path.end() );
        }
        return *this;
    }

    Polygon & Polygon::GlobalMirrorY() {
        // Mirrors the polygon along the Y axis (across the global (X=0) axis)
        for ( auto & path : *paths ) {
            for ( auto & pt : path ) {
                f32 x = static_cast<f32>(pt.x);
                pt.x = static_cast<double>(-x);
            }
            // Reverse the path to maintain correct winding order after mirroring
            std::reverse( path.begin(), path.end() );
        }
        return *this;
    }

    // Vertex manipulation
    void Polygon::SetVertex( i32 pathIndex, i32 vertexIndex, const Vec2 & newPos ) {
        if ( pathIndex < 0 || pathIndex >= static_cast<i32>( paths->size() ) ) {
            return;
        }
        PathD & path = (*paths)[pathIndex];
        if ( vertexIndex < 0 || vertexIndex >= static_cast<i32>( path.size() ) ) {
            return;
        }
        path[vertexIndex] = ToClipperPoint( newPos );
    }

    // Utility
    void Polygon::Clear() {
        paths->clear();
    }

    bool Polygon::IsEmpty() const {
        return paths->empty();
    }

    i32 Polygon::GetPathCount() const {
        return static_cast<i32>(paths->size());
    }

    i32 Polygon::GetVertexCount( i32 pathIndex ) const {
        if ( pathIndex < 0 || pathIndex >= static_cast<i32>( paths->size() ) ) {
            return 0;
        }
        return static_cast<i32>( (*paths)[pathIndex].size() );
    }

    void Polygon::AddPath( const std::vector<Vec2> & path ) {
        PathD pathD;
        for ( const auto & pt : path ) {
            pathD.push_back( ToClipperPoint( pt ) );
        }
        paths->push_back( std::move( pathD ) );
    }

    // Access vertices for rendering
    Vec2 Polygon::GetVertex( i32 pathIndex, i32 vertexIndex ) const {
        if ( pathIndex < 0 || pathIndex >= static_cast<i32>( paths->size() ) ) {
            return Vec2( 0, 0 );
        }
        const PathD & path = (*paths)[pathIndex];
        if ( vertexIndex < 0 || vertexIndex >= static_cast<i32>( path.size() ) ) {
            return Vec2( 0, 0 );
        }
        return FromClipperPoint( path[vertexIndex] );
    }

    std::vector<Vec2> Polygon::GetPath( i32 pathIndex ) const {
        std::vector<Vec2> result;
        if ( pathIndex < 0 || pathIndex >= static_cast<i32>( paths->size() ) ) {
            return result;
        }
        const PathD & path = (*paths)[pathIndex];
        result.reserve( path.size() );
        for ( const auto & pt : path ) {
            result.push_back( FromClipperPoint( pt ) );
        }
        return result;
    }

    std::vector<std::vector<Vec2>> Polygon::GetAllPaths() const {
        std::vector<std::vector<Vec2>> result;
        result.reserve( paths->size() );
        for ( const auto & path : *paths ) {
            std::vector<Vec2> verts;
            verts.reserve( path.size() );
            for ( const auto & pt : path ) {
                verts.push_back( FromClipperPoint( pt ) );
            }
            result.push_back( std::move( verts ) );
        }
        return result;
    }

    // Get bounding box
    Rect Polygon::GetBounds() const {
        if ( paths->empty() ) {
            return Rect();
        }

        RectD bounds = Clipper2Lib::GetBounds( *paths );
        return Rect(
            static_cast<f32>(bounds.left),
            static_cast<f32>(bounds.top),
            static_cast<f32>(bounds.right - bounds.left),
            static_cast<f32>(bounds.bottom - bounds.top)
        );
    }

    f64 Polygon::GetArea() const {
        f64 totalArea = 0.0;
        for ( const auto & path : *paths ) {
            totalArea += Clipper2Lib::Area( path );
        }
        return totalArea;
    }

    std::vector<Triangle> Polygon::Triangulate() const {
        std::vector<Triangle> result;

        if ( paths->empty() ) {
            return result;
        }

        PathsD triangles;
        TriangulateResult triResult = Clipper2Lib::Triangulate( *paths, 2, triangles, true );

        if ( triResult != TriangulateResult::success ) {
            ATTO_LOG_ERROR( "Polygon triangulation failed with error code: %d", static_cast<int>(triResult) );
            return result;
        }

        result.reserve( triangles.size() );
        for ( const auto & tri : triangles ) {
            if ( tri.size() >= 3 ) {
                result.push_back( Triangle(
                    FromClipperPoint( tri[0] ),
                    FromClipperPoint( tri[1] ),
                    FromClipperPoint( tri[2] )
                ) );
            }
        }

        return result;
    }

    void Polygon::Serialize( Serializer & serializer ) {
        if ( serializer.IsSaving() ) {
            std::vector<std::vector<Vec2>> serializablePaths;
            for ( const auto & path : *paths ) {
                std::vector<Vec2> serializablePath;
                for ( const auto & pt : path ) {
                    serializablePath.push_back( FromClipperPoint( pt ) );
                }
                serializablePaths.push_back( std::move( serializablePath ) );
            }

            serializer( "paths", serializablePaths );
        }
        else {
            std::vector<std::vector<Vec2>> serializablePaths;
            serializer( "paths", serializablePaths );

            if ( paths != nullptr ) {
                delete paths;
            }

            paths = new PathsD();
            for ( const auto & path : serializablePaths ) {
                PathD pathD;
                for ( const auto & pt : path ) {
                    pathD.push_back( ToClipperPoint( pt ) );
                }
                paths->push_back( std::move( pathD ) );
            }
        }
    }
}
