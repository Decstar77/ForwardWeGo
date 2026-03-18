/*
    Atto Engine - Camera System Implementation
*/

#include "atto_camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace atto {

    Camera2D::Camera2D() {
        targetPosition = position;
        targetZoom = zoom;
        UpdateMatrices();
    }

    void Camera2D::SetViewportSize( i32 width, i32 height ) {
        if ( viewportWidth != width || viewportHeight != height ) {
            viewportWidth = width;
            viewportHeight = height;
            matricesDirty = true;
        }
    }

    void Camera2D::SetPosition( const Vec2 & pos ) {
        position = pos;
        targetPosition = pos;
        matricesDirty = true;
    }

    void Camera2D::SetZoom( f32 z ) {
        zoom = Clamp( z, minZoom, maxZoom );
        targetZoom = zoom;
        matricesDirty = true;
    }

    void Camera2D::SetRotation( f32 radians ) {
        rotation = radians;
        matricesDirty = true;
    }

    void Camera2D::Move( const Vec2 & delta ) {
        position += delta;
        targetPosition = position;
        matricesDirty = true;
    }

    void Camera2D::MoveWithoutTargeting( const Vec2 & delta ) {
        position += delta;
        matricesDirty = true;
    }

    void Camera2D::ZoomBy( f32 factor ) {
        SetZoom( zoom * factor );
    }

    void Camera2D::ZoomAt( const Vec2 & screenPos, f32 factor ) {
        // Get world position before zoom
        Vec2 worldBefore = ScreenToWorld( screenPos );

        // Apply zoom
        f32 newZoom = Clamp( zoom * factor, minZoom, maxZoom );
        zoom = newZoom;
        targetZoom = newZoom;
        matricesDirty = true;
        UpdateMatrices();

        // Get world position after zoom
        Vec2 worldAfter = ScreenToWorld( screenPos );

        // Adjust position to keep the point under the cursor
        Vec2 diff = worldBefore - worldAfter;
        position += diff;
        targetPosition = position;
        matricesDirty = true;
    }

    void Camera2D::Rotate( f32 radians ) {
        rotation += radians;
        matricesDirty = true;
    }

    void Camera2D::SetZoomLimits( f32 min, f32 max ) {
        minZoom = min;
        maxZoom = max;
        zoom = Clamp( zoom, minZoom, maxZoom );
        targetZoom = Clamp( targetZoom, minZoom, maxZoom );
    }

    Vec2 Camera2D::ScreenToWorld( const Vec2 & screenPos ) const {
        // Ensure matrices are up to date
        const_cast<Camera2D *>(this)->UpdateMatrices();

        // Convert screen coords to normalized device coords (-1 to 1)
        f32 ndcX = (2.0f * screenPos.x / viewportWidth) - 1.0f;
        f32 ndcY = 1.0f - (2.0f * screenPos.y / viewportHeight);  // Flip Y

        // Transform by inverse view-projection
        Vec4 worldPos = inverseViewProjectionMatrix * Vec4( ndcX, ndcY, 0.0f, 1.0f );

        return Vec2( worldPos.x, worldPos.y );
    }

    Vec2 Camera2D::WorldToScreen( const Vec2 & worldPos ) const {
        // Ensure matrices are up to date
        const_cast<Camera2D *>(this)->UpdateMatrices();

        // Transform to clip space
        Vec4 clipPos = viewProjectionMatrix * Vec4( worldPos, 0.0f, 1.0f );

        // Convert from NDC to screen coords
        f32 screenX = (clipPos.x + 1.0f) * 0.5f * viewportWidth;
        f32 screenY = (1.0f - clipPos.y) * 0.5f * viewportHeight;  // Flip Y

        return Vec2( screenX, screenY );
    }

    Rect Camera2D::GetWorldBounds() const {
        // Get the four corners of the screen in world space
        Vec2 topLeft = ScreenToWorld( Vec2( 0.0f, 0.0f ) );
        Vec2 topRight = ScreenToWorld( Vec2( static_cast<f32>(viewportWidth), 0.0f ) );
        Vec2 bottomLeft = ScreenToWorld( Vec2( 0.0f, static_cast<f32>(viewportHeight) ) );
        Vec2 bottomRight = ScreenToWorld( Vec2( static_cast<f32>(viewportWidth), static_cast<f32>(viewportHeight) ) );

        // Find bounding box (handles rotation)
        f32 minX = Min( Min( topLeft.x, topRight.x ), Min( bottomLeft.x, bottomRight.x ) );
        f32 maxX = Max( Max( topLeft.x, topRight.x ), Max( bottomLeft.x, bottomRight.x ) );
        f32 minY = Min( Min( topLeft.y, topRight.y ), Min( bottomLeft.y, bottomRight.y ) );
        f32 maxY = Max( Max( topLeft.y, topRight.y ), Max( bottomLeft.y, bottomRight.y ) );

        return Rect( minX, minY, maxX - minX, maxY - minY );
    }

    bool Camera2D::IsPointVisible( const Vec2 & worldPos ) const {
        return GetWorldBounds().Contains( worldPos.x, worldPos.y );
    }

    bool Camera2D::IsRectVisible( const Rect & worldRect ) const {
        return GetWorldBounds().Intersects( worldRect );
    }

    Mat4 Camera2D::GetViewMatrix() const {
        const_cast<Camera2D *>(this)->UpdateMatrices();
        return viewMatrix;
    }

    Mat4 Camera2D::GetProjectionMatrix() const {
        const_cast<Camera2D *>(this)->UpdateMatrices();
        return projectionMatrix;
    }

    Mat4 Camera2D::GetViewProjectionMatrix() const {
        const_cast<Camera2D *>(this)->UpdateMatrices();
        return viewProjectionMatrix;
    }

    void Camera2D::SetTargetPosition( const Vec2 & target ) {
        targetPosition = target;
        useSmoothing = true;
    }

    void Camera2D::SetTargetZoom( f32 target ) {
        targetZoom = Clamp( target, minZoom, maxZoom );
        useSmoothing = true;
    }

    void Camera2D::SetSmoothSpeed( f32 speed ) {
        smoothSpeed = speed;
    }

    void Camera2D::UpdateSmooth( f32 deltaTime ) {
        if ( !useSmoothing ) return;

        f32 t = 1.0f - expf( -smoothSpeed * deltaTime );

        Vec2 oldPos = position;
        f32 oldZoom = zoom;

        position = Lerp( position, targetPosition, t );
        zoom = Lerp( zoom, targetZoom, t );

        if ( oldPos != position || oldZoom != zoom ) {
            matricesDirty = true;
        }

        // Stop smoothing when close enough
        if ( Distance( position, targetPosition ) < 0.01f && Abs( zoom - targetZoom ) < 0.001f ) {
            position = targetPosition;
            zoom = targetZoom;
            useSmoothing = false;
        }
    }

    void Camera2D::UpdateMatrices() {
        if ( !matricesDirty ) return;

        // Projection matrix: orthographic projection
        // Maps world units to NDC (-1 to 1)
        f32 halfWidth = (viewportWidth / 2.0f) / zoom;
        f32 halfHeight = (viewportHeight / 2.0f) / zoom;

        projectionMatrix = glm::ortho( -halfWidth, halfWidth, -halfHeight, halfHeight, -1.0f, 1.0f );

        // View matrix: camera transform (inverse of camera's world transform)
        viewMatrix = Mat4( 1.0f );
        viewMatrix = glm::rotate( viewMatrix, -rotation, Vec3( 0.0f, 0.0f, 1.0f ) );
        viewMatrix = glm::translate( viewMatrix, Vec3( -position, 0.0f ) );

        // Combined matrix
        viewProjectionMatrix = projectionMatrix * viewMatrix;
        inverseViewProjectionMatrix = glm::inverse( viewProjectionMatrix );

        matricesDirty = false;
    }

    // =============================================
    // FlyCamera
    // =============================================

    FlyCamera::FlyCamera() {
    }

    void FlyCamera::SetViewportSize( i32 width, i32 height ) {
        viewportWidth = width;
        viewportHeight = height;
    }

    void FlyCamera::SetPosition( const Vec3 & pos ) {
        position = pos;
    }

    void FlyCamera::SetFOV( f32 fovDegrees ) {
        fovDeg = fovDegrees;
    }

    void FlyCamera::SetClipPlanes( f32 nearPlane, f32 farPlane ) {
        nearClip = nearPlane;
        farClip = farPlane;
    }

    void FlyCamera::SetMoveSpeed( f32 speed ) {
        moveSpeed = speed;
    }

    void FlyCamera::SetLookSensitivity( f32 sensitivity ) {
        lookSensitivity = sensitivity;
    }

    void FlyCamera::SetPitch( f32 pitchRadians ) {
        constexpr f32 maxPitch = HALF_PI - 0.01f;
        pitch = Clamp( pitchRadians, -maxPitch, maxPitch );
    }

    Vec3 FlyCamera::GetForward() const {
        Vec3 fwd;
        fwd.x = Cos( yaw ) * Cos( pitch );
        fwd.y = Sin( pitch );
        fwd.z = Sin( yaw ) * Cos( pitch );
        return Normalize( fwd );
    }

    Vec3 FlyCamera::GetRight() const {
        return Normalize( Cross( GetForward(), Vec3( 0.0f, 1.0f, 0.0f ) ) );
    }

    Vec3 FlyCamera::GetUp() const {
        return Normalize( Cross( GetRight(), GetForward() ) );
    }

    void FlyCamera::Rotate( f32 yawDelta, f32 pitchDelta ) {
        yaw += yawDelta;
        pitch += pitchDelta;

        // Clamp pitch to avoid gimbal lock at the poles
        constexpr f32 maxPitch = HALF_PI - 0.01f;
        pitch = Clamp( pitch, -maxPitch, maxPitch );
    }

    void FlyCamera::MoveForward( f32 amount ) {
        position += GetForward() * amount;
    }

    void FlyCamera::MoveRight( f32 amount ) {
        position += GetRight() * amount;
    }

    void FlyCamera::MoveUp( f32 amount ) {
        position.y += amount;
    }

    Mat4 FlyCamera::GetViewMatrix() const {
        return glm::lookAt( position, position + GetForward(), Vec3( 0.0f, 1.0f, 0.0f ) );
    }

    Mat4 FlyCamera::GetProjectionMatrix() const {
        f32 aspect = (viewportHeight > 0) ? static_cast<f32>(viewportWidth) / static_cast<f32>(viewportHeight) : 1.0f;
        return glm::perspective( ToRadians( fovDeg ), aspect, nearClip, farClip );
    }

    Mat4 FlyCamera::GetViewProjectionMatrix() const {
        return GetProjectionMatrix() * GetViewMatrix();
    }

    // =============================================
    // FPSCamera
    // =============================================

    FPSCamera::FPSCamera() {
    }

    void FPSCamera::SetViewportSize( i32 width, i32 height ) {
        viewportWidth = width;
        viewportHeight = height;
    }

    void FPSCamera::SetPosition( const Vec3 & pos ) {
        position = pos;
    }

    void FPSCamera::SetFOV( f32 fovDegrees ) {
        fovDeg = fovDegrees;
    }

    void FPSCamera::SetClipPlanes( f32 nearPlane, f32 farPlane ) {
        nearClip = nearPlane;
        farClip = farPlane;
    }

    void FPSCamera::SetMoveSpeed( f32 speed ) {
        moveSpeed = speed;
    }

    void FPSCamera::SetLookSensitivity( f32 sensitivity ) {
        lookSensitivity = sensitivity;
    }

    Vec3 FPSCamera::GetForward() const {
        Vec3 fwd;
        fwd.x = Cos( yaw ) * Cos( pitch );
        fwd.y = Sin( pitch );
        fwd.z = Sin( yaw ) * Cos( pitch );
        return Normalize( fwd );
    }

    Vec3 FPSCamera::GetRight() const {
        return Normalize( Cross( GetForward(), Vec3( 0.0f, 1.0f, 0.0f ) ) );
    }

    Vec3 FPSCamera::GetUp() const {
        return Normalize( Cross( GetRight(), GetForward() ) );
    }

    Vec3 FPSCamera::GetHorizontalForward() const {
        return Normalize( Vec3( Cos( yaw ), 0.0f, Sin( yaw ) ) );
    }

    Vec3 FPSCamera::GetHorizontalRight() const {
        return Normalize( Cross( GetHorizontalForward(), Vec3( 0.0f, 1.0f, 0.0f ) ) );
    }

    void FPSCamera::Rotate( f32 yawDelta, f32 pitchDelta ) {
        yaw += yawDelta;
        pitch += pitchDelta;

        constexpr f32 maxPitch = HALF_PI - 0.01f;
        pitch = Clamp( pitch, -maxPitch, maxPitch );
    }

    void FPSCamera::MoveForward( f32 amount ) {
        position += GetHorizontalForward() * amount;
    }

    void FPSCamera::MoveRight( f32 amount ) {
        position += GetHorizontalRight() * amount;
    }

    void FPSCamera::MoveUp( f32 amount ) {
        position.y += amount;
    }

    Mat4 FPSCamera::GetViewMatrix() const {
        return glm::lookAt( position, position + GetForward(), Vec3( 0.0f, 1.0f, 0.0f ) );
    }

    Mat4 FPSCamera::GetProjectionMatrix() const {
        f32 aspect = (viewportHeight > 0) ? static_cast<f32>(viewportWidth) / static_cast<f32>(viewportHeight) : 1.0f;
        return glm::perspective( ToRadians( fovDeg ), aspect, nearClip, farClip );
    }

    Mat4 FPSCamera::GetViewProjectionMatrix() const {
        return GetProjectionMatrix() * GetViewMatrix();
    }

} // namespace atto
