#pragma once

#include "o2/Render/Camera.h"
#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Math/Vector3.h"

using namespace o2;

namespace Editor
{
    // ------------------------------------------------------------------------------------------
    // Orbit camera state for the 3D scene view mode. Holds target on the z=0 plane, yaw around
    // the plane normal and pitch from the straight-down view. All math is plain and headless-
    // testable: viewport points are in pixels, origin at left-bottom, y up
    // ------------------------------------------------------------------------------------------
    class SceneView3DState
    {
    public:
        Vec3F target;                              // Orbit target point
        float yaw = 0.0f;                          // Rotation around world Z (plane normal), radians
        float pitch = Math::Deg2rad(30.0f);        // Tilt from straight-down view, radians; 90 - horizon, above it - looking up
        float distance = 500.0f;                   // Distance from target to camera

        float fov = Math::Deg2rad(60.0f);          // Vertical field of view, radians
        float nearClip = 0.1f;                     // Near clipping plane
        float farClip = 100000.0f;                 // Far clipping plane

        static constexpr float maxPitch = 179.0f/180.0f*3.1415926f; // Pitch clamp bound, stops just before the poles flip
        static constexpr float minDistance = 1.0f;                  // Distance clamp bounds
        static constexpr float maxDistance = 1000000.0f;

    public:
        // Returns camera orientation quaternion; identity when looking straight down at the plane
        Quat GetRotation() const;

        // Returns perspective camera on the orbit sphere looking at target
        Camera BuildCamera() const;

        // Returns perspective camera with explicit projection parameters
        Camera BuildCamera(float fovRad, float nearClipValue, float farClipValue) const;

        // Unprojects viewport point to a ray and intersects the z=0 plane; false if parallel or behind camera
        bool ScreenToPlanePoint(const Vec2F& viewportPoint, const Vec2F& viewportSize, Vec2F& result) const;

        // Projects plane point (x, y, 0) to viewport coordinates
        Vec2F PlanePointToScreen(const Vec2F& planePoint, const Vec2F& viewportSize) const;

        // Projects arbitrary world point to viewport coordinates
        Vec2F WorldToScreen(const Vec3F& worldPoint, const Vec2F& viewportSize) const;

        // Projects world point with a precomputed view-projection; use it when projecting many points,
        // building the matrix per point costs far more than the projection itself
        Vec2F WorldToScreen(const Vec3F& worldPoint, const Vec2F& viewportSize, const Mat4& viewProjection) const;

        // Returns projection*view matrix for viewport size
        Mat4 GetViewProjection(const Vec2F& viewportSize) const;

        // Returns z of the closest point on the world Z line through (planeAnchor, 0) to the view ray;
        // false when the ray is parallel to the axis (looking straight down)
        bool ScreenToVerticalAxisZ(const Vec2F& viewportPoint, const Vec2F& viewportSize,
                                   const Vec2F& planeAnchor, float& result) const;

        // Returns parameter of the closest point on line axisOrigin + param*axisDir to the view ray;
        // false when the ray is parallel to the axis or the axis is degenerate
        bool ScreenToAxisParam(const Vec2F& viewportPoint, const Vec2F& viewportSize,
                               const Vec3F& axisOrigin, const Vec3F& axisDir, float& param) const;

        // Unprojects viewport point to a ray and intersects an arbitrary plane; false if parallel or behind camera
        bool ScreenToPlanePoint3D(const Vec2F& viewportPoint, const Vec2F& viewportSize,
                                  const Vec3F& planeOrigin, const Vec3F& planeNormal, Vec3F& result) const;

        // Builds view ray for viewport point: origin on near plane, normalized direction; false on degenerate viewport
        bool GetScreenRay(const Vec2F& viewportPoint, const Vec2F& viewportSize, Vec3F& origin, Vec3F& direction) const;

        // Returns camera position on the orbit sphere
        Vec3F GetCameraPosition() const;

        // Returns near clip plane: point on the plane and normal along the view direction. Geometry
        // behind it can't be projected - perspective divide mirrors it in front of the camera
        void GetNearClipPlane(Vec3F& origin, Vec3F& normal) const;

        // Rotates view: x adds to yaw, y adds to pitch (clamped)
        void Orbit(const Vec2F& deltaAnglesRad);

        // Rotates view around the camera position (FPS-style look): camera stays, target moves
        void Look(const Vec2F& deltaAnglesRad);

        // Moves camera and target by delta in camera-local axes: x - right, y - up, z - forward
        void Fly(const Vec3F& localDelta);

        // Moves target so the plane grabbed under the cursor follows it
        void Pan(const Vec2F& screenDelta, const Vec2F& viewportSize);

        // Multiplies distance by factor, clamped
        void Zoom(float factor);

    private:
        // Builds view ray for viewport point: origin on near plane, normalized direction
        bool GetViewRay(const Vec2F& viewportPoint, const Vec2F& viewportSize, Vec3F& origin, Vec3F& direction) const;
    };
}
