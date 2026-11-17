#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include <set>
#include <string>

#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Tree.h"
#include "o2Editor/UI/Preview/FontSampleView.h"
#include "o2Editor/Windows/SceneWindow/ScenePickMenu.h"
#include "o2Editor/Windows/TreeWindow/TreeWindow.h"

using namespace o2;
using namespace Editor;

// The create menu of the tree lists the base widgets under "UI"; the editor's own widgets go to "UI/Editor"
TEST(TreeCreateMenuPath, EditorWidgetsLeaveTheBaseWidgetsList)
{
    EXPECT_EQ(TreeWindow::GetCreateMenuPath(TypeOf(Button)), String("UI/Button"));
    EXPECT_EQ(TreeWindow::GetCreateMenuPath(TypeOf(Label)), String("UI/Label"));

    EXPECT_TRUE(TreeWindow::GetCreateMenuPath(TypeOf(FontSampleView)).StartsWith("UI/Editor/"));
    EXPECT_TRUE(TreeWindow::GetCreateMenuPath(TypeOf(ScenePickMenu)).StartsWith("UI/Editor/"));

    for (auto type : TypeOf(Widget).GetDerivedTypes())
    {
        if (type->GetName().StartsWith("Editor::"))
            EXPECT_TRUE(TreeWindow::GetCreateMenuPath(*type).StartsWith("UI/Editor/")) << type->GetName();
    }
}

// Every actor type has an icon in the create menu: its own, or the one of its nearest base type that has one
TEST(TreeCreateMenuPath, ActorTypesHaveIcons)
{
    EXPECT_EQ(TypeOf(Actor).InvokeStatic<String>("GetEditorIcon"), String("ui/UI4_node_actor_icon.svg"));
    EXPECT_EQ(TypeOf(Widget).InvokeStatic<String>("GetEditorIcon"), String("ui/UI4_node_widget_icon.svg"));
    EXPECT_EQ(TypeOf(Button).InvokeStatic<String>("GetEditorIcon"), String("ui/UI4_actor_button_icon.svg"));
    EXPECT_EQ(TypeOf(Label).InvokeStatic<String>("GetEditorIcon"), String("ui/UI4_actor_label_icon.svg"));
    EXPECT_EQ(TypeOf(FontSampleView).InvokeStatic<String>("GetEditorIcon"), String("ui/UI4_node_widget_icon.svg"))
        << "a type without its own icon takes the one of its base";

    std::set<std::string> frameworkIcons;
    for (auto type : TypeOf(Actor).GetDerivedTypes())
    {
        String icon = type->InvokeStatic<String>("GetEditorIcon");
        EXPECT_TRUE(icon.StartsWith("ui/") && icon.EndsWith(".svg")) << type->GetName();

        if (type->GetName().StartsWith("o2::"))
            frameworkIcons.insert(icon.Data());
    }

    EXPECT_GE(frameworkIcons.size(), 30u) << "the base widget and actor types have icons of their own";
}
