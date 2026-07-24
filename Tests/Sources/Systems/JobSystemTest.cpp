#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <vector>

#include "o2/Utils/Jobs/JobSystem.h"
#include "o2/Utils/Threading/Threading.h"

using namespace o2;

namespace
{
    // Small ref-counted holder used to allocate o2 objects inside job bodies
    class IntBag : public RefCounterable
    {
    public:
        Vector<int> data;
    };
}

TEST(JobSystem, RunsParallelJob)
{
    Atomic<int> ran(0);
    auto job = o2Jobs.Schedule([&] { ran.FetchAdd(1); });
    job->Wait();
    EXPECT_TRUE(job->IsDone());
    EXPECT_EQ(ran.Load(), 1);
}

TEST(JobSystem, ParallelJobRunsOnWorkerThread)
{
    Thread::Id mainId = Thread::GetCurrentThreadId();
    Atomic<bool> onWorker(false);
    auto job = o2Jobs.Schedule([&] { onWorker.Store(Thread::GetCurrentThreadId() != mainId); });
    job->Wait();
    EXPECT_TRUE(onWorker.Load());
    EXPECT_GE(o2Jobs.GetWorkersCount(), 1);
}

TEST(JobSystem, ManyParallelJobsComplete)
{
    const int count = 2000;
    Atomic<int> counter(0);
    Vector<SharedRef<Job>> jobs;
    jobs.Reserve(count);
    for (int i = 0; i < count; i++)
        jobs.Add(o2Jobs.Schedule([&] { counter.FetchAdd(1); }));

    for (auto& job : jobs)
        job->Wait();

    EXPECT_EQ(counter.Load(), count);
}

TEST(JobSystem, WaitForIdleDrainsAllParallelWork)
{
    const int count = 500;
    Atomic<int> counter(0);
    for (int i = 0; i < count; i++)
        o2Jobs.Schedule([&] { counter.FetchAdd(1); });

    o2Jobs.WaitForIdle();
    EXPECT_EQ(counter.Load(), count);
    EXPECT_EQ(o2Jobs.GetUnfinishedParallelJobsCount(), 0);
}

TEST(JobSystem, MainThreadJobsRunOnlyWhenDrained)
{
    Thread::Id mainId = Thread::GetCurrentThreadId();
    Atomic<int> ran(0);
    Atomic<bool> onMain(false);

    auto job = o2Jobs.Schedule([&] {
        onMain.Store(Thread::GetCurrentThreadId() == mainId);
        ran.FetchAdd(1);
    }, JobPriority::Normal, JobThread::Main);

    // Workers must never pick a Main job, so it stays unexecuted until we drain it
    Thread::SleepForMilliseconds(20);
    EXPECT_EQ(ran.Load(), 0);

    o2Jobs.ExecuteMainThreadJobs();
    EXPECT_EQ(ran.Load(), 1);
    EXPECT_TRUE(onMain.Load());
    EXPECT_TRUE(job->IsDone());
}

TEST(JobSystem, MainThreadPriorityOrder)
{
    std::vector<int> order;

    // Submit in scrambled priority order; draining must run them highest priority first
    o2Jobs.Schedule([&] { order.push_back(1); }, JobPriority::Low, JobThread::Main);
    o2Jobs.Schedule([&] { order.push_back(3); }, JobPriority::High, JobThread::Main);
    o2Jobs.Schedule([&] { order.push_back(2); }, JobPriority::Normal, JobThread::Main);
    o2Jobs.Schedule([&] { order.push_back(4); }, JobPriority::Critical, JobThread::Main);

    o2Jobs.ExecuteMainThreadJobs();

    ASSERT_EQ(order.size(), 4u);
    EXPECT_EQ(order[0], 4); // Critical
    EXPECT_EQ(order[1], 3); // High
    EXPECT_EQ(order[2], 2); // Normal
    EXPECT_EQ(order[3], 1); // Low
}

TEST(JobSystem, MainThreadQuotaLimitsExecution)
{
    const int count = 10;
    Atomic<int> ran(0);
    for (int i = 0; i < count; i++)
        o2Jobs.Schedule([&] { Thread::SleepForMilliseconds(10); ran.FetchAdd(1); },
                        JobPriority::Normal, JobThread::Main);

    // ~10ms per job, 25ms quota: best-effort stops after the running job overruns the quota
    o2Jobs.ExecuteMainThreadJobs(0.025f);
    int afterQuota = ran.Load();
    EXPECT_GE(afterQuota, 1);
    EXPECT_LT(afterQuota, count); // quota stopped it early

    // Draining with no quota runs the rest
    o2Jobs.ExecuteMainThreadJobs(-1.0f);
    EXPECT_EQ(ran.Load(), count);
}

TEST(JobSystem, DependencyRunsAfterDependency)
{
    Atomic<int> sequence(0);
    Atomic<int> aOrder(-1);
    Atomic<int> bOrder(-1);

    auto a = o2Jobs.CreateJob([&] { Thread::SleepForMilliseconds(15); aOrder.Store(sequence.FetchAdd(1)); });
    auto b = o2Jobs.CreateJob([&] { bOrder.Store(sequence.FetchAdd(1)); });
    b->DependsOn(a);

    o2Jobs.Submit(a);
    o2Jobs.Submit(b);
    b->Wait();

    EXPECT_TRUE(a->IsDone());
    EXPECT_EQ(aOrder.Load(), 0);
    EXPECT_EQ(bOrder.Load(), 1);
}

TEST(JobSystem, ThenSchedulesContinuation)
{
    Atomic<int> sequence(0);
    Atomic<int> aOrder(-1);
    Atomic<int> bOrder(-1);

    auto a = o2Jobs.Schedule([&] { Thread::SleepForMilliseconds(10); aOrder.Store(sequence.FetchAdd(1)); });
    auto b = a->Then([&] { bOrder.Store(sequence.FetchAdd(1)); });
    b->Wait();

    EXPECT_EQ(aOrder.Load(), 0);
    EXPECT_EQ(bOrder.Load(), 1);
}

TEST(JobSystem, DependencyOnCompletedJobRunsImmediately)
{
    auto a = o2Jobs.Schedule([] {});
    a->Wait(); // a is done now

    Atomic<int> ran(0);
    auto b = o2Jobs.CreateJob([&] { ran.FetchAdd(1); });
    b->DependsOn(a); // no-op, dependency already complete
    o2Jobs.Submit(b);
    b->Wait();

    EXPECT_EQ(ran.Load(), 1);
}

TEST(JobSystem, DiamondDependencies)
{
    // a -> b, a -> c, (b,c) -> d
    Atomic<int> order(0);
    Atomic<int> aOrder(-1), bOrder(-1), cOrder(-1), dOrder(-1);

    auto a = o2Jobs.CreateJob([&] { aOrder.Store(order.FetchAdd(1)); });
    auto b = o2Jobs.CreateJob([&] { bOrder.Store(order.FetchAdd(1)); });
    auto c = o2Jobs.CreateJob([&] { cOrder.Store(order.FetchAdd(1)); });
    auto d = o2Jobs.CreateJob([&] { dOrder.Store(order.FetchAdd(1)); });

    b->DependsOn(a);
    c->DependsOn(a);
    d->DependsOn(b);
    d->DependsOn(c);

    o2Jobs.Submit(a);
    o2Jobs.Submit(b);
    o2Jobs.Submit(c);
    o2Jobs.Submit(d);
    d->Wait();

    EXPECT_TRUE(a->IsDone() && b->IsDone() && c->IsDone() && d->IsDone());
    // a first, d last; b and c in between
    EXPECT_EQ(aOrder.Load(), 0);
    EXPECT_EQ(dOrder.Load(), 3);
    EXPECT_GT(bOrder.Load(), aOrder.Load());
    EXPECT_GT(cOrder.Load(), aOrder.Load());
    EXPECT_LT(bOrder.Load(), dOrder.Load());
    EXPECT_LT(cOrder.Load(), dOrder.Load());
}

TEST(JobSystem, OnCompletedCallbackFires)
{
    Atomic<int> completed(0);
    auto job = o2Jobs.CreateJob([] {});
    job->onCompleted = [&] { completed.FetchAdd(1); };
    o2Jobs.Submit(job);
    job->Wait();
    // onCompleted runs right after the body on the executing thread; give it a moment to be observed
    while (completed.Load() == 0)
        Thread::SleepForMilliseconds(1);
    EXPECT_EQ(completed.Load(), 1);
}

TEST(JobSystem, NestedSchedulingFromWorker)
{
    Atomic<int> inner(0);
    o2Jobs.Schedule([&] {
        o2Jobs.Schedule([&] { inner.FetchAdd(1); });
    });
    o2Jobs.WaitForIdle();
    EXPECT_EQ(inner.Load(), 1);
}

TEST(JobSystem, ParallelJobsAllocateO2ObjectsSafely)
{
    // Job bodies allocate/free thread-local o2 objects on the workers, stressing the thread-safe
    // memory manager. Refs are never shared across threads here, honoring the isolation contract
    const int count = 1000;
    Atomic<long long> sum(0);
    Vector<SharedRef<Job>> jobs;
    jobs.Reserve(count);
    for (int i = 0; i < count; i++)
        jobs.Add(o2Jobs.Schedule([&, i] {
            auto bag = mmake<IntBag>();
            for (int k = 0; k < 32; k++)
                bag->data.Add(i + k);
            sum.FetchAdd(bag->data.Count());
        }));

    for (auto& job : jobs)
        job->Wait();

    EXPECT_EQ(sum.Load(), (long long)count * 32);
}

TEST(JobSystem, ShutdownStopsSelfReschedulingWork)
{
    // Reproduces the shutdown hang: a parallel job that reschedules itself keeps the ready queue
    // non-empty, so Shutdown() (which used to drain the queue before exiting) would never finish.
    // Uses a private JobSystem instance so the shared o2Jobs singleton is left untouched
    JobSystem* savedInstance = JobSystem::InstancePtr();

    Ref<JobSystem> jobs = mmake<JobSystem>();
    jobs->Initialize(2);

    Atomic<bool> keepScheduling(true);
    struct SelfSched
    {
        JobSystem*    jobs;
        Atomic<bool>* go;
        void operator()() const
        {
            if (go->Load())
                jobs->Schedule([s = *this] { s(); });
        }
    };
    SelfSched seed{ jobs.Get(), &keepScheduling };
    for (int i = 0; i < jobs->GetWorkersCount() * 2; i++)
        jobs->Schedule([seed] { seed(); });

    // Run Shutdown on another thread and bound the wait, so a regression fails this test instead of
    // hanging the whole suite process
    Atomic<bool> returned(false);
    Thread shutdownThread([&] { jobs->Shutdown(); returned.Store(true); });

    for (int i = 0; i < 500 && !returned.Load(); i++)
        Thread::SleepForMilliseconds(10);

    keepScheduling.Store(false);

    EXPECT_TRUE(returned.Load()) << "JobSystem::Shutdown hung draining self-rescheduling work";

    if (returned.Load())
    {
        shutdownThread.Join();
        JobSystem::DestroySingleton(jobs);
    }
    else
        shutdownThread.Detach();

    JobSystem::mInstance = savedInstance; // restore the shared singleton for the rest of the suite
}
