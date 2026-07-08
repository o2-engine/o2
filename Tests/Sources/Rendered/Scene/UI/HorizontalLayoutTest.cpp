#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "Scene/SceneTestHelpers.h"
#include "Scene/UI/UITestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(HorizontalLayout, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto layout = mmake<HorizontalLayout>();
    ASSERT_TRUE(layout);
    EXPECT_TRUE(layout->IsWidthExpand());
    EXPECT_TRUE(layout->IsHeightExpand());
    EXPECT_FALSE(layout->IsFittingByChildren());
}

TEST(HorizontalLayout, CopyPreservesSettings)
{
    SceneCleanGuard guard;
    auto src = mmake<HorizontalLayout>();
    src->SetSpacing(10);
    src->SetBaseCorner(BaseCorner::Right);
    src->SetWidthExpand(false);
    auto copy = src->CloneAsRef<HorizontalLayout>();
    EXPECT_FLOAT_EQ(copy->GetSpacing(), 10);
    EXPECT_EQ(copy->GetBaseCorner(), BaseCorner::Right);
    EXPECT_FALSE(copy->IsWidthExpand());
}

// ===== Properties =====

TEST(HorizontalLayout, SetBaseCornerRoundTrip)
{
    SceneCleanGuard guard;
    auto layout = mmake<HorizontalLayout>();
    layout->SetBaseCorner(BaseCorner::Right);
    EXPECT_EQ(layout->GetBaseCorner(), BaseCorner::Right);
}

TEST(HorizontalLayout, SetSpacingRoundTrip)
{
    SceneCleanGuard guard;
    auto layout = mmake<HorizontalLayout>();
    layout->SetSpacing(15.5f);
    EXPECT_FLOAT_EQ(layout->GetSpacing(), 15.5f);
}

TEST(HorizontalLayout, SetBorderRoundTrip)
{
    SceneCleanGuard guard;
    auto layout = mmake<HorizontalLayout>();
    layout->SetBorder(BorderF(1, 2, 3, 4));
    EXPECT_EQ(layout->GetBorder(), BorderF(1, 2, 3, 4));
}

TEST(HorizontalLayout, SetBorderEdges)
{
    SceneCleanGuard guard;
    auto layout = mmake<HorizontalLayout>();
    layout->SetBorderLeft(5);
    layout->SetBorderRight(6);
    layout->SetBorderTop(7);
    layout->SetBorderBottom(8);
    EXPECT_FLOAT_EQ(layout->GetBorderLeft(), 5);
    EXPECT_FLOAT_EQ(layout->GetBorderRight(), 6);
    EXPECT_FLOAT_EQ(layout->GetBorderTop(), 7);
    EXPECT_FLOAT_EQ(layout->GetBorderBottom(), 8);
}

TEST(HorizontalLayout, SetExpandFlagsRoundTrip)
{
    SceneCleanGuard guard;
    auto layout = mmake<HorizontalLayout>();
    layout->SetWidthExpand(false);
    EXPECT_FALSE(layout->IsWidthExpand());
    layout->SetHeightExpand(false);
    EXPECT_FALSE(layout->IsHeightExpand());
}

TEST(HorizontalLayout, SetFitByChildrenRoundTrip)
{
    SceneCleanGuard guard;
    auto layout = mmake<HorizontalLayout>();
    layout->SetFitByChildren(true);
    EXPECT_TRUE(layout->IsFittingByChildren());
}

// ===== Arrangement =====

TEST(HorizontalLayout, ChildrenLayoutLeftToRightHaveIncreasingX)
{
    SceneCleanGuard guard;
    auto layout = mmake<HorizontalLayout>();
    layout->layout->SetSize2D(Vec2F(300, 100));
    layout->SetBaseCorner(BaseCorner::Left);
    layout->SetWidthExpand(false);
    layout->SetSpacing(5);

    auto a = MakeChildWidget(layout, "a");
    a->layout->SetMinimalSize(Vec2F(50, 30));
    auto b = MakeChildWidget(layout, "b");
    b->layout->SetMinimalSize(Vec2F(50, 30));
    auto c = MakeChildWidget(layout, "c");
    c->layout->SetMinimalSize(Vec2F(50, 30));

    TickAndUpdateLayout(2);

    auto posA = a->layout->GetPosition2D();
    auto posB = b->layout->GetPosition2D();
    auto posC = c->layout->GetPosition2D();
    EXPECT_LE(posA.x, posB.x);
    EXPECT_LE(posB.x, posC.x);
}

TEST(HorizontalLayout, ExpandedChildrenWidthIsProportionalToWeight)
{
    SceneCleanGuard guard;
    auto layout = mmake<HorizontalLayout>();
    layout->layout->SetSize2D(Vec2F(600, 100));
    layout->SetSpacing(0);
    layout->SetBorder(BorderF(0, 0, 0, 0));
    layout->SetWidthExpand(true);

    auto a = MakeChildWidget(layout, "a");
    auto b = MakeChildWidget(layout, "b");
    auto c = MakeChildWidget(layout, "c");
    a->layout->SetWidthWeight(1.0f);
    b->layout->SetWidthWeight(2.0f);
    c->layout->SetWidthWeight(3.0f);

    TickAndUpdateLayout(2);

    // Total weight = 6, total width = 600 → 100/200/300.
    EXPECT_NEAR(a->layout->GetWidth(), 100.0f, 1.0f);
    EXPECT_NEAR(b->layout->GetWidth(), 200.0f, 1.0f);
    EXPECT_NEAR(c->layout->GetWidth(), 300.0f, 1.0f);
}
