#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/BreakPrototype.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(BreakPrototypeAction, RedoBreaksLinkUndoRestoresIt)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();
    actor->MakePrototype();
    ASSERT_TRUE(actor->GetPrototypeDirectly());

    auto action = mmake<BreakPrototypeAction>(o2::Vector<Ref<Actor>>{ actor });

    action->Redo();
    EXPECT_FALSE(actor->GetPrototypeDirectly());

    action->Undo();
    EXPECT_TRUE(actor->GetPrototypeDirectly());
}

TEST(BreakPrototypeAction, RoundTripsAcrossUndoRedo)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();
    actor->MakePrototype();

    auto action = mmake<BreakPrototypeAction>(o2::Vector<Ref<Actor>>{ actor });

    action->Redo();
    action->Undo();
    action->Redo();
    EXPECT_FALSE(actor->GetPrototypeDirectly());

    action->Undo();
    EXPECT_TRUE(actor->GetPrototypeDirectly());
}

TEST(BreakPrototypeAction, ActorsWithoutPrototypeAreIgnored)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();
    ASSERT_FALSE(actor->GetPrototypeDirectly());

    auto action = mmake<BreakPrototypeAction>(o2::Vector<Ref<Actor>>{ actor });
    EXPECT_EQ(action->actors.Count(), 0);

    action->Redo();
    action->Undo();
    EXPECT_FALSE(actor->GetPrototypeDirectly());
}

TEST(BreakPrototypeAction, MissingActorIsTolerated)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();
    actor->MakePrototype();

    auto action = mmake<BreakPrototypeAction>(o2::Vector<Ref<Actor>>{ actor });
    action->Redo();

    o2Scene.DestroyActor(actor);
    actor = nullptr;
    o2Scene.UpdateDestroyingEntities();

    action->Undo();
    action->Redo();
}

TEST(BreakPrototypeAction, GetNameIsStable)
{
    auto action = mmake<BreakPrototypeAction>();
    EXPECT_EQ(action->GetName(), String("Break prototype link"));
}
