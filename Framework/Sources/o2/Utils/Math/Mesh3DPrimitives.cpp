#include "o2/stdafx.h"
#include "Mesh3DPrimitives.h"

#include "o2/Utils/Math/Geometry.h"
#include "o2/Utils/Math/Math.h"

namespace o2
{
    bool Mesh3DData::GetBounds(AABB& bounds) const
    {
        if (positions.IsEmpty())
            return false;

        bounds = AABB::Bound(&positions[0], positions.Count());
        return true;
    }
}

namespace o2::Mesh3DPrimitives
{
    namespace
    {
        void AddQuad(Mesh3DData& data, const Vec3F& p0, const Vec3F& p1, const Vec3F& p2, const Vec3F& p3,
                     const Vec3F& normal)
        {
            UInt base = data.positions.Count();

            data.positions.Add(p0);
            data.positions.Add(p1);
            data.positions.Add(p2);
            data.positions.Add(p3);

            for (int i = 0; i < 4; i++)
                data.normals.Add(normal);

            data.uvs.Add(Vec2F(0, 0));
            data.uvs.Add(Vec2F(1, 0));
            data.uvs.Add(Vec2F(1, 1));
            data.uvs.Add(Vec2F(0, 1));

            data.indices.Add(base); data.indices.Add(base + 1); data.indices.Add(base + 2);
            data.indices.Add(base); data.indices.Add(base + 2); data.indices.Add(base + 3);
        }
    }

    Mesh3DData BuildBox(const Vec3F& size)
    {
        Mesh3DData data;
        Vec3F h = size*0.5f;

        AddQuad(data, Vec3F(-h.x, -h.y, h.z), Vec3F(h.x, -h.y, h.z), Vec3F(h.x, h.y, h.z), Vec3F(-h.x, h.y, h.z),
                Vec3F(0, 0, 1));

        AddQuad(data, Vec3F(h.x, -h.y, -h.z), Vec3F(-h.x, -h.y, -h.z), Vec3F(-h.x, h.y, -h.z), Vec3F(h.x, h.y, -h.z),
                Vec3F(0, 0, -1));

        AddQuad(data, Vec3F(h.x, -h.y, h.z), Vec3F(h.x, -h.y, -h.z), Vec3F(h.x, h.y, -h.z), Vec3F(h.x, h.y, h.z),
                Vec3F(1, 0, 0));

        AddQuad(data, Vec3F(-h.x, -h.y, -h.z), Vec3F(-h.x, -h.y, h.z), Vec3F(-h.x, h.y, h.z), Vec3F(-h.x, h.y, -h.z),
                Vec3F(-1, 0, 0));

        AddQuad(data, Vec3F(-h.x, h.y, h.z), Vec3F(h.x, h.y, h.z), Vec3F(h.x, h.y, -h.z), Vec3F(-h.x, h.y, -h.z),
                Vec3F(0, 1, 0));

        AddQuad(data, Vec3F(-h.x, -h.y, -h.z), Vec3F(h.x, -h.y, -h.z), Vec3F(h.x, -h.y, h.z), Vec3F(-h.x, -h.y, h.z),
                Vec3F(0, -1, 0));

        return data;
    }

    Mesh3DData BuildSphere(float radius, int segments, int rings)
    {
        Mesh3DData data;

        segments = Math::Max(segments, 3);
        rings = Math::Max(rings, 2);

        for (int r = 0; r <= rings; r++)
        {
            float v = (float)r/(float)rings;
            float phi = Math::PI()*v;
            float y = Math::Cos(phi);
            float ringRadius = Math::Sin(phi);

            for (int s = 0; s <= segments; s++)
            {
                float u = (float)s/(float)segments;
                float theta = 2.0f*Math::PI()*u;

                Vec3F normal(ringRadius*Math::Cos(theta), y, ringRadius*Math::Sin(theta));
                data.positions.Add(normal*radius);
                data.normals.Add(normal.Normalized());
                data.uvs.Add(Vec2F(u, 1.0f - v));
            }
        }

        for (int r = 0; r < rings; r++)
        {
            for (int s = 0; s < segments; s++)
            {
                UInt a = r*(segments + 1) + s;
                UInt b = a + segments + 1;

                data.indices.Add(a); data.indices.Add(b); data.indices.Add(a + 1);
                data.indices.Add(a + 1); data.indices.Add(b); data.indices.Add(b + 1);
            }
        }

        return data;
    }

    Mesh3DData BuildPlane(const Vec2F& size)
    {
        Mesh3DData data;
        AddQuad(data, Vec3F(-size.x*0.5f, -size.y*0.5f, 0.0f), Vec3F(size.x*0.5f, -size.y*0.5f, 0.0f),
                Vec3F(size.x*0.5f, size.y*0.5f, 0.0f), Vec3F(-size.x*0.5f, size.y*0.5f, 0.0f),
                Vec3F(0, 0, 1));

        return data;
    }

    Mesh3DData BuildCylinder(float radius, float height, int segments)
    {
        Mesh3DData data;

        segments = Math::Max(segments, 3);
        float h = height*0.5f;

        for (int s = 0; s <= segments; s++)
        {
            float u = (float)s/(float)segments;
            float theta = 2.0f*Math::PI()*u;
            Vec3F normal(Math::Cos(theta), 0.0f, Math::Sin(theta));

            data.positions.Add(Vec3F(normal.x*radius, -h, normal.z*radius));
            data.normals.Add(normal);
            data.uvs.Add(Vec2F(u, 0.0f));

            data.positions.Add(Vec3F(normal.x*radius, h, normal.z*radius));
            data.normals.Add(normal);
            data.uvs.Add(Vec2F(u, 1.0f));
        }

        for (int s = 0; s < segments; s++)
        {
            UInt b = s*2;
            data.indices.Add(b); data.indices.Add(b + 2); data.indices.Add(b + 1);
            data.indices.Add(b + 1); data.indices.Add(b + 2); data.indices.Add(b + 3);
        }

        for (int cap = 0; cap < 2; cap++)
        {
            float y = cap == 0 ? h : -h;
            Vec3F normal(0.0f, cap == 0 ? 1.0f : -1.0f, 0.0f);

            UInt center = data.positions.Count();
            data.positions.Add(Vec3F(0.0f, y, 0.0f));
            data.normals.Add(normal);
            data.uvs.Add(Vec2F(0.5f, 0.5f));

            for (int s = 0; s <= segments; s++)
            {
                float theta = 2.0f*Math::PI()*(float)s/(float)segments;
                float cos = Math::Cos(theta), sin = Math::Sin(theta);

                data.positions.Add(Vec3F(cos*radius, y, sin*radius));
                data.normals.Add(normal);
                data.uvs.Add(Vec2F(0.5f + cos*0.5f, 0.5f + sin*0.5f));
            }

            for (int s = 0; s < segments; s++)
            {
                data.indices.Add(center);
                data.indices.Add(center + 1 + s);
                data.indices.Add(center + 2 + s);
            }
        }

        return data;
    }

    Mesh3DData BuildCone(float radius, float height, int segments)
    {
        Mesh3DData data;

        segments = Math::Max(segments, 3);
        float h = height*0.5f;

        // Side: per-segment apex duplicates keep smooth slope normals
        float slopeLength = Math::Sqrt(radius*radius + height*height);
        float normalY = slopeLength > FLT_EPSILON ? radius/slopeLength : 0.0f;
        float normalRadial = slopeLength > FLT_EPSILON ? height/slopeLength : 1.0f;

        for (int s = 0; s <= segments; s++)
        {
            float u = (float)s/(float)segments;
            float theta = 2.0f*Math::PI()*u;
            Vec3F radial(Math::Cos(theta), 0.0f, Math::Sin(theta));
            Vec3F normal = (radial*normalRadial + Vec3F(0.0f, normalY, 0.0f)).Normalized();

            data.positions.Add(Vec3F(radial.x*radius, -h, radial.z*radius));
            data.normals.Add(normal);
            data.uvs.Add(Vec2F(u, 0.0f));

            data.positions.Add(Vec3F(0.0f, h, 0.0f));
            data.normals.Add(normal);
            data.uvs.Add(Vec2F(u, 1.0f));
        }

        for (int s = 0; s < segments; s++)
        {
            UInt b = s*2;
            data.indices.Add(b); data.indices.Add(b + 2); data.indices.Add(b + 1);
        }

        Vec3F capNormal(0.0f, -1.0f, 0.0f);
        UInt center = data.positions.Count();
        data.positions.Add(Vec3F(0.0f, -h, 0.0f));
        data.normals.Add(capNormal);
        data.uvs.Add(Vec2F(0.5f, 0.5f));

        for (int s = 0; s <= segments; s++)
        {
            float theta = 2.0f*Math::PI()*(float)s/(float)segments;
            float cos = Math::Cos(theta), sin = Math::Sin(theta);

            data.positions.Add(Vec3F(cos*radius, -h, sin*radius));
            data.normals.Add(capNormal);
            data.uvs.Add(Vec2F(0.5f + cos*0.5f, 0.5f + sin*0.5f));
        }

        for (int s = 0; s < segments; s++)
        {
            data.indices.Add(center);
            data.indices.Add(center + 1 + s);
            data.indices.Add(center + 2 + s);
        }

        return data;
    }

    Mesh3DData BuildTorus(float radius, float tubeRadius, int segments, int tubeSegments)
    {
        Mesh3DData data;

        segments = Math::Max(segments, 3);
        tubeSegments = Math::Max(tubeSegments, 3);

        for (int s = 0; s <= segments; s++)
        {
            float u = (float)s/(float)segments;
            float theta = 2.0f*Math::PI()*u;
            Vec3F ringDir(Math::Cos(theta), 0.0f, Math::Sin(theta));
            Vec3F ringCenter = ringDir*radius;

            for (int t = 0; t <= tubeSegments; t++)
            {
                float v = (float)t/(float)tubeSegments;
                float phi = 2.0f*Math::PI()*v;

                Vec3F normal = ringDir*Math::Cos(phi) + Vec3F(0.0f, Math::Sin(phi), 0.0f);
                data.positions.Add(ringCenter + normal*tubeRadius);
                data.normals.Add(normal);
                data.uvs.Add(Vec2F(u, v));
            }
        }

        for (int s = 0; s < segments; s++)
        {
            for (int t = 0; t < tubeSegments; t++)
            {
                UInt a = s*(tubeSegments + 1) + t;
                UInt b = a + tubeSegments + 1;

                data.indices.Add(a); data.indices.Add(b); data.indices.Add(a + 1);
                data.indices.Add(a + 1); data.indices.Add(b); data.indices.Add(b + 1);
            }
        }

        return data;
    }

    Mesh3DData BuildFlatRing(float radius, float width, int segments)
    {
        Mesh3DData data;

        segments = Math::Max(segments, 3);
        float innerRadius = Math::Clamp(radius - width, 0.0f, radius);

        for (int s = 0; s <= segments; s++)
        {
            float u = (float)s/(float)segments;
            float theta = 2.0f*Math::PI()*u;
            Vec3F radial(Math::Cos(theta), 0.0f, Math::Sin(theta));

            data.positions.Add(radial*innerRadius);
            data.normals.Add(Vec3F(0.0f, 1.0f, 0.0f));
            data.uvs.Add(Vec2F(u, 0.0f));

            data.positions.Add(radial*radius);
            data.normals.Add(Vec3F(0.0f, 1.0f, 0.0f));
            data.uvs.Add(Vec2F(u, 1.0f));
        }

        for (int s = 0; s < segments; s++)
        {
            UInt b = s*2;
            data.indices.Add(b); data.indices.Add(b + 2); data.indices.Add(b + 1);
            data.indices.Add(b + 1); data.indices.Add(b + 2); data.indices.Add(b + 3);
        }

        return data;
    }

    namespace
    {
        void AppendGeometry(Mesh3DData& destination, const Mesh3DData& source, const Vec3F& offset)
        {
            UInt base = destination.positions.Count();

            for (auto& p : source.positions)
                destination.positions.Add(p + offset);

            destination.normals.Add(source.normals);
            destination.uvs.Add(source.uvs);

            for (auto index : source.indices)
                destination.indices.Add(base + index);
        }
    }

    Mesh3DData BuildArrowGeometry(float length, float shaftRadius, float headLength, float headRadius,
                                  bool cubeHead)
    {
        const int segments = 10;
        float shaftLength = Math::Max(length - headLength, 0.01f);

        Mesh3DData data = BuildCylinder(shaftRadius, shaftLength, segments);
        for (auto& p : data.positions)
            p.y += shaftLength*0.5f;

        Mesh3DData head = cubeHead
            ? BuildBox(Vec3F(headRadius*2.0f, headLength, headRadius*2.0f))
            : BuildCone(headRadius, headLength, segments);

        AppendGeometry(data, head, Vec3F(0.0f, shaftLength + headLength*0.5f, 0.0f));
        return data;
    }

    Mesh3DData BuildPlaneHandleGeometry(int planeNormalAxis, float offset, float size,
                                        const Vec3F& faceAwayDirection)
    {
        Vec3F u, v;
        Geometry::AxisPlaneBasis(planeNormalAxis, u, v);

        Vec3F normal = Vec3F::Axis(planeNormalAxis);
        if (normal.Dot(faceAwayDirection) > 0.0f)
            normal = normal*-1.0f;

        Mesh3DData data;
        Vec3F corners[4] = { u*offset + v*offset,
                             u*(offset + size) + v*offset,
                             u*(offset + size) + v*(offset + size),
                             u*offset + v*(offset + size) };

        for (int i = 0; i < 4; i++)
        {
            data.positions.Add(corners[i]);
            data.normals.Add(normal);
            data.uvs.Add(Vec2F(i == 1 || i == 2 ? 1.0f : 0.0f, i >= 2 ? 1.0f : 0.0f));
        }

        // Single winding: culling is off, and a second one would double the alpha blending
        data.indices.Add(0); data.indices.Add(1); data.indices.Add(2);
        data.indices.Add(0); data.indices.Add(2); data.indices.Add(3);

        return data;
    }

    Mesh3DData BuildCornerHandleGeometry(float armLength, float thickness)
    {
        Mesh3DData data = BuildBox(Vec3F(armLength, thickness, thickness));
        for (auto& p : data.positions)
            p.x += armLength*0.5f;

        Mesh3DData secondArm = BuildBox(Vec3F(thickness, armLength, thickness));
        AppendGeometry(data, secondArm, Vec3F(0.0f, armLength*0.5f, 0.0f));

        return data;
    }
}
