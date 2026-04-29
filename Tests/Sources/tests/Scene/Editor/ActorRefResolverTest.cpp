#include "o2/stdafx.h"

#if IS_EDITOR

#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/ActorLinkRef.h"
#include "o2/Scene/ActorRefResolver.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/ComponentLinkRef.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    // RAII wrapper that ensures lock depth returns to zero, even on assertion failure
    class LockGuard
    {
    public:
        LockGuard()
        {
            mInitial = ActorRefResolver::GetLockDepth();
            ActorRefResolver::LockResolving();
        }
        ~LockGuard()
        {
            while (ActorRefResolver::GetLockDepth() > mInitial)
                ActorRefResolver::UnlockResolving();
        }
    private:
        int mInitial = 0;
    };
}

// ===== Lock / Unlock =====

TEST(ActorRefResolver, InitialLockDepthIsZero)
{
    EXPECT_EQ(ActorRefResolver::GetLockDepth(), 0);
    EXPECT_FALSE(ActorRefResolver::IsLocked());
}

TEST(ActorRefResolver, LockResolvingIncrementsDepth)
{
    LockGuard guard;
    EXPECT_EQ(ActorRefResolver::GetLockDepth(), 1);
    EXPECT_TRUE(ActorRefResolver::IsLocked());
}

TEST(ActorRefResolver, UnlockReturnsToZero)
{
    int before = ActorRefResolver::GetLockDepth();

    ActorRefResolver::LockResolving();
    ActorRefResolver::UnlockResolving();

    EXPECT_EQ(ActorRefResolver::GetLockDepth(), before);
}

TEST(ActorRefResolver, NestedLocksTrackDepth)
{
    int before = ActorRefResolver::GetLockDepth();
    ActorRefResolver::LockResolving();
    ActorRefResolver::LockResolving();
    EXPECT_EQ(ActorRefResolver::GetLockDepth(), before + 2);

    ActorRefResolver::UnlockResolving();
    EXPECT_EQ(ActorRefResolver::GetLockDepth(), before + 1);

    ActorRefResolver::UnlockResolving();
    EXPECT_EQ(ActorRefResolver::GetLockDepth(), before);
}

TEST(ActorRefResolver, LockWithDepthArgIncrementsByValue)
{
    int before = ActorRefResolver::GetLockDepth();
    ActorRefResolver::LockResolving(3);
    EXPECT_EQ(ActorRefResolver::GetLockDepth(), before + 3);

    ActorRefResolver::UnlockResolving(3);
    EXPECT_EQ(ActorRefResolver::GetLockDepth(), before);
}

// ===== RequireResolve by SceneUID =====

TEST(ActorRefResolver, RequireResolveByActorIdResolvesAfterUnlock)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    TickFrame();

    LinkRef<Actor> link;
    EXPECT_FALSE(link.IsValid());

    ActorRefResolver::RequireResolve(link, a->GetID());
    ActorRefResolver::ResolveRefs();

    EXPECT_EQ(link.Get(), a.Get());
}

TEST(ActorRefResolver, ResolveRefsForcibleWorksUnderLock)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    TickFrame();

    LinkRef<Actor> link;
    {
        LockGuard lockGuard;
        ActorRefResolver::RequireResolve(link, a->GetID());
        ActorRefResolver::ResolveRefs(/*forcible*/ true);
    }

    EXPECT_EQ(link.Get(), a.Get());
}

TEST(ActorRefResolver, RequireResolveByComponentIdResolves)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();
    TickFrame();

    LinkRef<TestComponent> link;
    ActorRefResolver::RequireResolve(link, a->GetID(), comp->GetID());
    ActorRefResolver::ResolveRefs();

    EXPECT_EQ(link.Get(), comp.Get());
}

#endif // IS_EDITOR
