#include "o2/stdafx.h"
#include "Gizmos.h"

#include "o2/Render/Render.h"

namespace o2
{
    CREATE_SINGLETON(Gizmos);

    const Color4 Gizmos::colliderColor = Color4(60, 220, 120, 255);
    const Color4 Gizmos::jointColor = Color4(240, 200, 60, 255);
    const Color4 Gizmos::cameraColor = Color4(80, 180, 240, 255);

    Gizmos::Gizmos(RefCounter* refCounter):
        Singleton<Gizmos>(refCounter)
    {
        ResetProjection();
    }

    void Gizmos::SetProjection(const Function<Vec2F(const Vec3F&)>& projection)
    {
        mProjection = projection;
        mClipEnabled = false;
    }

    void Gizmos::SetProjection(const Function<Vec2F(const Vec3F&)>& projection, const Vec3F& clipPlaneOrigin,
                               const Vec3F& clipPlaneNormal)
    {
        SetProjection(projection);

        if (clipPlaneNormal.SqrLength() < FLT_EPSILON)
            return;

        mClipEnabled = true;
        mClipPlaneOrigin = clipPlaneOrigin;
        mClipPlaneNormal = clipPlaneNormal.Normalized();
    }

    void Gizmos::ResetProjection()
    {
        SetProjection([](const Vec3F& point) { return Vec2F(point.x, point.y); });
    }

    void Gizmos::SetColor(const Color4& color)
    {
        mColor = color;
    }

    const Color4& Gizmos::GetColor() const
    {
        return mColor;
    }

    void Gizmos::DrawLine(const Vec3F& begin, const Vec3F& end)
    {
        mLinePoints.Clear();
        mLinePoints.Add(begin);
        mLinePoints.Add(end);

        DrawProjectedLine(mLinePoints, false);
    }

    void Gizmos::DrawPolyLine(const Vector<Vec3F>& points, bool closed /*= false*/)
    {
        DrawProjectedLine(points, closed);
    }

    void Gizmos::DrawCircle(const Vec3F& center, const Vec3F& axisU, const Vec3F& axisV, float radius,
                            int segments /*= 32*/)
    {
        mLinePoints.Clear();

        for (int i = 0; i < segments; i++)
        {
            float angle = (float)i/(float)segments*2.0f*Math::PI();
            mLinePoints.Add(center + (axisU*Math::Cos(angle) + axisV*Math::Sin(angle))*radius);
        }

        DrawProjectedLine(mLinePoints, true);
    }

    void Gizmos::DrawCircle(const Vec3F& center, float radius, int segments /*= 32*/)
    {
        DrawCircle(center, Vec3F(1, 0, 0), Vec3F(0, 1, 0), radius, segments);
    }

    void Gizmos::DrawRect(const Vec3F& center, const Vec3F& halfAxisX, const Vec3F& halfAxisY)
    {
        mLinePoints.Clear();
        mLinePoints.Add(center - halfAxisX - halfAxisY);
        mLinePoints.Add(center + halfAxisX - halfAxisY);
        mLinePoints.Add(center + halfAxisX + halfAxisY);
        mLinePoints.Add(center - halfAxisX + halfAxisY);

        DrawProjectedLine(mLinePoints, true);
    }

    void Gizmos::DrawBox(const Vec3F& center, const Vec3F& halfAxisX, const Vec3F& halfAxisY, const Vec3F& halfAxisZ)
    {
        // Two opposite faces as closed rings plus the four edges between them: 6 poly lines instead of
        // 12 separate segments, each of which costs its own mesh build and draw batch
        DrawRect(center - halfAxisZ, halfAxisX, halfAxisY);
        DrawRect(center + halfAxisZ, halfAxisX, halfAxisY);

        for (int x = -1; x <= 1; x += 2)
        {
            for (int y = -1; y <= 1; y += 2)
            {
                Vec3F corner = center + halfAxisX*(float)x + halfAxisY*(float)y;
                DrawLine(corner - halfAxisZ, corner + halfAxisZ);
            }
        }
    }

    void Gizmos::DrawSphere(const Vec3F& center, float radius, int segments /*= 32*/)
    {
        DrawCircle(center, Vec3F(1, 0, 0), Vec3F(0, 1, 0), radius, segments);
        DrawCircle(center, Vec3F(1, 0, 0), Vec3F(0, 0, 1), radius, segments);
        DrawCircle(center, Vec3F(0, 1, 0), Vec3F(0, 0, 1), radius, segments);
    }

    void Gizmos::DrawCapsule(const Vec3F& center, const Vec3F& up, float radius, float height, int segments /*= 24*/)
    {
        Vec3F upAxis = up.Normalized();
        Vec3F sideAxis = Math::Abs(upAxis.y) < 0.99f ? upAxis.Cross(Vec3F(0, 1, 0)).Normalized() :
            upAxis.Cross(Vec3F(1, 0, 0)).Normalized();
        Vec3F frontAxis = upAxis.Cross(sideAxis).Normalized();

        Vec3F bottom = center - upAxis*height*0.5f;
        Vec3F top = center + upAxis*height*0.5f;

        DrawCircle(bottom, sideAxis, frontAxis, radius, segments);
        DrawCircle(top, sideAxis, frontAxis, radius, segments);

        DrawLine(bottom + sideAxis*radius, top + sideAxis*radius);
        DrawLine(bottom - sideAxis*radius, top - sideAxis*radius);
        DrawLine(bottom + frontAxis*radius, top + frontAxis*radius);
        DrawLine(bottom - frontAxis*radius, top - frontAxis*radius);

        DrawCircle(top, sideAxis, upAxis, radius, segments);
        DrawCircle(top, frontAxis, upAxis, radius, segments);
        DrawCircle(bottom, sideAxis, upAxis, radius, segments);
        DrawCircle(bottom, frontAxis, upAxis, radius, segments);
    }

    void Gizmos::DrawPoint(const Vec3F& point, float size /*= 5.0f*/)
    {
        if (GetClipDistance(point) < 0.0f)
            return;

        Vec2F screenPoint = mProjection(point);

        o2Render.DrawAALine(screenPoint - Vec2F(size, size), screenPoint + Vec2F(size, size), mColor);
        o2Render.DrawAALine(screenPoint - Vec2F(size, -size), screenPoint + Vec2F(size, -size), mColor);

        mDrawnPrimitives += 2;
    }

    int Gizmos::GetDrawnPrimitives() const
    {
        return mDrawnPrimitives;
    }

    void Gizmos::ResetDrawnPrimitives()
    {
        mDrawnPrimitives = 0;
    }

    float Gizmos::GetClipDistance(const Vec3F& point) const
    {
        if (!mClipEnabled)
            return 1.0f;

        return (point - mClipPlaneOrigin).Dot(mClipPlaneNormal);
    }

    void Gizmos::DrawProjectedLine(const Vector<Vec3F>& points, bool closed)
    {
        if (points.Count() < 2)
            return;

        if (mClipEnabled)
        {
            DrawClippedLine(points, closed);
            return;
        }

        // Reused buffer: a wireframe scene issues thousands of these per frame
        Vector<Vec2F>& projected = mProjectedPoints;
        projected.Clear();

        for (auto& point : points)
            projected.Add(mProjection(point));

        if (closed)
            projected.Add(projected[0]);

        o2Render.DrawAALine(projected, mColor);

        mDrawnPrimitives++;
    }

    void Gizmos::DrawClippedLine(const Vector<Vec3F>& points, bool closed)
    {
        Vector<Vec2F>& projected = mProjectedPoints;
        projected.Clear();

        auto flush = [&]()
        {
            if (projected.Count() > 1)
            {
                o2Render.DrawAALine(projected, mColor);
                mDrawnPrimitives++;
            }

            projected.Clear();
        };

        int segments = closed ? points.Count() : points.Count() - 1;
        for (int i = 0; i < segments; i++)
        {
            const Vec3F& begin = points[i];
            const Vec3F& end = points[(i + 1)%points.Count()];

            float beginDistance = GetClipDistance(begin);
            float endDistance = GetClipDistance(end);

            if (beginDistance < 0.0f && endDistance < 0.0f)
            {
                flush();
                continue;
            }

            if (projected.IsEmpty() && beginDistance >= 0.0f)
                projected.Add(mProjection(begin));

            if (beginDistance >= 0.0f && endDistance >= 0.0f)
            {
                projected.Add(mProjection(end));
                continue;
            }

            Vec3F crossing = Vec3F::Lerp(begin, end, beginDistance/(beginDistance - endDistance));

            if (beginDistance >= 0.0f)
            {
                projected.Add(mProjection(crossing));
                flush();
            }
            else
            {
                projected.Add(mProjection(crossing));
                projected.Add(mProjection(end));
            }
        }

        flush();
    }
}
