#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Math/Geometry.h"
#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Math/Vector3.h"

using namespace o2;

TEST(GeometryRay, CylinderPerpendicularHitAndMiss)
{
    Vec3F start(0, 0, 0), end(0, 10, 0);
    float radius = 1.0f;
    float distance = 0.0f;

    // Hit the side at the middle of the segment
    ASSERT_TRUE(Geometry::RayIntersectsCylinder(Vec3F(5, 5, 0), Vec3F(-1, 0, 0), start, end, radius, distance));
    EXPECT_NEAR(distance, 4.0f, 1e-4f);

    // Miss beyond the radius
    EXPECT_FALSE(Geometry::RayIntersectsCylinder(Vec3F(5, 5, 2), Vec3F(-1, 0, 0), start, end, radius, distance));

    // Miss beyond the segment ends
    EXPECT_FALSE(Geometry::RayIntersectsCylinder(Vec3F(5, 12, 0), Vec3F(-1, 0, 0), start, end, radius, distance));
    EXPECT_FALSE(Geometry::RayIntersectsCylinder(Vec3F(5, -2, 0), Vec3F(-1, 0, 0), start, end, radius, distance));
}

TEST(GeometryRay, CylinderAlongAxisHitsCap)
{
    Vec3F start(0, 0, 0), end(0, 10, 0);
    float distance = 0.0f;

    // Ray along the axis inside the radius hits the near cap
    ASSERT_TRUE(Geometry::RayIntersectsCylinder(Vec3F(0.5f, -5, 0), Vec3F(0, 1, 0), start, end, 1.0f, distance));
    EXPECT_NEAR(distance, 5.0f, 1e-4f);

    // Ray along the axis outside the radius misses
    EXPECT_FALSE(Geometry::RayIntersectsCylinder(Vec3F(2, -5, 0), Vec3F(0, 1, 0), start, end, 1.0f, distance));
}

TEST(GeometryRay, CylinderDiagonalCapHit)
{
    Vec3F start(0, 0, 0), end(0, 10, 0);
    float distance = 0.0f;

    // Diagonal ray entering through the top cap
    Vec3F direction = Vec3F(0.1f, -1.0f, 0.0f).Normalized();
    ASSERT_TRUE(Geometry::RayIntersectsCylinder(Vec3F(-0.5f, 15, 0), direction, start, end, 1.0f, distance));

    Vec3F hit = Vec3F(-0.5f, 15, 0) + direction*distance;
    EXPECT_NEAR(hit.y, 10.0f, 1e-3f);
    EXPECT_LE(Vec2F(hit.x, hit.z).Length(), 1.0f + 1e-3f);
}

TEST(GeometryRay, CylinderBehindRayIsRejected)
{
    Vec3F start(0, 0, 0), end(0, 10, 0);
    float distance = 0.0f;
    EXPECT_FALSE(Geometry::RayIntersectsCylinder(Vec3F(5, 5, 0), Vec3F(1, 0, 0), start, end, 1.0f, distance));
}

TEST(GeometryRay, QuadCenterAndEdgesHit)
{
    Vec3F corner(0, 0, 0), edgeU(2, 0, 0), edgeV(0, 3, 0);
    float distance = 0.0f;

    // Center hit
    ASSERT_TRUE(Geometry::RayIntersectsQuad(Vec3F(1.0f, 1.5f, 5.0f), Vec3F(0, 0, -1), corner, edgeU, edgeV, distance));
    EXPECT_NEAR(distance, 5.0f, 1e-4f);

    // Just inside the edge
    EXPECT_TRUE(Geometry::RayIntersectsQuad(Vec3F(1.99f, 0.01f, 5.0f), Vec3F(0, 0, -1), corner, edgeU, edgeV, distance));

    // Just outside the edge
    EXPECT_FALSE(Geometry::RayIntersectsQuad(Vec3F(2.01f, 1.0f, 5.0f), Vec3F(0, 0, -1), corner, edgeU, edgeV, distance));
    EXPECT_FALSE(Geometry::RayIntersectsQuad(Vec3F(1.0f, -0.01f, 5.0f), Vec3F(0, 0, -1), corner, edgeU, edgeV, distance));
}

TEST(GeometryRay, QuadTwoSidedAndParallel)
{
    Vec3F corner(0, 0, 0), edgeU(2, 0, 0), edgeV(0, 3, 0);
    float distance = 0.0f;

    // Backside hit counts (two-sided)
    EXPECT_TRUE(Geometry::RayIntersectsQuad(Vec3F(1.0f, 1.0f, -5.0f), Vec3F(0, 0, 1), corner, edgeU, edgeV, distance));

    // Ray parallel to the quad plane misses
    EXPECT_FALSE(Geometry::RayIntersectsQuad(Vec3F(1.0f, 1.0f, 5.0f), Vec3F(1, 0, 0), corner, edgeU, edgeV, distance));
}

TEST(GeometryRay, SkewedQuadParallelogram)
{
    // Non-orthogonal edges: parallelogram
    Vec3F corner(0, 0, 0), edgeU(2, 0, 0), edgeV(1, 2, 0);
    float distance = 0.0f;

    // Point inside the parallelogram (u=0.5, v=0.5 -> (1.5, 1, 0))
    EXPECT_TRUE(Geometry::RayIntersectsQuad(Vec3F(1.5f, 1.0f, 3.0f), Vec3F(0, 0, -1), corner, edgeU, edgeV, distance));

    // Point inside the bounding rect but outside the parallelogram
    EXPECT_FALSE(Geometry::RayIntersectsQuad(Vec3F(0.1f, 1.9f, 3.0f), Vec3F(0, 0, -1), corner, edgeU, edgeV, distance));
}
