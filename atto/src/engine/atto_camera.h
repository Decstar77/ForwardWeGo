#pragma once

#include "atto_core.h"
#include "atto_math.h"
#include "atto_shapes_2D.h"
#include "atto_shapes_3D.h"

namespace atto {

    class Camera2D {
    public:
        Camera2D();

        // Setup
        void SetViewportSize( i32 width, i32 height );
        void SetPosition( const Vec2 & pos );
        void SetZoom( f32 z );
        void SetRotation( f32 radians );

        // Getters
        Vec2 GetPosition() const { return position; }
        f32 GetZoom() const { return zoom; }
        f32 GetRotation() const { return rotation; }
        Vec2 GetViewportSize() const { return Vec2( static_cast<f32>(viewportWidth), static_cast<f32>(viewportHeight) ); }

        // Movement
        void Move( const Vec2 & delta );
        void MoveWithoutTargeting( const Vec2 & delta );
        void ZoomBy( f32 factor );
        void ZoomAt( const Vec2 & screenPos, f32 factor );
        void Rotate( f32 radians );

        // Zoom limits
        void SetZoomLimits( f32 min, f32 max );

        // Coordinate conversion
        Vec2 ScreenToWorld( const Vec2 & screenPos ) const;
        Vec2 WorldToScreen( const Vec2 & worldPos ) const;

        // Frustum/bounds
        Rect GetWorldBounds() const;
        bool IsPointVisible( const Vec2 & worldPos ) const;
        bool IsRectVisible( const Rect & worldRect ) const;

        // Matrices
        Mat4 GetViewMatrix() const;
        Mat4 GetProjectionMatrix() const;
        Mat4 GetViewProjectionMatrix() const;

        // Smooth camera movement (call each frame)
        void SetTargetPosition( const Vec2 & target );
        void SetTargetZoom( f32 target );
        void SetSmoothSpeed( f32 speed );
        void UpdateSmooth( f32 deltaTime );

    private:
        void UpdateMatrices();

        // Viewport
        i32 viewportWidth = 1280;
        i32 viewportHeight = 720;

        // Transform
        Vec2 position = Vec2( 0.0f );  // Center of the camera in world space
        f32 zoom = 1.0f;              // 1.0 = 100%, 2.0 = 200% (closer), 0.5 = 50% (further)
        f32 rotation = 0.0f;          // Rotation in radians

        // Zoom limits
        f32 minZoom = 0.1f;
        f32 maxZoom = 10.0f;

        // Smooth movement
        Vec2 targetPosition = Vec2( 0.0f );
        f32 targetZoom = 1.0f;
        f32 smoothSpeed = 5.0f;
        bool useSmoothing = false;

        // Cached matrices
        Mat4 viewMatrix = Mat4( 1.0f );
        Mat4 projectionMatrix = Mat4( 1.0f );
        Mat4 viewProjectionMatrix = Mat4( 1.0f );
        Mat4 inverseViewProjectionMatrix = Mat4( 1.0f );
        bool matricesDirty = true;
    };

    class FlyCamera {
    public:
        FlyCamera();

        void SetViewportSize( i32 width, i32 height );
        void SetPosition( const Vec3 & pos );
        void SetYaw( f32 yawRadians ) { yaw = yawRadians; }
        void SetPitch( f32 pitchRadians );
        void SetFOV( f32 fovDegrees );
        void SetClipPlanes( f32 nearPlane, f32 farPlane );
        void SetMoveSpeed( f32 speed );
        void SetLookSensitivity( f32 sensitivity );

        Vec3 GetPosition() const { return position; }
        f32  GetYaw() const { return yaw; }
        f32  GetPitch() const { return pitch; }
        f32  GetFOV() const { return fovDeg; }
        f32  GetMoveSpeed() const { return moveSpeed; }
        f32  GetLookSensitivity() const { return lookSensitivity; }

        Vec3 GetForward() const;
        Vec3 GetRight() const;
        Vec3 GetUp() const;

        void Rotate( f32 yawDelta, f32 pitchDelta );
        void MoveForward( f32 amount );
        void MoveRight( f32 amount );
        void MoveUp( f32 amount );

        Mat4 GetViewMatrix() const;
        Mat4 GetProjectionMatrix() const;
        Mat4 GetViewProjectionMatrix() const;

        i32 GetViewportWidth() const { return viewportWidth; }
        i32 GetViewportHeight() const { return viewportHeight; }

    private:
        Vec3 position = Vec3( 0.0f, 0.0f, 3.0f );
        f32  yaw = -HALF_PI;
        f32  pitch = 0.0f;

        f32  fovDeg = 60.0f;
        f32  nearClip = 0.1f;
        f32  farClip = 1000.0f;
        f32  moveSpeed = 5.0f;
        
        f32  lookSensitivity = 0.1f;
        i32  viewportWidth = 1280;
        i32  viewportHeight = 720;
    };

    class FPSCamera {
    public:
        FPSCamera();

        void SetViewportSize( i32 width, i32 height );
        void SetPosition( const Vec3 & pos );
        void SetFOV( f32 fovDegrees );
        void SetClipPlanes( f32 nearPlane, f32 farPlane );
        void SetMoveSpeed( f32 speed );
        void SetLookSensitivity( f32 sensitivity );

        Vec3 GetPosition() const { return position; }
        f32  GetYaw() const { return yaw; }
        f32  GetPitch() const { return pitch; }
        f32  GetFOV() const { return fovDeg; }
        f32  GetMoveSpeed() const { return moveSpeed; }
        f32  GetLookSensitivity() const { return lookSensitivity; }

        Vec3 GetForward() const;
        Vec3 GetRight() const;
        Vec3 GetUp() const;

        // Movement is projected onto the XZ (ground) plane
        Vec3 GetHorizontalForward() const;
        Vec3 GetHorizontalRight() const;

        void Rotate( f32 yawDelta, f32 pitchDelta );
        void MoveForward( f32 amount );
        void MoveRight( f32 amount );
        void MoveUp( f32 amount );

        Mat4 GetViewMatrix() const;
        Mat4 GetProjectionMatrix() const;
        Mat4 GetViewProjectionMatrix() const;

        i32 GetViewportWidth() const { return viewportWidth; }
        i32 GetViewportHeight() const { return viewportHeight; }

    private:
        Vec3 position = Vec3( 0.0f, 0.0f, 3.0f );
        f32  yaw = -HALF_PI;
        f32  pitch = 0.0f;

        f32  fovDeg = 60.0f;
        f32  nearClip = 0.1f;
        f32  farClip = 1000.0f;
        f32  moveSpeed = 5.0f;
        f32  lookSensitivity = 0.1f;

        i32  viewportWidth = 1280;
        i32  viewportHeight = 720;
    };

} // namespace atto
