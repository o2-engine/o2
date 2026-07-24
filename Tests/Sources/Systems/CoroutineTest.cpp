#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <chrono>

#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2/Utils/Jobs/JobSystem.h"
#include "o2/Utils/Threading/Threading.h"

using namespace o2;

namespace
{
    // Drives main-thread job execution and frame advances until the predicate holds or it times out.
    // Stands in for the real frame loop that would pump these each frame
    template<typename _predicate>
    bool PumpUntil(const _predicate& predicate, float timeoutSeconds = 5.0f)
    {
        auto start = std::chrono::steady_clock::now();
        while (!predicate())
        {
            o2Jobs.ExecuteMainThreadJobs(-1.0f);
            o2Coroutines.OnNewFrame();

            float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
            if (elapsed > timeoutSeconds)
                return false;

            Thread::SleepForMilliseconds(1);
        }

        o2Jobs.ExecuteMainThreadJobs(-1.0f);
        return true;
    }

    Coroutine<int> DoubleValue(int x)
    {
        co_return x * 2;
    }

    Coroutine<int> AddOne(int x)
    {
        co_return x + 1;
    }

    Coroutine<int> ChainedChildren(int x)
    {
        int a = co_await AddOne(x);
        int b = co_await AddOne(a);
        co_return b;
    }

    Coroutine<void> SleepCoroutine(float seconds)
    {
        co_await WaitTime(seconds);
    }
}

TEST(Coroutine, ParallelReturnsValue)
{
    auto coroutine = DoubleValue(21);
    coroutine.Start(JobThread::Any);
    coroutine.Wait();
    EXPECT_TRUE(coroutine.IsDone());
    EXPECT_EQ(coroutine.GetResult(), 42);
}

TEST(Coroutine, AsyncStartsAndReturnsHandle)
{
    auto coroutine = Async(DoubleValue(5), JobThread::Any);
    coroutine.Wait();
    EXPECT_EQ(coroutine.GetResult(), 10);
}

TEST(Coroutine, AwaitsSubCoroutines)
{
    auto coroutine = ChainedChildren(10);
    coroutine.Start(JobThread::Any);
    coroutine.Wait();
    EXPECT_EQ(coroutine.GetResult(), 12);
}

TEST(Coroutine, WaitTimeDelaysResumption)
{
    auto start = std::chrono::steady_clock::now();
    auto coroutine = SleepCoroutine(0.05f);
    coroutine.Start(JobThread::Any);
    coroutine.Wait();
    float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
    EXPECT_GE(elapsed, 0.04f);
}

namespace
{
    Coroutine<void> NextFrameCoroutine(Atomic<int>* stage)
    {
        stage->Store(1);
        co_await WaitNextFrame();
        stage->Store(2);
    }
}

TEST(Coroutine, WaitNextFrameResumesOnNextFrame)
{
    Atomic<int> stage(0);
    auto coroutine = NextFrameCoroutine(&stage);
    coroutine.Start(JobThread::Main);

    // Run main jobs: the coroutine advances to WaitNextFrame and suspends
    for (int i = 0; i < 100 && stage.Load() < 1; i++)
    {
        o2Jobs.ExecuteMainThreadJobs(-1.0f);
        Thread::SleepForMilliseconds(1);
    }
    EXPECT_EQ(stage.Load(), 1);
    EXPECT_FALSE(coroutine.IsDone());

    // Advancing a frame wakes it up
    o2Coroutines.OnNewFrame();
    for (int i = 0; i < 100 && !coroutine.IsDone(); i++)
    {
        o2Jobs.ExecuteMainThreadJobs(-1.0f);
        Thread::SleepForMilliseconds(1);
    }
    EXPECT_EQ(stage.Load(), 2);
    EXPECT_TRUE(coroutine.IsDone());
}

namespace
{
    Coroutine<void> WaitSignalCoroutine(Signal signal, Atomic<int>* done)
    {
        co_await signal;
        done->Store(1);
    }
}

TEST(Coroutine, SignalWaitAndSynchronize)
{
    Signal signal;
    Atomic<int> done(0);
    auto coroutine = WaitSignalCoroutine(signal, &done);
    coroutine.Start(JobThread::Any);

    // Give it time to reach the co_await and suspend; nothing should complete yet
    Thread::SleepForMilliseconds(20);
    EXPECT_EQ(done.Load(), 0);
    EXPECT_FALSE(coroutine.IsDone());

    signal.Synchronize(); // release the waiter from this (main) thread
    coroutine.Wait();
    EXPECT_EQ(done.Load(), 1);
}

namespace
{
    Coroutine<int> Square(int x)
    {
        co_return x * x;
    }

    // Runs on the main thread, launches N children on the worker pool, awaits them all without
    // blocking the main thread, then continues on the main thread. This is the exact pattern from
    // the task description
    Coroutine<void> ParallelThenContinueOnMain(int count, Thread::Id mainThreadId,
                                               Atomic<int>* sum, Atomic<int>* resumedOnMain)
    {
        Vector<Coroutine<int>> children;
        for (int i = 0; i < count; i++)
            children.Add(Async(Square(i + 1), JobThread::Any));

        co_await WaitAll(children);

        if (Thread::GetCurrentThreadId() == mainThreadId)
            resumedOnMain->FetchAdd(1);

        int total = 0;
        for (auto& child : children)
            total += child.GetResult();
        sum->Store(total);
    }
}

TEST(Coroutine, ParallelChildrenAwaitedOnMainThread)
{
    const int count = 8;
    Atomic<int> sum(0);
    Atomic<int> resumedOnMain(0);
    Thread::Id mainThreadId = Thread::GetCurrentThreadId();

    auto coroutine = ParallelThenContinueOnMain(count, mainThreadId, &sum, &resumedOnMain);
    coroutine.Start(JobThread::Main);

    ASSERT_TRUE(PumpUntil([&] { return coroutine.IsDone(); }));

    // Sum of squares 1..8
    EXPECT_EQ(sum.Load(), 1 + 4 + 9 + 16 + 25 + 36 + 49 + 64);
    EXPECT_GE(resumedOnMain.Load(), 1);
}

namespace
{
    Coroutine<int> DelayedValue(float seconds, int value)
    {
        co_await WaitTime(seconds);
        co_return value;
    }

    Coroutine<int> WaitAnyPattern(Atomic<int>* indexOut)
    {
        Vector<Coroutine<int>> children;
        children.Add(Async(DelayedValue(0.20f, 100)));
        children.Add(Async(DelayedValue(0.02f, 200))); // fastest
        children.Add(Async(DelayedValue(0.30f, 300)));

        int index = co_await WaitAny(children);
        indexOut->Store(index);
        co_return children[index].GetResult();
    }
}

TEST(Coroutine, WaitAnyReturnsFirstFinished)
{
    Atomic<int> index(-1);
    auto coroutine = WaitAnyPattern(&index);
    coroutine.Start(JobThread::Any);
    coroutine.Wait();

    EXPECT_EQ(index.Load(), 1);
    EXPECT_EQ(coroutine.GetResult(), 200);
}

namespace
{
    Coroutine<void> SwitchThreadsCoroutine(Thread::Id mainThreadId, Atomic<int>* onWorker,
                                           Atomic<int>* backOnMain)
    {
        co_await SwitchToWorker();
        onWorker->Store(Thread::GetCurrentThreadId() != mainThreadId ? 1 : 0);

        co_await SwitchToMain();
        backOnMain->Store(Thread::GetCurrentThreadId() == mainThreadId ? 1 : 0);
    }
}

TEST(Coroutine, SwitchBetweenMainAndWorker)
{
    Thread::Id mainThreadId = Thread::GetCurrentThreadId();
    Atomic<int> onWorker(-1);
    Atomic<int> backOnMain(-1);

    auto coroutine = SwitchThreadsCoroutine(mainThreadId, &onWorker, &backOnMain);
    coroutine.Start(JobThread::Main);

    ASSERT_TRUE(PumpUntil([&] { return coroutine.IsDone(); }));
    EXPECT_EQ(onWorker.Load(), 1);
    EXPECT_EQ(backOnMain.Load(), 1);
}

namespace
{
    Coroutine<void> NestedParallelCoroutine(Atomic<int>* counter)
    {
        // Launch a batch of parallel children from within a coroutine and await them
        Vector<Coroutine<int>> children;
        for (int i = 0; i < 16; i++)
            children.Add(Async(Square(1), JobThread::Any));

        co_await WaitAll(children);

        for (auto& child : children)
            counter->FetchAdd(child.GetResult());
    }
}

TEST(Coroutine, WaitAllFromWorkerCoroutine)
{
    Atomic<int> counter(0);
    auto coroutine = NestedParallelCoroutine(&counter);
    coroutine.Start(JobThread::Any);
    coroutine.Wait();
    EXPECT_EQ(counter.Load(), 16); // 16 children each returning 1*1
}
