#include "o2/stdafx.h"
#include "VectorImage.h"

#include "o2/Utils/Math/Math.h"

namespace o2
{
    VectorGradientStop::VectorGradientStop(float offset, const Color4& color):
        offset(offset), color(color)
    {}

    VectorPaint VectorPaint::Solid(const Color4& color)
    {
        VectorPaint res;
        res.type = VectorPaintType::Solid;
        res.color = color;
        return res;
    }

    VectorPaint VectorPaint::Linear(const Vec2F& begin, const Vec2F& end, const Vector<VectorGradientStop>& stops)
    {
        VectorPaint res;
        res.type = VectorPaintType::LinearGradient;
        res.begin = begin;
        res.end = end;
        res.stops = stops;
        return res;
    }

    VectorPaint VectorPaint::Radial(const Vec2F& center, float radius, const Vector<VectorGradientStop>& stops)
    {
        VectorPaint res;
        res.type = VectorPaintType::RadialGradient;
        res.center = center;
        res.focal = center;
        res.radius = radius;
        res.stops = stops;
        return res;
    }

    bool VectorPaint::IsNone() const
    {
        if (type == VectorPaintType::None)
            return true;

        if (type == VectorPaintType::Solid)
            return color.a <= 0;

        return stops.empty();
    }

    Color4 VectorPaint::GetRampColor(float offset) const
    {
        if (stops.empty())
            return color;

        if (offset <= stops.front().offset)
            return stops.front().color;

        if (offset >= stops.back().offset)
            return stops.back().color;

        for (size_t i = 1; i < stops.size(); i++)
        {
            const VectorGradientStop& prev = stops[i - 1];
            const VectorGradientStop& next = stops[i];
            if (offset > next.offset)
                continue;

            float range = next.offset - prev.offset;
            if (range <= 0.0f)
                return next.color;

            float coef = (offset - prev.offset)/range;
            return Color4(Math::RoundToInt(Math::Lerp((float)prev.color.r, (float)next.color.r, coef)),
                          Math::RoundToInt(Math::Lerp((float)prev.color.g, (float)next.color.g, coef)),
                          Math::RoundToInt(Math::Lerp((float)prev.color.b, (float)next.color.b, coef)),
                          Math::RoundToInt(Math::Lerp((float)prev.color.a, (float)next.color.a, coef)));
        }

        return stops.back().color;
    }

    float VectorPaint::GetRampOffset(const Vec2F& point) const
    {
        if (type == VectorPaintType::LinearGradient)
        {
            Vec2F local = transform.Inverted().Transform(point);
            Vec2F axis = end - begin;
            float sqrLength = axis.SqrLength();
            if (sqrLength <= 0.0f)
                return 1.0f;

            return (local - begin).Dot(axis)/sqrLength;
        }

        if (type == VectorPaintType::RadialGradient)
        {
            if (radius <= 0.0f)
                return 1.0f;

            Vec2F local = transform.Inverted().Transform(point);
            Vec2F focalOffset = center - focal;
            float maxFocalOffset = radius*0.99f;
            if (focalOffset.SqrLength() > maxFocalOffset*maxFocalOffset)
                focalOffset = focalOffset.Normalized()*maxFocalOffset;

            Vec2F delta = local - (center - focalOffset);
            float a = focalOffset.SqrLength() - radius*radius;
            float b = delta.Dot(focalOffset);
            float discriminant = b*b - a*delta.SqrLength();
            return (b - Math::Sqrt(Math::Max(discriminant, 0.0f)))/a;
        }

        return 0.0f;
    }

    VectorSegment::VectorSegment(const Vec2F& end):
        end(end)
    {}

    VectorSegment::VectorSegment(const Vec2F& control1, const Vec2F& control2, const Vec2F& end):
        cubic(true), control1(control1), control2(control2), end(end)
    {}

    void VectorSubPath::LineTo(const Vec2F& point)
    {
        segments.Add(VectorSegment(point));
    }

    void VectorSubPath::CubicTo(const Vec2F& control1, const Vec2F& control2, const Vec2F& point)
    {
        segments.Add(VectorSegment(control1, control2, point));
    }

    void VectorSubPath::Flatten(float tolerance, Vector<Vec2F>& points) const
    {
        const int maxSteps = 256;

        points.Add(start);

        Vec2F current = start;
        for (const VectorSegment& segment : segments)
        {
            if (segment.cubic)
            {
                Vec2F bend1 = current - segment.control1*2.0f + segment.control2;
                Vec2F bend2 = segment.control1 - segment.control2*2.0f + segment.end;
                float bend = Math::Max(bend1.Length(), bend2.Length());
                float count = Math::Ceil(Math::Sqrt(0.75f*bend/Math::Max(tolerance, 1e-4f)));
                int steps = Math::Clamp((int)count, 1, maxSteps);

                for (int i = 1; i < steps; i++)
                {
                    float t = (float)i/(float)steps;
                    float it = 1.0f - t;
                    points.Add(current*(it*it*it) + segment.control1*(3.0f*it*it*t) +
                               segment.control2*(3.0f*it*t*t) + segment.end*(t*t*t));
                }
            }

            points.Add(segment.end);
            current = segment.end;
        }
    }

    void VectorShape::AddRect(const Vec2F& position, const Vec2F& size, const Vec2F& cornerRadius /*= Vec2F()*/)
    {
        const float kappa = 0.5522847498f;

        float left = position.x, top = position.y;
        float right = position.x + size.x, bottom = position.y + size.y;
        float rx = Math::Clamp(cornerRadius.x, 0.0f, size.x*0.5f);
        float ry = Math::Clamp(cornerRadius.y, 0.0f, size.y*0.5f);

        VectorSubPath path;
        path.closed = true;

        if (rx <= 0.0f || ry <= 0.0f)
        {
            path.start = Vec2F(left, top);
            path.LineTo(Vec2F(right, top));
            path.LineTo(Vec2F(right, bottom));
            path.LineTo(Vec2F(left, bottom));
        }
        else
        {
            float kx = rx*(1.0f - kappa), ky = ry*(1.0f - kappa);

            path.start = Vec2F(left + rx, top);
            path.LineTo(Vec2F(right - rx, top));
            path.CubicTo(Vec2F(right - kx, top), Vec2F(right, top + ky), Vec2F(right, top + ry));
            path.LineTo(Vec2F(right, bottom - ry));
            path.CubicTo(Vec2F(right, bottom - ky), Vec2F(right - kx, bottom), Vec2F(right - rx, bottom));
            path.LineTo(Vec2F(left + rx, bottom));
            path.CubicTo(Vec2F(left + kx, bottom), Vec2F(left, bottom - ky), Vec2F(left, bottom - ry));
            path.LineTo(Vec2F(left, top + ry));
            path.CubicTo(Vec2F(left, top + ky), Vec2F(left + kx, top), Vec2F(left + rx, top));
        }

        subPaths.Add(path);
    }

    void VectorShape::AddEllipse(const Vec2F& center, const Vec2F& radius)
    {
        const float kappa = 0.5522847498f;

        float kx = radius.x*kappa, ky = radius.y*kappa;
        float cx = center.x, cy = center.y;

        VectorSubPath path;
        path.closed = true;
        path.start = Vec2F(cx + radius.x, cy);
        path.CubicTo(Vec2F(cx + radius.x, cy + ky), Vec2F(cx + kx, cy + radius.y), Vec2F(cx, cy + radius.y));
        path.CubicTo(Vec2F(cx - kx, cy + radius.y), Vec2F(cx - radius.x, cy + ky), Vec2F(cx - radius.x, cy));
        path.CubicTo(Vec2F(cx - radius.x, cy - ky), Vec2F(cx - kx, cy - radius.y), Vec2F(cx, cy - radius.y));
        path.CubicTo(Vec2F(cx + kx, cy - radius.y), Vec2F(cx + radius.x, cy - ky), Vec2F(cx + radius.x, cy));

        subPaths.Add(path);
    }

    void VectorShape::AddPolyline(const Vector<Vec2F>& points, bool closed)
    {
        if (points.empty())
            return;

        VectorSubPath path;
        path.closed = closed;
        path.start = points[0];
        for (size_t i = 1; i < points.size(); i++)
            path.LineTo(points[i]);

        subPaths.Add(path);
    }

    void VectorShape::Transform(const Basis& transform)
    {
        for (VectorSubPath& path : subPaths)
        {
            path.start = transform.Transform(path.start);
            for (VectorSegment& segment : path.segments)
            {
                segment.control1 = transform.Transform(segment.control1);
                segment.control2 = transform.Transform(segment.control2);
                segment.end = transform.Transform(segment.end);
            }
        }

        strokeWidth *= Math::Sqrt(Math::Abs(transform.xv.x*transform.yv.y - transform.yv.x*transform.xv.y));

        fill.transform = fill.transform*transform;
        stroke.transform = stroke.transform*transform;
    }

    bool VectorShape::GetBounds(Vec2F& min, Vec2F& max) const
    {
        const float boundsTolerance = 0.01f;

        Vector<Vec2F> points;
        for (const VectorSubPath& path : subPaths)
            path.Flatten(boundsTolerance, points);

        if (points.empty())
            return false;

        min = max = points[0];
        for (const Vec2F& point : points)
        {
            min.x = Math::Min(min.x, point.x);
            min.y = Math::Min(min.y, point.y);
            max.x = Math::Max(max.x, point.x);
            max.y = Math::Max(max.y, point.y);
        }

        return true;
    }

    void VectorImage::Clear()
    {
        size = Vec2F();
        viewBoxOrigin = Vec2F();
        viewBoxSize = Vec2F();
        shapes.Clear();
    }
}
// --- META ---

ENUM_META(o2::VectorFillRule, o2__VectorFillRule)
{
    ENUM_ENTRY(EvenOdd);
    ENUM_ENTRY(NonZero);
}
END_ENUM_META;

ENUM_META(o2::VectorLineCap, o2__VectorLineCap)
{
    ENUM_ENTRY(Butt);
    ENUM_ENTRY(Round);
    ENUM_ENTRY(Square);
}
END_ENUM_META;

ENUM_META(o2::VectorLineJoin, o2__VectorLineJoin)
{
    ENUM_ENTRY(Bevel);
    ENUM_ENTRY(Miter);
    ENUM_ENTRY(Round);
}
END_ENUM_META;

ENUM_META(o2::VectorPaintType, o2__VectorPaintType)
{
    ENUM_ENTRY(LinearGradient);
    ENUM_ENTRY(None);
    ENUM_ENTRY(RadialGradient);
    ENUM_ENTRY(Solid);
}
END_ENUM_META;
// --- END META ---
