#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/WeakRef.h"

using namespace o2;

namespace
{
    class RefTrackedBase : public RefCounterable
    {
    public:
        static int sAliveCount;

        int value = 0;

        RefTrackedBase() { sAliveCount++; }
        explicit RefTrackedBase(int v) : value(v) { sAliveCount++; }
        ~RefTrackedBase() override { sAliveCount--; }
    };

    int RefTrackedBase::sAliveCount = 0;

    class RefTrackedDerived : public RefTrackedBase
    {
    public:
        int extra = 0;

        RefTrackedDerived() = default;
        explicit RefTrackedDerived(int v, int e) : RefTrackedBase(v), extra(e) {}
    };
}

TEST(Ref, MakeStartsWithOneStrongReference) {
    RefTrackedBase::sAliveCount = 0;
    {
        Ref<RefTrackedBase> r = mmake<RefTrackedBase>(42);
        ASSERT_TRUE(r);
        EXPECT_EQ(r->value, 42);
        EXPECT_EQ(r->GetStrongReferencesCount(), 1);
        EXPECT_EQ(r->GetWeakReferencesCount(), 0);
        EXPECT_EQ(RefTrackedBase::sAliveCount, 1);
    }
    EXPECT_EQ(RefTrackedBase::sAliveCount, 0);
}

TEST(Ref, CopyIncrementsStrongCount) {
    Ref<RefTrackedBase> a = mmake<RefTrackedBase>();
    EXPECT_EQ(a->GetStrongReferencesCount(), 1);

    Ref<RefTrackedBase> b = a;
    EXPECT_EQ(a->GetStrongReferencesCount(), 2);
    EXPECT_EQ(b.Get(), a.Get());

    {
        Ref<RefTrackedBase> c = a;
        EXPECT_EQ(a->GetStrongReferencesCount(), 3);
    }
    EXPECT_EQ(a->GetStrongReferencesCount(), 2);
}

TEST(Ref, AssignmentReleasesPreviousAndAcquiresNew) {
    RefTrackedBase::sAliveCount = 0;
    Ref<RefTrackedBase> a = mmake<RefTrackedBase>(1);
    Ref<RefTrackedBase> b = mmake<RefTrackedBase>(2);
    EXPECT_EQ(RefTrackedBase::sAliveCount, 2);

    a = b;
    EXPECT_EQ(RefTrackedBase::sAliveCount, 1);
    EXPECT_EQ(a.Get(), b.Get());
    EXPECT_EQ(a->value, 2);
    EXPECT_EQ(a->GetStrongReferencesCount(), 2);
}

TEST(Ref, MoveTransfersOwnership) {
    Ref<RefTrackedBase> a = mmake<RefTrackedBase>(99);
    RefTrackedBase* rawPtr = a.Get();
    EXPECT_EQ(a->GetStrongReferencesCount(), 1);

    Ref<RefTrackedBase> b = std::move(a);
    EXPECT_FALSE(a);
    EXPECT_EQ(a.Get(), nullptr);
    EXPECT_TRUE(b);
    EXPECT_EQ(b.Get(), rawPtr);
    EXPECT_EQ(b->GetStrongReferencesCount(), 1);
}

TEST(Ref, NullAssignmentReleasesObject) {
    RefTrackedBase::sAliveCount = 0;
    Ref<RefTrackedBase> a = mmake<RefTrackedBase>();
    EXPECT_EQ(RefTrackedBase::sAliveCount, 1);

    a = nullptr;
    EXPECT_FALSE(a);
    EXPECT_EQ(RefTrackedBase::sAliveCount, 0);
}

TEST(Ref, DestructorRunsWhenLastReferenceReleased) {
    RefTrackedBase::sAliveCount = 0;

    Ref<RefTrackedBase> a = mmake<RefTrackedBase>();
    Ref<RefTrackedBase> b = a;
    Ref<RefTrackedBase> c = a;
    EXPECT_EQ(RefTrackedBase::sAliveCount, 1);

    b = nullptr;
    EXPECT_EQ(RefTrackedBase::sAliveCount, 1);
    c = nullptr;
    EXPECT_EQ(RefTrackedBase::sAliveCount, 1);
    a = nullptr;
    EXPECT_EQ(RefTrackedBase::sAliveCount, 0);
}

TEST(Ref, EqualityComparesPointers) {
    Ref<RefTrackedBase> a = mmake<RefTrackedBase>();
    Ref<RefTrackedBase> b = a;
    Ref<RefTrackedBase> c = mmake<RefTrackedBase>();

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_TRUE(a == a.Get());
}

TEST(Ref, BoolConversion) {
    Ref<RefTrackedBase> empty;
    EXPECT_FALSE(empty);
    EXPECT_FALSE(empty.IsValid());

    Ref<RefTrackedBase> filled = mmake<RefTrackedBase>();
    EXPECT_TRUE(filled);
    EXPECT_TRUE(filled.IsValid());
}

TEST(Ref, PolymorphicAssignment) {
    RefTrackedBase::sAliveCount = 0;
    Ref<RefTrackedDerived> derived = mmake<RefTrackedDerived>(7, 13);

    Ref<RefTrackedBase> base = derived;
    EXPECT_EQ(base.Get(), static_cast<RefTrackedBase*>(derived.Get()));
    EXPECT_EQ(base->GetStrongReferencesCount(), 2);
    EXPECT_EQ(base->value, 7);

    derived = nullptr;
    EXPECT_EQ(RefTrackedBase::sAliveCount, 1);
    EXPECT_EQ(base->GetStrongReferencesCount(), 1);

    base = nullptr;
    EXPECT_EQ(RefTrackedBase::sAliveCount, 0);
}

TEST(Ref, DynamicCastUpAndDown) {
    Ref<RefTrackedDerived> derived = mmake<RefTrackedDerived>(1, 2);
    Ref<RefTrackedBase> base = derived;

    Ref<RefTrackedDerived> casted = DynamicCast<RefTrackedDerived>(base);
    ASSERT_TRUE(casted);
    EXPECT_EQ(casted.Get(), derived.Get());
    EXPECT_EQ(casted->extra, 2);
}

TEST(WeakRef, DoesNotKeepObjectAlive) {
    RefTrackedBase::sAliveCount = 0;

    WeakRef<RefTrackedBase> weak;
    {
        Ref<RefTrackedBase> strong = mmake<RefTrackedBase>(123);
        weak = strong;
        EXPECT_FALSE(weak.IsExpired());
        EXPECT_EQ(strong->GetStrongReferencesCount(), 1);
        EXPECT_GE(strong->GetWeakReferencesCount(), 1);
    }
    EXPECT_TRUE(weak.IsExpired());
    EXPECT_EQ(RefTrackedBase::sAliveCount, 0);
}

TEST(WeakRef, LockReturnsValidRefWhenAlive) {
    Ref<RefTrackedBase> strong = mmake<RefTrackedBase>(55);
    WeakRef<RefTrackedBase> weak = strong;

    Ref<RefTrackedBase> locked = weak.Lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(locked.Get(), strong.Get());
    EXPECT_EQ(locked->value, 55);
    EXPECT_EQ(strong->GetStrongReferencesCount(), 2);
}

TEST(WeakRef, LockReturnsNullAfterExpiry) {
    WeakRef<RefTrackedBase> weak;
    {
        Ref<RefTrackedBase> strong = mmake<RefTrackedBase>();
        weak = strong;
    }
    Ref<RefTrackedBase> locked = weak.Lock();
    EXPECT_FALSE(locked);
}

TEST(WeakRef, DefaultIsExpired) {
    WeakRef<RefTrackedBase> weak;
    EXPECT_TRUE(weak.IsExpired());
    EXPECT_FALSE(weak.IsValid());
    EXPECT_FALSE(weak.Lock());
}

TEST(WeakRef, MultipleWeakRefsTrackedCorrectly) {
    Ref<RefTrackedBase> strong = mmake<RefTrackedBase>();
    WeakRef<RefTrackedBase> w1 = strong;
    WeakRef<RefTrackedBase> w2 = strong;
    WeakRef<RefTrackedBase> w3 = w1;

    EXPECT_FALSE(w1.IsExpired());
    EXPECT_FALSE(w2.IsExpired());
    EXPECT_FALSE(w3.IsExpired());

    EXPECT_GE(strong->GetWeakReferencesCount(), 3);

    strong = nullptr;
    EXPECT_TRUE(w1.IsExpired());
    EXPECT_TRUE(w2.IsExpired());
    EXPECT_TRUE(w3.IsExpired());
}
