#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/GridLayoutScrollArea.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(GridLayoutScrollArea, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto g = mmake<GridLayoutScrollArea>();
    ASSERT_TRUE(g);
}

// ===== Item sample =====

TEST(GridLayoutScrollArea, SetItemSampleStores)
{
    SceneCleanGuard guard;
    auto g = mmake<GridLayoutScrollArea>();
    auto sample = mmake<Widget>();
    g->SetItemSample(sample);
    EXPECT_TRUE(g->GetItemSample());
}

// ===== Spacing =====

TEST(GridLayoutScrollArea, SetItemsSpacingRoundTrip)
{
    SceneCleanGuard guard;
    auto g = mmake<GridLayoutScrollArea>();
    g->SetItemsSpacing(Vec2F(5, 7));
    EXPECT_EQ(g->GetItemsSpacing(), Vec2F(5, 7));
}

// ===== Callbacks =====

