#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Render/Text.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/TreeWindow/SceneHierarchyTree.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
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

    Ref<Text> Name(const Ref<TreeNode>& node)
    {
        return node->GetLayerDrawable<Text>("name");
    }

    bool SameRGB(const Color4& a, const Color4& b)
    {
        return a.r == b.r && a.g == b.g && a.b == b.b;
    }

    Ref<Actor> MakeChild(const Ref<Actor>& parent, const String& name)
    {
        auto child = MakeActor();
        child->SetName(name);
        parent->AddChild(child);
        return child;
    }
}

// Objects of a prototype instance are painted with the prototype color, plain ones are not
TEST(SceneHierarchyTreePrototypeView, PrototypeObjectsAreColored)
{
    SceneCleanGuard guard;

    auto plain = MakeActor();
    plain->SetName("plain");

    auto root = MakeActor();
    root->SetName("prototype");
    auto child = MakeChild(root, "prototypeChild");
    TickScene();

    root->MakePrototype();
    TickScene();

    auto tree = MakeTree();
    Reveal(tree, child);
    Pump(tree);

    auto plainNode = NodeOf(tree, plain);
    auto rootNode = NodeOf(tree, root);
    auto childNode = NodeOf(tree, child);
    ASSERT_NE(plainNode, nullptr);
    ASSERT_NE(rootNode, nullptr);
    ASSERT_NE(childNode, nullptr);

    EXPECT_TRUE(SameRGB(Name(rootNode)->GetColor(), SceneHierarchyTree::GetPrototypeLevelColor(1)));
    EXPECT_TRUE(SameRGB(Name(childNode)->GetColor(), Name(rootNode)->GetColor()))
        << "objects of one prototype instance share the color";
    EXPECT_FALSE(SameRGB(Name(plainNode)->GetColor(), Name(rootNode)->GetColor()))
        << "plain object name must keep the style color";
}

// A prototype instantiated inside another prototype instance gets the next level color
TEST(SceneHierarchyTreePrototypeView, NestedPrototypeGetsOwnColor)
{
    SceneCleanGuard guard;

    auto innerSource = MakeActor();
    innerSource->SetName("innerSource");
    TickScene();
    auto innerAsset = innerSource->MakePrototype();

    auto outer = MakeActor();
    outer->SetName("outer");
    auto outerChild = MakeChild(outer, "outerChild");
    TickScene();
    outer->MakePrototype();

    auto nested = mmake<Actor>(innerAsset, ActorCreateMode::InScene);
    nested->SetName("nested");
    outerChild->AddChild(nested);
    TickScene();

    auto tree = MakeTree();
    Reveal(tree, nested);
    Pump(tree);

    auto outerNode = NodeOf(tree, outer);
    auto outerChildNode = NodeOf(tree, outerChild);
    auto nestedNode = NodeOf(tree, nested);
    ASSERT_NE(outerNode, nullptr);
    ASSERT_NE(outerChildNode, nullptr);
    ASSERT_NE(nestedNode, nullptr);

    EXPECT_EQ(SceneHierarchyTree::GetPrototypeLevel(DynamicCast<SceneEditableObject>(nested)), 2);

    EXPECT_TRUE(SameRGB(Name(outerChildNode)->GetColor(), Name(outerNode)->GetColor()));
    EXPECT_TRUE(SameRGB(Name(nestedNode)->GetColor(), SceneHierarchyTree::GetPrototypeLevelColor(2)));
    EXPECT_FALSE(SameRGB(Name(nestedNode)->GetColor(), Name(outerNode)->GetColor()))
        << "nested prototype must be distinguishable from the one it is nested in";
}

// Node widgets are reused on refill, prototype color must not stick to them
TEST(SceneHierarchyTreePrototypeView, NodeViewResetsWhenPrototypeBroken)
{
    SceneCleanGuard guard;

    auto root = MakeActor();
    root->SetName("prototype");
    TickScene();
    root->MakePrototype();
    TickScene();

    auto tree = MakeTree();
    Pump(tree);

    auto node = NodeOf(tree, root);
    ASSERT_NE(node, nullptr);
    ASSERT_TRUE(SameRGB(Name(node)->GetColor(), SceneHierarchyTree::GetPrototypeLevelColor(1)));

    auto widget = node.Get();

    root->BreakPrototypeLink();
    tree->UpdateNodesView(true);
    Pump(tree);

    node = NodeOf(tree, root);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node.Get(), widget) << "node widget is expected to be reused for the same object";

    EXPECT_FALSE(SameRGB(Name(node)->GetColor(), SceneHierarchyTree::GetPrototypeLevelColor(1)));
}
