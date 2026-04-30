#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Types/Containers/Pool.h"
#include "o2/Utils/Types/Ref.h"

using namespace o2;

namespace
{
    class PoolTrackedObject : public RefCounterable
    {
    public:
        static int sAliveCount;

        int value = 0;

        PoolTrackedObject() { sAliveCount++; }
        ~PoolTrackedObject() { sAliveCount--; }
    };

    int PoolTrackedObject::sAliveCount = 0;
}

TEST(Pool, ConstructorCreatesInitialObjects) {
    PoolTrackedObject::sAliveCount = 0;
    {
        Pool<PoolTrackedObject> pool(7);
        EXPECT_EQ(PoolTrackedObject::sAliveCount, 7);
    }
    EXPECT_EQ(PoolTrackedObject::sAliveCount, 0);
}

TEST(Pool, TakeReducesPoolAndReturnsValidObject) {
    Pool<PoolTrackedObject> pool(3);

    Ref<PoolTrackedObject> a = pool.Take();
    Ref<PoolTrackedObject> b = pool.Take();
    Ref<PoolTrackedObject> c = pool.Take();

    EXPECT_TRUE(a);
    EXPECT_TRUE(b);
    EXPECT_TRUE(c);
    EXPECT_NE(a.Get(), b.Get());
    EXPECT_NE(b.Get(), c.Get());
}

TEST(Pool, TakeFromEmptyAllocatesChunk) {
    PoolTrackedObject::sAliveCount = 0;
    Pool<PoolTrackedObject> pool(0, 4); // 0 initial, chunk=4

    EXPECT_EQ(PoolTrackedObject::sAliveCount, 0);

    Ref<PoolTrackedObject> a = pool.Take();
    // Pool created 4 fresh objects, popped 1; 3 remain inside, plus our held Ref = 4 alive.
    EXPECT_EQ(PoolTrackedObject::sAliveCount, 4);
    EXPECT_TRUE(a);
}

TEST(Pool, FreeReturnsObjectAndTakeReusesIt) {
    Pool<PoolTrackedObject> pool(0, 1);
    Ref<PoolTrackedObject> obj = pool.Take();
    obj->value = 42;
    PoolTrackedObject* rawPtr = obj.Get();

    pool.Free(obj);
    obj = nullptr;

    Ref<PoolTrackedObject> taken = pool.Take();
    // Pool's only stored object was the one we freed, so we get it back.
    EXPECT_EQ(taken.Get(), rawPtr);
    EXPECT_EQ(taken->value, 42);
}

TEST(Pool, ChunkSizeAccessor) {
    Pool<PoolTrackedObject> pool(0, 8);
    EXPECT_EQ(pool.GetChunkSize(), 8);

    pool.SetChunkSize(16);
    EXPECT_EQ(pool.GetChunkSize(), 16);
}
