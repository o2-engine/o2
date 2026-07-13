#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Add/Remove =====

TEST(ActorComponents, AddComponentTemplateAttachesAndOwns)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();

    ASSERT_TRUE(comp);
    EXPECT_EQ(a->GetComponents().Count(), 1);
    EXPECT_EQ(comp->GetActor(), a);
}

TEST(ActorComponents, AddComponentRefAttaches)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = mmake<TestComponent>();
    auto added = a->AddComponent(comp);

    EXPECT_EQ(added, comp);
    EXPECT_EQ(a->GetComponents().Count(), 1);
    EXPECT_EQ(comp->GetActor(), a);
}

TEST(ActorComponents, RemoveComponentDetaches)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();

    a->RemoveComponent(comp);

    EXPECT_EQ(a->GetComponents().Count(), 0);
    EXPECT_FALSE(comp->GetActor());
}

TEST(ActorComponents, RemoveAllComponentsClearsList)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->AddComponent<TestComponent>();
    a->AddComponent<TestComponent2>();

    a->RemoveAllComponents();

    EXPECT_EQ(a->GetComponents().Count(), 0);
}

// ===== Lookup =====

TEST(ActorComponents, GetComponentByTypeReturnsAttached)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto first = a->AddComponent<TestComponent>();
    auto second = a->AddComponent<TestComponent2>();

    EXPECT_EQ(a->GetComponent<TestComponent>(), first);
    EXPECT_EQ(a->GetComponent<TestComponent2>(), second);
}

TEST(ActorComponents, GetComponentByTypePtrReturnsAttached)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();

    auto byType = a->GetComponent(&TypeOf(TestComponent));
    EXPECT_EQ(byType, comp);
}

TEST(ActorComponents, GetComponentBySceneUIDReturnsAttached)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();

    auto byId = a->GetComponent(comp->GetID());
    EXPECT_EQ(byId, comp);
}

TEST(ActorComponents, GetComponentMissingReturnsNull)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    EXPECT_FALSE(a->GetComponent<TestComponent>());
}

TEST(ActorComponents, GetComponentInChildrenSearchesRecursively)
{
    SceneCleanGuard guard;
    auto root = mmake<Actor>(ActorCreateMode::InScene);
    auto leaf = mmake<Actor>(ActorCreateMode::InScene);
    root->AddChild(leaf);
    auto comp = leaf->AddComponent<TestComponent>();

    EXPECT_EQ(root->GetComponentInChildren<TestComponent>(), comp);
}

TEST(ActorComponents, GetComponentsInChildrenCollectsAll)
{
    SceneCleanGuard guard;
    auto root = mmake<Actor>(ActorCreateMode::InScene);
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto b = mmake<Actor>(ActorCreateMode::InScene);
    root->AddChild(a);
    root->AddChild(b);

    auto rootComp = root->AddComponent<TestComponent>();
    auto aComp = a->AddComponent<TestComponent>();
    auto bComp = b->AddComponent<TestComponent>();

    auto all = root->GetComponentsInChildren<TestComponent>();

    EXPECT_EQ(all.Count(), 3);
    EXPECT_TRUE(all.Contains(rootComp));
    EXPECT_TRUE(all.Contains(aComp));
    EXPECT_TRUE(all.Contains(bComp));
}

// ===== Lifecycle =====

TEST(ActorComponents, NewComponentReceivesOnInitialized)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();
    TickFrame();
    EXPECT_GT(comp->onInitializedCount, 0);
}

// ActorComponents.ComponentReceivesOnAddToSceneWhenActorAdded moved to
// Rendered/Actor/AddToSceneTest.cpp — same NotInScene→AddToScene crash pattern.

TEST(ActorComponents, ComponentReceivesOnTransformUpdated)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();
    TickFrame();

    int base = comp->onTransformUpdatedCount;
    a->transform->SetPosition2D(Vec2F(10, 0));
    TickFrame();

    EXPECT_GT(comp->onTransformUpdatedCount, base);
}

TEST(ActorComponents, ComponentReceivesOnParentChangedWhenActorMoves)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    TickFrame();

    int base = comp->onParentChangedCount;
    a->SetParent(parent);
    TickFrame();

    EXPECT_GT(comp->onParentChangedCount, base);
}

TEST(ActorComponents, ComponentReceivesOnComponentAddedFromOtherComponent)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto witness = a->AddComponent<TestComponent>();
    TickFrame();

    int base = witness->onComponentAddedCount;
    a->AddComponent<TestComponent2>();
    TickFrame();

    EXPECT_GT(witness->onComponentAddedCount, base);
}

TEST(ActorComponents, SetComponentEnabledTogglesIsEnabled)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();
    TickFrame();

    EXPECT_TRUE(comp->IsEnabled());
    comp->SetEnabled(false);
    EXPECT_FALSE(comp->IsEnabled());
    comp->Enable();
    EXPECT_TRUE(comp->IsEnabled());
}
