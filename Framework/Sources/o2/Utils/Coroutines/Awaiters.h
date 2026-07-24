#pragma once

#include <coroutine>

#include "o2/Utils/Coroutines/Coroutine.h"
#include "o2/Utils/Coroutines/CoroutineControlBlock.h"
#include "o2/Utils/Coroutines/CoroutineScheduler.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Jobs/Job.h"
#include "o2/Utils/Threading/Atomic.h"
#include "o2/Utils/Threading/Mutex.h"
#include "o2/Utils/Threading/ScopeLock.h"
#include "o2/Utils/Threading/SharedRef.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    // ------------------------------------------------------------------------------------------
    // Awaitable that moves the rest of the current coroutine to another thread. co_await it to hop
    // the coroutine's execution to the parallel workers or back to the main thread
    // ------------------------------------------------------------------------------------------
    struct SwitchToThread
    {
        JobThread thread;

        explicit SwitchToThread(JobThread targetThread): thread(targetThread) {}

        bool await_ready() const noexcept { return false; }

        template<typename _promise>
        void await_suspend(std::coroutine_handle<_promise> handle) const
        {
            auto controlBlock = handle.promise().GetControlBlock();
            controlBlock->resumeThread.Store((int)thread);
            ScheduleCoroutineResume(controlBlock);
        }

        void await_resume() const noexcept {}
    };

    // Continues the coroutine on the parallel worker pool
    inline SwitchToThread SwitchToWorker() { return SwitchToThread(JobThread::Any); }

    // Continues the coroutine on the main thread (under the main-thread job quota)
    inline SwitchToThread SwitchToMain() { return SwitchToThread(JobThread::Main); }

    // ------------------------------------------------------------------------------------------
    // Awaitable that suspends the coroutine for the given number of seconds, resuming it from the
    // coroutine scheduler's timer thread
    // ------------------------------------------------------------------------------------------
    struct WaitTime
    {
        float seconds;

        explicit WaitTime(float delaySeconds): seconds(delaySeconds) {}

        bool await_ready() const noexcept { return seconds <= 0.0f; }

        template<typename _promise>
        void await_suspend(std::coroutine_handle<_promise> handle) const
        {
            auto controlBlock = handle.promise().GetControlBlock();
            o2Coroutines.ScheduleAfter(seconds, [controlBlock] { ScheduleCoroutineResume(controlBlock); });
        }

        void await_resume() const noexcept {}
    };

    // ------------------------------------------------------------------------------------------
    // Awaitable that suspends the coroutine until the next frame (the next OnNewFrame call)
    // ------------------------------------------------------------------------------------------
    struct WaitNextFrame
    {
        bool await_ready() const noexcept { return false; }

        template<typename _promise>
        void await_suspend(std::coroutine_handle<_promise> handle) const
        {
            auto controlBlock = handle.promise().GetControlBlock();
            o2Coroutines.ScheduleNextFrame([controlBlock] { ScheduleCoroutineResume(controlBlock); });
        }

        void await_resume() const noexcept {}
    };

    // ---------------------------------------------------------------------------------------------
    // Thread-safe synchronization primitive with wait/synchronize semantics. A coroutine co_awaits it
    // to suspend; any thread calls Synchronize() to release all current waiters. Also serves the
    // "wait for a callback" case: hand out a callback that calls Synchronize()
    // ---------------------------------------------------------------------------------------------
    class Signal
    {
    protected:
        // Shared, atomically counted state, so a Signal can be copied and used across threads
        struct SignalState: public ThreadSafeRefCounterable
        {
            Atomic<int>              signaled{ 0 }; // 1 once synchronized
            Mutex                    mutex;         // Guards waiters
            Vector<Function<void()>> waiters;       // Wake-up actions
        };

    public:
        // Awaiter returned by Wait()/co_await
        struct Awaiter
        {
            SharedRef<SignalState> state;

            bool await_ready() const { return state->signaled.Load() != 0; }

            template<typename _promise>
            void await_suspend(std::coroutine_handle<_promise> handle) const
            {
                auto controlBlock = handle.promise().GetControlBlock();
                bool fireNow = false;
                {
                    ScopeLock<Mutex> lock(state->mutex);
                    if (state->signaled.Load() != 0)
                        fireNow = true;
                    else
                        state->waiters.Add([controlBlock] { ScheduleCoroutineResume(controlBlock); });
                }

                if (fireNow)
                    ScheduleCoroutineResume(controlBlock);
            }

            void await_resume() const noexcept {}
        };

    public:
        // Constructor, creates a fresh unsignaled state
        Signal(): mState(MakeShared<SignalState>()) {}

        // Returns an awaiter that suspends the coroutine until the signal is synchronized
        Awaiter Wait() const { return Awaiter{ mState }; }

        // co_await support, same as Wait()
        Awaiter operator co_await() const { return Awaiter{ mState }; }

        // Releases all current waiters. Thread-safe, callable from any thread. Const because it mutates
        // the shared state, not the handle — so it works from a by-value capture in a job/lambda
        void Synchronize() const
        {
            Vector<Function<void()>> toFire;
            {
                ScopeLock<Mutex> lock(mState->mutex);
                mState->signaled.Store(1);
                toFire = mState->waiters;
                mState->waiters.Clear();
            }

            for (auto& waiter : toFire)
                waiter();
        }

        // Returns true if the signal has been synchronized
        bool IsSignaled() const { return mState->signaled.Load() != 0; }

        // Resets the signal back to the unsignaled state
        void Reset() const
        {
            ScopeLock<Mutex> lock(mState->mutex);
            mState->signaled.Store(0);
        }

    protected:
        SharedRef<SignalState> mState; // Shared signal state
    };

    // ---------------------------------------------------------------------------------------------
    // Awaitable that suspends the coroutine until every coroutine in the array has finished. Not-yet
    // started coroutines are started automatically. Results are read from the individual handles
    // ---------------------------------------------------------------------------------------------
    template<typename _type>
    struct WaitAllAwaiter
    {
        // Shared count of not-yet-finished coroutines
        struct State: public ThreadSafeRefCounterable
        {
            Atomic<int>                      remaining{ 0 };
            SharedRef<CoroutineControlBlock> parent;
        };

        Vector<Coroutine<_type>> coroutines;
        mutable SharedRef<State> state;

        bool await_ready() const
        {
            for (auto& coroutine : coroutines)
            {
                if (!coroutine.IsDone())
                    return false;
            }
            return true;
        }

        template<typename _promise>
        void await_suspend(std::coroutine_handle<_promise> handle) const
        {
            state = MakeShared<State>();
            state->parent = handle.promise().GetControlBlock();
            state->remaining.Store(coroutines.Count());

            if (coroutines.IsEmpty())
            {
                ScheduleCoroutineResume(state->parent);
                return;
            }

            auto sharedState = state;
            for (auto& coroutine : coroutines)
            {
                coroutine.StartIfNeeded();
                coroutine.GetControlBlock()->AddContinuation([sharedState] {
                    if (sharedState->remaining.FetchSub(1) == 1)
                        ScheduleCoroutineResume(sharedState->parent);
                });
            }
        }

        void await_resume() const noexcept {}
    };

    // Waits for all coroutines in the array to finish
    template<typename _type>
    WaitAllAwaiter<_type> WaitAll(const Vector<Coroutine<_type>>& coroutines)
    {
        return WaitAllAwaiter<_type>{ coroutines };
    }

    // ---------------------------------------------------------------------------------------------
    // Awaitable that suspends the coroutine until any coroutine in the array finishes. Returns the
    // index of the first one that finished. Not-yet-started coroutines are started automatically
    // ---------------------------------------------------------------------------------------------
    template<typename _type>
    struct WaitAnyAwaiter
    {
        // Shared trigger, so only the first completion resumes the parent
        struct State: public ThreadSafeRefCounterable
        {
            Atomic<int>                      triggered{ 0 };
            Atomic<int>                      index{ -1 };
            SharedRef<CoroutineControlBlock> parent;
        };

        Vector<Coroutine<_type>> coroutines;
        mutable SharedRef<State> state;

        bool await_ready() const noexcept { return false; }

        template<typename _promise>
        void await_suspend(std::coroutine_handle<_promise> handle) const
        {
            state = MakeShared<State>();
            state->parent = handle.promise().GetControlBlock();

            if (coroutines.IsEmpty())
            {
                ScheduleCoroutineResume(state->parent);
                return;
            }

            auto sharedState = state;
            for (int i = 0; i < coroutines.Count(); i++)
            {
                coroutines[i].StartIfNeeded();
                coroutines[i].GetControlBlock()->AddContinuation([sharedState, i] {
                    int expected = 0;
                    if (sharedState->triggered.CompareExchange(expected, 1))
                    {
                        sharedState->index.Store(i);
                        ScheduleCoroutineResume(sharedState->parent);
                    }
                });
            }
        }

        // Returns index of the first finished coroutine
        int await_resume() const { return state ? state->index.Load() : -1; }
    };

    // Waits until any coroutine in the array finishes, returns its index
    template<typename _type>
    WaitAnyAwaiter<_type> WaitAny(const Vector<Coroutine<_type>>& coroutines)
    {
        return WaitAnyAwaiter<_type>{ coroutines };
    }

    // Starts a coroutine on the given thread and returns its handle (for the "launch now, await later"
    // pattern used to run several coroutines in parallel)
    template<typename _type>
    Coroutine<_type> Async(Coroutine<_type> coroutine, JobThread thread = JobThread::Any)
    {
        coroutine.Start(thread);
        return coroutine;
    }
}
