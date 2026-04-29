#include "o2/stdafx.h"

#if IS_EDITOR

#include <gtest/gtest.h>

#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Scene/Actor.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    bool VecNearTest(const Vec2F& a, const Vec2F& b, float eps = 0.001f)
    {
        return Math::Abs(a.x - b.x) < eps && Math::Abs(a.y - b.y) < eps;
    }
}

// ===== MakePrototype =====

TEST(Prototype, MakePrototypeReturnsAssetAndSetsPrototype)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->SetName("hero_to_protoize");

    auto asset = a->MakePrototype();

    ASSERT_TRUE(asset);
    EXPECT_EQ(a->GetPrototype(), asset);
    EXPECT_EQ(a->GetPrototypeDirectly(), asset);
    EXPECT_TRUE(a->GetPrototypeLink());
}

TEST(Prototype, MakePrototypeCreatesActorClone)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->SetName("clone_check");
    a->transform->SetPosition(Vec2F(15, 25));

    auto asset = a->MakePrototype();
    auto protoActor = asset->GetActor();

    ASSERT_TRUE(protoActor);
    EXPECT_EQ(protoActor->GetName(), "clone_check");
    EXPECT_TRUE(VecNearTest(protoActor->transform->GetPosition(), Vec2F(15, 25)));
}

TEST(Prototype, GetPrototypeDirectlyReturnsNullForChild)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    a->AddChild(child);

    a->MakePrototype();

    // Child of a prototype-linked actor doesn't have its own prototype directly
    // (but inherits via GetPrototype if API does parent-walk)
    EXPECT_FALSE(child->GetPrototypeDirectly());
}

// ===== IsLinkedToActor =====

TEST(Prototype, IsLinkedToActorMatchesPrototypeLink)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto asset = a->MakePrototype();
    auto protoActor = asset->GetActor();

    EXPECT_TRUE(a->IsLinkedToActor(protoActor));
}

// ===== BreakPrototypeLink =====

TEST(Prototype, BreakPrototypeLinkClearsLinkAndPrototype)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->MakePrototype();
    EXPECT_TRUE(a->GetPrototypeDirectly());

    a->BreakPrototypeLink();
    EXPECT_FALSE(a->GetPrototypeDirectly());
}

TEST(Prototype, BreakPrototypeLinkRecursivelyBreaksChildren)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    a->AddChild(child);

    a->MakePrototype();

    EXPECT_TRUE(child->GetPrototypeLink());

    a->BreakPrototypeLink();
    EXPECT_FALSE(child->GetPrototypeLink());
}

// ===== Component prototype links =====

TEST(Prototype, MakePrototypeLinksComponents)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();

    a->MakePrototype();

    EXPECT_TRUE(comp->GetPrototypeLink().Lock());
}

// ===== RevertToPrototype =====

TEST(Prototype, RevertToPrototypeRestoresName)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->SetName("originalName");

    a->MakePrototype();
    a->SetName("changedName");
    EXPECT_EQ(a->GetName(), "changedName");

    a->RevertToPrototype();
    EXPECT_EQ(a->GetName(), "originalName");
}

#endif // IS_EDITOR
