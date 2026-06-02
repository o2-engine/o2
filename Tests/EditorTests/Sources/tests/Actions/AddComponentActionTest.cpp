#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Components/EditorTestComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Actions/AddComponent.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    DataDocument TestComponentData()
    {
        DataDocument data;
        data = DynamicCast<Component>(mmake<EditorTestComponent>());
        return data;
    }

    Ref<AddComponentAction> MakeAction(const o2::Vector<Ref<Actor>>& actors)
    {
        return mmake<AddComponentAction>(actors, TestComponentData());
    }
}

TEST(AddComponentAction, RedoAddsComponentUndoRemovesIt)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();
    ASSERT_EQ(actor->GetComponents().Count(), 0);

    auto action = MakeAction({ actor });

    action->Redo();
    ASSERT_EQ(actor->GetComponents().Count(), 1);
    EXPECT_EQ(&actor->GetComponents()[0]->GetType(), &TypeOf(EditorTestComponent));

    action->Undo();
    EXPECT_EQ(actor->GetComponents().Count(), 0);
}

TEST(AddComponentAction, RoundTripsAcrossUndoRedo)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();

    auto action = MakeAction({ actor });

    action->Redo();
    action->Undo();
    action->Redo();
    EXPECT_EQ(actor->GetComponents().Count(), 1);

    action->Undo();
    EXPECT_EQ(actor->GetComponents().Count(), 0);
}

TEST(AddComponentAction, AddsToEveryTargetActor)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();
    TickScene();

    auto action = MakeAction({ a, b });

    action->Redo();
    EXPECT_EQ(a->GetComponents().Count(), 1);
    EXPECT_EQ(b->GetComponents().Count(), 1);

    action->Undo();
    EXPECT_EQ(a->GetComponents().Count(), 0);
    EXPECT_EQ(b->GetComponents().Count(), 0);
}

TEST(AddComponentAction, MissingActorIsTolerated)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();

    auto action = MakeAction({ actor });
    action->Redo();

    o2Scene.DestroyActor(actor);
    actor = nullptr;
    o2Scene.UpdateDestroyingEntities();

    action->Undo();
    action->Redo();
}

TEST(AddComponentAction, GetNameIsStable)
{
    auto action = mmake<AddComponentAction>();
    EXPECT_EQ(action->GetName(), String("Add component"));
}
