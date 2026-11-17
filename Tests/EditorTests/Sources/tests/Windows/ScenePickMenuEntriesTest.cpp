#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2Editor/Windows/SceneWindow/ScenePickMenu.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Ref<Actor> Child(const Ref<Actor>& parent, const String& name)
    {
        auto actor = MakeActor();
        actor->SetName(name);
        if (parent)
            actor->SetParent(parent);

        return actor;
    }

    Ref<SceneEditableObject> Editable(const Ref<Actor>& actor)
    {
        return DynamicCast<SceneEditableObject>(actor);
    }

    Vector<String> Names(const Vector<Ref<SceneEditableObject>>& objects)
    {
        return objects.Convert<String>([](const Ref<SceneEditableObject>& x) { return x->GetName(); });
    }
}

// Only the objects under the cursor make the tree: an object hangs on its nearest ancestor that is under the cursor too
TEST(ScenePickMenuEntries, TheTreeHoldsOnlyTheObjectsUnderTheCursor)
{
    SceneCleanGuard guard;
    auto root = Child(nullptr, "root");
    auto panel = Child(root, "panel");
    auto row = Child(panel, "row");
    auto icon = Child(row, "icon");
    auto other = Child(nullptr, "other");
    TickScene();

    auto entries = ScenePickMenu::BuildEntries({ Editable(other), Editable(icon), Editable(panel) });

    ASSERT_EQ(entries.Count(), 3);
    EXPECT_EQ(entries[0].object, Editable(panel));
    EXPECT_EQ(entries[0].depth, 0);
    EXPECT_EQ(Names(entries[0].hiddenParents), Vector<String>({ "root" }));

    EXPECT_EQ(entries[1].object, Editable(icon));
    EXPECT_EQ(entries[1].depth, 1);
    EXPECT_EQ(Names(entries[1].hiddenParents), Vector<String>({ "root", "row" }));
    EXPECT_EQ(entries[1].hiddenDepths, Vector<int>({ 0, 2 }));

    EXPECT_EQ(entries[2].object, Editable(other));
    EXPECT_EQ(entries[2].depth, 0);
    EXPECT_TRUE(entries[2].hiddenParents.IsEmpty());
}

// The items go depth-first in the order of the scene hierarchy, whatever order the objects were found in
TEST(ScenePickMenuEntries, ItemsFollowTheHierarchyOrder)
{
    SceneCleanGuard guard;
    auto root = Child(nullptr, "root");
    auto first = Child(root, "first");
    auto second = Child(root, "second");
    auto firstChild = Child(first, "first child");
    TickScene();

    auto entries = ScenePickMenu::BuildEntries({ Editable(second), Editable(firstChild), Editable(root),
                                                 Editable(firstChild) });

    ASSERT_EQ(entries.Count(), 3);
    EXPECT_EQ(entries[0].object, Editable(root));
    EXPECT_EQ(entries[1].object, Editable(firstChild));
    EXPECT_EQ(entries[1].depth, 1);
    EXPECT_EQ(Names(entries[1].hiddenParents), Vector<String>({ "first" }));
    EXPECT_EQ(entries[2].object, Editable(second));
    EXPECT_EQ(entries[2].depth, 1);
}

// A widget layer hangs on its widget; the "layers" group between them is a container, not a node
TEST(ScenePickMenuEntries, LayersHangOnTheirWidgets)
{
    SceneCleanGuard guard;
    auto screen = mmake<Widget>(ActorCreateMode::InScene);
    screen->SetName("screen");
    auto button = mmake<Widget>(ActorCreateMode::InScene);
    button->SetName("button");
    screen->AddChild(button);

    auto back = button->AddLayer("back", nullptr);
    auto shine = back->AddChildLayer("shine", nullptr);
    TickScene();

    auto entries = ScenePickMenu::BuildEntries({ DynamicCast<SceneEditableObject>(shine),
                                                 DynamicCast<SceneEditableObject>(button) });

    ASSERT_EQ(entries.Count(), 2);
    EXPECT_EQ(entries[0].object, DynamicCast<SceneEditableObject>(button));
    EXPECT_EQ(Names(entries[0].hiddenParents), Vector<String>({ "screen" }));

    EXPECT_EQ(entries[1].object, DynamicCast<SceneEditableObject>(shine));
    EXPECT_EQ(entries[1].depth, 1);
    EXPECT_EQ(Names(entries[1].hiddenParents), Vector<String>({ "screen", "back" }));

    EXPECT_EQ(ScenePickMenu::GetObjectIcon(DynamicCast<SceneEditableObject>(shine)), String("ui/UI4_node_layer_icon.svg"));
    EXPECT_EQ(ScenePickMenu::GetObjectIcon(DynamicCast<SceneEditableObject>(button)), String("ui/UI4_node_widget_icon.svg"));
}
