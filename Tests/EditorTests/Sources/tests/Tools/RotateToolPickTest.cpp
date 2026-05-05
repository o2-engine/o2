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
