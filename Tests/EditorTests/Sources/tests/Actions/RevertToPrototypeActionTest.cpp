#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/RevertToPrototype.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(RevertToPrototypeAction, RedoRevertsUndoRestoresLocalChange)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    actor->SetName("originalName");
    TickScene();
    actor->MakePrototype();

    actor->SetName("changedName");
    ASSERT_EQ(actor->GetName(), String("changedName"));

    auto action = mmake<RevertToPrototypeAction>(o2::Vector<Ref<Actor>>{ actor });

    action->Redo();
    EXPECT_EQ(actor->GetName(), String("originalName"));

    action->Undo();
    EXPECT_EQ(actor->GetName(), String("changedName"));
}

TEST(RevertToPrototypeAction, RoundTripsAcrossUndoRedo)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    actor->SetName("originalName");
    TickScene();
    actor->MakePrototype();
    actor->SetName("changedName");

    auto action = mmake<RevertToPrototypeAction>(o2::Vector<Ref<Actor>>{ actor });

    action->Redo();
    action->Undo();
    EXPECT_EQ(actor->GetName(), String("changedName"));

    action->Redo();
    EXPECT_EQ(actor->GetName(), String("originalName"));
}

TEST(RevertToPrototypeAction, ActorsWithoutPrototypeAreIgnored)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    actor->SetName("name");
    TickScene();

    auto action = mmake<RevertToPrototypeAction>(o2::Vector<Ref<Actor>>{ actor });
    EXPECT_EQ(action->actors.Count(), 0);

    action->Redo();
    action->Undo();
    EXPECT_EQ(actor->GetName(), String("name"));
}

TEST(RevertToPrototypeAction, MissingActorIsTolerated)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();
    actor->MakePrototype();
    actor->SetName("changedName");

    auto action = mmake<RevertToPrototypeAction>(o2::Vector<Ref<Actor>>{ actor });
    action->Redo();

    o2Scene.DestroyActor(actor);
    actor = nullptr;
    o2Scene.UpdateDestroyingEntities();

    action->Undo();
    action->Redo();
}

TEST(RevertToPrototypeAction, GetNameIsStable)
{
    auto action = mmake<RevertToPrototypeAction>();
    EXPECT_EQ(action->GetName(), String("Revert to prototype"));
}
