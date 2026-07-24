#pragma once

#include <deque>

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Jobs/Job.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Threading/Atomic.h"
#include "o2/Utils/Threading/ConditionVariable.h"
#include "o2/Utils/Threading/Mutex.h"
#include "o2/Utils/Threading/SharedRef.h"
#include "o2/Utils/Threading/Thread.h"
#include "o2/Utils/Types/Containers/Vector.h"

// Job system access macro
#define o2Jobs o2::JobSystem::Instance()

namespace o2
{
    // --------------------------------------------------------------------------------------------
    // Job system singleton: a pool of worker threads that execute jobs off priority queues. Jobs can
    // run on the parallel workers (JobThread::Any) or be deferred to the main thread (JobThread::Main),
    // support priorities and dependencies, and the main-thread queue is drained under a time quota so
    // main-thread work never overruns its budget by more than one job
    // --------------------------------------------------------------------------------------------
    class JobSystem: public Singleton<JobSystem>
    {
    public:
        // Constructor
        JobSystem(RefCounter* refCounter);

        // Destructor, shuts the worker pool down
        ~JobSystem();

        // Starts the worker pool. workersCount < 0 uses (hardware threads - 1), minimum 1
        void Initialize(int workersCount = -1);

        // Stops and joins all worker threads and clears the queues
        void Shutdown();

        // Creates a job without submitting it, so dependencies can be set up first
        SharedRef<Job> CreateJob(const Function<void()>& body, JobPriority priority = JobPriority::Normal,
                                 JobThread thread = JobThread::Any);

        // Submits a previously created job for execution
        void Submit(const SharedRef<Job>& job);

        // Creates and immediately submits a job, returns its handle
        SharedRef<Job> Schedule(const Function<void()>& body, JobPriority priority = JobPriority::Normal,
                                JobThread thread = JobThread::Any);

        // Declares that `job` must run only after `dependency` completes. Call before submitting `job`
        void AddDependency(const SharedRef<Job>& job, const SharedRef<Job>& dependency);

        // Runs queued main-thread jobs on the calling (main) thread until the queue is empty or the
        // time quota is exceeded. quotaSeconds < 0 means no limit. Best-effort: a running job is never
        // interrupted, so the quota may be overrun by at most the duration of one job
        void ExecuteMainThreadJobs(float quotaSeconds = -1.0f);

        // Blocks until there are no unfinished parallel (JobThread::Any) jobs
        void WaitForIdle();

        // Returns number of worker threads
        int GetWorkersCount() const;

        // Returns number of parallel jobs that are queued or running but not finished
        int GetUnfinishedParallelJobsCount() const;

    protected:
        static constexpr int kPriorityCount = 4; // Number of priority levels (matches JobPriority)

        Vector<Thread>             mWorkers;                       // Worker threads
        mutable Mutex              mMutex;                         // Guards queues and the dependency graph
        ConditionVariable          mWorkAvailable;                 // Signals workers when parallel work appears
        std::deque<SharedRef<Job>> mParallelReady[kPriorityCount]; // Ready parallel jobs, per priority
        std::deque<SharedRef<Job>> mMainReady[kPriorityCount];     // Ready main-thread jobs, per priority
        Atomic<bool>               mStopping{ false };             // True while shutting down
        Atomic<int>                mUnfinishedParallel{ 0 };       // Submitted-but-not-finished parallel jobs
        int                        mWorkersCount = 0;              // Number of started workers

    protected:
        // Worker thread main loop
        void WorkerLoop();

        // Pushes a ready job to its priority queue (parallel or main). Must hold mMutex
        void EnqueueReadyLocked(const SharedRef<Job>& job);

        // Pops the highest-priority job from the given queue array, or empty if none. Must hold mMutex
        SharedRef<Job> PopHighestPriorityLocked(std::deque<SharedRef<Job>>* queues);

        // Returns true if any parallel job is ready. Must hold mMutex
        bool HasParallelReadyLocked() const;

        // Marks a job done, resolves its dependents and wakes waiters
        void OnJobCompleted(const SharedRef<Job>& job);

        friend class Job;
    };
}
