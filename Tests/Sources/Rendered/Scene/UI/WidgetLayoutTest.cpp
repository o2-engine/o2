#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "Scene/SceneTestHelpers.h"
#include "Scene/UI/UITestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(WidgetLayout, DefaultConstructionHasZeroAnchors)
{
    WidgetLayout l;
    EXPECT_EQ(l.GetAnchorMin(), Vec2F(0, 0));
    EXPECT_EQ(l.GetAnchorMax(), Vec2F(0, 0));
}

TEST(WidgetLayout, ConstructorWithAnchorsAndOffsetsStores)
{
    WidgetLayout l(Vec2F(0.1f, 0.2f), Vec2F(0.7f, 0.8f),
                   Vec2F(1, 2), Vec2F(3, 4));
    EXPECT_EQ(l.GetAnchorMin(), Vec2F(0.1f, 0.2f));
    EXPECT_EQ(l.GetAnchorMax(), Vec2F(0.7f, 0.8f));
    EXPECT_EQ(l.GetOffsetMin(), Vec2F(1, 2));
    EXPECT_EQ(l.GetOffsetMax(), Vec2F(3, 4));
}

TEST(WidgetLayout, CopyConstructorPreservesAnchorsAndOffsets)
{
    WidgetLayout src(Vec2F(0.1f, 0.2f), Vec2F(0.7f, 0.8f),
                     Vec2F(1, 2), Vec2F(3, 4));
    WidgetLayout copy(src);
    EXPECT_TRUE(copy == src);
}

TEST(WidgetLayout, EqualsOperatorChecksAllFields)
{
    WidgetLayout a(Vec2F(0, 0), Vec2F(1, 1), Vec2F(0, 0), Vec2F(0, 0));
    WidgetLayout b(Vec2F(0, 0), Vec2F(1, 1), Vec2F(0, 0), Vec2F(0, 0));
    WidgetLayout c(Vec2F(0, 0), Vec2F(1, 1), Vec2F(0, 0), Vec2F(1, 1));
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

// ===== Anchors =====

TEST(WidgetLayout, SetAnchorMinRoundTrip)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->layout->SetAnchorMin(Vec2F(0.25f, 0.5f));
    EXPECT_EQ(w->layout->GetAnchorMin(), Vec2F(0.25f, 0.5f));
}

TEST(WidgetLayout, SetAnchorMaxRoundTrip)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->layout->SetAnchorMax(Vec2F(0.75f, 1.0f));
    EXPECT_EQ(w->layout->GetAnchorMax(), Vec2F(0.75f, 1.0f));
}

TEST(WidgetLayout, SetAnchorLeftRightTopBottom)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->layout->SetAnchorLeft(0.1f);
    w->layout->SetAnchorRight(0.9f);
    w->layout->SetAnchorBottom(0.2f);
    w->layout->SetAnchorTop(0.8f);
    EXPECT_FLOAT_EQ(w->layout->GetAnchorLeft(), 0.1f);
    EXPECT_FLOAT_EQ(w->layout->GetAnchorRight(), 0.9f);
    EXPECT_FLOAT_EQ(w->layout->GetAnchorBottom(), 0.2f);
    EXPECT_FLOAT_EQ(w->layout->GetAnchorTop(), 0.8f);
}

// ===== Offsets =====

TEST(WidgetLayout, SetOffsetMinMaxRoundTrip)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->layout->SetOffsetMin(Vec2F(5, 10));
    w->layout->SetOffsetMax(Vec2F(15, 20));
    EXPECT_EQ(w->layout->GetOffsetMin(), Vec2F(5, 10));
    EXPECT_EQ(w->layout->GetOffsetMax(), Vec2F(15, 20));
}

TEST(WidgetLayout, SetOffsetLeftRightTopBottom)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->layout->SetOffsetLeft(5);
    w->layout->SetoffsetRight(15);
    w->layout->SetOffsetBottom(7);
    w->layout->SetOffsetTop(17);
    EXPECT_FLOAT_EQ(w->layout->GetOffsetLeft(), 5);
    EXPECT_FLOAT_EQ(w->layout->GetoffsetRight(), 15);
    EXPECT_FLOAT_EQ(w->layout->GetOffsetBottom(), 7);
    EXPECT_FLOAT_EQ(w->layout->GetOffsetTop(), 17);
}

// ===== Size and position =====

TEST(WidgetLayout, SetSizeRoundTrip)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->layout->SetSize2D(Vec2F(123, 45));
    TickFrame();
    EXPECT_NEAR(w->layout->GetSize2D().x, 123, 0.01f);
    EXPECT_NEAR(w->layout->GetSize2D().y, 45, 0.01f);
}

TEST(WidgetLayout, SetWidthHeightRoundTrip)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->layout->SetWidth(50);
    w->layout->SetHeight(70);
    TickFrame();
    EXPECT_NEAR(w->layout->GetWidth(), 50, 0.01f);
    EXPECT_NEAR(w->layout->GetHeight(), 70, 0.01f);
}

// ===== Min/Max =====

TEST(WidgetLayout, SetMinimalSizeRoundTrip)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->layout->SetMinimalSize(Vec2F(50, 60));
    EXPECT_EQ(w->layout->GetMinimalSize(), Vec2F(50, 60));
}

TEST(WidgetLayout, SetMaximalSizeRoundTrip)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->layout->SetMaximalSize(Vec2F(500, 600));
    EXPECT_EQ(w->layout->GetMaximalSize(), Vec2F(500, 600));
}

TEST(WidgetLayout, SetMinimalWidthHeight)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->layout->SetMinimalWidth(33);
    w->layout->SetMinimalHeight(44);
    EXPECT_FLOAT_EQ(w->layout->GetMinWidth(), 33);
    EXPECT_FLOAT_EQ(w->layout->GetMinHeight(), 44);
}

TEST(WidgetLayout, SetMaximalWidthHeight)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->layout->SetMaximalWidth(333);
    w->layout->SetMaximalHeight(444);
    EXPECT_FLOAT_EQ(w->layout->GetMaxWidth(), 333);
    EXPECT_FLOAT_EQ(w->layout->GetMaxHeight(), 444);
}

// ===== Weight =====

TEST(WidgetLayout, SetWeightRoundTrip)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->layout->SetWeight(Vec2F(2, 3));
    EXPECT_EQ(w->layout->GetWeight(), Vec2F(2, 3));
}

TEST(WidgetLayout, SetWidthHeightWeight)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->layout->SetWidthWeight(2.5f);
    w->layout->SetHeightWeight(3.5f);
    EXPECT_FLOAT_EQ(w->layout->GetWidthWeight(), 2.5f);
    EXPECT_FLOAT_EQ(w->layout->GetHeightWeight(), 3.5f);
}

// ===== Static factories =====

TEST(WidgetLayout, BothStretchProducesFullAnchors)
{
    auto l = WidgetLayout::BothStretch();
    EXPECT_EQ(l.GetAnchorMin(), Vec2F(0, 0));
    EXPECT_EQ(l.GetAnchorMax(), Vec2F(1, 1));
}

TEST(WidgetLayout, BasedSetsZeroAnchorsAndOffsets)
{
    auto l = WidgetLayout::Based(BaseCorner::Center, Vec2F(100, 50));
    EXPECT_EQ(l.GetAnchorMin(), l.GetAnchorMax());
}

TEST(WidgetLayout, HorStretchHasFullHorizontalAnchors)
{
    auto l = WidgetLayout::HorStretch(VerAlign::Middle, 10, 20, 50);
    EXPECT_FLOAT_EQ(l.GetAnchorLeft(), 0);
    EXPECT_FLOAT_EQ(l.GetAnchorRight(), 1);
}

TEST(WidgetLayout, VerStretchHasFullVerticalAnchors)
{
    auto l = WidgetLayout::VerStretch(HorAlign::Middle, 10, 20, 50);
    EXPECT_FLOAT_EQ(l.GetAnchorBottom(), 0);
    EXPECT_FLOAT_EQ(l.GetAnchorTop(), 1);
}

// ===== CopyFrom =====

TEST(WidgetLayout, CopyFromCopiesAnchorsAndOffsets)
{
    SceneCleanGuard guard;
    auto wA = mmake<Widget>(ActorCreateMode::InScene);
    auto wB = mmake<Widget>(ActorCreateMode::InScene);
    wA->layout->SetAnchorMin(Vec2F(0.1f, 0.2f));
    wA->layout->SetAnchorMax(Vec2F(0.7f, 0.8f));
    wA->layout->SetOffsetMin(Vec2F(1, 2));
    wA->layout->SetOffsetMax(Vec2F(3, 4));

    wB->layout->CopyFrom(*wA->layout);
    EXPECT_EQ(wB->layout->GetAnchorMin(), wA->layout->GetAnchorMin());
    EXPECT_EQ(wB->layout->GetAnchorMax(), wA->layout->GetAnchorMax());
    EXPECT_EQ(wB->layout->GetOffsetMin(), wA->layout->GetOffsetMin());
    EXPECT_EQ(wB->layout->GetOffsetMax(), wA->layout->GetOffsetMax());
}

// ===== Integration =====

TEST(WidgetLayout, ChildSizeFollowsBothStretchInsideParent)
{
    SceneCleanGuard guard;
    auto parent = mmake<Widget>(ActorCreateMode::InScene);
    parent->layout->SetSize2D(Vec2F(200, 100));

    auto child = mmake<Widget>(ActorCreateMode::InScene);
    parent->AddChildWidget(child);
    child->layout->CopyFrom(WidgetLayout::BothStretch());
    TickAndUpdateLayout();

    EXPECT_NEAR(child->layout->GetWidth(), 200, 0.5f);
    EXPECT_NEAR(child->layout->GetHeight(), 100, 0.5f);
}

TEST(WidgetLayout, AnchorMinHalfPlacesChildAtParentCenter)
{
    SceneCleanGuard guard;
    auto parent = mmake<Widget>(ActorCreateMode::InScene);
    parent->layout->SetSize2D(Vec2F(200, 100));

    auto child = mmake<Widget>(ActorCreateMode::InScene);
    parent->AddChildWidget(child);
    child->layout->SetAnchorMin(Vec2F(0.5f, 0.5f));
    child->layout->SetAnchorMax(Vec2F(0.5f, 0.5f));
    child->layout->SetSize2D(Vec2F(40, 20));
    TickAndUpdateLayout(2);

    EXPECT_NEAR(child->layout->GetWidth(), 40, 0.5f);
    EXPECT_NEAR(child->layout->GetHeight(), 20, 0.5f);
}

TEST(WidgetLayout, OffsetsTranslateChildPosition)
{
    SceneCleanGuard guard;
    auto parent = mmake<Widget>(ActorCreateMode::InScene);
    parent->layout->SetSize2D(Vec2F(200, 100));

    auto child = mmake<Widget>(ActorCreateMode::InScene);
    parent->AddChildWidget(child);
    child->layout->SetAnchorMin(Vec2F(0, 0));
    child->layout->SetAnchorMax(Vec2F(0, 0));
    child->layout->SetOffsetMin(Vec2F(10, 20));
    child->layout->SetOffsetMax(Vec2F(60, 70));
    TickAndUpdateLayout(2);

    EXPECT_NEAR(child->layout->GetWidth(), 50, 0.5f);
    EXPECT_NEAR(child->layout->GetHeight(), 50, 0.5f);
}
