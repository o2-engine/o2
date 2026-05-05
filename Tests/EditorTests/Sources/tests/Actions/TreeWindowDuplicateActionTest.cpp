#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/Create.h"
#include "support/EditorTestScene.h"
#include "support/MockActionsUIBridge.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Ref<SceneEditableObject> DuplicateAndWrap(const Ref<Actor>& source)
    {
        auto copy = source->CloneAsRef<SceneEditableObject>();
        copy->GenerateNewID();
        copy->SetEditableParent(source->GetEditableParent());
        return copy;
    }
}

TEST(TreeWindowDuplicate, RedoReinsertsCloneAfterRemoval)
{
    SceneCleanGuard guard;
    RecordingActionsUIBridge recorder;
    ScopedActionsUIBridge host(recorder);

    auto parent = MakeActor();
    auto source = MakeActor();
    source->SetEditableParent(parent);
    TickScene();

    auto clone = DuplicateAndWrap(source);
    SceneUID cloneId = clone->GetID();
    auto editableParent = AsEditable({ parent }).First();
    auto editableSource = AsEditable({ source }).First();

    ASSERT_EQ(parent->GetEditableChildren().Count(), 2);
    EXPECT_NE(cloneId, source->GetID());

    auto action = mmake<CreateAction>(Vector<Ref<SceneEditableObject>>{ clone },
                                      editableParent, editableSource);

    if (auto cloneActor = DynamicCast<Actor>(clone))
        o2Scene.DestroyActor(cloneActor);
    clone = nullptr;
    o2Scene.UpdateDestroyingEntities();
    EXPECT_EQ(parent->GetEditableChildren().Count(), 1);

    action->Redo();

    EXPECT_EQ(parent->GetEditableChildren().Count(), 2);
    ASSERT_EQ(recorder.selectObjectsCalls.Count(), 1);
    EXPECT_EQ(recorder.selectObjectsCalls[0].objectIds[0], cloneId);
}

TEST(TreeWindowDuplicate, CloneGetsDistinctId)
{
    SceneCleanGuard guard;
    auto source = MakeActor();
    TickScene();
    auto clone = DuplicateAndWrap(source);

    EXPECT_NE(clone->GetID(), source->GetID());
}

TEST(TreeWindowDuplicate, MultipleClonesShareSingleAction)
{
    SceneCleanGuard guard;
    auto parent = MakeActor();
    auto src1 = MakeActor();
    src1->SetEditableParent(parent);
    auto src2 = MakeActor();
    src2->SetEditableParent(parent);
    TickScene();

    auto clone1 = DuplicateAndWrap(src1);
    auto clone2 = DuplicateAndWrap(src2);

    auto editableParent = AsEditable({ parent }).First();
    auto action = mmake<CreateAction>(Vector<Ref<SceneEditableObject>>{ clone1, clone2 },
                                      editableParent, AsEditable({ src2 }).First());

    ASSERT_EQ(action->objectsIds.Count(), 2);
    EXPECT_EQ(action->objectsIds[0], clone1->GetID());
    EXPECT_EQ(action->objectsIds[1], clone2->GetID());
    EXPECT_EQ(action->insertParentId, parent->GetID());
    EXPECT_EQ(action->insertPrevObjectId, src2->GetID());
}
