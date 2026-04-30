#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "Scene/SceneTestHelpers.h"
#include "Scene/UI/UITestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(VerticalLayout, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto layout = mmake<VerticalLayout>();
    ASSERT_TRUE(layout);
    EXPECT_TRUE(layout->IsWidthExpand());
    EXPECT_TRUE(layout->IsHeightExpand());
    EXPECT_FALSE(layout->IsFittingByChildren());
}

TEST(VerticalLayout, CopyPreservesSettings)
{
    SceneCleanGuard guard;
    auto src = mmake<VerticalLayout>();
    src->SetSpacing(7);
    src->SetBaseCorner(BaseCorner::Bottom);
    auto copy = src->CloneAsRef<VerticalLayout>();
    EXPECT_FLOAT_EQ(copy->GetSpacing(), 7);
    EXPECT_EQ(copy->GetBaseCorner(), BaseCorner::Bottom);
}

// ===== Properties =====

TEST(VerticalLayout, SetBaseCornerRoundTrip)
{
    SceneCleanGuard guard;
    auto layout = mmake<VerticalLayout>();
    layout->SetBaseCorner(BaseCorner::Bottom);
    EXPECT_EQ(layout->GetBaseCorner(), BaseCorner::Bottom);
}

TEST(VerticalLayout, SetSpacingRoundTrip)
{
    SceneCleanGuard guard;
    auto layout = mmake<VerticalLayout>();
    layout->SetSpacing(11.5f);
    EXPECT_FLOAT_EQ(layout->GetSpacing(), 11.5f);
}

TEST(VerticalLayout, SetBorderRoundTrip)
{
    SceneCleanGuard guard;
    auto layout = mmake<VerticalLayout>();
    layout->SetBorder(BorderF(1, 2, 3, 4));
    EXPECT_EQ(layout->GetBorder(), BorderF(1, 2, 3, 4));
}

TEST(VerticalLayout, SetBorderEdges)
{
    SceneCleanGuard guard;
    auto layout = mmake<VerticalLayout>();
    layout->SetBorderLeft(5);
    layout->SetBorderRight(6);
    layout->SetBorderTop(7);
    layout->SetBorderBottom(8);
    EXPECT_FLOAT_EQ(layout->GetBorderLeft(), 5);
    EXPECT_FLOAT_EQ(layout->GetBorderRight(), 6);
    EXPECT_FLOAT_EQ(layout->GetBorderTop(), 7);
    EXPECT_FLOAT_EQ(layout->GetBorderBottom(), 8);
}

TEST(VerticalLayout, SetExpandFlagsRoundTrip)
{
    SceneCleanGuard guard;
    auto layout = mmake<VerticalLayout>();
    layout->SetWidthExpand(false);
    layout->SetHeightExpand(false);
    EXPECT_FALSE(layout->IsWidthExpand());
    EXPECT_FALSE(layout->IsHeightExpand());
}

TEST(VerticalLayout, SetFitByChildrenRoundTrip)
{
    SceneCleanGuard guard;
    auto layout = mmake<VerticalLayout>();
    layout->SetFitByChildren(true);
    EXPECT_TRUE(layout->IsFittingByChildren());
}

// ===== Arrangement =====

TEST(VerticalLayout, ChildrenLayoutTopToBottomHaveOrderedY)
{
    SceneCleanGuard guard;
    auto layout = mmake<VerticalLayout>();
    layout->layout->SetSize(Vec2F(100, 300));
    layout->SetBaseCorner(BaseCorner::Top);
    layout->SetHeightExpand(false);
    layout->SetSpacing(5);

    auto a = MakeChildWidget(layout, "a");
    a->layout->SetMinimalSize(Vec2F(50, 30));
    auto b = MakeChildWidget(layout, "b");
    b->layout->SetMinimalSize(Vec2F(50, 30));
    auto c = MakeChildWidget(layout, "c");
    c->layout->SetMinimalSize(Vec2F(50, 30));

    TickAndUpdateLayout(2);

    auto posA = a->layout->GetPosition();
    auto posB = b->layout->GetPosition();
    auto posC = c->layout->GetPosition();
    EXPECT_GE(posA.y, posB.y);
    EXPECT_GE(posB.y, posC.y);
}
