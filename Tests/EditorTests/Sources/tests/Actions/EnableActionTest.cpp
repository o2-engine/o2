#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/Enable.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(EnableAction, CtorCapturesObjectIdsAndFlag)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();

    auto action = mmake<EnableAction>(AsEditable({a, b}), false);

    ASSERT_EQ(action->objectsIds.Count(), 2);
    EXPECT_EQ(action->objectsIds[0], a->GetID());
    EXPECT_EQ(action->objectsIds[1], b->GetID());
    EXPECT_FALSE(action->enable);
}

TEST(EnableAction, RedoEnables_UndoDisables_SingleActor)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    TickScene();
    a->SetEnabled(false);
    EXPECT_FALSE(a->IsEnabled());

    auto action = mmake<EnableAction>(AsEditable({a}), true);

    action->Redo();
    EXPECT_TRUE(a->IsEnabled());

    action->Undo();
    EXPECT_FALSE(a->IsEnabled());
}

TEST(EnableAction, RedoDisables_UndoEnables_MultipleActors)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();
    auto c = MakeActor();
    TickScene();

    auto action = mmake<EnableAction>(AsEditable({a, b, c}), false);

    action->Redo();
    EXPECT_FALSE(a->IsEnabled());
    EXPECT_FALSE(b->IsEnabled());
    EXPECT_FALSE(c->IsEnabled());

    action->Undo();
    EXPECT_TRUE(a->IsEnabled());
    EXPECT_TRUE(b->IsEnabled());
    EXPECT_TRUE(c->IsEnabled());
}

TEST(EnableAction, MissingObjectIsTolerated)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();
    TickScene();

    auto action = mmake<EnableAction>(AsEditable({a, b}), false);

    o2Scene.DestroyActor(a);
    o2Scene.UpdateDestroyingEntities();

    action->Redo();
    EXPECT_FALSE(b->IsEnabled());

    action->Undo();
    EXPECT_TRUE(b->IsEnabled());
}

TEST(EnableAction, GetName_DependsOnFlag)
{
    auto enable = mmake<EnableAction>(Vector<Ref<SceneEditableObject>>(), true);
    auto disable = mmake<EnableAction>(Vector<Ref<SceneEditableObject>>(), false);

    EXPECT_EQ(enable->GetName(), String("Enable actors"));
    EXPECT_EQ(disable->GetName(), String("Disable actors"));
}
