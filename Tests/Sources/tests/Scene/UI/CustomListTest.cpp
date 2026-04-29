#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/CustomList.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    Ref<CustomList> MakeListWithSample()
    {
        auto list = mmake<CustomList>();
        auto sample = mmake<Widget>();
        list->SetItemSample(sample);
        return list;
    }
}

// ===== Construction =====

TEST(CustomList, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto list = mmake<CustomList>();
    ASSERT_TRUE(list);
    EXPECT_EQ(list->GetItemsCount(), 0);
}

// ===== Item sample =====

TEST(CustomList, SetItemSampleStores)
{
    SceneCleanGuard guard;
    auto list = mmake<CustomList>();
    auto sample = mmake<Widget>();
    list->SetItemSample(sample);
    EXPECT_TRUE(list->GetItemSample());
}

// ===== AddItem / GetItemsCount =====

TEST(CustomList, AddItemIncrementsCount)
{
    SceneCleanGuard guard;
    auto list = MakeListWithSample();
    auto a = list->AddItem();
    auto b = list->AddItem();
    EXPECT_EQ(list->GetItemsCount(), 2);
    EXPECT_TRUE(a);
    EXPECT_TRUE(b);
}

TEST(CustomList, GetItemReturnsAddedWidget)
{
    SceneCleanGuard guard;
    auto list = MakeListWithSample();
    auto a = list->AddItem();
    EXPECT_EQ(list->GetItem(0), a);
}

TEST(CustomList, GetItemPositionReturnsIndex)
{
    SceneCleanGuard guard;
    auto list = MakeListWithSample();
    auto a = list->AddItem();
    auto b = list->AddItem();
    EXPECT_EQ(list->GetItemPosition(a), 0);
    EXPECT_EQ(list->GetItemPosition(b), 1);
}

// ===== RemoveItem =====

TEST(CustomList, RemoveItemDecrementsCount)
{
    SceneCleanGuard guard;
    auto list = MakeListWithSample();
    auto a = list->AddItem();
    list->AddItem();
    list->RemoveItem(a);
    EXPECT_EQ(list->GetItemsCount(), 1);
}

TEST(CustomList, RemoveItemAtIndex)
{
    SceneCleanGuard guard;
    auto list = MakeListWithSample();
    list->AddItem();
    list->AddItem();
    list->RemoveItem(0);
    EXPECT_EQ(list->GetItemsCount(), 1);
}

TEST(CustomList, RemoveAllItemsClears)
{
    SceneCleanGuard guard;
    auto list = MakeListWithSample();
    list->AddItem();
    list->AddItem();
    list->RemoveAllItems();
    EXPECT_EQ(list->GetItemsCount(), 0);
}

// ===== Move =====

TEST(CustomList, MoveItemSwapsPositions)
{
    SceneCleanGuard guard;
    auto list = MakeListWithSample();
    auto a = list->AddItem();
    auto b = list->AddItem();
    auto c = list->AddItem();
    list->MoveItem(0, 2);
    EXPECT_EQ(list->GetItem(2), a);
}

// ===== Selection =====

TEST(CustomList, SelectItemAtSetsSelectedItemPos)
{
    SceneCleanGuard guard;
    auto list = MakeListWithSample();
    list->AddItem();
    list->AddItem();
    list->AddItem();
    list->SelectItemAt(1);
    EXPECT_EQ(list->GetSelectedItemPos(), 1);
}

TEST(CustomList, ClearSelectionResetsSelection)
{
    SceneCleanGuard guard;
    auto list = MakeListWithSample();
    list->AddItem();
    list->AddItem();
    list->SelectItemAt(0);
    list->ClearSelection();
    EXPECT_LE(list->GetSelectedItems().Count(), 0);
}

// ===== Multi selection =====

TEST(CustomList, SetMultiselectionAvailableRoundTrip)
{
    SceneCleanGuard guard;
    auto list = mmake<CustomList>();
    list->SetMultiselectionAvailable(false);
    EXPECT_FALSE(list->IsMultiselectionAvailable());
    list->SetMultiselectionAvailable(true);
    EXPECT_TRUE(list->IsMultiselectionAvailable());
}

// ===== Selection / hover layouts =====

TEST(CustomList, SetSelectionDrawableLayoutRoundTrip)
{
    SceneCleanGuard guard;
    auto list = mmake<CustomList>();
    auto layout = Layout::Based(BaseCorner::Center, Vec2F(50, 50));
    list->SetSelectionDrawableLayout(layout);
    auto retrieved = list->GetSelectionDrawableLayout();
    EXPECT_EQ(retrieved.anchorMin, layout.anchorMin);
}

TEST(CustomList, SetHoverDrawableLayoutRoundTrip)
{
    SceneCleanGuard guard;
    auto list = mmake<CustomList>();
    auto layout = Layout::Based(BaseCorner::Center, Vec2F(50, 50));
    list->SetHoverDrawableLayout(layout);
    auto retrieved = list->GetHoverDrawableLayout();
    EXPECT_EQ(retrieved.anchorMin, layout.anchorMin);
}
