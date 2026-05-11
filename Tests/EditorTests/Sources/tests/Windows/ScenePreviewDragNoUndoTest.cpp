#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/ActorTransform.h"
#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/ActionsList.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

// Invariant guarded: SceneEditScreen drag-preview path
// (OnDragEnter -> OnDraggedAbove -> OnDragExit) instantiates
// temporary actors and moves them around for visual feedback,
// but must NOT register any IAction in the undo history. The
// final CreateAction is registered only on OnDropped. If a
// regression adds DoneAction(...) into the preview path, this
// test catches it.
TEST(ScenePreviewDragNoUndo, InstantiateMoveDestroyDoesNotPushAction)
{
    SceneCleanGuard guard;
    ActionsList list;
    EXPECT_EQ(list.GetUndoActionsCount(), 0);

    auto preview = MakeActor();
    TickScene();
    EXPECT_EQ(list.GetUndoActionsCount(), 0);

    for (int i = 0; i < 8; ++i)
    {
        preview->transform->SetPosition(Vec2F((float)i, (float)i));
        TickScene();
    }
    EXPECT_EQ(list.GetUndoActionsCount(), 0);

    o2Scene.DestroyActor(preview);
    o2Scene.UpdateDestroyingEntities();
    EXPECT_EQ(list.GetUndoActionsCount(), 0);
}
