#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"
#include "o2Editor/Actions/LayerReorder.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(LayerReorderAction, CtorCapturesNameAndIndices)
{
    auto action = mmake<LayerReorderAction>("Foo", 1, 3);
    EXPECT_EQ(action->layerName, String("Foo"));
    EXPECT_EQ(action->fromIdx, 1);
    EXPECT_EQ(action->toIdx, 3);
}

TEST(LayerReorderAction, RedoMoves_UndoReverts)
{
    SceneCleanGuard guard;
    auto a = o2Scene.AddLayer("A");
    auto b = o2Scene.AddLayer("B");
    auto c = o2Scene.AddLayer("C");

    int fromIdx = o2Scene.GetLayers().IndexOf(a);
    int toIdx = o2Scene.GetLayers().IndexOf(c);

    auto action = mmake<LayerReorderAction>("A", fromIdx, toIdx);

    action->Redo();
    EXPECT_EQ(o2Scene.GetLayers().IndexOf(a), toIdx);

    action->Undo();
    EXPECT_EQ(o2Scene.GetLayers().IndexOf(a), fromIdx);
}

TEST(LayerReorderAction, ToleratesMissingLayer)
{
    SceneCleanGuard guard;
    auto action = mmake<LayerReorderAction>("Ghost", 0, 1);

    action->Redo();
    action->Undo();
    SUCCEED();
}

TEST(LayerReorderAction, GetName)
{
    auto action = mmake<LayerReorderAction>("X", 0, 1);
    EXPECT_EQ(action->GetName(), String("Reorder layer"));
}
