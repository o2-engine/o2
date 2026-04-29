#include "o2/stdafx.h"

#if IS_EDITOR

#include <gtest/gtest.h>

#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "o2/Utils/Editor/ActorDifferences.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    Ref<Actor> MakeProtoSibling(const Ref<Actor>& source)
    {
        return mmake<Actor>(source->GetPrototype(), ActorCreateMode::InScene);
    }
}

// ===== Empty diff =====

TEST(ActorDifferences, NoChangesYieldsEmptyDiff)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->SetName("untouched");

    auto asset = a->MakePrototype();
    auto proto = asset->GetActor();

    auto diff = ActorDifferences::GetDifference(a, proto);

    EXPECT_EQ(diff.removedChildren.Count(), 0);
    EXPECT_EQ(diff.newChildren.Count(), 0);
    EXPECT_EQ(diff.movedChildren.Count(), 0);
    EXPECT_EQ(diff.removedComponents.Count(), 0);
    EXPECT_EQ(diff.newComponents.Count(), 0);
    EXPECT_EQ(diff.changedActorFields.Count(), 0);
    EXPECT_EQ(diff.changedComponentFields.Count(), 0);
}

// ===== ChangedActorField =====

TEST(ActorDifferences, ChangedTransformFieldIsDetected)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPosition(Vec2F(0, 0));

    auto asset = a->MakePrototype();
    auto proto = asset->GetActor();

    a->transform->SetPosition(Vec2F(100, 50));

    auto diff = ActorDifferences::GetDifference(a, proto);

    EXPECT_GT(diff.changedActorFields.Count(), 0);
}

// ===== ChangedComponentField =====

TEST(ActorDifferences, ChangedComponentFieldIsDetected)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();
    comp->testValue = 0;

    auto asset = a->MakePrototype();
    auto proto = asset->GetActor();

    comp->testValue = 42;

    auto diff = ActorDifferences::GetDifference(a, proto);

    EXPECT_GT(diff.changedComponentFields.Count(), 0);
}

// ===== NewChild =====

TEST(ActorDifferences, NewChildIsDetected)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);

    auto asset = a->MakePrototype();
    auto proto = asset->GetActor();

    auto newKid = mmake<Actor>(ActorCreateMode::InScene);
    a->AddChild(newKid);

    auto diff = ActorDifferences::GetDifference(a, proto);

    EXPECT_EQ(diff.newChildren.Count(), 1);
    EXPECT_EQ(diff.removedChildren.Count(), 0);
}

// ===== RemovedChild =====

TEST(ActorDifferences, RemovedChildIsDetected)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto kid = mmake<Actor>(ActorCreateMode::InScene);
    a->AddChild(kid);

    auto asset = a->MakePrototype();
    auto proto = asset->GetActor();

    a->RemoveChild(kid);

    auto diff = ActorDifferences::GetDifference(a, proto);

    EXPECT_EQ(diff.removedChildren.Count(), 1);
    EXPECT_EQ(diff.newChildren.Count(), 0);
}

// ===== NewComponent =====

TEST(ActorDifferences, NewComponentIsDetected)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);

    auto asset = a->MakePrototype();
    auto proto = asset->GetActor();

    a->AddComponent<TestComponent>();

    auto diff = ActorDifferences::GetDifference(a, proto);

    EXPECT_EQ(diff.newComponents.Count(), 1);
    EXPECT_EQ(diff.removedComponents.Count(), 0);
}

// ===== RemovedComponent =====

TEST(ActorDifferences, RemovedComponentIsDetected)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();

    auto asset = a->MakePrototype();
    auto proto = asset->GetActor();

    a->RemoveComponent(comp);

    auto diff = ActorDifferences::GetDifference(a, proto);

    EXPECT_EQ(diff.removedComponents.Count(), 1);
    EXPECT_EQ(diff.newComponents.Count(), 0);
}

// ===== Reorder children =====

TEST(ActorDifferences, ReorderChildrenIsNotMoveDiff)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto k1 = mmake<Actor>(ActorCreateMode::InScene);
    auto k2 = mmake<Actor>(ActorCreateMode::InScene);
    a->AddChild(k1);
    a->AddChild(k2);

    auto asset = a->MakePrototype();
    auto proto = asset->GetActor();

    k2->SetIndexInSiblings(0);

    auto diff = ActorDifferences::GetDifference(a, proto);

    EXPECT_EQ(diff.movedChildren.Count(), 0);
}

// ===== GetFieldPath helper =====

TEST(ActorDifferences, GetFieldPathEmptyForEmptyStack)
{
    Vector<const FieldInfo*> stack;
    EXPECT_EQ(ActorDifferences::GetFieldPath(stack), "");
}

// ===== Apply round-trip =====

namespace
{
    // Apply every diff in the right order, mirroring Actor::ApplyChangesToPrototype
    // but without invoking the asset Save() that hits the filesystem.
    void ApplyDiffsManually(ActorDifferences& diff, const Ref<Actor>& source, const Ref<Actor>& proto)
    {
        ApplyActorInfo sourceInfo(source);
        ApplyActorInfo protoInfo(proto);
        Vector<ApplyActorInfo> empty;

        diff.newChildren.ForEach([&](auto d) { d->Apply(sourceInfo, protoInfo, empty); });
        diff.movedChildren.ForEach([&](auto d) { d->Apply(sourceInfo, protoInfo, empty); });
        diff.newComponents.ForEach([&](auto d) { d->Apply(sourceInfo, protoInfo, empty); });
        diff.changedActorFields.ForEach([&](auto d) { d->Apply(sourceInfo, protoInfo, empty); });
        diff.changedComponentFields.ForEach([&](auto d) { d->Apply(sourceInfo, protoInfo, empty); });
        diff.removedComponents.ForEach([&](auto d) { d->Apply(sourceInfo, protoInfo, empty); });
        diff.removedChildren.ForEach([&](auto d) { d->Apply(sourceInfo, protoInfo, empty); });
    }
}

TEST(ActorDifferences, ApplyChangedTransformPropagatesToPrototype)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPosition(Vec2F(0, 0));

    auto asset = a->MakePrototype();
    auto proto = asset->GetActor();

    a->transform->SetPosition(Vec2F(123, 456));
    EXPECT_FALSE(Math::Equals(proto->transform->GetPosition().x, 123.0f));

    auto diff = ActorDifferences::GetDifference(a, proto);
    ASSERT_GT(diff.changedActorFields.Count(), 0);

    ApplyDiffsManually(diff, a, proto);

    EXPECT_FLOAT_EQ(proto->transform->GetPosition().x, 123.0f);
    EXPECT_FLOAT_EQ(proto->transform->GetPosition().y, 456.0f);
}

TEST(ActorDifferences, ApplyChangedTransformLeavesEmptyDiff)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->transform->SetPosition(Vec2F(0, 0));

    auto asset = a->MakePrototype();
    auto proto = asset->GetActor();

    a->transform->SetPosition(Vec2F(50, 75));

    auto diff = ActorDifferences::GetDifference(a, proto);
    ApplyDiffsManually(diff, a, proto);

    auto reDiff = ActorDifferences::GetDifference(a, proto);
    EXPECT_EQ(reDiff.changedActorFields.Count(), 0);
    EXPECT_EQ(reDiff.changedComponentFields.Count(), 0);
    EXPECT_EQ(reDiff.newChildren.Count(), 0);
    EXPECT_EQ(reDiff.removedChildren.Count(), 0);
    EXPECT_EQ(reDiff.movedChildren.Count(), 0);
    EXPECT_EQ(reDiff.newComponents.Count(), 0);
    EXPECT_EQ(reDiff.removedComponents.Count(), 0);
}

TEST(ActorDifferences, ApplyChangedComponentFieldPropagatesToPrototype)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();
    comp->testValue = 0;

    auto asset = a->MakePrototype();
    auto proto = asset->GetActor();
    auto protoComp = proto->GetComponent<TestComponent>();
    ASSERT_TRUE(protoComp);
    EXPECT_EQ(protoComp->testValue, 0);

    comp->testValue = 99;

    auto diff = ActorDifferences::GetDifference(a, proto);
    ASSERT_GT(diff.changedComponentFields.Count(), 0);

    ApplyDiffsManually(diff, a, proto);

    EXPECT_EQ(protoComp->testValue, 99);

    auto reDiff = ActorDifferences::GetDifference(a, proto);
    EXPECT_EQ(reDiff.changedComponentFields.Count(), 0);
}

#endif // IS_EDITOR
