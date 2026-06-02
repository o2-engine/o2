#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Components/EditorTestComponent.h"
#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/RemoveComponent.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Ref<Component> AddTestComponent(const Ref<Actor>& actor)
    {
        Ref<Component> component = mmake<EditorTestComponent>();
        actor->AddComponent(component);
        TickScene();
        return component;
    }
}

TEST(RemoveComponentAction, RedoRemovesComponentUndoRestoresIt)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();
    auto component = AddTestComponent(actor);
    ASSERT_EQ(actor->GetComponents().Count(), 1);

    auto action = mmake<RemoveComponentAction>(o2::Vector<Ref<Component>>{ component });

    action->Redo();
    EXPECT_EQ(actor->GetComponents().Count(), 0);

    action->Undo();
    ASSERT_EQ(actor->GetComponents().Count(), 1);
    EXPECT_EQ(&actor->GetComponents()[0]->GetType(), &TypeOf(EditorTestComponent));
}

TEST(RemoveComponentAction, RoundTripsAcrossUndoRedo)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();
    auto component = AddTestComponent(actor);

    auto action = mmake<RemoveComponentAction>(o2::Vector<Ref<Component>>{ component });

    action->Redo();
    action->Undo();
    action->Redo();
    EXPECT_EQ(actor->GetComponents().Count(), 0);

    action->Undo();
    EXPECT_EQ(actor->GetComponents().Count(), 1);
}

TEST(RemoveComponentAction, RemovesSeveralComponents)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();
    auto first = AddTestComponent(actor);
    auto second = AddTestComponent(actor);
    ASSERT_EQ(actor->GetComponents().Count(), 2);

    auto action = mmake<RemoveComponentAction>(o2::Vector<Ref<Component>>{ first, second });

    action->Redo();
    EXPECT_EQ(actor->GetComponents().Count(), 0);

    action->Undo();
    EXPECT_EQ(actor->GetComponents().Count(), 2);
}

TEST(RemoveComponentAction, MissingActorIsTolerated)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();
    auto component = AddTestComponent(actor);

    auto action = mmake<RemoveComponentAction>(o2::Vector<Ref<Component>>{ component });
    action->Redo();

    o2Scene.DestroyActor(actor);
    actor = nullptr;
    o2Scene.UpdateDestroyingEntities();

    action->Undo();
    action->Redo();
}

TEST(RemoveComponentAction, GetNameIsStable)
{
    auto action = mmake<RemoveComponentAction>();
    EXPECT_EQ(action->GetName(), String("Remove component"));
}
