#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/CustomDropDown.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    Ref<CustomDropDown> MakeDropDownWithSample()
    {
        auto dd = mmake<CustomDropDown>();
        auto sample = mmake<Widget>();
        dd->SetItemSample(sample);
        return dd;
    }
}

// ===== Construction =====

TEST(CustomDropDown, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto dd = mmake<CustomDropDown>();
    ASSERT_TRUE(dd);
    EXPECT_EQ(dd->GetItemsCount(), 0);
}

// ===== Items =====

TEST(CustomDropDown, AddItemIncrementsCount)
{
    SceneCleanGuard guard;
    auto dd = MakeDropDownWithSample();
    auto a = dd->AddItem();
    auto b = dd->AddItem();
    EXPECT_EQ(dd->GetItemsCount(), 2);
    EXPECT_TRUE(a);
    EXPECT_TRUE(b);
}

TEST(CustomDropDown, GetItemReturnsAddedWidget)
{
    SceneCleanGuard guard;
    auto dd = MakeDropDownWithSample();
    auto a = dd->AddItem();
    EXPECT_EQ(dd->GetItem(0), a);
}

TEST(CustomDropDown, RemoveItemByRefDecrementsCount)
{
    SceneCleanGuard guard;
    auto dd = MakeDropDownWithSample();
    auto a = dd->AddItem();
    dd->AddItem();
    dd->RemoveItem(a);
    EXPECT_EQ(dd->GetItemsCount(), 1);
}

TEST(CustomDropDown, RemoveAllItemsClears)
{
    SceneCleanGuard guard;
    auto dd = MakeDropDownWithSample();
    dd->AddItem();
    dd->AddItem();
    dd->RemoveAllItems();
    EXPECT_EQ(dd->GetItemsCount(), 0);
}

TEST(CustomDropDown, MoveItemSwapsPositions)
{
    SceneCleanGuard guard;
    auto dd = MakeDropDownWithSample();
    auto a = dd->AddItem();
    dd->AddItem();
    dd->MoveItem(0, 1);
    EXPECT_EQ(dd->GetItem(1), a);
}

// ===== Expand / Collapse =====

TEST(CustomDropDown, ExpandSetsIsExpanded)
{
    SceneCleanGuard guard;
    auto dd = MakeDropDownWithSample();
    dd->Expand();
    EXPECT_TRUE(dd->IsExpanded());
    dd->Collapse();
    EXPECT_FALSE(dd->IsExpanded());
}

// ===== Selection =====

TEST(CustomDropDown, SelectItemAtSetsSelectedItemPosition)
{
    SceneCleanGuard guard;
    auto dd = MakeDropDownWithSample();
    dd->AddItem();
    dd->AddItem();
    dd->AddItem();
    dd->SelectItemAt(2);
    EXPECT_EQ(dd->GetSelectedItemPosition(), 2);
}

// ===== Clipping =====

TEST(CustomDropDown, SetClippingLayoutRoundTrip)
{
    SceneCleanGuard guard;
    auto dd = mmake<CustomDropDown>();
    auto layout = Layout::Based(BaseCorner::Center, Vec2F(50, 50));
    dd->SetClippingLayout(layout);
    auto retrieved = dd->GetClippingLayout();
    EXPECT_EQ(retrieved.anchorMin, layout.anchorMin);
}
