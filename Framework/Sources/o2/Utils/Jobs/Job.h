#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Threading/Atomic.h"
#include "o2/Utils/Threading/SharedRef.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    class JobSystem;

    // Job scheduling priority. Higher priority jobs are picked by workers before lower ones
    enum class JobPriority { Low = 0, Normal = 1, High = 2, Critical = 3 };

    // Thread affinity of a job: Any runs on the parallel worker pool, Main runs only on the main
    // thread (drained under a time quota by JobSystem::ExecuteMainThreadJobs)
    enum class JobThread { Any, Main };

    // Lifecycle state of a job
    enum class JobState { Created, Waiting, Ready, Running, Done };

    // ------------------------------------------------------------------------------------------
    // A unit of work executed by the JobSystem. Carries a callable body, a priority and a thread
    // affinity, tracks completion and dependencies. Referenced through an atomic SharedRef so its
    // handle is safe to hold and drop on any thread.
    //
    // Isolation contract: a body that runs on the parallel workers (JobThread::Any) must not touch
    // shared o2::Ref state that the main thread concurrently mutates (o2::Ref is non-atomic). Work
    // that needs the scene/assets must use JobThread::Main
    // ------------------------------------------------------------------------------------------
    class Job : public ThreadSafeRefCounterable
    {
    public:
        // Optional callback invoked right after the body finishes, on the executing thread
        Function<void()> onCompleted;

    public:
        // Default constructor
        Job() = default;

        // Constructor from a body callable, priority and thread affinity
        Job(const Function<void()>& body, JobPriority priority = JobPriority::Normal, JobThread thread = JobThread::Any);

        // Returns job priority
        JobPriority GetPriority() const;

        // Returns thread affinity
        JobThread GetThread() const;

        // Returns true if the job has finished executing
        bool IsDone() const;

        // Returns true if the job is currently executing
        bool IsRunning() const;

        // Blocks the calling thread until the job is done. Never call it on a Main job from the main
        // thread, or on a job that transitively depends on the calling context — it would deadlock
        void Wait();

        // Declares that this job must start only after `dependency` completes. Set dependencies
        // before submitting the job to the JobSystem
        void DependsOn(const SharedRef<Job>& dependency);

        // Schedules a continuation job that runs after this one completes, returns its handle
        SharedRef<Job> Then(const Function<void()>& body, JobPriority priority = JobPriority::Normal,
                            JobThread thread = JobThread::Any);

    protected:
        JobSystem*             mSystem = nullptr;                       // Owning job system
        Function<void()>       mBody;                                   // Work to execute
        JobPriority            mPriority = JobPriority::Normal;         // Scheduling priority
        JobThread              mThread = JobThread::Any;                // Thread affinity
        Atomic<int>            mState{ (int)JobState::Created };        // Current lifecycle state
        Atomic<int>            mRemainingDependencies{ 0 };            // Unfinished dependencies count
        Vector<SharedRef<Job>> mDependents;                            // Jobs waiting on this one
        bool                   mSubmitted = false;                      // True once submitted to the system

        friend class JobSystem;
    };
}
// --- META ---

PRE_ENUM_META(o2::JobPriority);

PRE_ENUM_META(o2::JobThread);

PRE_ENUM_META(o2::JobState);
// --- END META ---
