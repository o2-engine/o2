#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2Editor/UIRoot.h"

using namespace o2;
using namespace Editor;

namespace
{
    struct MenuRoot
    {
        MenuRoot()
        {
            PushEditorScopeOnStack scope;
            if (!UIRoot::IsSingletonInitialzed())
                mmake<UIRoot>();
        }

        void Frame()
        {
            PushEditorScopeOnStack scope;
            auto root = EditorUIRoot.GetRootWidget();
            root->Update(1.0f / 60.0f);
            root->UpdateChildren(1.0f / 60.0f);
            root->UpdateChildrenTransforms();
            o2Render.Begin();
            root->Draw();
            o2Render.End();
        }
    };

    Ref<ContextMenu> SubMenuOf(const Ref<ContextMenu>& menu, const WString& text)
    {
        auto item = menu->FindChildByTypeAndName<ContextMenuItem>((String)(WString("Context Item ") + text));
        return item ? item->GetSubMenu() : nullptr;
    }
}

// Items changed while the menu is hidden appear the next time it is shown
TEST(ContextMenuRebuild, HiddenMenuShowsItsNewItems)
{
    MenuRoot root;
    PushEditorScopeOnStack scope;
    auto menu = o2UI.CreateWidget<ContextMenu>();
    EditorUIRoot.AddWidget(menu);

    menu->AddItem("Old/a");
    menu->Show(Vec2F());
    root.Frame();
    menu->Hide(true);
    root.Frame();

    menu->RemoveAllItems();
    menu->AddItem("New/b");
    menu->AddItem("New/c");
    menu->Show(Vec2F());
    root.Frame();

    EXPECT_FALSE(SubMenuOf(menu, "Old"));
    auto sub = SubMenuOf(menu, "New");
    ASSERT_TRUE(sub);
    EXPECT_EQ(sub->GetItems().Count(), 2);

    menu->Hide(true);
    EditorUIRoot.RemoveWidget(menu);
}

// Reused item widgets carry exactly the icon of the item they show now, never the one they showed before
TEST(ContextMenuRebuild, ReusedItemsDoNotStackIcons)
{
    MenuRoot root;
    PushEditorScopeOnStack scope;
    auto menu = o2UI.CreateWidget<ContextMenu>();
    EditorUIRoot.AddWidget(menu);

    AssetRef<ImageAsset> icon("ui/UI4_small_trash_icon.png");
    auto fill = [&](bool withIcons)
    {
        menu->RemoveAllItems();
        for (auto name : { "one", "two", "three" })
            menu->AddItem(name, Function<void()>(), withIcons ? icon : AssetRef<ImageAsset>());
        menu->Show(Vec2F());
        root.Frame();
    };
    auto iconLayersOf = [&](const char* name)
    {
        auto item = menu->FindChildByTypeAndName<ContextMenuItem>((String)"Context Item " + name);
        auto layer = item ? item->FindLayer("icon") : nullptr;
        return layer ? layer->GetChildren().Count() : -1;
    };

    fill(true);
    fill(true);
    fill(true);
    for (auto name : { "one", "two", "three" })
        EXPECT_EQ(iconLayersOf(name), 1) << name;

    fill(false);
    for (auto name : { "one", "two", "three" })
        EXPECT_EQ(iconLayersOf(name), 0) << name;

    menu->Hide(true);
    EditorUIRoot.RemoveWidget(menu);
}

// An item's icon fits the item's height with its proportions kept, also on the first show, before any layout pass
TEST(ContextMenuRebuild, IconsFitTheItem)
{
    MenuRoot root;
    PushEditorScopeOnStack scope;
    auto menu = o2UI.CreateWidget<ContextMenu>();
    EditorUIRoot.AddWidget(menu);

    menu->AddItem("vector", Function<void()>(), o2Assets.GetAssetRef("ui/UI4_small_trash_icon.svg"));
    menu->AddItem("raster", Function<void()>(), o2Assets.GetAssetRef("ui/UI4_small_trash_icon.png"));
    menu->Show(Vec2F());
    root.Frame();

    for (auto name : { "vector", "raster" })
    {
        auto item = menu->FindChildByTypeAndName<ContextMenuItem>((String)"Context Item " + name);
        ASSERT_TRUE(item) << name;
        auto iconLayer = item->FindLayer("icon");
        ASSERT_TRUE(iconLayer && iconLayer->GetChildren().Count() == 1) << name;
        auto icon = iconLayer->GetChildren()[0]->GetDrawable();
        ASSERT_TRUE(icon) << name;

        RectF rect = icon->GetRect();
        EXPECT_GT(rect.Width(), 4.0f) << name;
        EXPECT_GT(rect.Height(), 4.0f) << name;
        EXPECT_LE(rect.Height(), item->layout->GetHeight() + 0.5f) << name;
        EXPECT_NEAR(rect.Width()/rect.Height(), 1.0f, 0.15f) << name;
    }

    menu->Hide(true);
    EditorUIRoot.RemoveWidget(menu);
}

// A sub menu is restyled by its owner's onSubMenuCreated, as the items of the owner are
TEST(ContextMenuRebuild, SubMenusAreRestyledByTheirOwner)
{
    MenuRoot root;
    PushEditorScopeOnStack scope;
    auto menu = o2UI.CreateWidget<ContextMenu>();
    EditorUIRoot.AddWidget(menu);

    int restyled = 0;
    menu->onSubMenuCreated = [&](const Ref<ContextMenu>& subMenu) { restyled++; subMenu->SetIconsColor(Color4(10, 20, 30)); };
    menu->AddItem("First/a");
    menu->AddItem("Second/b");
    menu->Show(Vec2F());
    root.Frame();

    EXPECT_EQ(restyled, 2);
    auto sub = SubMenuOf(menu, "First");
    ASSERT_TRUE(sub);
    EXPECT_EQ(sub->GetIconsColor(), Color4(10, 20, 30));

    menu->Hide(true);
    EditorUIRoot.RemoveWidget(menu);
}

// The icons of the items take the menu's icons colour
TEST(ContextMenuRebuild, IconsTakeTheMenuColor)
{
    MenuRoot root;
    PushEditorScopeOnStack scope;
    auto menu = o2UI.CreateWidget<ContextMenu>();
    EditorUIRoot.AddWidget(menu);

    menu->SetIconsColor(Color4(150, 160, 170));
    menu->AddItem("item", Function<void()>(), o2Assets.GetAssetRef("ui/UI4_small_trash_icon.svg"));
    menu->Show(Vec2F());
    root.Frame();

    auto item = menu->FindChildByTypeAndName<ContextMenuItem>("Context Item item");
    ASSERT_TRUE(item);
    auto iconLayer = item->FindLayer("icon");
    ASSERT_TRUE(iconLayer && iconLayer->GetChildren().Count() == 1);
    auto icon = iconLayer->GetChildren()[0]->GetDrawable();
    ASSERT_TRUE(icon);
    Color4 color = icon->GetColor();
    EXPECT_EQ(Color4(color.r, color.g, color.b), Color4(150, 160, 170));

    menu->Hide(true);
    EditorUIRoot.RemoveWidget(menu);
}
