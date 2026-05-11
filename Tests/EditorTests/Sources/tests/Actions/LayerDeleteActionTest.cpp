#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"
#include "o2Editor/Actions/LayerDelete.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(LayerDeleteAction, CtorCapturesNameVisibilityIndex)
{
    SceneCleanGuard guard;

    o2Scene.AddLayer("A");
    auto target = o2Scene.AddLayer("B");
    target->visible = false;
    o2Scene.AddLayer("C");

    int expectedIdx = o2Scene.GetLayers().IndexOf(target);

    auto action = mmake<LayerDeleteAction>(target);

    EXPECT_EQ(action->layerName, String("B"));
    EXPECT_FALSE(action->savedVisible);
    EXPECT_EQ(action->savedIdx, expectedIdx);
}

TEST(LayerDeleteAction, RedoRemoves_UndoRestoresWithVisibility)
{
    SceneCleanGuard guard;

    auto layer = o2Scene.AddLayer("Foo");
    layer->visible = false;

    auto action = mmake<LayerDeleteAction>(layer);

    action->Redo();
    EXPECT_FALSE(o2Scene.HasLayer("Foo"));

    action->Undo();
    ASSERT_TRUE(o2Scene.HasLayer("Foo"));
    EXPECT_FALSE(o2Scene.GetLayer("Foo")->visible);
}

TEST(LayerDeleteAction, UndoRestoresIndexAmongSiblings)
{
    SceneCleanGuard guard;

    o2Scene.AddLayer("A");
    auto middle = o2Scene.AddLayer("B");
    o2Scene.AddLayer("C");

    int originalIdx = o2Scene.GetLayers().IndexOf(middle);

    auto action = mmake<LayerDeleteAction>(middle);
    action->Redo();
    ASSERT_FALSE(o2Scene.HasLayer("B"));

    action->Undo();
    ASSERT_TRUE(o2Scene.HasLayer("B"));
    EXPECT_EQ(o2Scene.GetLayers().IndexOf(o2Scene.GetLayer("B")), originalIdx);
}

TEST(LayerDeleteAction, RedoToleratesMissingLayer)
{
    SceneCleanGuard guard;
    auto layer = o2Scene.AddLayer("Foo");
    auto action = mmake<LayerDeleteAction>(layer);

    o2Scene.RemoveLayer(layer);
    ASSERT_FALSE(o2Scene.HasLayer("Foo"));

    action->Redo();
    EXPECT_FALSE(o2Scene.HasLayer("Foo"));
}

TEST(LayerDeleteAction, GetName)
{
    auto action = mmake<LayerDeleteAction>();
    EXPECT_EQ(action->GetName(), String("Delete layer"));
}
