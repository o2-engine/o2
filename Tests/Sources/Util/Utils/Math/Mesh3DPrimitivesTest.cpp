#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Math/Mesh3DPrimitives.h"

using namespace o2;

namespace
{
    constexpr float kEps = 0.001f;

    void ExpectDataConsistent(const Mesh3DData& data)
    {
        ASSERT_EQ(data.positions.Count(), data.normals.Count());
        ASSERT_EQ(data.positions.Count(), data.uvs.Count());
        ASSERT_EQ(data.indices.Count()%3, 0);

        for (auto& normal : data.normals)
            EXPECT_NEAR(normal.Length(), 1.0f, kEps);

        for (auto index : data.indices)
            EXPECT_LT(index, (UInt)data.positions.Count());
    }

    Vec3F MaxAbsBounds(const Vector<Vec3F>& positions)
    {
        Vec3F bounds;
        for (auto& p : positions)
        {
            bounds.x = Math::Max(bounds.x, Math::Abs(p.x));
            bounds.y = Math::Max(bounds.y, Math::Abs(p.y));
            bounds.z = Math::Max(bounds.z, Math::Abs(p.z));
        }

        return bounds;
    }
}

TEST(Mesh3DPrimitives, BoxCountsAndBounds)
{
    auto data = Mesh3DPrimitives::BuildBox(Vec3F(100, 60, 40));

    EXPECT_EQ(data.positions.Count(), 24);
    EXPECT_EQ(data.indices.Count(), 36);
    ExpectDataConsistent(data);

    Vec3F bounds = MaxAbsBounds(data.positions);
    EXPECT_NEAR(bounds.x, 50.0f, kEps);
    EXPECT_NEAR(bounds.y, 30.0f, kEps);
    EXPECT_NEAR(bounds.z, 20.0f, kEps);
}

TEST(Mesh3DPrimitives, SphereCountsAndRadius)
{
    const int segments = 16, rings = 8;
    const float radius = 50.0f;
    auto data = Mesh3DPrimitives::BuildSphere(radius, segments, rings);

    EXPECT_EQ(data.positions.Count(), (segments + 1)*(rings + 1));
    EXPECT_EQ(data.indices.Count(), segments*rings*6);
    ExpectDataConsistent(data);

    for (auto& p : data.positions)
        EXPECT_NEAR(p.Length(), radius, kEps);
}

TEST(Mesh3DPrimitives, PlaneCountsAndBounds)
{
    auto data = Mesh3DPrimitives::BuildPlane(Vec2F(200, 100));

    EXPECT_EQ(data.positions.Count(), 4);
    EXPECT_EQ(data.indices.Count(), 6);
    ExpectDataConsistent(data);

    Vec3F bounds = MaxAbsBounds(data.positions);
    EXPECT_NEAR(bounds.x, 100.0f, kEps);
    EXPECT_NEAR(bounds.y, 50.0f, kEps);
    EXPECT_NEAR(bounds.z, 0.0f, kEps);

    // Plane faces +Z towards the default 2D camera
    for (auto& normal : data.normals)
        EXPECT_NEAR(normal.z, 1.0f, kEps);
}

TEST(Mesh3DPrimitives, CylinderCountsAndBounds)
{
    const int segments = 12;
    const float radius = 30.0f, height = 80.0f;
    auto data = Mesh3DPrimitives::BuildCylinder(radius, height, segments);

    EXPECT_EQ(data.positions.Count(), (segments + 1)*2 + (segments + 2)*2);
    EXPECT_EQ(data.indices.Count(), segments*12);
    ExpectDataConsistent(data);

    Vec3F bounds = MaxAbsBounds(data.positions);
    EXPECT_NEAR(bounds.x, radius, kEps);
    EXPECT_NEAR(bounds.y, height*0.5f, kEps);
    EXPECT_NEAR(bounds.z, radius, kEps);

    for (auto& p : data.positions)
        EXPECT_LE(Vec2F(p.x, p.z).Length(), radius + kEps);
}

TEST(Mesh3DPrimitives, ConeCountsAndBounds)
{
    const int segments = 12;
    const float radius = 30.0f, height = 80.0f;
    auto data = Mesh3DPrimitives::BuildCone(radius, height, segments);

    EXPECT_EQ(data.positions.Count(), (segments + 1)*2 + segments + 2);
    EXPECT_EQ(data.indices.Count(), segments*6);
    ExpectDataConsistent(data);

    Vec3F bounds = MaxAbsBounds(data.positions);
    EXPECT_NEAR(bounds.x, radius, kEps);
    EXPECT_NEAR(bounds.y, height*0.5f, kEps);
    EXPECT_NEAR(bounds.z, radius, kEps);

    // Base ring lies at -height/2, apex at +height/2
    float minY = FLT_MAX, maxY = -FLT_MAX;
    for (auto& p : data.positions)
    {
        minY = Math::Min(minY, p.y);
        maxY = Math::Max(maxY, p.y);
    }
    EXPECT_NEAR(minY, -height*0.5f, kEps);
    EXPECT_NEAR(maxY, height*0.5f, kEps);
}

TEST(Mesh3DPrimitives, TorusCountsAndBounds)
{
    const int segments = 16, tubeSegments = 8;
    const float radius = 50.0f, tubeRadius = 5.0f;
    auto data = Mesh3DPrimitives::BuildTorus(radius, tubeRadius, segments, tubeSegments);

    EXPECT_EQ(data.positions.Count(), (segments + 1)*(tubeSegments + 1));
    EXPECT_EQ(data.indices.Count(), segments*tubeSegments*6);
    ExpectDataConsistent(data);

    Vec3F bounds = MaxAbsBounds(data.positions);
    EXPECT_NEAR(bounds.x, radius + tubeRadius, kEps);
    EXPECT_NEAR(bounds.y, tubeRadius, kEps);
    EXPECT_NEAR(bounds.z, radius + tubeRadius, kEps);

    // Every vertex sits exactly tubeRadius from the ring circle in the XZ plane
    for (auto& p : data.positions)
    {
        float ringDistance = Vec2F(p.x, p.z).Length() - radius;
        EXPECT_NEAR(Vec2F(ringDistance, p.y).Length(), tubeRadius, kEps);
    }
}

TEST(Mesh3DPrimitives, FlatRingCountsAndBounds)
{
    const int segments = 32;
    const float radius = 50.0f, width = 10.0f;
    auto data = Mesh3DPrimitives::BuildFlatRing(radius, width, segments);

    EXPECT_EQ(data.positions.Count(), (segments + 1)*2);
    EXPECT_EQ(data.indices.Count(), segments*6);
    ExpectDataConsistent(data);

    // Flat in the XZ plane, all normals +Y, radial distance between inner and outer radius
    for (auto& p : data.positions)
    {
        EXPECT_NEAR(p.y, 0.0f, kEps);

        float radial = Vec2F(p.x, p.z).Length();
        EXPECT_GE(radial, radius - width - kEps);
        EXPECT_LE(radial, radius + kEps);
    }

    for (auto& normal : data.normals)
        EXPECT_NEAR(normal.y, 1.0f, kEps);
}

TEST(Mesh3DPrimitives, DegenerateParamsAreClamped)
{
    auto sphere = Mesh3DPrimitives::BuildSphere(10.0f, 1, 1);
    EXPECT_EQ(sphere.positions.Count(), 4*3);
    ExpectDataConsistent(sphere);

    auto cylinder = Mesh3DPrimitives::BuildCylinder(10.0f, 10.0f, 1);
    EXPECT_EQ(cylinder.positions.Count(), 4*2 + 5*2);
    ExpectDataConsistent(cylinder);

    auto cone = Mesh3DPrimitives::BuildCone(10.0f, 10.0f, 1);
    EXPECT_EQ(cone.positions.Count(), 4*2 + 5);
    ExpectDataConsistent(cone);

    auto torus = Mesh3DPrimitives::BuildTorus(10.0f, 1.0f, 1, 1);
    EXPECT_EQ(torus.positions.Count(), 4*4);
    ExpectDataConsistent(torus);

    // Width over radius clamps the inner radius to zero
    auto ring = Mesh3DPrimitives::BuildFlatRing(10.0f, 20.0f, 1);
    EXPECT_EQ(ring.positions.Count(), 4*2);
    ExpectDataConsistent(ring);
    for (auto& p : ring.positions)
        EXPECT_LE(Vec2F(p.x, p.z).Length(), 10.0f + kEps);
}

TEST(Mesh3DPrimitives, ArrowGeometrySpansPlusY)
{
    auto data = Mesh3DPrimitives::BuildArrowGeometry(1.0f, 0.02f, 0.25f, 0.07f, false);
    ASSERT_FALSE(data.positions.IsEmpty());
    ExpectDataConsistent(data);

    float minY = FLT_MAX, maxY = -FLT_MAX, maxRadial = 0.0f;
    for (auto& p : data.positions)
    {
        minY = Math::Min(minY, p.y);
        maxY = Math::Max(maxY, p.y);
        maxRadial = Math::Max(maxRadial, Vec2F(p.x, p.z).Length());
    }

    EXPECT_NEAR(minY, 0.0f, 1e-3f);
    EXPECT_NEAR(maxY, 1.0f, 1e-3f);
    EXPECT_NEAR(maxRadial, 0.07f, 1e-3f);
}

TEST(Mesh3DPrimitives, PlaneHandleGeometryLiesInPlaneAndFacesAgainstDirection)
{
    Vec3F faceAway = Vec3F(0.3f, -0.5f, -0.8f).Normalized();
    auto data = Mesh3DPrimitives::BuildPlaneHandleGeometry(2, 0.35f, 0.4f, faceAway);
    ASSERT_EQ(data.positions.Count(), 4);
    ASSERT_EQ(data.indices.Count(), 6);

    for (auto& p : data.positions)
    {
        EXPECT_NEAR(p.z, 0.0f, 1e-4f);
        EXPECT_GE(p.x, 0.35f - 1e-4f);
        EXPECT_LE(p.x, 0.75f + 1e-4f);
        EXPECT_GE(p.y, 0.35f - 1e-4f);
        EXPECT_LE(p.y, 0.75f + 1e-4f);
    }

    // The normal is oriented against the given direction for every plane axis and direction sign
    for (int axis = 0; axis < 3; axis++)
    {
        for (float sign : { 1.0f, -1.0f })
        {
            Vec3F direction = Vec3F::Axis(axis)*sign;
            auto quad = Mesh3DPrimitives::BuildPlaneHandleGeometry(axis, 0.1f, 0.5f, direction);

            for (auto& normal : quad.normals)
            {
                EXPECT_NEAR(normal.Length(), 1.0f, 1e-4f);
                EXPECT_LE(normal.Dot(direction), 0.0f);
            }
        }
    }
}

TEST(Mesh3DPrimitives, CornerHandleGeometryTwoArmsAlongXAndY)
{
    const float armLength = 1.0f, thickness = 0.15f;
    auto data = Mesh3DPrimitives::BuildCornerHandleGeometry(armLength, thickness);

    // Two boxes: 24 vertices and 36 indices each
    ASSERT_EQ(data.positions.Count(), 48);
    ASSERT_EQ(data.indices.Count(), 72);
    ExpectDataConsistent(data);

    float half = thickness*0.5f;
    o2::AABB bounds;
    ASSERT_TRUE(data.GetBounds(bounds));

    EXPECT_NEAR(bounds.min.x, -half, 1e-4f);
    EXPECT_NEAR(bounds.max.x, armLength, 1e-4f);
    EXPECT_NEAR(bounds.min.y, -half, 1e-4f);
    EXPECT_NEAR(bounds.max.y, armLength, 1e-4f);
    EXPECT_NEAR(bounds.min.z, -half, 1e-4f);
    EXPECT_NEAR(bounds.max.z, half, 1e-4f);

    // Every vertex belongs to the X arm or to the Y arm
    for (auto& p : data.positions)
        EXPECT_TRUE(Math::Abs(p.y) <= half + 1e-4f || Math::Abs(p.x) <= half + 1e-4f);
}

TEST(Mesh3DPrimitives, DataBounds)
{
    auto data = Mesh3DPrimitives::BuildBox(Vec3F(100, 60, 40));

    o2::AABB bounds;
    ASSERT_TRUE(data.GetBounds(bounds));
    EXPECT_NEAR(bounds.min.x, -50.0f, kEps);
    EXPECT_NEAR(bounds.max.y, 30.0f, kEps);
    EXPECT_NEAR(bounds.max.z, 20.0f, kEps);

    Mesh3DData empty;
    EXPECT_FALSE(empty.GetBounds(bounds));
}
