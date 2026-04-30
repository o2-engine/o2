#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Actions/Select.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(SelectAction, CtorCapturesObjectIds)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();
    auto c = MakeActor();

    auto action = mmake<SelectAction>(AsEditable({a, b}), AsEditable({c}),
                                      [](const Vector<SceneUID>&) {});

    ASSERT_EQ(action->selectedObjectsIds.Count(), 2);
    EXPECT_EQ(action->selectedObjectsIds[0], a->GetID());
    EXPECT_EQ(action->selectedObjectsIds[1], b->GetID());

    ASSERT_EQ(action->prevSelectedObjectsIds.Count(), 1);
    EXPECT_EQ(action->prevSelectedObjectsIds[0], c->GetID());
}

TEST(SelectAction, RedoInvokesApplyWithSelectedIds)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();

    Vector<SceneUID> applied;
    auto action = mmake<SelectAction>(AsEditable({a, b}), AsEditable({}),
                                      [&applied](const Vector<SceneUID>& ids) { applied = ids; });

    action->Redo();

    ASSERT_EQ(applied.Count(), 2);
    EXPECT_EQ(applied[0], a->GetID());
    EXPECT_EQ(applied[1], b->GetID());
}

TEST(SelectAction, UndoInvokesApplyWithPrevIds)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();
    auto c = MakeActor();

    Vector<SceneUID> applied;
    auto action = mmake<SelectAction>(AsEditable({a}), AsEditable({b, c}),
                                      [&applied](const Vector<SceneUID>& ids) { applied = ids; });

    action->Undo();

    ASSERT_EQ(applied.Count(), 2);
    EXPECT_EQ(applied[0], b->GetID());
    EXPECT_EQ(applied[1], c->GetID());
}

TEST(SelectAction, RedoUndoRoundTripPingsCallback)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();

    Vector<Vector<SceneUID>> log;
    auto action = mmake<SelectAction>(AsEditable({a}), AsEditable({b}),
                                      [&log](const Vector<SceneUID>& ids) { log.Add(ids); });

    action->Redo();
    action->Undo();
    action->Redo();

    ASSERT_EQ(log.Count(), 3);
    EXPECT_EQ(log[0][0], a->GetID());
    EXPECT_EQ(log[1][0], b->GetID());
    EXPECT_EQ(log[2][0], a->GetID());
}

TEST(SelectAction, NullCallbackIsNoOp)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();

    auto action = mmake<SelectAction>();
    action->selectedObjectsIds = { a->GetID() };
    action->prevSelectedObjectsIds = { b->GetID() };

    action->Redo();
    action->Undo();
}
