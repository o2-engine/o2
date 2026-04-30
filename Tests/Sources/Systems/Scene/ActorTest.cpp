#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(Actor, DefaultConstructionAssignsUniqueIds)
{
    SceneCleanGuard guard;

    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto b = mmake<Actor>(ActorCreateMode::InScene);

    EXPECT_NE(a->GetID(), b->GetID());
}

TEST(Actor, DefaultNameIsNotEmpty)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    EXPECT_FALSE(a->GetName().IsEmpty());
}

TEST(Actor, NotInSceneActorIsNotOnScene)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::NotInScene);
    EXPECT_FALSE(a->IsOnScene());
}

TEST(Actor, InSceneActorIsOnSceneAfterTick)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    TickFrame();
    EXPECT_TRUE(a->IsOnScene());
}

TEST(Actor, ConstructorWithComponentsAddsThem)
{
    SceneCleanGuard guard;

    Vector<Ref<Component>> components;
    auto comp1 = mmake<TestComponent>();
    auto comp2 = mmake<TestComponent2>();
    components.Add(comp1);
    components.Add(comp2);

    auto a = mmake<Actor>(components, ActorCreateMode::InScene);

    EXPECT_EQ(a->GetComponents().Count(), 2);
    EXPECT_EQ(a->GetComponent<TestComponent>(), comp1);
    EXPECT_EQ(a->GetComponent<TestComponent2>(), comp2);
}

// ===== Naming and ID =====

TEST(Actor, SetNameRoundTrip)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->SetName("hero");
    EXPECT_EQ(a->GetName(), "hero");
}

TEST(Actor, GenerateNewIdChangesId)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto oldId = a->GetID();
    a->GenerateNewID(false);
    EXPECT_NE(a->GetID(), oldId);
}

TEST(Actor, GenerateNewIdWithChildrenAlsoChangesChildIds)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);

    auto oldChildId = child->GetID();
    parent->GenerateNewID(true);
    EXPECT_NE(child->GetID(), oldChildId);
}

TEST(Actor, GenerateNewIdWithoutChildrenKeepsChildIds)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);

    auto oldChildId = child->GetID();
    parent->GenerateNewID(false);
    EXPECT_EQ(child->GetID(), oldChildId);
}

// ===== Hierarchy =====

TEST(Actor, AddChildSetsParent)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);

    parent->AddChild(child);

    EXPECT_EQ(child->GetParent().Lock(), parent);
    EXPECT_EQ(parent->GetChildren().Count(), 1);
    EXPECT_EQ(parent->GetChildren()[0], child);
}

TEST(Actor, AddChildAtIndexInsertsAtPosition)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto b = mmake<Actor>(ActorCreateMode::InScene);
    auto c = mmake<Actor>(ActorCreateMode::InScene);

    parent->AddChild(a);
    parent->AddChild(b);
    parent->AddChild(c, 1);

    ASSERT_EQ(parent->GetChildren().Count(), 3);
    EXPECT_EQ(parent->GetChildren()[0], a);
    EXPECT_EQ(parent->GetChildren()[1], c);
    EXPECT_EQ(parent->GetChildren()[2], b);
}

TEST(Actor, AddChildrenAddsAll)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    Vector<Ref<Actor>> kids;
    kids.Add(mmake<Actor>(ActorCreateMode::InScene));
    kids.Add(mmake<Actor>(ActorCreateMode::InScene));
    kids.Add(mmake<Actor>(ActorCreateMode::InScene));

    parent->AddChildren(kids);

    EXPECT_EQ(parent->GetChildren().Count(), 3);
}

TEST(Actor, RemoveChildClearsParent)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);

    parent->AddChild(child);
    parent->RemoveChild(child);

    EXPECT_EQ(parent->GetChildren().Count(), 0);
    EXPECT_FALSE(child->GetParent().IsValid());
}

TEST(Actor, RemoveAllChildrenEmpty)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(mmake<Actor>(ActorCreateMode::InScene));
    parent->AddChild(mmake<Actor>(ActorCreateMode::InScene));

    parent->RemoveAllChildren();

    EXPECT_EQ(parent->GetChildren().Count(), 0);
}

TEST(Actor, SetParentMovesActor)
{
    SceneCleanGuard guard;
    auto p1 = mmake<Actor>(ActorCreateMode::InScene);
    auto p2 = mmake<Actor>(ActorCreateMode::InScene);
    auto c = mmake<Actor>(ActorCreateMode::InScene);

    p1->AddChild(c);
    EXPECT_EQ(p1->GetChildren().Count(), 1);

    c->SetParent(p2);
    EXPECT_EQ(p1->GetChildren().Count(), 0);
    EXPECT_EQ(p2->GetChildren().Count(), 1);
    EXPECT_EQ(c->GetParent().Lock(), p2);
}

TEST(Actor, SetIndexInSiblingsReorders)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto b = mmake<Actor>(ActorCreateMode::InScene);
    auto c = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(a);
    parent->AddChild(b);
    parent->AddChild(c);

    c->SetIndexInSiblings(0);

    EXPECT_EQ(parent->GetChildren()[0], c);
    EXPECT_EQ(parent->GetChildren()[1], a);
    EXPECT_EQ(parent->GetChildren()[2], b);
}

// ===== Search =====

TEST(Actor, FindChildReturnsByName)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->SetName("alpha");
    auto b = mmake<Actor>(ActorCreateMode::InScene);
    b->SetName("beta");
    parent->AddChild(a);
    parent->AddChild(b);

    EXPECT_EQ(parent->FindChild("alpha"), a);
    EXPECT_EQ(parent->FindChild("beta"), b);
    EXPECT_FALSE(parent->FindChild("gamma"));
}

TEST(Actor, GetChildResolvesPath)
{
    SceneCleanGuard guard;
    auto root = mmake<Actor>(ActorCreateMode::InScene);
    root->SetName("root");
    auto level1 = mmake<Actor>(ActorCreateMode::InScene);
    level1->SetName("level1");
    auto level2 = mmake<Actor>(ActorCreateMode::InScene);
    level2->SetName("level2");

    root->AddChild(level1);
    level1->AddChild(level2);

    EXPECT_EQ(root->GetChild("level1/level2"), level2);
}

TEST(Actor, FindActorByIdRecursivelySearchesChildren)
{
    SceneCleanGuard guard;
    auto root = mmake<Actor>(ActorCreateMode::InScene);
    auto level1 = mmake<Actor>(ActorCreateMode::InScene);
    auto level2 = mmake<Actor>(ActorCreateMode::InScene);
    root->AddChild(level1);
    level1->AddChild(level2);

    auto found = root->FindActorById(level2->GetID());
    EXPECT_EQ(found, level2);
}

TEST(Actor, GetAllChildrenActorsCollectsRecursively)
{
    SceneCleanGuard guard;
    auto root = mmake<Actor>(ActorCreateMode::InScene);
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto b = mmake<Actor>(ActorCreateMode::InScene);
    auto c = mmake<Actor>(ActorCreateMode::InScene);
    root->AddChild(a);
    a->AddChild(b);
    b->AddChild(c);

    Vector<Ref<Actor>> all;
    root->GetAllChildrenActors(all);

    EXPECT_GE(all.Count(), 3);
    EXPECT_TRUE(all.Contains(a));
    EXPECT_TRUE(all.Contains(b));
    EXPECT_TRUE(all.Contains(c));
}

// ===== Enable/Disable =====

TEST(Actor, SetEnabledTogglesIsEnabled)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    EXPECT_TRUE(a->IsEnabled());

    a->SetEnabled(false);
    EXPECT_FALSE(a->IsEnabled());

    a->SetEnabled(true);
    EXPECT_TRUE(a->IsEnabled());
}

TEST(Actor, DisabledParentDisablesChildInHierarchy)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);

    TickFrame();
    EXPECT_TRUE(child->IsEnabledInHierarchy());

    parent->SetEnabled(false);
    TickFrame();
    EXPECT_FALSE(child->IsEnabledInHierarchy());
    EXPECT_TRUE(child->IsEnabled()); // logically still enabled

    parent->SetEnabled(true);
    TickFrame();
    EXPECT_TRUE(child->IsEnabledInHierarchy());
}

TEST(Actor, EnableDisableHelpersChangeState)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);

    a->Disable();
    EXPECT_FALSE(a->IsEnabled());

    a->Enable();
    EXPECT_TRUE(a->IsEnabled());
}

// ===== Scene membership =====

TEST(Actor, AddedActorAppearsInRootActorsAfterTick)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->SetName("uniqueRootName_42");

    TickFrame();

    bool found = false;
    for (auto& root : o2Scene.GetRootActors())
    {
        if (root == a) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(Actor, ChildIsNotInRootActors)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);

    TickFrame();

    bool foundAsRoot = false;
    for (auto& root : o2Scene.GetRootActors())
    {
        if (root == child) { foundAsRoot = true; break; }
    }
    EXPECT_FALSE(foundAsRoot);
}

TEST(Actor, RemoveFromSceneClearsIsOnScene)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    TickFrame();
    EXPECT_TRUE(a->IsOnScene());

    a->RemoveFromScene();
    EXPECT_FALSE(a->IsOnScene());
}

// Actor.AddToSceneSetsIsOnScene moved to Rendered/Actor/AddToSceneTest.cpp — the
// NotInScene→AddToScene path crashes on isolated process exit in headless mode.

// ===== Lifecycle callbacks =====

TEST(Actor, ComponentReceivesOnStartOnceAfterFirstTick)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();

    EXPECT_EQ(comp->onStartCount, 0);

    TickFrame();
    EXPECT_EQ(comp->onStartCount, 1);

    TickFrame();
    TickFrame();
    EXPECT_EQ(comp->onStartCount, 1);
}

TEST(Actor, ComponentReceivesOnUpdateEachTick)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();

    TickFrame();
    int afterFirst = comp->onUpdateCount;
    TickFrame();
    TickFrame();

    EXPECT_GT(comp->onUpdateCount, afterFirst);
}

TEST(Actor, ComponentOnEnabledFiresWhenActorEnabled)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();

    TickFrame();
    int baseEnabled = comp->onEnabledCount;

    a->SetEnabled(false);
    TickFrame();
    EXPECT_GT(comp->onDisabledCount, 0);

    a->SetEnabled(true);
    TickFrame();
    EXPECT_GT(comp->onEnabledCount, baseEnabled);
}

TEST(Actor, DestroyEventuallyRemovesActor)
{
    SceneCleanGuard guard;
    WeakRef<Actor> weak;
    {
        auto a = mmake<Actor>(ActorCreateMode::InScene);
        weak = a;
        TickFrame();
        a->Destroy();
    }
    TickFrame();
    EXPECT_FALSE(weak.IsValid());
}

TEST(Actor, DestroyTriggersOnRemoveFromSceneOnComponents)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();

    TickFrame();
    int baseRemove = comp->onRemoveFromSceneCount;

    a->Destroy();
    TickFrame();

    EXPECT_GT(comp->onRemoveFromSceneCount, baseRemove);
}

TEST(Actor, DestroyOnlyTargetActor)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);

    WeakRef<Actor> childWeak = child;
    auto childRef = child;
    child = nullptr;

    TickFrame();

    childRef->Destroy();
    childRef = nullptr;
    TickFrames(2);

    EXPECT_FALSE(childWeak.IsValid());
}

// ===== Layer =====

TEST(Actor, NewActorHasNoExplicitLayer)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    EXPECT_FALSE(a->GetLayer().IsValid());
}

TEST(Actor, SetLayerByNameChangesLayer)
{
    SceneCleanGuard guard;
    auto layer = o2Scene.AddLayer("custom_test_layer_42");
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->SetLayer(layer);
    EXPECT_EQ(a->GetLayer(), layer);
}
