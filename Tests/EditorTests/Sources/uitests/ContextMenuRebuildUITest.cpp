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
