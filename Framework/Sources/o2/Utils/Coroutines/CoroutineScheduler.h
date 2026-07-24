#pragma once

#include <vector>

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Threading/Atomic.h"
#include "o2/Utils/Threading/ConditionVariable.h"
#include "o2/Utils/Threading/Mutex.h"
#include "o2/Utils/Threading/Thread.h"
#include "o2/Utils/Types/Containers/Vector.h"

// Coroutine scheduler access macro
#define o2Coroutines o2::CoroutineScheduler::Instance()

namespace o2
{
    // ------------------------------------------------------------------------------------------
    // Coroutine scheduler singleton. Provides the time-based and frame-based wake-ups that the
    // coroutine awaiters need: a background timer thread wakes WaitTime awaiters when their delay
    // elapses, and a per-frame pump wakes WaitNextFrame awaiters. Wake-up actions themselves just
    // reschedule the coroutine's resumption on the job system
    // ------------------------------------------------------------------------------------------
    class CoroutineScheduler: public Singleton<CoroutineScheduler>
    {
    public:
        // Constructor
        CoroutineScheduler(RefCounter* refCounter);

        // Destructor, shuts the timer thread down
        ~CoroutineScheduler();

        // Starts the timer thread
        void Initialize();

        // Stops the timer thread and drops pending wake-ups
        void Shutdown();

        // Runs the action after the given delay in seconds (from a background timer thread)
        void ScheduleAfter(float seconds, const Function<void()>& action);

        // Runs the action on the next OnNewFrame() call
        void ScheduleNextFrame(const Function<void()>& action);

        // Fires all next-frame wake-ups registered up to now. Call once per frame on the main thread
        void OnNewFrame();

        // Returns number of pending timers
        int GetPendingTimersCount() const;

        // Returns number of pending next-frame wake-ups
        int GetPendingNextFrameCount() const;

    protected:
        // A single pending timer
        struct TimerEntry
        {
            double           deadline; // Absolute deadline, seconds on the steady clock
            Function<void()> action;   // Wake-up action
        };

        Thread                   mTimerThread;               // Background thread firing timers
        mutable Mutex            mTimerMutex;                // Guards mTimers
        ConditionVariable        mTimerCondition;            // Wakes the timer thread
        std::vector<TimerEntry>  mTimers;                    // Min-heap of pending timers by deadline
        Atomic<bool>             mStopping{ false };         // True while shutting down
        bool                     mInitialized = false;       // True once the timer thread runs

        mutable Mutex            mNextFrameMutex;            // Guards mNextFrameWaiters
        Vector<Function<void()>> mNextFrameWaiters;          // Actions to run next frame

    protected:
        // Timer thread main loop
        void TimerLoop();

        // Returns current steady-clock time in seconds
        static double NowSeconds();

        // Heap comparator making the nearest deadline the heap top
        static bool TimerIsLater(const TimerEntry& a, const TimerEntry& b) { return a.deadline > b.deadline; }
    };
}
