#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/GridLayout.h"
#include "Scene/SceneTestHelpers.h"
#include "Scene/UI/UITestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(GridLayout, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto g = mmake<GridLayout>();
    ASSERT_TRUE(g);
    EXPECT_GT(g->GetCellSize().x, 0);
    EXPECT_GT(g->GetCellSize().y, 0);
}

TEST(GridLayout, CopyPreservesSettings)
{
    SceneCleanGuard guard;
    auto src = mmake<GridLayout>();
    src->SetCellSize(Vec2F(40, 30));
    src->SetSpacing(2);
    src->SetArrangeAxis(TwoDirection::Vertical);
    src->SetArrangeAxisMaxCells(3);
    auto copy = src->CloneAsRef<GridLayout>();
    EXPECT_EQ(copy->GetCellSize(), Vec2F(40, 30));
    EXPECT_FLOAT_EQ(copy->GetSpacing(), 2);
    EXPECT_EQ(copy->GetArrangeAxis(), TwoDirection::Vertical);
    EXPECT_EQ(copy->GetArrangeAxisMaxCells(), 3);
}

// ===== Properties =====

TEST(GridLayout, SetCellSizeRoundTrip)
{
    SceneCleanGuard guard;
    auto g = mmake<GridLayout>();
    g->SetCellSize(Vec2F(80, 60));
    EXPECT_EQ(g->GetCellSize(), Vec2F(80, 60));
}

TEST(GridLayout, SetSpacingRoundTrip)
{
    SceneCleanGuard guard;
    auto g = mmake<GridLayout>();
    g->SetSpacing(3.5f);
    EXPECT_FLOAT_EQ(g->GetSpacing(), 3.5f);
}

TEST(GridLayout, SetBorderRoundTrip)
{
    SceneCleanGuard guard;
    auto g = mmake<GridLayout>();
    g->SetBorder(RectF(1, 2, 3, 4));
    EXPECT_EQ(g->GetBorder(), RectF(1, 2, 3, 4));
}

TEST(GridLayout, SetBaseCornerRoundTrip)
{
    SceneCleanGuard guard;
    auto g = mmake<GridLayout>();
    g->SetBaseCorner(BaseCorner::RightTop);
    EXPECT_EQ(g->GetBaseCorner(), BaseCorner::RightTop);
}

TEST(GridLayout, SetArrangeAxisRoundTrip)
{
    SceneCleanGuard guard;
    auto g = mmake<GridLayout>();
    g->SetArrangeAxis(TwoDirection::Vertical);
    EXPECT_EQ(g->GetArrangeAxis(), TwoDirection::Vertical);
    g->SetArrangeAxis(TwoDirection::Horizontal);
    EXPECT_EQ(g->GetArrangeAxis(), TwoDirection::Horizontal);
}

TEST(GridLayout, SetArrangeAxisMaxCellsRoundTrip)
{
    SceneCleanGuard guard;
    auto g = mmake<GridLayout>();
    g->SetArrangeAxisMaxCells(7);
    EXPECT_EQ(g->GetArrangeAxisMaxCells(), 7);
}

TEST(GridLayout, SetFitByChildrenRoundTrip)
{
    SceneCleanGuard guard;
    auto g = mmake<GridLayout>();
    g->SetFitByChildren(true);
    EXPECT_TRUE(g->IsFittingByChildren());
}

TEST(GridLayout, SetBorderEdges)
{
    SceneCleanGuard guard;
    auto g = mmake<GridLayout>();
    g->SetBorderLeft(5);
    g->SetBorderRight(6);
    g->SetBorderTop(7);
    g->SetBorderBottom(8);
    EXPECT_FLOAT_EQ(g->GetBorderLeft(), 5);
    EXPECT_FLOAT_EQ(g->GetBorderRight(), 6);
    EXPECT_FLOAT_EQ(g->GetBorderTop(), 7);
    EXPECT_FLOAT_EQ(g->GetBorderBottom(), 8);
}
