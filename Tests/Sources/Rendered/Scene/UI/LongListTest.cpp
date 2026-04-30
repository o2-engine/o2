#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/LongList.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(LongList, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto list = mmake<LongList>();
    ASSERT_TRUE(list);
}

// ===== Item sample =====

TEST(LongList, SetItemSampleStores)
{
    SceneCleanGuard guard;
    auto list = mmake<LongList>();
    auto sample = mmake<Widget>();
    list->SetItemSample(sample);
    EXPECT_TRUE(list->GetItemSample());
}

// ===== Selection =====

TEST(LongList, SelectItemAtStoresPositionWhenInRange)
{
    SceneCleanGuard guard;
    auto list = mmake<LongList>();
    list->getItemsCountFunc = []() { return 10; };
    list->SelectItemAt(3);
    EXPECT_EQ(list->GetSelectedItemPosition(), 3);
}

TEST(LongList, SelectItemAtOutOfRangeStoresMinusOne)
{
    SceneCleanGuard guard;
    auto list = mmake<LongList>();
    list->getItemsCountFunc = []() { return 5; };
    list->SelectItemAt(99);
    EXPECT_EQ(list->GetSelectedItemPosition(), -1);
}

// ===== Drawables =====

TEST(LongList, GetSelectionDrawableExistsByDefault)
{
    SceneCleanGuard guard;
    auto list = mmake<LongList>();
    EXPECT_TRUE(list->GetSelectionDrawable());
}

TEST(LongList, SetSelectionDrawableLayoutRoundTrip)
{
    SceneCleanGuard guard;
    auto list = mmake<LongList>();
    auto layout = Layout::Based(BaseCorner::Center, Vec2F(50, 50));
    list->SetSelectionDrawableLayout(layout);
    auto retrieved = list->GetSelectionDrawableLayout();
    EXPECT_EQ(retrieved.anchorMin, layout.anchorMin);
}

TEST(LongList, SetHoverDrawableLayoutRoundTrip)
{
    SceneCleanGuard guard;
    auto list = mmake<LongList>();
    auto layout = Layout::Based(BaseCorner::Center, Vec2F(50, 50));
    list->SetHoverDrawableLayout(layout);
    auto retrieved = list->GetHoverDrawableLayout();
    EXPECT_EQ(retrieved.anchorMin, layout.anchorMin);
}

// ===== Callbacks =====

