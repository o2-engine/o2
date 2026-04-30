#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/EditBoxDropDown.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(EditBoxDropDown, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto ebdd = mmake<EditBoxDropDown>();
    ASSERT_TRUE(ebdd);
    EXPECT_EQ(ebdd->GetItemsCount(), 0);
}

// ===== Text =====

TEST(EditBoxDropDown, SetTextRoundTrip)
{
    SceneCleanGuard guard;
    auto ebdd = mmake<EditBoxDropDown>();
    ebdd->SetText("hello");
    EXPECT_EQ(ebdd->GetText(), WString("hello"));
}

// ===== Items =====

TEST(EditBoxDropDown, AddItemReturnsPosition)
{
    SceneCleanGuard guard;
    auto ebdd = mmake<EditBoxDropDown>();
    int p0 = ebdd->AddItem("a");
    int p1 = ebdd->AddItem("b");
    EXPECT_EQ(p0, 0);
    EXPECT_EQ(p1, 1);
    EXPECT_EQ(ebdd->GetItemsCount(), 2);
}

TEST(EditBoxDropDown, AddItemsAddsAll)
{
    SceneCleanGuard guard;
    auto ebdd = mmake<EditBoxDropDown>();
    Vector<WString> items;
    items.Add("a");
    items.Add("b");
    items.Add("c");
    ebdd->AddItems(items);
    EXPECT_EQ(ebdd->GetItemsCount(), 3);
}

TEST(EditBoxDropDown, FindItemReturnsIndexOrMinusOne)
{
    SceneCleanGuard guard;
    auto ebdd = mmake<EditBoxDropDown>();
    ebdd->AddItem("alpha");
    ebdd->AddItem("beta");
    EXPECT_EQ(ebdd->FindItem("alpha"), 0);
    EXPECT_EQ(ebdd->FindItem("missing"), -1);
}

TEST(EditBoxDropDown, RemoveItemByText)
{
    SceneCleanGuard guard;
    auto ebdd = mmake<EditBoxDropDown>();
    ebdd->AddItem("a");
    ebdd->AddItem("b");
    ebdd->RemoveItem(WString("a"));
    EXPECT_EQ(ebdd->FindItem("a"), -1);
}

TEST(EditBoxDropDown, RemoveAllItemsClears)
{
    SceneCleanGuard guard;
    auto ebdd = mmake<EditBoxDropDown>();
    ebdd->AddItem("a");
    ebdd->AddItem("b");
    ebdd->RemoveAllItems();
    EXPECT_EQ(ebdd->GetItemsCount(), 0);
}

TEST(EditBoxDropDown, GetAllItemsTextReturnsAddedItems)
{
    SceneCleanGuard guard;
    auto ebdd = mmake<EditBoxDropDown>();
    ebdd->AddItem("a");
    ebdd->AddItem("b");
    auto all = ebdd->GetAllItemsText();
    EXPECT_EQ(all.Count(), 2);
}

// ===== Selection =====

TEST(EditBoxDropDown, SelectItemAtSetsSelectedItemPosition)
{
    SceneCleanGuard guard;
    auto ebdd = mmake<EditBoxDropDown>();
    ebdd->AddItem("a");
    ebdd->AddItem("b");
    ebdd->AddItem("c");
    ebdd->SelectItemAt(2);
    EXPECT_EQ(ebdd->GetSelectedItemPosition(), 2);
}

TEST(EditBoxDropDown, SelectItemTextSelectsByText)
{
    SceneCleanGuard guard;
    auto ebdd = mmake<EditBoxDropDown>();
    ebdd->AddItem("a");
    ebdd->AddItem("b");
    ebdd->SelectItemText("b");
    EXPECT_EQ(ebdd->GetSelectedItemPosition(), 1);
}

// ===== Expand =====

TEST(EditBoxDropDown, ExpandSetsIsExpanded)
{
    SceneCleanGuard guard;
    auto ebdd = mmake<EditBoxDropDown>();
    ebdd->AddItem("a");
    ebdd->Expand();
    EXPECT_TRUE(ebdd->IsExpanded());
    ebdd->Collapse();
    EXPECT_FALSE(ebdd->IsExpanded());
}

