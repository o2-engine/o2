#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// NotInScene→AddToScene tests live in the rendered tier because they touched a
// teardown bug specific to the headless tier (Application init without Render).
// They pass with full Application::Initialize.

TEST(Actor, AddToSceneSetsIsOnScene)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::NotInScene);
    EXPECT_FALSE(a->IsOnScene());

    a->AddToScene();
    TickFrame();
    EXPECT_TRUE(a->IsOnScene());
}

TEST(ActorComponents, ComponentReceivesOnAddToSceneWhenActorAdded)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::NotInScene);
    auto comp = a->AddComponent<TestComponent>();

    EXPECT_EQ(comp->onAddToSceneCount, 0);

    a->AddToScene();
    TickFrame();

    EXPECT_GT(comp->onAddToSceneCount, 0);
}
