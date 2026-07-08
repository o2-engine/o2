#include "o2/stdafx.h"
#include "Camera.h"

#include "o2/Application/Application.h"
#include "o2/Render/Render.h"

namespace o2
{
    Camera::Camera(const Vec2F& position /*= Vec2F()*/, const Vec2F& size /*= o2Render.GetResolution()*/, 
                   float angle /*= 0.0f*/):
        Transform(size, position, angle)
    {
        if (size == Vec2F() && Render::IsSingletonInitialzed())
            SetSize(Vec3F((Vec2F)o2Render.GetCurrentResolution()));
    }

    bool Camera::operator==(const Camera& other) const
    {
        return Transform::operator==(other) &&
            projection == other.projection &&
            Math::Equals(fov, other.fov) &&
            Math::Equals(nearClip, other.nearClip) &&
            Math::Equals(farClip, other.farClip);
    }

    bool Camera::operator!=(const Camera& other) const
    {
        return !operator==(other);
    }

    Mat4 Camera::GetViewMatrix3D() const
    {
        return Mat4::TRS(GetPosition(), GetRotation(), Vec3F(1.0f, 1.0f, 1.0f)).Inverted();
    }

    Mat4 Camera::GetProjectionMatrix(const Vec2F& viewportSize) const
    {
        if (projection == Projection::Perspective)
        {
            float aspect = viewportSize.y > FLT_EPSILON ? viewportSize.x/viewportSize.y : 1.0f;
            return Mat4::Perspective(fov, aspect, nearClip, farClip);
        }

        if (projection == Projection::Orthographic3D)
        {
            Vec2F size = GetSize2D();
            return Mat4::Ortho(-size.x*0.5f, size.x*0.5f, -size.y*0.5f, size.y*0.5f, nearClip, farClip);
        }

        return Mat4::Ortho(0.0f, viewportSize.x, viewportSize.y, 0.0f, 0.0f, 10.0f);
    }

    Camera Camera::Default()
    {
        return Camera();
    }

    Camera Camera::FixedSize(const Vec2F& size)
    {        
        return Camera(Vec2F(), size);
    }

    Camera Camera::FittedSize(const Vec2F& size)
    {
        Vec2F resolution = o2Render.GetCurrentResolution();

        Vec2F scaledResolution = resolution*(size.x/resolution.x); 
        if (scaledResolution.y < size.y)
            scaledResolution = resolution*(size.y/resolution.y);

        return Camera(Vec2F(), scaledResolution);
    }

    Camera Camera::PhysicalCorrect(Units units)
    {
        Vec2F resolution = o2Render.GetCurrentResolution();
        Vec2F dpi = o2Render.GetDPI();
        float inchesInCentimeter = 2.5400013716f;

        Vec2F pixelsInUnit(1.0f, 1.0f);
        if (units == Units::Inches)
            pixelsInUnit = dpi;
        else if (units == Units::Centimeters)
            pixelsInUnit = dpi/inchesInCentimeter;
        else if (units == Units::Millimeters)
            pixelsInUnit = dpi/inchesInCentimeter/10.0f;

        return Camera(Vec2F(), resolution/pixelsInUnit);
    }

    Camera Camera::Perspective(float fov, float nearClip, float farClip)
    {
        Camera camera;
        camera.projection = Projection::Perspective;
        camera.fov = fov;
        camera.nearClip = nearClip;
        camera.farClip = farClip;
        return camera;
    }

    Camera Camera::Orthographic3D(float width, float height, float nearClip, float farClip)
    {
        Camera camera;
        camera.projection = Projection::Orthographic3D;
        camera.SetSize(Vec3F(width, height, 0.0f));
        camera.nearClip = nearClip;
        camera.farClip = farClip;
        return camera;
    }

    void Camera::OnDeserialized(const DataValue& node)
    {
        Transform::OnDeserialized(node);

        if (auto member = node.FindMember("position3D"))
        {
            Vec3F legacyPosition;
            member->Get(legacyPosition);
            SetPosition(legacyPosition);
        }

        if (auto member = node.FindMember("rotation3D"))
        {
            Quat legacyRotation;
            member->Get(legacyRotation);
            SetRotation(legacyRotation);
        }
    }

}
// --- META ---

ENUM_META(o2::Camera::Projection, o2__Camera__Projection)
{
    ENUM_ENTRY(Orthographic);
    ENUM_ENTRY(Orthographic3D);
    ENUM_ENTRY(Perspective);
}
END_ENUM_META;

DECLARE_CLASS(o2::Camera, o2__Camera);
// --- END META ---
