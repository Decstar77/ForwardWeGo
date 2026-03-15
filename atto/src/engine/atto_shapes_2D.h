#pragma once

#include "atto_math.h"

// Forward declare Clipper2 types to avoid header pollution
namespace Clipper2Lib {
    template <typename T> struct Point;
    template <typename T> using Path = std::vector<Point<T>>;
    template <typename T> using Paths = std::vector<Path<T>>;
    using PathD = Path<double>;
    using PathsD = Paths<double>;
}

namespace atto {
    class Serializer;

    struct Circle {
        Circle();
        Circle( f32 x, f32 y, f32 r );

        bool    Intersects( const Circle & other ) const;

        void    Serialize( Serializer & serializer );

        Vec2    position;
        f32     radius;
    };

    struct Rect {
        Rect();
        Rect( f32 x, f32 y, f32 w, f32 h );

        static Rect FromCenter( const Vec2 & center, f32 width, f32 height );

        f32     Left() const;
        f32     Right() const;
        f32     Top() const;
        f32     Bottom() const;

        Vec2    Size() const;
        Vec2    GetCenter() const;
        bool    Contains( f32 px, f32 py ) const;
        bool    Contains( const Vec2 & point ) const;

        void    Translate( const Vec2 & delta );

        bool    Intersects( const Rect & other ) const;
        bool    Intersects( const Vec2 & circlePos, f32 circleRadius ) const;
        bool    Intersects( const Vec2 & circlePos, f32 circleRadius, f32 & penetration, Vec2 & normal ) const;

        void    InvertY();
        void    InvertX();
        Vec2    ClosestPoint( const Vec2 & p ) const;

        void    Serialize( Serializer & serializer );

        f32     x;
        f32     y;
        f32     width;
        f32     height;
    };

    struct Triangle {
        Triangle();
        Triangle( const Vec2 & a, const Vec2 & b, const Vec2 & c );
        Triangle( f32 ax, f32 ay, f32 bx, f32 by, f32 cx, f32 cy );

        void    Serialize( Serializer & serializer );

        Vec2 a;
        Vec2 b;
        Vec2 c;
    };

    class Polygon {
    public:
        Polygon();
        ~Polygon();

        // Copy and move semantics
        Polygon( const Polygon & other );
        Polygon( Polygon && other ) noexcept;
        Polygon & operator=( const Polygon & other );
        Polygon & operator=( Polygon && other ) noexcept;

        // Factory methods for creating primitives
        static Polygon CreateRect( f32 x, f32 y, f32 width, f32 height );
        static Polygon CreateRect( const Rect & rect );
        static Polygon CreateCircle( const Vec2 & center, f32 radius, i32 segments = 32 );
        static Polygon CreateCircle( const Circle & circle, i32 segments = 32 );
        static Polygon CreateTriangle( const Vec2 & a, const Vec2 & b, const Vec2 & c );
        static Polygon CreateTriangle( const Triangle & triangle );

        // Boolean operations - return new polygon
        Polygon Union( const Polygon & other ) const;
        Polygon Difference( const Polygon & other ) const;
        Polygon Intersect( const Polygon & other ) const;

        // In-place boolean operations
        Polygon & UnionWith( const Polygon & other );
        Polygon & DifferenceWith( const Polygon & other );
        Polygon & IntersectWith( const Polygon & other );

        // Transform operations
        Polygon & Translate( const Vec2 & offset );
        Polygon & Translate( f32 dx, f32 dy );
        Polygon Translated( const Vec2 & offset ) const;
        Polygon Translated( f32 dx, f32 dy ) const;

        // Rotation (angle in radians, around pivot point)
        Polygon & Rotate( f32 angle, const Vec2 & pivot );
        Polygon Rotated( f32 angle, const Vec2 & pivot ) const;
        Polygon & RotateAroundCenter( f32 angle );
        Polygon RotatedAroundCenter( f32 angle ) const;

        // Scale
        Polygon & Scale( f32 factor, const Vec2 & pivot );
        Polygon Scaled( f32 factor, const Vec2 & pivot ) const;
        Polygon & ScaleAroundCenter( f32 factor );

        // Mirror
        Polygon & CenterMirrorX();
        Polygon & CenterMirrorY();
        Polygon & GlobalMirrorX();
        Polygon & GlobalMirrorY();

        // Vertex manipulation
        void SetVertex( i32 pathIndex, i32 vertexIndex, const Vec2 & newPos );

        // Utility
        void Clear();
        bool IsEmpty() const;
        i32 GetPathCount() const;
        i32 GetVertexCount( i32 pathIndex = 0 ) const;

        // Add a path
        void AddPath( const std::vector<Vec2> & path );

        // Access vertices for rendering
        // Returns vertices for a specific path (outer polygon or hole)
        Vec2 GetVertex( i32 pathIndex, i32 vertexIndex ) const;
        std::vector<Vec2> GetPath( i32 pathIndex = 0 ) const;
        std::vector<std::vector<Vec2>> GetAllPaths() const;

        // Get bounding box
        Rect GetBounds() const;

        // Area calculation (positive for CCW, negative for CW/holes)
        f64 GetArea() const;

        // Triangulate the polygon
        std::vector<Triangle> Triangulate() const;

        // Serialization
        void Serialize( Serializer & serializer );

    private:
        // Internal Clipper2 paths storage
        Clipper2Lib::PathsD * paths;

        // Private constructor from Clipper2 paths
        explicit Polygon( const Clipper2Lib::PathsD & clipperPaths );
    };

}
