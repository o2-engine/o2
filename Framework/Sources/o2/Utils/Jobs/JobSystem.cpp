#include "o2/stdafx.h"
#include "JobSystem.h"

#include <chrono>

namespace o2
{
    DECLARE_SINGLETON(JobSystem);

    JobSystem::JobSystem(RefCounter* refCounter):
        Singleton<JobSystem>(refCounter)
    {}

    JobSystem::~JobSystem()
    {
        Shutdown();
    }

    // Default cap on the number of worker threads. The automatic count (hardware threads - 1) is clamped
    // to this, leaving the rest of the cores for the main/render threads and the OS. Pass an explicit
    // count to Initialize() to override
    static const int kDefaultMaxWorkers = 4;

    void JobSystem::Initialize(int workersCount)
    {
        if (mWorkersCount > 0)
            return;

#if !O2_HAS_THREADS
        // No OS threads on this platform (single-threaded WebAssembly): force cooperative mode regardless
        // of the requested count
        workersCount = 0;
#else
        if (workersCount < 0)
        {
            workersCount = (int)Thread::HardwareConcurrency() - 1;
            if (workersCount > kDefaultMaxWorkers)
                workersCount = kDefaultMaxWorkers;
            if (workersCount < 1)
                workersCount = 1;
        }
#endif

        // workersCount == 0 is cooperative single-threaded mode: no worker threads are started and parallel
        // jobs are drained on the main thread (EnqueueReadyLocked / ExecuteMainThreadJobs / WaitForIdle).
        // Always used on WebAssembly; can be requested explicitly to exercise that path on other platforms
        mStopping.Store(false);
        mWorkersCount = workersCount;
        mWorkers.Reserve(workersCount);
        for (int i = 0; i < workersCount; i++)
            mWorkers.emplace_back([this] { WorkerLoop(); });
    }

    void JobSystem::Shutdown()
    {
        if (mWorkersCount == 0)
            return;

        mStopping.Store(true);
        mWorkAvailable.NotifyAll();

        for (auto& worker : mWorkers)
            worker.Join();

        mWorkers.Clear();
        mWorkersCount = 0;

        UniqueLock lock(mMutex);
        for (int p = 0; p < kPriorityCount; p++)
        {
            mParallelReady[p].clear();
            mMainReady[p].clear();
        }
        mUnfinishedParallel.Store(0);
    }

    SharedRef<Job> JobSystem::CreateJob(const Function<void()>& body, JobPriority priority, JobThread thread)
    {
        auto job = MakeShared<Job>(body, priority, thread);
        job->mSystem = this;
        return job;
    }

    void JobSystem::Submit(const SharedRef<Job>& job)
    {
        if (!job)
            return;

        UniqueLock lock(mMutex);

        if (job->mSubmitted)
            return;

        job->mSubmitted = true;

        if (job->mThread == JobThread::Any)
            mUnfinishedParallel.FetchAdd(1);

        if (job->mRemainingDependencies.Load() == 0)
            EnqueueReadyLocked(job);
        else
            job->mState.Store((int)JobState::Waiting);
    }

    SharedRef<Job> JobSystem::Schedule(const Function<void()>& body, JobPriority priority, JobThread thread)
    {
        auto job = CreateJob(body, priority, thread);
        Submit(job);
        return job;
    }

    void JobSystem::AddDependency(const SharedRef<Job>& job, const SharedRef<Job>& dependency)
    {
        if (!job || !dependency)
            return;

        UniqueLock lock(mMutex);

        // If the dependency already finished there is nothing to wait for
        if (dependency->mState.Load() == (int)JobState::Done)
            return;

        job->mRemainingDependencies.FetchAdd(1);
        dependency->mDependents.Add(job);
    }

    void JobSystem::ExecuteMainThreadJobs(float quotaSeconds)
    {
        PROFILE_SAMPLE_FUNC();

        auto start = std::chrono::steady_clock::now();

        // Cooperative single-threaded mode: bound the pass to the jobs already queued when it starts, so a
        // job that re-schedules itself (a continuous coroutine loop) advances one step per frame instead
        // of looping forever within one frame. -1 (with workers) means "drain until empty"
        int budget = -1;
        if (mWorkersCount == 0)
        {
            UniqueLock lock(mMutex);
            budget = 0;
            for (int p = 0; p < kPriorityCount; p++)
                budget += (int)mMainReady[p].size();
        }

        for (;;)
        {
            if (budget == 0)
                break;

            SharedRef<Job> job;
            {
                UniqueLock lock(mMutex);
                job = PopHighestPriorityLocked(mMainReady);
            }

            if (!job)
                break;

            if (budget > 0)
                budget--;

            job->mState.Store((int)JobState::Running);
            {
                PROFILE_SAMPLE("o2 Main Thread Job");
                if (job->mBody)
                    job->mBody();
            }
            OnJobCompleted(job);

            if (quotaSeconds >= 0.0f)
            {
                float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
                if (elapsed >= quotaSeconds)
                    break;
            }
        }
    }

    void JobSystem::WaitForIdle()
    {
        // With no worker threads there is nothing to block on: drain the queued jobs on the calling
        // (main) thread until all parallel work has finished
        if (mWorkersCount == 0)
        {
            while (mUnfinishedParallel.Load() != 0)
                ExecuteMainThreadJobs(-1.0f);
            return;
        }

        int count;
        while ((count = mUnfinishedParallel.Load()) != 0)
            mUnfinishedParallel.WaitWhileEquals(count);
    }

    int JobSystem::GetWorkersCount() const
    {
        return mWorkersCount;
    }

    int JobSystem::GetUnfinishedParallelJobsCount() const
    {
        return mUnfinishedParallel.Load();
    }

    void JobSystem::WorkerLoop()
    {
        Thread::SetCurrentThreadName("o2JobWorker");
        PROFILE_THREAD("o2 Job Worker");

        for (;;)
        {
            SharedRef<Job> job;
            {
                UniqueLock lock(mMutex);
                mWorkAvailable.Wait(lock, [this] { return mStopping.Load() || HasParallelReadyLocked(); });

                // On shutdown exit at once rather than draining the queue: a coroutine can re-schedule
                // itself (e.g. a continuous fan-out loop), so draining might never finish. The pending
                // jobs are dropped and their queues cleared by Shutdown()
                if (mStopping.Load())
                    return;

                job = PopHighestPriorityLocked(mParallelReady);
            }

            if (!job)
                continue;

            job->mState.Store((int)JobState::Running);
            {
                PROFILE_SAMPLE("o2 Job");
                if (job->mBody)
                    job->mBody();
            }
            OnJobCompleted(job);
        }
    }

    void JobSystem::EnqueueReadyLocked(const SharedRef<Job>& job)
    {
        job->mState.Store((int)JobState::Ready);
        int priority = (int)job->mPriority;

        // With no worker threads (single-threaded WebAssembly) parallel jobs have nowhere to run, so they
        // join the main-thread queue and are drained cooperatively by ExecuteMainThreadJobs / WaitForIdle
        if (job->mThread == JobThread::Main || mWorkersCount == 0)
        {
            mMainReady[priority].push_back(job);
        }
        else
        {
            mParallelReady[priority].push_back(job);
            mWorkAvailable.NotifyOne();
        }
    }

    SharedRef<Job> JobSystem::PopHighestPriorityLocked(std::deque<SharedRef<Job>>* queues)
    {
        for (int priority = kPriorityCount - 1; priority >= 0; priority--)
        {
            if (!queues[priority].empty())
            {
                SharedRef<Job> job = std::move(queues[priority].front());
                queues[priority].pop_front();
                return job;
            }
        }

        return nullptr;
    }

    bool JobSystem::HasParallelReadyLocked() const
    {
        for (int priority = 0; priority < kPriorityCount; priority++)
        {
            if (!mParallelReady[priority].empty())
                return true;
        }

        return false;
    }

    void JobSystem::OnJobCompleted(const SharedRef<Job>& job)
    {
        Vector<SharedRef<Job>> newlyReady;
        {
            UniqueLock lock(mMutex);

            // Mark done first, so any AddDependency racing in after this sees Done and skips
            job->mState.Store((int)JobState::Done);

            for (auto& dependent : job->mDependents)
            {
                if (dependent->mRemainingDependencies.FetchSub(1) == 1 && dependent->mSubmitted)
                    newlyReady.Add(dependent);
            }
            job->mDependents.Clear();

            for (auto& ready : newlyReady)
                EnqueueReadyLocked(ready);
        }

        // Wake threads blocked in Job::Wait()
        job->mState.NotifyAll();

        if (job->mThread == JobThread::Any)
        {
            mUnfinishedParallel.FetchSub(1);
            mUnfinishedParallel.NotifyAll(); // wake WaitForIdle on every completion so it can re-check
        }

        if (job->onCompleted)
            job->onCompleted();
    }
}
