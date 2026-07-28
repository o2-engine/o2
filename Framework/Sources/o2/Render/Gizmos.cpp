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
    }

    void Gizmos::ResetProjection()
    {
        mProjection = [](const Vec3F& point) { return Vec2F(point.x, point.y); };
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
        DrawProjectedLine({ begin, end }, false);
    }

    void Gizmos::DrawPolyLine(const Vector<Vec3F>& points, bool closed /*= false*/)
    {
        DrawProjectedLine(points, closed);
    }

    void Gizmos::DrawCircle(const Vec3F& center, const Vec3F& axisU, const Vec3F& axisV, float radius,
                            int segments /*= 32*/)
    {
        Vector<Vec3F> points;
        points.Reserve(segments);

        for (int i = 0; i < segments; i++)
        {
            float angle = (float)i/(float)segments*2.0f*Math::PI();
            points.Add(center + (axisU*Math::Cos(angle) + axisV*Math::Sin(angle))*radius);
        }

        DrawProjectedLine(points, true);
    }

    void Gizmos::DrawCircle(const Vec3F& center, float radius, int segments /*= 32*/)
    {
        DrawCircle(center, Vec3F(1, 0, 0), Vec3F(0, 1, 0), radius, segments);
    }

    void Gizmos::DrawRect(const Vec3F& center, const Vec3F& halfAxisX, const Vec3F& halfAxisY)
    {
        DrawProjectedLine({ center - halfAxisX - halfAxisY, center + halfAxisX - halfAxisY,
                            center + halfAxisX + halfAxisY, center - halfAxisX + halfAxisY }, true);
    }

    void Gizmos::DrawBox(const Vec3F& center, const Vec3F& halfAxisX, const Vec3F& halfAxisY, const Vec3F& halfAxisZ)
    {
        Vec3F corners[8];
        int i = 0;
        for (int x = -1; x <= 1; x += 2)
        {
            for (int y = -1; y <= 1; y += 2)
            {
                for (int z = -1; z <= 1; z += 2)
                    corners[i++] = center + halfAxisX*(float)x + halfAxisY*(float)y + halfAxisZ*(float)z;
            }
        }

        static const int edges[12][2] = {
            { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 },
            { 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 },
            { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
        };

        for (auto& edge : edges)
            DrawLine(corners[edge[0]], corners[edge[1]]);
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

    void Gizmos::DrawProjectedLine(const Vector<Vec3F>& points, bool closed)
    {
        if (points.Count() < 2)
            return;

        Vector<Vec2F> projected;
        projected.Reserve(points.Count() + 1);

        for (auto& point : points)
            projected.Add(mProjection(point));

        if (closed)
            projected.Add(projected[0]);

        o2Render.DrawAALine(projected, mColor);

        mDrawnPrimitives++;
    }
}
