#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/Reparent.h"
#include "support/EditorTestScene.h"
#include "support/MockActionsUIBridge.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    void Reparent(const Ref<SceneEditableObject>& object,
                  const Ref<SceneEditableObject>& newParent,
                  const Ref<SceneEditableObject>& prevSibling)
    {
        object->SetEditableParent(nullptr);
        if (newParent)
        {
            int idx = newParent->GetEditableChildren().IndexOf(prevSibling) + 1;
            newParent->AddEditableChild(object, idx);
        }
        else
        {
            int idx = o2Scene.GetRootEditableObjects().IndexOf(prevSibling) + 1;
            object->SetIndexInSiblings(idx);
        }
    }
}

TEST(ReparentAction, CtorCapturesPreviousParents)
{
    SceneCleanGuard guard;
    auto parentA = MakeActor();
    auto child = MakeActor();
    child->SetEditableParent(parentA);
    TickScene();

    auto action = mmake<ReparentAction>(AsEditable({ child }));

    ASSERT_EQ(action->objectsInfos.Count(), 1);
    EXPECT_EQ(action->objectsInfos[0].objectId, child->GetID());
    EXPECT_EQ(action->objectsInfos[0].lastParentId, parentA->GetID());
}

TEST(ReparentAction, RedoMovesToNewParent_UndoRestoresOldParent)
{
    SceneCleanGuard guard;
    RecordingActionsUIBridge recorder;
    ScopedActionsUIBridge host(recorder);

    auto parentA = MakeActor();
    auto parentB = MakeActor();
    auto child = MakeActor();
    child->SetEditableParent(parentA);
    TickScene();

    auto editableChild = AsEditable({ child }).First();
    auto editableParentA = AsEditable({ parentA }).First();
    auto editableParentB = AsEditable({ parentB }).First();

    auto action = mmake<ReparentAction>(Vector<Ref<SceneEditableObject>>{ editableChild });

    Reparent(editableChild, editableParentB, Ref<SceneEditableObject>());
    action->ObjectsReparented(editableParentB, Ref<SceneEditableObject>());

    EXPECT_EQ(parentA->GetEditableChildren().Count(), 0);
    EXPECT_EQ(parentB->GetEditableChildren().Count(), 1);

    action->Undo();
    EXPECT_EQ(parentA->GetEditableChildren().Count(), 1);
    EXPECT_EQ(parentB->GetEditableChildren().Count(), 0);

    action->Redo();
    EXPECT_EQ(parentA->GetEditableChildren().Count(), 0);
    EXPECT_EQ(parentB->GetEditableChildren().Count(), 1);

    EXPECT_EQ(recorder.updateTreeViewCalls, 2); // one per Undo + Redo
}

TEST(ReparentAction, ReorderUnderSameParentRoundTrips)
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

    auto editableFirst = AsEditable({ first }).First();
    auto editableThird = AsEditable({ third }).First();
    auto editableParent = AsEditable({ parent }).First();

    auto action = mmake<ReparentAction>(Vector<Ref<SceneEditableObject>>{ editableFirst });

    Reparent(editableFirst, editableParent, editableThird);
    action->ObjectsReparented(editableParent, editableThird);
    TickScene();

    auto children = parent->GetEditableChildren();
    ASSERT_EQ(children.Count(), 3);
    EXPECT_EQ(children[2]->GetID(), first->GetID());

    action->Undo();
    TickScene();
    children = parent->GetEditableChildren();
    ASSERT_EQ(children.Count(), 3);
    EXPECT_EQ(children[0]->GetID(), first->GetID());

    action->Redo();
    TickScene();
    children = parent->GetEditableChildren();
    ASSERT_EQ(children.Count(), 3);
    EXPECT_EQ(children[2]->GetID(), first->GetID());
}

TEST(ReparentAction, MoveFromChildToRoot_RoundTrips)
{
    SceneCleanGuard guard;
    RecordingActionsUIBridge recorder;
    ScopedActionsUIBridge host(recorder);

    auto parent = MakeActor();
    auto child = MakeActor();
    child->SetEditableParent(parent);
    TickScene();

    auto editableChild = AsEditable({ child }).First();

    auto action = mmake<ReparentAction>(Vector<Ref<SceneEditableObject>>{ editableChild });

    Reparent(editableChild, Ref<SceneEditableObject>(), Ref<SceneEditableObject>());
    action->ObjectsReparented(Ref<SceneEditableObject>(), Ref<SceneEditableObject>());

    EXPECT_EQ(parent->GetEditableChildren().Count(), 0);

    action->Undo();
    EXPECT_EQ(parent->GetEditableChildren().Count(), 1);

    action->Redo();
    EXPECT_EQ(parent->GetEditableChildren().Count(), 0);
}

TEST(ReparentAction, RedoAndUndoTriggerTreeViewRefresh)
{
    SceneCleanGuard guard;
    RecordingActionsUIBridge recorder;
    ScopedActionsUIBridge host(recorder);

    auto parent = MakeActor();
    auto child = MakeActor();
    child->SetEditableParent(parent);
    TickScene();

    auto editableChild = AsEditable({ child }).First();
    auto action = mmake<ReparentAction>(Vector<Ref<SceneEditableObject>>{ editableChild });
    action->ObjectsReparented(Ref<SceneEditableObject>(), Ref<SceneEditableObject>());

    action->Redo();
    EXPECT_EQ(recorder.updateTreeViewCalls, 1);

    action->Undo();
    EXPECT_EQ(recorder.updateTreeViewCalls, 2);
}

TEST(ReparentAction, GetNameIsStable)
{
    auto action = mmake<ReparentAction>();
    EXPECT_EQ(action->GetName(), String("Actors rearrange"));
}
