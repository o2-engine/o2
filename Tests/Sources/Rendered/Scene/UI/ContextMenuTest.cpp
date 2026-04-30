#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// ===== ContextMenu::Item: Construction =====

TEST(ContextMenuItem, DefaultConstructionIsValid)
{
    auto item = mmake<ContextMenu::Item>();
    ASSERT_TRUE(item);
    EXPECT_FALSE(item->checkable);
    EXPECT_FALSE(item->checked);
}

TEST(ContextMenuItem, ConstructorWithTextStores)
{
    Vector<Ref<ContextMenu::Item>> subs;
    auto item = mmake<ContextMenu::Item>("hello", subs);
    EXPECT_EQ(item->text, WString("hello"));
}

TEST(ContextMenuItem, ConstructorWithClickFunctionStoresClick)
{
    int count = 0;
    Function<void()> click = [&]() { count++; };
    auto item = mmake<ContextMenu::Item>("hello", click);
    item->onClick();
    EXPECT_EQ(count, 1);
}

TEST(ContextMenuItem, ConstructorWithCheckedFunctionSetsCheckable)
{
    auto item = mmake<ContextMenu::Item>("toggle", true);
    EXPECT_TRUE(item->checkable);
    EXPECT_TRUE(item->checked);
}

// ===== ContextMenu::Item: Operators =====

TEST(ContextMenuItem, EqualsOperatorChecksContent)
{
    Vector<Ref<ContextMenu::Item>> subs;
    auto a = mmake<ContextMenu::Item>("a", subs);
    auto b = mmake<ContextMenu::Item>("a", subs);
    auto c = mmake<ContextMenu::Item>("c", subs);
    EXPECT_TRUE(*a == *b);
    EXPECT_FALSE(*a == *c);
}

// ===== ContextMenu::Item: Static =====

TEST(ContextMenuItem, SeparatorReturnsItem)
{
    auto sep = ContextMenu::Item::Separator();
    ASSERT_TRUE(sep);
}

// ===== ContextMenu: Construction =====

TEST(ContextMenu, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto cm = mmake<ContextMenu>();
    ASSERT_TRUE(cm);
    EXPECT_EQ(cm->GetItems().Count(), 0);
}

TEST(ContextMenu, ConstructorFromItemsAddsThem)
{
    SceneCleanGuard guard;
    Vector<Ref<ContextMenu::Item>> items;
    Vector<Ref<ContextMenu::Item>> subs;
    items.Add(mmake<ContextMenu::Item>("a", subs));
    items.Add(mmake<ContextMenu::Item>("b", subs));
    auto cm = mmake<ContextMenu>(items);
    EXPECT_EQ(cm->GetItems().Count(), 2);
}

// ===== ContextMenu: Items =====

TEST(ContextMenu, AddItemByPathReturnsItem)
{
    SceneCleanGuard guard;
    auto cm = mmake<ContextMenu>();
    auto item = cm->AddItem("File");
    ASSERT_TRUE(item);
    EXPECT_GT(cm->GetItems().Count(), 0);
}

TEST(ContextMenu, AddItemByRefAddsToItems)
{
    SceneCleanGuard guard;
    auto cm = mmake<ContextMenu>();
    Vector<Ref<ContextMenu::Item>> subs;
    auto item = mmake<ContextMenu::Item>("X", subs);
    cm->AddItem(item);
    EXPECT_GT(cm->GetItems().Count(), 0);
}

TEST(ContextMenu, RemoveAllItemsClears)
{
    SceneCleanGuard guard;
    auto cm = mmake<ContextMenu>();
    cm->AddItem("a");
    cm->AddItem("b");
    cm->RemoveAllItems();
    EXPECT_EQ(cm->GetItems().Count(), 0);
}

TEST(ContextMenu, AddToggleItemReturnsItem)
{
    SceneCleanGuard guard;
    auto cm = mmake<ContextMenu>();
    auto item = cm->AddToggleItem("flag", true);
    ASSERT_TRUE(item);
    EXPECT_TRUE(item->checkable);
}

// ===== ContextMenu: Search =====

TEST(ContextMenu, SetSearchEnabledRoundTrip)
{
    SceneCleanGuard guard;
    auto cm = mmake<ContextMenu>();
    cm->SetSearchEnabled(true);
    EXPECT_TRUE(cm->IsSearchEnabled());
    cm->SetSearchEnabled(false);
    EXPECT_FALSE(cm->IsSearchEnabled());
}
