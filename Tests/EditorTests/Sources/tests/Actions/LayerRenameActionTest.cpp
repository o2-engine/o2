#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"
#include "o2Editor/Actions/LayerRename.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(LayerRenameAction, CtorCapturesNames)
{
    auto action = mmake<LayerRenameAction>("Old", "New");
    EXPECT_EQ(action->oldName, String("Old"));
    EXPECT_EQ(action->newName, String("New"));
}

TEST(LayerRenameAction, RedoRenames_UndoRevertsName)
{
    SceneCleanGuard guard;
    o2Scene.AddLayer("Old");
    auto action = mmake<LayerRenameAction>("Old", "New");

    action->Redo();
    EXPECT_FALSE(o2Scene.HasLayer("Old"));
    EXPECT_TRUE(o2Scene.HasLayer("New"));

    action->Undo();
    EXPECT_TRUE(o2Scene.HasLayer("Old"));
    EXPECT_FALSE(o2Scene.HasLayer("New"));
}

TEST(LayerRenameAction, RedoKeepsSameLayerInstance)
{
    SceneCleanGuard guard;
    auto original = o2Scene.AddLayer("Old");

    auto action = mmake<LayerRenameAction>("Old", "New");
    action->Redo();

    auto renamed = o2Scene.GetLayer("New");
    EXPECT_EQ(renamed, original);
}

TEST(LayerRenameAction, RedoToleratesMissingLayer)
{
    SceneCleanGuard guard;
    auto action = mmake<LayerRenameAction>("Ghost", "Spirit");

    action->Redo();
    EXPECT_FALSE(o2Scene.HasLayer("Spirit"));
}

TEST(LayerRenameAction, GetName)
{
    auto action = mmake<LayerRenameAction>("a", "b");
    EXPECT_EQ(action->GetName(), String("Rename layer"));
}
