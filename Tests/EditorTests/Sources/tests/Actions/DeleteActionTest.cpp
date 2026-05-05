#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/Delete.h"
#include "support/EditorTestScene.h"
#include "support/MockActionsUIBridge.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(DeleteAction, CtorCapturesObjectInfos)
{
    SceneCleanGuard guard;
    auto parent = MakeActor();
    auto a = MakeActor();
    a->SetEditableParent(parent);
    auto b = MakeActor();
    b->SetEditableParent(parent);
    TickScene();

    auto action = mmake<DeleteAction>(AsEditable({ a, b }));

    ASSERT_EQ(action->objectsInfos.Count(), 2);
    EXPECT_EQ(action->objectsInfos[0].parentId, parent->GetID());
    EXPECT_EQ(action->objectsInfos[1].parentId, parent->GetID());
}

TEST(DeleteAction, UndoRestoresObjectAsChild)
{
    SceneCleanGuard guard;
    RecordingActionsUIBridge recorder;
    ScopedActionsUIBridge host(recorder);

    auto parent = MakeActor();
    auto child = MakeActor();
    child->SetEditableParent(parent);
    TickScene();

    SceneUID childId = child->GetID();

    auto action = mmake<DeleteAction>(AsEditable({ child }));

    o2Scene.DestroyActor(child);
    child = nullptr;
    o2Scene.UpdateDestroyingEntities();
    EXPECT_EQ(parent->GetEditableChildren().Count(), 0);
    EXPECT_EQ(o2Scene.GetEditableObjectByID(childId), nullptr);

    action->Undo();
    TickScene();

    EXPECT_EQ(parent->GetEditableChildren().Count(), 1);
    EXPECT_NE(o2Scene.GetEditableObjectByID(childId), nullptr);

    ASSERT_EQ(recorder.selectObjectCalls.Count(), 1);
    EXPECT_EQ(recorder.selectObjectCalls[0].objectId, childId);
    ASSERT_EQ(recorder.highlightedObjectIds.Count(), 1);
    EXPECT_EQ(recorder.highlightedObjectIds[0], childId);
    EXPECT_EQ(recorder.updateTreeViewCalls, 1);
}

TEST(DeleteAction, UndoRestoresRootObject)
{
    SceneCleanGuard guard;
    RecordingActionsUIBridge recorder;
    ScopedActionsUIBridge host(recorder);

    auto a = MakeActor();
    TickScene();
    SceneUID aId = a->GetID();

    auto action = mmake<DeleteAction>(AsEditable({ a }));

    o2Scene.DestroyActor(a);
    a = nullptr;
    o2Scene.UpdateDestroyingEntities();
    EXPECT_EQ(o2Scene.GetEditableObjectByID(aId), nullptr);

    action->Undo();
    TickScene();

    EXPECT_NE(o2Scene.GetEditableObjectByID(aId), nullptr);
    ASSERT_EQ(recorder.highlightedObjectIds.Count(), 1);
    EXPECT_EQ(recorder.highlightedObjectIds[0], aId);
}

TEST(DeleteAction, UndoPreservesSiblingOrder)
{
    SceneCleanGuard guard;
    RecordingActionsUIBridge recorder;
    ScopedActionsUIBridge host(recorder);

    auto parent = MakeActor();
    auto first = MakeActor();
    first->SetEditableParent(parent);
    auto middle = MakeActor();
    middle->SetEditableParent(parent);
    auto last = MakeActor();
    last->SetEditableParent(parent);
    TickScene();

    SceneUID middleId = middle->GetID();

    auto action = mmake<DeleteAction>(AsEditable({ middle }));

    o2Scene.DestroyActor(middle);
    middle = nullptr;
    o2Scene.UpdateDestroyingEntities();
    ASSERT_EQ(parent->GetEditableChildren().Count(), 2);

    action->Undo();

    auto children = parent->GetEditableChildren();
    ASSERT_EQ(children.Count(), 3);
    EXPECT_EQ(children[1]->GetID(), middleId);
}

TEST(DeleteAction, RedoNotifiesUIWithoutTouchingScene)
{
    SceneCleanGuard guard;
    RecordingActionsUIBridge recorder;
    ScopedActionsUIBridge host(recorder);

    auto a = MakeActor();
    TickScene();

    auto action = mmake<DeleteAction>(AsEditable({ a }));

    action->Redo();

    ASSERT_EQ(recorder.clearSelectionCalls.Count(), 1);
    EXPECT_EQ(recorder.clearSelectionCalls[0], 1);
    EXPECT_EQ(recorder.updateTreeViewCalls, 1);
}

TEST(DeleteAction, GetNameIsStable)
{
    auto action = mmake<DeleteAction>();
    EXPECT_EQ(action->GetName(), String("Actors deletion"));
}
