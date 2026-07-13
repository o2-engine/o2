#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Tools/RotateTool.h"

using namespace o2;
using namespace Editor;

// Pure ring-pick math used by RotateTool::IsPointInRotateRing. Bug repro:
// the test asserts on a screen-pixel offset (80 px from the pivot) that the
// real editor uses to highlight the ring on hover. A click at the same offset
// must resolve to true — otherwise OnCursorPressed falls through to
// SelectionTool and the click drops the selection instead of starting a rotate.
TEST(RotateToolPick, ScreenPointInsideRingMatchesHover)
{
    const Vec2F pivot(0.0f, 0.0f);
    const float inner = 60.0f;
    const float outer = 100.0f;

    EXPECT_TRUE(RotateTool::IsScreenPointInRing(pivot, Vec2F(80.0f, 0.0f), inner, outer));
    EXPECT_TRUE(RotateTool::IsScreenPointInRing(pivot, Vec2F(0.0f, 80.0f), inner, outer));
    EXPECT_TRUE(RotateTool::IsScreenPointInRing(pivot, Vec2F(-56.57f, -56.57f), inner, outer));
}

TEST(RotateToolPick, ScreenPointInsideInnerDiscIsRejected)
{
    const Vec2F pivot(100.0f, 100.0f);
    EXPECT_FALSE(RotateTool::IsScreenPointInRing(pivot, pivot, 60.0f, 100.0f));
    EXPECT_FALSE(RotateTool::IsScreenPointInRing(pivot, pivot + Vec2F(40.0f, 0.0f), 60.0f, 100.0f));
}

TEST(RotateToolPick, ScreenPointOutsideOuterRingIsRejected)
{
    const Vec2F pivot(100.0f, 100.0f);
    EXPECT_FALSE(RotateTool::IsScreenPointInRing(pivot, pivot + Vec2F(150.0f, 0.0f), 60.0f, 100.0f));
    EXPECT_FALSE(RotateTool::IsScreenPointInRing(pivot, pivot + Vec2F(0.0f, -200.0f), 60.0f, 100.0f));
}

TEST(RotateToolPick, ExactBoundariesAreRejected)
{
    const Vec2F pivot(0.0f, 0.0f);
    EXPECT_FALSE(RotateTool::IsScreenPointInRing(pivot, Vec2F(60.0f, 0.0f), 60.0f, 100.0f));
    EXPECT_FALSE(RotateTool::IsScreenPointInRing(pivot, Vec2F(100.0f, 0.0f), 60.0f, 100.0f));
}

TEST(RotateToolPick, PivotOffsetIsRespected)
{
    const Vec2F pivot(500.0f, -250.0f);
    EXPECT_TRUE(RotateTool::IsScreenPointInRing(pivot, pivot + Vec2F(80.0f, 0.0f), 60.0f, 100.0f));
    EXPECT_FALSE(RotateTool::IsScreenPointInRing(pivot, Vec2F(0.0f, 0.0f), 60.0f, 100.0f));
}

// 3D ring picking: ray vs the flat annulus band the tool actually draws
TEST(RotateToolPick, RayHitInRingBand)
{
    Vec3F pivot(10.0f, 20.0f, 0.0f);
    Vec3F normal(0.0f, 0.0f, 1.0f);
    float inner = 60.0f, outer = 100.0f;
    float hitDistance = 0.0f;

    // Straight-down ray onto the band
    ASSERT_TRUE(RotateTool::IsRayHitInRingBand(pivot + Vec3F(80.0f, 0.0f, 50.0f), Vec3F(0, 0, -1),
                                               pivot, normal, inner, outer, 0.0f, hitDistance));
    EXPECT_NEAR(hitDistance, 50.0f, 1e-3f);

    // Inner disc miss
    EXPECT_FALSE(RotateTool::IsRayHitInRingBand(pivot + Vec3F(30.0f, 0.0f, 50.0f), Vec3F(0, 0, -1),
                                                pivot, normal, inner, outer, 0.0f, hitDistance));

    // Outside miss
    EXPECT_FALSE(RotateTool::IsRayHitInRingBand(pivot + Vec3F(130.0f, 0.0f, 50.0f), Vec3F(0, 0, -1),
                                                pivot, normal, inner, outer, 0.0f, hitDistance));

    // Tolerance extends the band on both sides
    EXPECT_TRUE(RotateTool::IsRayHitInRingBand(pivot + Vec3F(57.0f, 0.0f, 50.0f), Vec3F(0, 0, -1),
                                               pivot, normal, inner, outer, 5.0f, hitDistance));
    EXPECT_TRUE(RotateTool::IsRayHitInRingBand(pivot + Vec3F(103.0f, 0.0f, 50.0f), Vec3F(0, 0, -1),
                                               pivot, normal, inner, outer, 5.0f, hitDistance));
}

TEST(RotateToolPick, RayHitInRingBandEdgeOnAndBehindRejected)
{
    Vec3F pivot;
    Vec3F normal(0.0f, 0.0f, 1.0f);
    float hitDistance = 0.0f;

    // Ray nearly parallel to the ring plane: refused, matching the drag guard
    EXPECT_FALSE(RotateTool::IsRayHitInRingBand(Vec3F(-200.0f, 0.0f, 0.5f), Vec3F(1, 0, 0),
                                                pivot, normal, 60.0f, 100.0f, 0.0f, hitDistance));

    // Plane behind the ray origin
    EXPECT_FALSE(RotateTool::IsRayHitInRingBand(Vec3F(80.0f, 0.0f, -50.0f), Vec3F(0, 0, -1),
                                                pivot, normal, 60.0f, 100.0f, 0.0f, hitDistance));
}

TEST(RotateToolPick, RayHitInRingBandTiltedPlane)
{
    // 45-degree tilted ring
    Vec3F pivot(0.0f, 0.0f, 100.0f);
    Vec3F normal = Vec3F(0.0f, 1.0f, 1.0f).Normalized();
    float hitDistance = 0.0f;

    // Point on the ring: pivot + u*80 where u = (1,0,0) lies in the plane
    Vec3F pointOnRing = pivot + Vec3F(80.0f, 0.0f, 0.0f);
    ASSERT_TRUE(RotateTool::IsRayHitInRingBand(pointOnRing + normal*50.0f, normal*-1.0f,
                                               pivot, normal, 60.0f, 100.0f, 0.0f, hitDistance));
    EXPECT_NEAR(hitDistance, 50.0f, 1e-3f);
}
