#include "o2Editor/stdafx.h"
#include "SceneView3DState.h"

#include "o2/Utils/Math/Matrix4.h"
#include "o2/Utils/Math/Quaternion.h"

namespace Editor
{
    Quat SceneView3DState::GetRotation() const
    {
        return Quat::FromEuler(Vec3F(pitch, 0.0f, yaw));
    }

    Camera SceneView3DState::BuildCamera() const
    {
        return BuildCamera(fov, nearClip, farClip);
    }

    Camera SceneView3DState::BuildCamera(float fovRad, float nearClipValue, float farClipValue) const
    {
        Camera camera = Camera::Perspective(fovRad, nearClipValue, farClipValue);
        Quat rotation = GetRotation();
        camera.rotation = rotation;
        camera.position = target + rotation*Vec3F(0.0f, 0.0f, distance);
        return camera;
    }

    Mat4 SceneView3DState::GetViewProjection(const Vec2F& viewportSize) const
    {
        Camera camera = BuildCamera();
        return camera.GetProjectionMatrix(viewportSize)*camera.GetViewMatrix3D();
    }

    bool SceneView3DState::GetViewRay(const Vec2F& viewportPoint, const Vec2F& viewportSize,
                                      Vec3F& origin, Vec3F& direction) const
    {
        if (viewportSize.x < FLT_EPSILON || viewportSize.y < FLT_EPSILON)
            return false;

        Vec2F ndc(viewportPoint.x/viewportSize.x*2.0f - 1.0f,
                  viewportPoint.y/viewportSize.y*2.0f - 1.0f);

        Mat4 inv = GetViewProjection(viewportSize).Inverted();
        Vec3F nearPoint = inv.TransformPoint(Vec3F(ndc.x, ndc.y, -1.0f));
        Vec3F farPoint = inv.TransformPoint(Vec3F(ndc.x, ndc.y, 1.0f));

        Vec3F dir = farPoint - nearPoint;
        if (dir.Length() < FLT_EPSILON)
            return false;

        origin = nearPoint;
        direction = dir.Normalized();
        return true;
    }

    bool SceneView3DState::GetScreenRay(const Vec2F& viewportPoint, const Vec2F& viewportSize,
                                        Vec3F& origin, Vec3F& direction) const
    {
        return GetViewRay(viewportPoint, viewportSize, origin, direction);
    }

    bool SceneView3DState::ScreenToPlanePoint(const Vec2F& viewportPoint, const Vec2F& viewportSize, Vec2F& result) const
    {
        Vec3F origin, direction;
        if (!GetViewRay(viewportPoint, viewportSize, origin, direction))
            return false;

        if (Math::Abs(direction.z) < 1e-6f)
            return false;

        float t = -origin.z/direction.z;
        if (t < 0.0f)
            return false;

        Vec3F hit = origin + direction*t;
        result = Vec2F(hit.x, hit.y);
        return true;
    }

    Vec2F SceneView3DState::PlanePointToScreen(const Vec2F& planePoint, const Vec2F& viewportSize) const
    {
        return WorldToScreen(Vec3F(planePoint.x, planePoint.y, 0.0f), viewportSize);
    }

    Vec2F SceneView3DState::WorldToScreen(const Vec3F& worldPoint, const Vec2F& viewportSize) const
    {
        Vec3F ndc = GetViewProjection(viewportSize).TransformPoint(worldPoint);
        return Vec2F((ndc.x + 1.0f)*0.5f*viewportSize.x,
                     (ndc.y + 1.0f)*0.5f*viewportSize.y);
    }

    bool SceneView3DState::ScreenToVerticalAxisZ(const Vec2F& viewportPoint, const Vec2F& viewportSize,
                                                 const Vec2F& planeAnchor, float& result) const
    {
        return ScreenToAxisParam(viewportPoint, viewportSize, Vec3F(planeAnchor.x, planeAnchor.y, 0.0f),
                                 Vec3F(0.0f, 0.0f, 1.0f), result);
    }

    bool SceneView3DState::ScreenToAxisParam(const Vec2F& viewportPoint, const Vec2F& viewportSize,
                                             const Vec3F& axisOrigin, const Vec3F& axisDir, float& param) const
    {
        Vec3F origin, direction;
        if (!GetViewRay(viewportPoint, viewportSize, origin, direction))
            return false;

        // Closest point parameter on line axisOrigin + s*axisDir to the view ray
        Vec3F w0 = axisOrigin - origin;

        float a = axisDir.Dot(axisDir);
        float b = axisDir.Dot(direction);
        float c = direction.Dot(direction);
        float d = axisDir.Dot(w0);
        float e = direction.Dot(w0);

        float denominator = a*c - b*b;
        if (Math::Abs(denominator) < 1e-6f)
            return false;

        param = (b*e - c*d)/denominator;
        return true;
    }

    bool SceneView3DState::ScreenToPlanePoint3D(const Vec2F& viewportPoint, const Vec2F& viewportSize,
                                                const Vec3F& planeOrigin, const Vec3F& planeNormal, Vec3F& result) const
    {
        Vec3F origin, direction;
        if (!GetViewRay(viewportPoint, viewportSize, origin, direction))
            return false;

        float denominator = direction.Dot(planeNormal);
        if (Math::Abs(denominator) < 1e-6f)
            return false;

        float t = (planeOrigin - origin).Dot(planeNormal)/denominator;
        if (t < 0.0f)
            return false;

        result = origin + direction*t;
        return true;
    }

    Vec3F SceneView3DState::GetCameraPosition() const
    {
        return target + GetRotation()*Vec3F(0.0f, 0.0f, distance);
    }

    void SceneView3DState::Orbit(const Vec2F& deltaAnglesRad)
    {
        yaw += deltaAnglesRad.x;
        pitch = Math::Clamp(pitch + deltaAnglesRad.y, -maxPitch, maxPitch);
    }

    void SceneView3DState::Look(const Vec2F& deltaAnglesRad)
    {
        Vec3F cameraPosition = GetCameraPosition();
        Orbit(deltaAnglesRad);
        target = cameraPosition - GetRotation()*Vec3F(0.0f, 0.0f, distance);
    }

    void SceneView3DState::Fly(const Vec3F& localDelta)
    {
        Quat rotation = GetRotation();
        target += rotation*Vec3F(1.0f, 0.0f, 0.0f)*localDelta.x +
                  rotation*Vec3F(0.0f, 1.0f, 0.0f)*localDelta.y +
                  rotation*Vec3F(0.0f, 0.0f, -1.0f)*localDelta.z;
    }

    void SceneView3DState::Pan(const Vec2F& screenDelta, const Vec2F& viewportSize)
    {
        Vec2F center = viewportSize*0.5f;
        Vec2F fromPoint, toPoint;
        if (ScreenToPlanePoint(center, viewportSize, fromPoint) &&
            ScreenToPlanePoint(center + screenDelta, viewportSize, toPoint))
        {
            target -= Vec3F(toPoint.x - fromPoint.x, toPoint.y - fromPoint.y, 0.0f);
            return;
        }

        // Near-horizon fallback: pan in the camera plane
        float unitsPerPixel = viewportSize.y > FLT_EPSILON
            ? 2.0f*distance*Math::Sin(fov*0.5f)/Math::Cos(fov*0.5f)/viewportSize.y
            : 1.0f;

        Vec3F shift = GetRotation()*Vec3F(screenDelta.x*unitsPerPixel, screenDelta.y*unitsPerPixel, 0.0f);
        target -= shift;
    }

    void SceneView3DState::Zoom(float factor)
    {
        distance = Math::Clamp(distance*factor, minDistance, maxDistance);
    }
}
