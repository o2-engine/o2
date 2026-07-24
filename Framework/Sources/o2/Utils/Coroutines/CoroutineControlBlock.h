#pragma once

#include <coroutine>

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Jobs/Job.h"
#include "o2/Utils/Threading/Atomic.h"
#include "o2/Utils/Threading/Mutex.h"
#include "o2/Utils/Threading/ScopeLock.h"
#include "o2/Utils/Threading/SharedRef.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    // ---------------------------------------------------------------------------------------------
    // Shared, atomically reference-counted control block of a coroutine. It OWNS the coroutine frame
    // (destroys it when the last reference drops) and stores the completion state and the list of
    // continuations woken when the coroutine finishes. Because it is atomically counted (SharedRef),
    // a coroutine handle is safe to hold, copy and drop on any thread.
    //
    // Ownership: the promise holds only a RAW back-pointer to its control block (never a strong ref),
    // otherwise the block (which owns the frame that contains the promise) could never be destroyed
    // ---------------------------------------------------------------------------------------------
    class CoroutineControlBlock: public ThreadSafeRefCounterable
    {
    public:
        std::coroutine_handle<> handle;                     // Owned coroutine frame handle
        Atomic<int>             started{ 0 };               // 0 = not yet started, 1 = started
        Atomic<int>             done{ 0 };                  // 1 once the coroutine reached final suspend
        Atomic<int>             resumeThread{ (int)JobThread::Any };       // Thread affinity of resumptions
        Atomic<int>             resumePriority{ (int)JobPriority::Normal }; // Job priority of resumptions
        const char*             fiberName = nullptr;        // If set, resumptions are shown as this Tracy fiber

    public:
        // Destructor, destroys the owned coroutine frame
        ~CoroutineControlBlock() override
        {
            if (handle)
                handle.destroy();
        }

        // Returns true if the coroutine has finished
        bool IsDone() const { return done.Load() != 0; }

        // Marks the coroutine finished and fires all registered continuations. Called from final suspend
        void Complete()
        {
            Vector<Function<void()>> toFire;
            {
                ScopeLock<Mutex> lock(mMutex);
                mCompleted = true;
                done.Store(1);
                toFire = mContinuations;
                mContinuations.Clear();
            }

            done.NotifyAll(); // wake blocking waiters

            for (auto& continuation : toFire)
                continuation();
        }

        // Registers a continuation invoked when the coroutine finishes. If it already finished, the
        // continuation is invoked immediately (on the calling thread)
        void AddContinuation(const Function<void()>& continuation)
        {
            bool fireNow = false;
            {
                ScopeLock<Mutex> lock(mMutex);
                if (mCompleted)
                    fireNow = true;
                else
                    mContinuations.Add(continuation);
            }

            if (fireNow)
                continuation();
        }

    protected:
        Mutex                    mMutex;         // Guards continuations and completed flag
        Vector<Function<void()>> mContinuations; // Actions to run on completion
        bool                     mCompleted = false; // True once completed (guarded, paired with done)
    };

    // -----------------------------------------------------------------------------------------------
    // Common base of every coroutine promise. Holds a raw back-pointer to the control block so generic
    // awaiters can reach it through a std::coroutine_handle<Promise> without knowing the coroutine's
    // result type
    // -----------------------------------------------------------------------------------------------
    class CoroutinePromiseBase
    {
    public:
        // Returns raw control block pointer
        CoroutineControlBlock* GetControlBlockPtr() const { return mControlBlock; }

        // Returns a shared reference to the control block
        SharedRef<CoroutineControlBlock> GetControlBlock() const { return SharedRef<CoroutineControlBlock>(mControlBlock); }

    protected:
        CoroutineControlBlock* mControlBlock = nullptr; // Non-owning back-pointer to the control block
    };

    // Schedules the next resumption of a coroutine as a job on the worker system, on the coroutine's
    // current thread affinity
    void ScheduleCoroutineResume(const SharedRef<CoroutineControlBlock>& controlBlock);

    // Starts a coroutine if it hasn't started yet: sets its thread affinity and resume priority and
    // schedules the first resumption. Safe to call repeatedly; only the first call has effect.
    // fiberName, if not null, must be a persistent unique string and makes the coroutine's resumptions
    // appear on their own Tracy fiber track (following it across threads)
    void StartCoroutine(const SharedRef<CoroutineControlBlock>& controlBlock, JobThread thread,
                        JobPriority priority = JobPriority::Normal, const char* fiberName = nullptr);
}
