#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Threading/Threading.h"
#include "o2/Utils/Types/Ref.h"

using namespace o2;

namespace
{
    // Runs the given function on `threadCount` threads and joins them all
    template<typename _fn_type>
    void RunOnThreads(int threadCount, const _fn_type& function)
    {
        Vector<Thread> threads;
        threads.Reserve(threadCount);
        for (int i = 0; i < threadCount; i++)
            threads.emplace_back(function, i);

        for (auto& thread : threads)
            thread.Join();
    }
}

TEST(Threading, ThreadRunsAndJoins)
{
    Atomic<int> ran(0);
    {
        Thread thread([&] { ran.FetchAdd(1); });
        thread.Join();
        EXPECT_FALSE(thread.IsJoinable());
    }
    EXPECT_EQ(ran.Load(), 1);
}

TEST(Threading, ThreadAutoJoinsOnDestruction)
{
    Atomic<int> ran(0);
    {
        Thread thread([&] { Thread::SleepForMilliseconds(1); ran.FetchAdd(1); });
        // No explicit Join, destructor must join
    }
    EXPECT_EQ(ran.Load(), 1);
}

TEST(Threading, ThreadHasDistinctId)
{
    Thread::Id worker;
    Thread thread([&] { worker = Thread::GetCurrentThreadId(); });
    thread.Join();
    EXPECT_NE(worker, Thread::GetCurrentThreadId());
}

TEST(Threading, HardwareConcurrencyIsSane)
{
    // Not guaranteed but practically always >= 1 on our targets
    EXPECT_GE((int)Thread::HardwareConcurrency(), 1);
}

TEST(Threading, MutexProtectsSharedCounter)
{
    Mutex mutex;
    int counter = 0;
    const int perThread = 10000;

    RunOnThreads(8, [&](int) {
        for (int i = 0; i < perThread; i++)
        {
            ScopeLock<Mutex> lock(mutex);
            counter++;
        }
    });

    EXPECT_EQ(counter, 8 * perThread);
}

TEST(Threading, RecursiveMutexReentrant)
{
    RecursiveMutex mutex;
    ScopeLock<RecursiveMutex> outer(mutex);
    // Locking again on the same thread must not deadlock
    EXPECT_TRUE(mutex.TryLock());
    mutex.Unlock();
}

TEST(Threading, SharedMutexAllowsConcurrentReaders)
{
    SharedMutex mutex;
    Atomic<int> concurrentReaders(0);
    Atomic<int> maxConcurrentReaders(0);

    RunOnThreads(8, [&](int) {
        SharedLock lock(mutex);
        int current = concurrentReaders.FetchAdd(1) + 1;
        int previousMax = maxConcurrentReaders.Load();
        while (current > previousMax && !maxConcurrentReaders.CompareExchange(previousMax, current)) {}
        Thread::SleepForMilliseconds(2);
        concurrentReaders.FetchSub(1);
    });

    EXPECT_EQ(concurrentReaders.Load(), 0);
    // With true shared access we expect at least two readers to overlap at some point
    EXPECT_GE(maxConcurrentReaders.Load(), 2);
}

TEST(Threading, AtomicConcurrentIncrement)
{
    Atomic<int> counter(0);
    const int perThread = 100000;

    RunOnThreads(8, [&](int) {
        for (int i = 0; i < perThread; i++)
            counter.FetchAdd(1);
    });

    EXPECT_EQ(counter.Load(), 8 * perThread);
}

TEST(Threading, AtomicOperators)
{
    Atomic<int> value(5);
    EXPECT_EQ(++value, 6);
    EXPECT_EQ(value++, 6);
    EXPECT_EQ(value.Load(), 7);
    EXPECT_EQ(--value, 6);
    EXPECT_EQ(value--, 6);
    EXPECT_EQ(value.Load(), 5);

    int expected = 5;
    EXPECT_TRUE(value.CompareExchange(expected, 42));
    EXPECT_EQ(value.Load(), 42);

    expected = 5; // wrong expected now
    EXPECT_FALSE(value.CompareExchange(expected, 0));
    EXPECT_EQ(expected, 42); // written back with the actual value
}

TEST(Threading, ConditionVariableSignals)
{
    Mutex mutex;
    ConditionVariable condition;
    bool ready = false;
    Atomic<bool> gotSignal(false);

    Thread waiter([&] {
        UniqueLock lock(mutex);
        condition.Wait(lock, [&] { return ready; });
        gotSignal.Store(true);
    });

    {
        ScopeLock<Mutex> lock(mutex);
        ready = true;
    }
    condition.NotifyOne();
    waiter.Join();

    EXPECT_TRUE(gotSignal.Load());
}

TEST(Threading, ConditionVariableWaitForTimesOut)
{
    Mutex mutex;
    ConditionVariable condition;
    UniqueLock lock(mutex);
    // Nothing will notify — must return false (predicate never satisfied) after timeout
    bool satisfied = condition.WaitFor(lock, 0.02f, [] { return false; });
    EXPECT_FALSE(satisfied);
}

TEST(Threading, ThreadSafeQueueBasic)
{
    ThreadSafeQueue<int> queue;
    EXPECT_TRUE(queue.IsEmpty());

    queue.Push(1);
    queue.Push(2);
    EXPECT_EQ(queue.Count(), 2);

    int value = 0;
    EXPECT_TRUE(queue.TryPop(value));
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(queue.TryPop(value));
    EXPECT_EQ(value, 2);
    EXPECT_FALSE(queue.TryPop(value));
}

TEST(Threading, ThreadSafeQueueMultiProducerConsumer)
{
    ThreadSafeQueue<int> queue;
    const int producers = 4;
    const int perProducer = 25000;
    Atomic<int> consumedCount(0);
    Atomic<long long> consumedSum(0);

    Vector<Thread> consumers;
    const int consumerCount = 4;
    for (int i = 0; i < consumerCount; i++)
    {
        consumers.emplace_back([&] {
            int value = 0;
            while (queue.WaitAndPop(value))
            {
                consumedSum.FetchAdd(value);
                consumedCount.FetchAdd(1);
            }
        });
    }

    RunOnThreads(producers, [&](int) {
        for (int i = 0; i < perProducer; i++)
            queue.Push(1);
    });

    // Wait until everything is drained, then close to release consumers
    while (consumedCount.Load() < producers * perProducer)
        Thread::SleepForMilliseconds(1);

    queue.Close();
    for (auto& consumer : consumers)
        consumer.Join();

    EXPECT_EQ(consumedCount.Load(), producers * perProducer);
    EXPECT_EQ(consumedSum.Load(), (long long)producers * perProducer);
}

TEST(Threading, ThreadSafeQueueCloseWakesWaiters)
{
    ThreadSafeQueue<int> queue;
    Atomic<bool> returned(false);

    Thread waiter([&] {
        int value = 0;
        bool got = queue.WaitAndPop(value); // blocks until Close()
        EXPECT_FALSE(got);
        returned.Store(true);
    });

    Thread::SleepForMilliseconds(5);
    EXPECT_FALSE(returned.Load()); // still blocked
    queue.Close();
    waiter.Join();
    EXPECT_TRUE(returned.Load());
}

namespace
{
    // Small ref-counted object used to stress concurrent mmake/free through the memory manager
    class ThreadingProbe : public RefCounterable
    {
    public:
        int payload = 0;
        ThreadingProbe() = default;
        explicit ThreadingProbe(int value) : payload(value) {}
    };
}

TEST(Threading, ConcurrentAllocationsAreSafe)
{
    // Exercises the thread-safe MemoryManager: many threads allocate and free o2 objects at once.
    // Would corrupt the DEBUG allocation map without the guarding mutex
    const int perThread = 5000;
    Atomic<long long> sum(0);

    RunOnThreads(8, [&](int threadIndex) {
        for (int i = 0; i < perThread; i++)
        {
            auto probe = mmake<ThreadingProbe>(threadIndex + i);
            sum.FetchAdd(probe->payload);
        }
    });

    // If we got here without crashing/corrupting the allocator map, the guard works
    SUCCEED();
    EXPECT_GT(sum.Load(), 0);
}
