#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/Create.h"
#include "support/EditorTestScene.h"
#include "support/MockActionsUIBridge.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(CreateAction, CtorCapturesIdsAndPlacement)
{
    SceneCleanGuard guard;
    auto parent = MakeActor();
    auto prev = MakeActor();
    prev->SetEditableParent(parent);
    auto child = MakeActor();
    child->SetEditableParent(parent);
    TickScene();

    auto action = mmake<CreateAction>(AsEditable({ child }), AsEditable({ parent }).First(),
                                      AsEditable({ prev }).First());

    ASSERT_EQ(action->objectsIds.Count(), 1);
    EXPECT_EQ(action->objectsIds[0], child->GetID());
    EXPECT_EQ(action->insertParentId, parent->GetID());
    EXPECT_EQ(action->insertPrevObjectId, prev->GetID());
}

TEST(CreateAction, RedoReinsertsObjectAsChildOfParent)
{
    SceneCleanGuard guard;
    RecordingActionsUIBridge recorder;
    ScopedActionsUIBridge host(recorder);

    auto parent = MakeActor();
    auto child = MakeActor();
    child->SetEditableParent(parent);
    TickScene();

    SceneUID childId = child->GetID();
    EXPECT_EQ(parent->GetEditableChildren().Count(), 1);

    auto action = mmake<CreateAction>(AsEditable({ child }), AsEditable({ parent }).First(),
                                      Ref<SceneEditableObject>());

    o2Scene.DestroyActor(child);
    child = nullptr;
    o2Scene.UpdateDestroyingEntities();
    EXPECT_EQ(parent->GetEditableChildren().Count(), 0);

    action->Redo();
    TickScene();

    EXPECT_EQ(parent->GetEditableChildren().Count(), 1);
    EXPECT_NE(o2Scene.GetEditableObjectByID(childId), nullptr);

    ASSERT_EQ(recorder.highlightedObjectIds.Count(), 1);
    EXPECT_EQ(recorder.highlightedObjectIds[0], childId);
    ASSERT_EQ(recorder.selectObjectsCalls.Count(), 1);
    EXPECT_FALSE(recorder.selectObjectsCalls[0].additive);
    ASSERT_EQ(recorder.selectObjectsCalls[0].objectIds.Count(), 1);
    EXPECT_EQ(recorder.selectObjectsCalls[0].objectIds[0], childId);
}

TEST(CreateAction, RedoReinsertsAsRootObject)
{
    SceneCleanGuard guard;
    RecordingActionsUIBridge recorder;
    ScopedActionsUIBridge host(recorder);

    auto a = MakeActor();
    TickScene();
    SceneUID aId = a->GetID();

    auto action = mmake<CreateAction>(AsEditable({ a }), Ref<SceneEditableObject>(),
                                      Ref<SceneEditableObject>());

    o2Scene.DestroyActor(a);
    a = nullptr;
    o2Scene.UpdateDestroyingEntities();
    EXPECT_EQ(o2Scene.GetEditableObjectByID(aId), nullptr);

    action->Redo();
    TickScene();

    EXPECT_NE(o2Scene.GetEditableObjectByID(aId), nullptr);
    ASSERT_EQ(recorder.selectObjectsCalls.Count(), 1);
    EXPECT_EQ(recorder.selectObjectsCalls[0].objectIds[0], aId);
}

TEST(CreateAction, RedoRespectsPrevSiblingPosition)
{
    SceneCleanGuard guard;
    RecordingActionsUIBridge recorder;
    ScopedActionsUIBridge host(recorder);

    auto parent = MakeActor();
    auto first = MakeActor();
    first->SetEditableParent(parent);
    auto second = MakeActor();
    second->SetEditableParent(parent);
    auto third = MakeActor();
    third->SetEditableParent(parent);
    TickScene();

    SceneUID thirdId = third->GetID();

    auto action = mmake<CreateAction>(AsEditable({ third }),
                                      AsEditable({ parent }).First(),
                                      AsEditable({ first }).First());

    o2Scene.DestroyActor(third);
    third = nullptr;
    o2Scene.UpdateDestroyingEntities();
    ASSERT_EQ(parent->GetEditableChildren().Count(), 2);

    action->Redo();

    auto children = parent->GetEditableChildren();
    ASSERT_EQ(children.Count(), 3);
    EXPECT_EQ(children[1]->GetID(), thirdId);
}

TEST(CreateAction, UndoNotifiesSelectionToClear)
{
    SceneCleanGuard guard;
    RecordingActionsUIBridge recorder;
    ScopedActionsUIBridge host(recorder);

    auto a = MakeActor();
    TickScene();

    auto action = mmake<CreateAction>(AsEditable({ a }), Ref<SceneEditableObject>(),
                                      Ref<SceneEditableObject>());

    action->Undo();

    ASSERT_EQ(recorder.clearSelectionCalls.Count(), 1);
    EXPECT_EQ(recorder.clearSelectionCalls[0], 1);
}

TEST(CreateAction, GetNameIsStable)
{
    auto action = mmake<CreateAction>();
    EXPECT_EQ(action->GetName(), String("Create objects"));
}
