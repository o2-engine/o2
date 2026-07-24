#include "o2/stdafx.h"
#include "Job.h"

#include "o2/Utils/Jobs/JobSystem.h"

namespace o2
{
    Job::Job(const Function<void()>& body, JobPriority priority, JobThread thread):
        mBody(body), mPriority(priority), mThread(thread)
    {}

    JobPriority Job::GetPriority() const
    {
        return mPriority;
    }

    JobThread Job::GetThread() const
    {
        return mThread;
    }

    bool Job::IsDone() const
    {
        return mState.Load() == (int)JobState::Done;
    }

    bool Job::IsRunning() const
    {
        return mState.Load() == (int)JobState::Running;
    }

    void Job::Wait()
    {
        int state;
        while ((state = mState.Load()) != (int)JobState::Done)
            mState.WaitWhileEquals(state);
    }

    void Job::DependsOn(const SharedRef<Job>& dependency)
    {
        if (mSystem)
            mSystem->AddDependency(SharedRef<Job>(this), dependency);
    }

    SharedRef<Job> Job::Then(const Function<void()>& body, JobPriority priority, JobThread thread)
    {
        auto continuation = mSystem->CreateJob(body, priority, thread);
        mSystem->AddDependency(continuation, SharedRef<Job>(this));
        mSystem->Submit(continuation);
        return continuation;
    }
}
// --- META ---

ENUM_META(o2::JobPriority, o2__JobPriority)
{
    ENUM_ENTRY(Critical);
    ENUM_ENTRY(High);
    ENUM_ENTRY(Low);
    ENUM_ENTRY(Normal);
}
END_ENUM_META;

ENUM_META(o2::JobThread, o2__JobThread)
{
    ENUM_ENTRY(Any);
    ENUM_ENTRY(Main);
}
END_ENUM_META;

ENUM_META(o2::JobState, o2__JobState)
{
    ENUM_ENTRY(Created);
    ENUM_ENTRY(Done);
    ENUM_ENTRY(Ready);
    ENUM_ENTRY(Running);
    ENUM_ENTRY(Waiting);
}
END_ENUM_META;
// --- END META ---
