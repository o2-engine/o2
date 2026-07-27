#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Text.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/TreeWindow/SceneHierarchyTree.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    const float kEnabledAlpha = 1.0f;
    const float kDisabledAlpha = 0.5f;
    const float kAlphaEps = 0.01f; // color alpha is stored as a byte

    Ref<SceneHierarchyTree> MakeTree()
    {
        PushEditorScopeOnStack scope; // editor widgets must not go to the scene, as in the editor

        auto tree = o2UI.CreateWidget<SceneHierarchyTree>("standard");
        TickScene(); // initializes freshly created widget actors, without it the tree stays disabled

        *tree->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(300, 400));
        tree->UpdateTransform();
        tree->AttachToSceneEvents();
        tree->UpdateNodesView(true);
        return tree;
    }

    void Pump(const Ref<SceneHierarchyTree>& tree, int frames = 4)
    {
        for (int i = 0; i < frames; i++)
        {
            tree->Update(0.1f);
            tree->UpdateChildren(0.1f);
        }
    }

    // expands parents so the object gets its own node; expanding everything would flood the tree with
    // the editor's own widgets and push test objects out of the visible range
    void Reveal(const Ref<SceneHierarchyTree>& tree, const Ref<Actor>& actor)
    {
        tree->ExpandParentObjects(actor.Get());
    }

    Ref<TreeNode> NodeOf(const Ref<SceneHierarchyTree>& tree, const Ref<Actor>& actor)
    {
        return tree->GetNode(DynamicCast<SceneEditableObject>(actor));
    }

    float NameAlpha(const Ref<TreeNode>& node)
    {
        return node->GetLayerDrawable<Text>("name")->GetTransparency();
    }

    // toggle graphics live inside its half hide layer, its result transparency is what gets drawn
    float EnableToggleAlpha(const Ref<TreeNode>& node)
    {
        auto toggle = node->GetChildByType<Toggle>("enableToggle");
        return toggle->GetLayer("halfHide")->GetResTransparency();
    }
}

// Building a node widget for a newly appeared object must not brighten the already dimmed nodes
TEST(SceneHierarchyTreeEnabledView, DimmedNodeSurvivesNewNodeWidget)
{
    SceneCleanGuard guard;

    auto disabled = MakeActor();
    disabled->SetName("disabled");
    disabled->SetEnabled(false);

    auto other = MakeActor();
    other->SetName("other");
    TickScene();

    auto tree = MakeTree();
    Pump(tree);

    auto node = NodeOf(tree, disabled);
    ASSERT_NE(node, nullptr);
    EXPECT_NEAR(NameAlpha(node), kDisabledAlpha, kAlphaEps) << "disabled object node must be dimmed";

    auto added = MakeActor();
    added->SetName("added");
    TickScene();
    Pump(tree);

    node = NodeOf(tree, disabled);
    ASSERT_NE(node, nullptr);
    EXPECT_NEAR(NameAlpha(node), kDisabledAlpha, kAlphaEps)
        << "disabled object node must stay dimmed after tree created a node widget for another object";
    EXPECT_NEAR(EnableToggleAlpha(node), kDisabledAlpha, kAlphaEps);
}

// Disabling a parent dims its children nodes too - they are disabled in hierarchy
TEST(SceneHierarchyTreeEnabledView, ChildNodeDimsWithDisabledParent)
{
    SceneCleanGuard guard;

    auto parent = MakeActor();
    parent->SetName("parent");

    auto child = MakeActor();
    child->SetName("child");
    parent->AddChild(child);
    TickScene();

    auto tree = MakeTree();
    Reveal(tree, child);
    Pump(tree);

    auto childNode = NodeOf(tree, child);
    ASSERT_NE(childNode, nullptr);
    EXPECT_NEAR(NameAlpha(childNode), kEnabledAlpha, kAlphaEps);

    parent->SetEnabled(false);
    Pump(tree);

    childNode = NodeOf(tree, child);
    ASSERT_NE(childNode, nullptr);
    EXPECT_NEAR(NameAlpha(childNode), kDisabledAlpha, kAlphaEps)
        << "child of disabled parent must look disabled";
    EXPECT_NEAR(EnableToggleAlpha(childNode), kDisabledAlpha, kAlphaEps);

    parent->SetEnabled(true);
    Pump(tree);

    childNode = NodeOf(tree, child);
    ASSERT_NE(childNode, nullptr);
    EXPECT_NEAR(NameAlpha(childNode), kEnabledAlpha, kAlphaEps);
    EXPECT_NEAR(EnableToggleAlpha(childNode), kEnabledAlpha, kAlphaEps);
}

// Name editing animates the name layer transparency, dimming must come back when editing ends
TEST(SceneHierarchyTreeEnabledView, DimmingRestoredAfterNameEditing)
{
    SceneCleanGuard guard;

    auto disabled = MakeActor();
    disabled->SetName("disabled");
    disabled->SetEnabled(false);
    TickScene();

    auto tree = MakeTree();
    Pump(tree);

    auto node = NodeOf(tree, disabled);
    ASSERT_NE(node, nullptr);

    DynamicCast<SceneHierarchyTreeNode>(node)->EnableEditName();
    Pump(tree, 5);
    EXPECT_NEAR(NameAlpha(node), 0.0f, kAlphaEps) << "name is hidden while editing";

    node->SetState("edit", false);
    Pump(tree, 5);

    EXPECT_NEAR(NameAlpha(node), kDisabledAlpha, kAlphaEps)
        << "disabled node must be dimmed again after name editing";
}

// Node widgets are pooled: one taken from an object without enabling support must not show a
// disabled object as enabled
TEST(SceneHierarchyTreeEnabledView, ReusedNodeWidgetDimsDisabledObject)
{
    SceneCleanGuard guard;

    auto widget = mmake<Widget>(ActorCreateMode::InScene);
    widget->SetName("widget");
    widget->AddLayer("back", nullptr);

    auto disabled = MakeActor();
    disabled->SetName("disabled");
    disabled->SetEnabled(false);
    widget->AddChild(disabled);
    TickScene();

    auto tree = MakeTree();
    Reveal(tree, disabled);
    Pump(tree);

    ASSERT_NE(NodeOf(tree, disabled), nullptr);

    // collapse returns node widgets to the pool, expand takes them back for other objects
    tree->GetNode(DynamicCast<SceneEditableObject>(widget))->Collapse();
    Pump(tree);
    tree->GetNode(DynamicCast<SceneEditableObject>(widget))->Expand();
    Pump(tree);

    auto node = NodeOf(tree, disabled);
    ASSERT_NE(node, nullptr);
    EXPECT_NEAR(NameAlpha(node), kDisabledAlpha, kAlphaEps);
    EXPECT_NEAR(EnableToggleAlpha(node), kDisabledAlpha, kAlphaEps)
        << "enable toggle of disabled object must be dimmed";
}
