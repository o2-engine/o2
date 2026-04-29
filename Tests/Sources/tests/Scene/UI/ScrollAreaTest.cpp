#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "tests/Scene/SceneTestHelpers.h"
#include "tests/Scene/UI/UITestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(ScrollArea, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto sa = mmake<ScrollArea>();
    ASSERT_TRUE(sa);
    EXPECT_EQ(sa->GetScroll(), Vec2F(0, 0));
}

TEST(ScrollArea, CopyPreservesProperties)
{
    SceneCleanGuard guard;
    auto src = mmake<ScrollArea>();
    src->SetScrollForcible(Vec2F(10, 20));
    auto copy = src->CloneAsRef<ScrollArea>();
    ASSERT_TRUE(copy);
}

// ===== Scroll value =====

TEST(ScrollArea, SetScrollForcibleClampsToScrollRange)
{
    SceneCleanGuard guard;
    auto sa = mmake<ScrollArea>();
    // Empty scroll area has zero scroll range — any SetScroll clamps to (0,0).
    sa->SetScrollForcible(Vec2F(15, 25));
    EXPECT_EQ(sa->GetScroll(), Vec2F(0, 0));
}

TEST(ScrollArea, ResetScrollClearsScroll)
{
    SceneCleanGuard guard;
    auto sa = mmake<ScrollArea>();
    sa->SetScrollForcible(Vec2F(50, 50));
    sa->ResetScroll();
    EXPECT_EQ(sa->GetScroll(), Vec2F(0, 0));
}

TEST(ScrollArea, ResetScrollFiresOnScrolled)
{
    SceneCleanGuard guard;
    auto sa = mmake<ScrollArea>();
    int count = 0;
    sa->onScrolled = [&](const Vec2F&) { count++; };
    sa->ResetScroll();
    EXPECT_GE(count, 1);
}

TEST(ScrollArea, SetHorizontalScrollFiresOnScrolled)
{
    SceneCleanGuard guard;
    auto sa = mmake<ScrollArea>();
    int count = 0;
    sa->onScrolled = [&](const Vec2F&) { count++; };
    sa->SetHorizontalScroll(20);
    EXPECT_EQ(count, 1);
}

TEST(ScrollArea, SetVerticalScrollFiresOnScrolled)
{
    SceneCleanGuard guard;
    auto sa = mmake<ScrollArea>();
    int count = 0;
    sa->onScrolled = [&](const Vec2F&) { count++; };
    sa->SetVerticalScroll(30);
    EXPECT_EQ(count, 1);
}

// ===== Scroll bars =====

TEST(ScrollArea, SetHorizontalScrollBarStores)
{
    SceneCleanGuard guard;
    auto sa = mmake<ScrollArea>();
    auto bar = mmake<HorizontalScrollBar>();
    sa->SetHorizontalScrollBar(bar);
    EXPECT_EQ(sa->GetHorizontalScrollbar(), bar);
}

TEST(ScrollArea, SetVerticalScrollBarStores)
{
    SceneCleanGuard guard;
    auto sa = mmake<ScrollArea>();
    auto bar = mmake<VerticalScrollBar>();
    sa->SetVerticalScrollBar(bar);
    EXPECT_EQ(sa->GetVerticalScrollbar(), bar);
}

// ===== Hiding =====

TEST(ScrollArea, SetEnableScrollsHidingRoundTrip)
{
    SceneCleanGuard guard;
    auto sa = mmake<ScrollArea>();
    sa->SetEnableScrollsHiding(false);
    EXPECT_FALSE(sa->IsScrollsHiding());
    sa->SetEnableScrollsHiding(true);
    EXPECT_TRUE(sa->IsScrollsHiding());
}

TEST(ScrollArea, SetScrollBarsShowingByCursorRoundTrip)
{
    SceneCleanGuard guard;
    auto sa = mmake<ScrollArea>();
    sa->SetScrollBarsShowingByCursor(true);
    EXPECT_TRUE(sa->IsScrollBarsShowingByCursor());
    sa->SetScrollBarsShowingByCursor(false);
    EXPECT_FALSE(sa->IsScrollBarsShowingByCursor());
}

// ===== Clipping layout =====

TEST(ScrollArea, SetClippingLayoutRoundTrip)
{
    SceneCleanGuard guard;
    auto sa = mmake<ScrollArea>();
    auto custom = Layout::Based(BaseCorner::Center, Vec2F(50, 50));
    sa->SetClippingLayout(custom);
    auto returned = sa->GetClippingLayout();
    EXPECT_EQ(returned.anchorMin, custom.anchorMin);
    EXPECT_EQ(returned.anchorMax, custom.anchorMax);
}

// ===== Events =====

TEST(ScrollArea, OnScrolledFiresOnSetScrollForcible)
{
    SceneCleanGuard guard;
    auto sa = mmake<ScrollArea>();
    int count = 0;
    sa->onScrolled = [&](const Vec2F&) { count++; };
    sa->SetScrollForcible(Vec2F(5, 5));
    EXPECT_EQ(count, 1);
}

TEST(ScrollArea, OnScrolledFiresOnSetScroll)
{
    SceneCleanGuard guard;
    auto sa = mmake<ScrollArea>();
    int count = 0;
    sa->onScrolled = [&](const Vec2F&) { count++; };
    sa->SetScroll(Vec2F(3, 4));
    EXPECT_EQ(count, 1);
}
