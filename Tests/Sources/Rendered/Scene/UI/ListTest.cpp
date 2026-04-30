#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/List.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    Ref<List> MakeListWithLabelSample()
    {
        auto list = mmake<List>();
        auto sample = mmake<Label>();
        list->SetItemSample(sample);
        return list;
    }
}

// ===== Construction =====

TEST(List, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto list = mmake<List>();
    ASSERT_TRUE(list);
    EXPECT_EQ(list->GetItemsCount(), 0);
}

// ===== Add / count =====

TEST(List, AddItemReturnsPositionAndIncrementsCount)
{
    SceneCleanGuard guard;
    auto list = MakeListWithLabelSample();
    int p0 = list->AddItem("alpha");
    int p1 = list->AddItem("beta");
    EXPECT_EQ(p0, 0);
    EXPECT_EQ(p1, 1);
    EXPECT_EQ(list->GetItemsCount(), 2);
}

TEST(List, AddItemsAddsAll)
{
    SceneCleanGuard guard;
    auto list = MakeListWithLabelSample();
    Vector<WString> items;
    items.Add("a");
    items.Add("b");
    items.Add("c");
    list->AddItems(items);
    EXPECT_EQ(list->GetItemsCount(), 3);
}

TEST(List, FindItemReturnsIndexOrMinusOne)
{
    SceneCleanGuard guard;
    auto list = MakeListWithLabelSample();
    list->AddItem("alpha");
    list->AddItem("beta");
    EXPECT_EQ(list->FindItem("alpha"), 0);
    EXPECT_EQ(list->FindItem("beta"), 1);
    EXPECT_EQ(list->FindItem("gamma"), -1);
}

// ===== Remove =====

TEST(List, RemoveItemByText)
{
    SceneCleanGuard guard;
    auto list = MakeListWithLabelSample();
    list->AddItem("a");
    list->AddItem("b");
    list->RemoveItem(WString("a"));
    EXPECT_EQ(list->FindItem("a"), -1);
    EXPECT_EQ(list->GetItemsCount(), 1);
}

// ===== Selection =====

TEST(List, SelectItemTextSetsSelected)
{
    SceneCleanGuard guard;
    auto list = MakeListWithLabelSample();
    list->AddItem("a");
    list->AddItem("b");
    list->AddItem("c");
    list->SelectItemText("b");
    EXPECT_EQ(list->GetSelectedItemPos(), 1);
}

TEST(List, SelectItemAtFiresOnSelectedPosAndOnSelectedItem)
{
    SceneCleanGuard guard;
    auto list = MakeListWithLabelSample();
    list->AddItem("alpha");
    list->AddItem("beta");

    int posCalls = 0;
    int itemCalls = 0;
    int observedPos = -1;
    list->onSelectedPos = [&](int p) { posCalls++; observedPos = p; };
    list->onSelectedItem = [&](const Ref<Widget>&) { itemCalls++; };

    list->SelectItemAt(1);
    EXPECT_EQ(posCalls, 1);
    EXPECT_EQ(itemCalls, 1);
    EXPECT_EQ(observedPos, 1);
}

TEST(List, SelectItemAtOutOfRangeDoesNotFireCallbacks)
{
    SceneCleanGuard guard;
    auto list = MakeListWithLabelSample();
    list->AddItem("alpha");

    int posCalls = 0;
    list->onSelectedPos = [&](int) { posCalls++; };
    list->SelectItemAt(99);
    EXPECT_EQ(posCalls, 0);
}

TEST(List, MultiSelectionAllowsTwoSelectedItems)
{
    SceneCleanGuard guard;
    auto list = MakeListWithLabelSample();
    list->SetMultiselectionAvailable(true);
    list->AddItem("a");
    list->AddItem("b");
    list->AddItem("c");

    list->SelectItemAt(0);
    list->SelectItemAt(2);
    EXPECT_EQ(list->GetSelectedItems().Count(), 2);
}

TEST(List, SingleSelectionReplacesPreviousSelection)
{
    SceneCleanGuard guard;
    auto list = MakeListWithLabelSample();
    list->SetMultiselectionAvailable(false);
    list->AddItem("a");
    list->AddItem("b");

    list->SelectItemAt(0);
    list->SelectItemAt(1);
    EXPECT_EQ(list->GetSelectedItems().Count(), 1);
    EXPECT_EQ(list->GetSelectedItemPos(), 1);
}

// ===== Item text =====

TEST(List, GetAllItemsTextReturnsAddedItems)
{
    SceneCleanGuard guard;
    auto list = MakeListWithLabelSample();
    list->AddItem("a");
    list->AddItem("b");
    auto all = list->GetAllItemsText();
    EXPECT_EQ(all.Count(), 2);
}
