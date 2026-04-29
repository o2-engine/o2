#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    Ref<DropDown> MakeDropDownWithLabelSample()
    {
        auto dd = mmake<DropDown>();
        auto sample = mmake<Label>();
        dd->SetItemSample(sample);
        return dd;
    }
}

// ===== Construction =====

TEST(DropDown, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto dd = mmake<DropDown>();
    ASSERT_TRUE(dd);
    EXPECT_EQ(dd->GetItemsCount(), 0);
}

// ===== Add / count =====

TEST(DropDown, AddItemReturnsPosition)
{
    SceneCleanGuard guard;
    auto dd = MakeDropDownWithLabelSample();
    int p0 = dd->AddItem("a");
    int p1 = dd->AddItem("b");
    EXPECT_EQ(p0, 0);
    EXPECT_EQ(p1, 1);
    EXPECT_EQ(dd->GetItemsCount(), 2);
}

TEST(DropDown, AddItemsAddsAll)
{
    SceneCleanGuard guard;
    auto dd = MakeDropDownWithLabelSample();
    Vector<WString> items;
    items.Add("a");
    items.Add("b");
    items.Add("c");
    dd->AddItems(items);
    EXPECT_EQ(dd->GetItemsCount(), 3);
}

TEST(DropDown, FindItemReturnsIndexOrMinusOne)
{
    SceneCleanGuard guard;
    auto dd = MakeDropDownWithLabelSample();
    dd->AddItem("alpha");
    dd->AddItem("beta");
    EXPECT_EQ(dd->FindItem("alpha"), 0);
    EXPECT_EQ(dd->FindItem("missing"), -1);
}

// ===== Remove =====

TEST(DropDown, RemoveItemByText)
{
    SceneCleanGuard guard;
    auto dd = MakeDropDownWithLabelSample();
    dd->AddItem("a");
    dd->AddItem("b");
    dd->RemoveItem(WString("a"));
    EXPECT_EQ(dd->FindItem("a"), -1);
}

// ===== Selection =====

TEST(DropDown, SelectItemTextSetsSelected)
{
    SceneCleanGuard guard;
    auto dd = MakeDropDownWithLabelSample();
    dd->AddItem("a");
    dd->AddItem("b");
    dd->AddItem("c");
    dd->SelectItemText("c");
    EXPECT_EQ(dd->GetSelectedItemPosition(), 2);
}

// ===== All items text =====

TEST(DropDown, GetAllItemsTextReturnsAddedItems)
{
    SceneCleanGuard guard;
    auto dd = MakeDropDownWithLabelSample();
    dd->AddItem("a");
    dd->AddItem("b");
    auto all = dd->GetAllItemsText();
    EXPECT_EQ(all.Count(), 2);
}
