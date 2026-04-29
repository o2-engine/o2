#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/Widgets/MenuPanel.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

// ===== MenuPanel::Item =====

TEST(MenuPanelItem, DefaultConstructionIsValid)
{
    MenuPanel::Item item;
    EXPECT_TRUE(item.text.IsEmpty());
}

TEST(MenuPanelItem, ConstructorWithTextAndSubItemsStores)
{
    Vector<Ref<ContextMenu::Item>> subs;
    MenuPanel::Item item("File", subs);
    EXPECT_EQ(item.text, WString("File"));
}

TEST(MenuPanelItem, ConstructorWithClickFunctionStoresClick)
{
    int count = 0;
    Function<void()> click = [&]() { count++; };
    MenuPanel::Item item("File", click);
    item.onClick();
    EXPECT_EQ(count, 1);
}

TEST(MenuPanelItem, EqualsOperatorChecksContent)
{
    Vector<Ref<ContextMenu::Item>> subs;
    MenuPanel::Item a("x", subs);
    MenuPanel::Item b("x", subs);
    EXPECT_TRUE(a == b);
}

// ===== MenuPanel: Construction =====

TEST(MenuPanel, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto mp = mmake<MenuPanel>();
    ASSERT_TRUE(mp);
    EXPECT_EQ(mp->GetItems().Count(), 0);
}

// ===== MenuPanel: Items =====

TEST(MenuPanel, AddItemByPathAddsToItems)
{
    SceneCleanGuard guard;
    auto mp = mmake<MenuPanel>();
    mp->AddItem("File");
    EXPECT_EQ(mp->GetItems().Count(), 1);
}

TEST(MenuPanel, AddItemByItemReturnsWidget)
{
    SceneCleanGuard guard;
    auto mp = mmake<MenuPanel>();
    Vector<Ref<ContextMenu::Item>> subs;
    MenuPanel::Item item("File", subs);
    auto widget = mp->AddItem(item);
    EXPECT_TRUE(widget);
    EXPECT_EQ(mp->GetItems().Count(), 1);
}

TEST(MenuPanel, AddItemsAddsAll)
{
    SceneCleanGuard guard;
    auto mp = mmake<MenuPanel>();
    Vector<MenuPanel::Item> items;
    Vector<Ref<ContextMenu::Item>> subs;
    items.Add(MenuPanel::Item("a", subs));
    items.Add(MenuPanel::Item("b", subs));
    mp->AddItems(items);
    EXPECT_EQ(mp->GetItems().Count(), 2);
}

TEST(MenuPanel, RemoveAllItemsClears)
{
    SceneCleanGuard guard;
    auto mp = mmake<MenuPanel>();
    mp->AddItem("a");
    mp->AddItem("b");
    mp->RemoveAllItems();
    EXPECT_EQ(mp->GetItems().Count(), 0);
}

// AddToggleItem with no '/' returns silently without sub-context;
// no item is added. Document the contract.
TEST(MenuPanel, AddToggleItemWithoutSlashIsNoOp)
{
    SceneCleanGuard guard;
    auto mp = mmake<MenuPanel>();
    mp->AddToggleItem("flag", true);
    EXPECT_EQ(mp->GetItems().Count(), 0);
}

// ===== Item sample =====

TEST(MenuPanel, SetItemSampleStores)
{
    SceneCleanGuard guard;
    auto mp = mmake<MenuPanel>();
    auto sample = mmake<Widget>();
    mp->SetItemSample(sample);
    EXPECT_TRUE(mp->GetItemSample());
}

// ===== Selection drawable layout =====

TEST(MenuPanel, SetSelectionDrawableLayoutRoundTrip)
{
    SceneCleanGuard guard;
    auto mp = mmake<MenuPanel>();
    auto layout = Layout::Based(BaseCorner::Center, Vec2F(50, 50));
    mp->SetSelectionDrawableLayout(layout);
    auto retrieved = mp->GetSelectionDrawableLayout();
    EXPECT_EQ(retrieved.anchorMin, layout.anchorMin);
}
