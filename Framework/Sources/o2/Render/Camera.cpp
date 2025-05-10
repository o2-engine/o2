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
            SetSize(o2Render.GetCurrentResolution());
    }

    bool Camera::operator==(const Camera& other) const
    {
        return Transform::operator==(other);
    }

    bool Camera::operator!=(const Camera& other) const
    {
        return !Transform::operator==(other);
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

}
// --- META ---

DECLARE_CLASS(o2::Camera, o2__Camera);
// --- END META ---
