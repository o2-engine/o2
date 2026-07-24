#include "o2/stdafx.h"
#include "CoroutineScheduler.h"

#include <algorithm>
#include <chrono>

namespace o2
{
    DECLARE_SINGLETON(CoroutineScheduler);

    CoroutineScheduler::CoroutineScheduler(RefCounter* refCounter):
        Singleton<CoroutineScheduler>(refCounter)
    {}

    CoroutineScheduler::~CoroutineScheduler()
    {
        Shutdown();
    }

    void CoroutineScheduler::Initialize()
    {
        if (mInitialized)
            return;

        mStopping.Store(false);
        mInitialized = true;
        mTimerThread = Thread([this] { TimerLoop(); });
    }

    void CoroutineScheduler::Shutdown()
    {
        if (!mInitialized)
            return;

        mStopping.Store(true);
        mTimerCondition.NotifyAll();
        mTimerThread.Join();
        mInitialized = false;

        {
            ScopeLock<Mutex> lock(mTimerMutex);
            mTimers.clear();
        }
        {
            ScopeLock<Mutex> lock(mNextFrameMutex);
            mNextFrameWaiters.Clear();
        }
    }

    void CoroutineScheduler::ScheduleAfter(float seconds, const Function<void()>& action)
    {
        double deadline = NowSeconds() + (seconds > 0.0f ? seconds : 0.0f);
        {
            ScopeLock<Mutex> lock(mTimerMutex);
            mTimers.push_back({ deadline, action });
            std::push_heap(mTimers.begin(), mTimers.end(), TimerIsLater);
        }
        mTimerCondition.NotifyAll();
    }

    void CoroutineScheduler::ScheduleNextFrame(const Function<void()>& action)
    {
        ScopeLock<Mutex> lock(mNextFrameMutex);
        mNextFrameWaiters.Add(action);
    }

    void CoroutineScheduler::OnNewFrame()
    {
        PROFILE_SAMPLE_FUNC();

        Vector<Function<void()>> toFire;
        {
            ScopeLock<Mutex> lock(mNextFrameMutex);
            toFire = mNextFrameWaiters;
            mNextFrameWaiters.Clear();
        }

        for (auto& action : toFire)
            action();
    }

    int CoroutineScheduler::GetPendingTimersCount() const
    {
        ScopeLock<Mutex> lock(mTimerMutex);
        return (int)mTimers.size();
    }

    int CoroutineScheduler::GetPendingNextFrameCount() const
    {
        ScopeLock<Mutex> lock(mNextFrameMutex);
        return mNextFrameWaiters.Count();
    }

    void CoroutineScheduler::TimerLoop()
    {
        Thread::SetCurrentThreadName("o2CoroutineTimer");
        PROFILE_THREAD("o2 Coroutine Timer");

        UniqueLock lock(mTimerMutex);
        while (!mStopping.Load())
        {
            if (mTimers.empty())
            {
                mTimerCondition.Wait(lock);
                continue;
            }

            double now = NowSeconds();
            double nearest = mTimers.front().deadline;

            if (nearest <= now)
            {
                // Collect all timers that are due and fire them outside the lock
                std::vector<Function<void()>> due;
                while (!mTimers.empty() && mTimers.front().deadline <= NowSeconds())
                {
                    std::pop_heap(mTimers.begin(), mTimers.end(), TimerIsLater);
                    due.push_back(std::move(mTimers.back().action));
                    mTimers.pop_back();
                }

                lock.Unlock();
                for (auto& action : due)
                    action();
                lock.Lock();
            }
            else
            {
                // Sleep until the nearest deadline, waking early if a nearer timer is added or we stop
                mTimerCondition.WaitFor(lock, (float)(nearest - now), [this, nearest] {
                    return mStopping.Load() || (!mTimers.empty() && mTimers.front().deadline < nearest);
                });
            }
        }
    }

    double CoroutineScheduler::NowSeconds()
    {
        return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
}
