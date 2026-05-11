#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"
#include "o2Editor/Actions/LayerCreate.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(LayerCreateAction, CtorCapturesName)
{
    auto action = mmake<LayerCreateAction>("Foo");
    EXPECT_EQ(action->layerName, String("Foo"));
}

TEST(LayerCreateAction, RedoCreates_UndoRemoves)
{
    SceneCleanGuard guard;
    ASSERT_FALSE(o2Scene.HasLayer("Foo"));

    auto action = mmake<LayerCreateAction>("Foo");

    action->Redo();
    EXPECT_TRUE(o2Scene.HasLayer("Foo"));

    action->Undo();
    EXPECT_FALSE(o2Scene.HasLayer("Foo"));
}

TEST(LayerCreateAction, RoundTripRedoUndoRedo)
{
    SceneCleanGuard guard;
    auto action = mmake<LayerCreateAction>("Bar");

    action->Redo();
    action->Undo();
    action->Redo();

    EXPECT_TRUE(o2Scene.HasLayer("Bar"));
}

TEST(LayerCreateAction, GetName)
{
    auto action = mmake<LayerCreateAction>("X");
    EXPECT_EQ(action->GetName(), String("Create layer"));
}
