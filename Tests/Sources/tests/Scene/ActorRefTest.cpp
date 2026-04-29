#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/ActorLinkRef.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/ComponentLinkRef.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Ref<Actor> =====

TEST(ActorRef, RefKeepsActorAlive)
{
    SceneCleanGuard guard;
    WeakRef<Actor> weak;
    Ref<Actor> strong = mmake<Actor>(ActorCreateMode::InScene);
    weak = strong;
    EXPECT_FALSE(weak.IsExpired());
    TickFrame();

    strong = nullptr;
    o2Scene.Clear(true);
    TickFrames(3);

    EXPECT_TRUE(weak.IsExpired());
}

TEST(ActorRef, WeakRefBecomesInvalidAfterDestroy)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    WeakRef<Actor> weak = a;
    TickFrame();

    a->Destroy();
    a = nullptr;
    TickFrames(2);

    EXPECT_FALSE(weak.IsValid());
}

// ===== LinkRef<Actor> =====

TEST(ActorRef, ActorLinkRefSetGetRoundTrip)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);

    LinkRef<Actor> link(a);
    EXPECT_EQ(link.Get(), a.Get());
    EXPECT_TRUE(link.IsValid());
}

TEST(ActorRef, ActorLinkRefDefaultIsNull)
{
    LinkRef<Actor> link;
    EXPECT_EQ(link.Get(), nullptr);
    EXPECT_FALSE(link.IsValid());
}

TEST(ActorRef, ActorLinkRefKeepsActorAliveAfterDestroyMark)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    LinkRef<Actor> link(a);
    TickFrame();

    Actor* rawBefore = a.Get();
    a->Destroy();
    a = nullptr;
    TickFrames(2);

    // LinkRef holds a strong reference — actor stays alive while link exists
    EXPECT_EQ(link.Get(), rawBefore);
}

TEST(ActorRef, ActorLinkRefAssignNullClearsReference)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    LinkRef<Actor> link(a);
    EXPECT_TRUE(link.IsValid());

    link.Set(nullptr);
    EXPECT_FALSE(link.IsValid());
}

TEST(ActorRef, ActorLinkRefCopyKeepsReference)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    LinkRef<Actor> link(a);

    LinkRef<Actor> copy = link;
    EXPECT_EQ(copy.Get(), a.Get());
}

// ===== LinkRef<Component> =====

TEST(ActorRef, ComponentLinkRefSetGetRoundTrip)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();

    LinkRef<TestComponent> link(comp);
    EXPECT_EQ(link.Get(), comp.Get());
}

TEST(ActorRef, ComponentLinkRefKeepsComponentAliveAfterRemove)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();
    LinkRef<TestComponent> link(comp);
    TickFrame();

    Component* before = comp.Get();
    a->RemoveComponent(comp);
    comp = nullptr;
    TickFrames(2);

    // LinkRef holds a strong reference — component stays alive while link exists
    EXPECT_EQ(link.Get(), before);
}

TEST(ActorRef, WeakRefToComponentBecomesNullAfterRemove)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();
    WeakRef<TestComponent> weak = comp;
    TickFrame();

    a->RemoveComponent(comp);
    comp = nullptr;
    TickFrames(2);

    EXPECT_TRUE(weak.IsExpired());
}

// ===== Identity =====

TEST(ActorRef, TwoLinkRefsToSameActorAreEqual)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    LinkRef<Actor> l1(a);
    LinkRef<Actor> l2(a);
    EXPECT_TRUE(l1 == l2);
}

TEST(ActorRef, LinkRefsToDifferentActorsAreNotEqual)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto b = mmake<Actor>(ActorCreateMode::InScene);
    LinkRef<Actor> l1(a);
    LinkRef<Actor> l2(b);
    EXPECT_TRUE(l1 != l2);
}
