#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"
#include "o2Editor/Actions/LayerCreate.h"
#include "o2Editor/Actions/LayerDelete.h"
#include "o2Editor/Actions/LayerRename.h"
#include "o2Editor/Actions/LayerReorder.h"
#include "o2Editor/Actions/LayerVisibility.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

// Records calls to o2Scene.onLayersListChanged across the scope.
class LayersListChangedProbe
{
public:
    int  count = 0;
    int  baseline;

    LayersListChangedProbe()
    {
        baseline = 0;
        o2Scene.onLayersListChanged += [this]() { ++count; };
    }

    int Delta() { int d = count - baseline; baseline = count; return d; }
};

TEST(LayerActionsNotification, CreateFiresOnRedoAndUndo)
{
    SceneCleanGuard guard;
    LayersListChangedProbe probe;
    probe.Delta(); // reset

    auto action = mmake<LayerCreateAction>("Foo");
    action->Redo();
    EXPECT_GE(probe.Delta(), 1);

    action->Undo();
    EXPECT_GE(probe.Delta(), 1);
}

TEST(LayerActionsNotification, DeleteFiresOnRedoAndUndo)
{
    SceneCleanGuard guard;
    auto layer = o2Scene.AddLayer("Foo");

    LayersListChangedProbe probe;
    probe.Delta();

    auto action = mmake<LayerDeleteAction>(layer);
    action->Redo();
    EXPECT_GE(probe.Delta(), 1);

    action->Undo();
    EXPECT_GE(probe.Delta(), 1);
}

TEST(LayerActionsNotification, RenameFiresOnRedoAndUndo)
{
    SceneCleanGuard guard;
    o2Scene.AddLayer("Old");

    LayersListChangedProbe probe;
    probe.Delta();

    auto action = mmake<LayerRenameAction>("Old", "New");
    action->Redo();
    EXPECT_GE(probe.Delta(), 1);

    action->Undo();
    EXPECT_GE(probe.Delta(), 1);
}

TEST(LayerActionsNotification, ReorderFiresOnRedoAndUndo)
{
    SceneCleanGuard guard;
    o2Scene.AddLayer("A");
    o2Scene.AddLayer("B");

    LayersListChangedProbe probe;
    probe.Delta();

    auto action = mmake<LayerReorderAction>("A", 0, 1);
    action->Redo();
    EXPECT_GE(probe.Delta(), 1);

    action->Undo();
    EXPECT_GE(probe.Delta(), 1);
}

TEST(LayerActionsNotification, VisibilityFiresOnRedoAndUndo)
{
    SceneCleanGuard guard;
    o2Scene.AddLayer("Foo");

    LayersListChangedProbe probe;
    probe.Delta();

    auto action = mmake<LayerVisibilityAction>("Foo", false);
    action->Redo();
    EXPECT_GE(probe.Delta(), 1);

    action->Undo();
    EXPECT_GE(probe.Delta(), 1);
}
