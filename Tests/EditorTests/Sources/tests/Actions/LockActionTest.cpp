#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/Lock.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(LockAction, CtorCapturesObjectIdsAndFlag)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();

    auto action = mmake<LockAction>(AsEditable({a, b}), true);

    ASSERT_EQ(action->objectsIds.Count(), 2);
    EXPECT_EQ(action->objectsIds[0], a->GetID());
    EXPECT_EQ(action->objectsIds[1], b->GetID());
    EXPECT_TRUE(action->lock);
}

TEST(LockAction, RedoLocks_UndoUnlocks_SingleActor)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    TickScene();
    EXPECT_FALSE(a->IsLocked());

    auto action = mmake<LockAction>(AsEditable({a}), true);

    action->Redo();
    EXPECT_TRUE(a->IsLocked());

    action->Undo();
    EXPECT_FALSE(a->IsLocked());
}

TEST(LockAction, RedoUnlocks_UndoLocks_MultipleActors)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();
    auto c = MakeActor();
    TickScene();

    a->SetLocked(true);
    b->SetLocked(true);
    c->SetLocked(true);

    auto action = mmake<LockAction>(AsEditable({a, b, c}), false);

    action->Redo();
    EXPECT_FALSE(a->IsLocked());
    EXPECT_FALSE(b->IsLocked());
    EXPECT_FALSE(c->IsLocked());

    action->Undo();
    EXPECT_TRUE(a->IsLocked());
    EXPECT_TRUE(b->IsLocked());
    EXPECT_TRUE(c->IsLocked());
}

TEST(LockAction, MissingObjectIsTolerated)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();
    TickScene();

    auto action = mmake<LockAction>(AsEditable({a, b}), true);

    o2Scene.DestroyActor(a);
    o2Scene.UpdateDestroyingEntities();

    action->Redo();
    EXPECT_TRUE(b->IsLocked());

    action->Undo();
    EXPECT_FALSE(b->IsLocked());
}

TEST(LockAction, GetName_DependsOnFlag)
{
    auto lock = mmake<LockAction>(Vector<Ref<SceneEditableObject>>(), true);
    auto unlock = mmake<LockAction>(Vector<Ref<SceneEditableObject>>(), false);

    EXPECT_EQ(lock->GetName(), String("Lock actors"));
    EXPECT_EQ(unlock->GetName(), String("Unlock actors"));
}
