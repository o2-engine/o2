#pragma once

#include <condition_variable>
#include <chrono>
#include "o2/Utils/Threading/ScopeLock.h"

namespace o2
{
    // -----------------------------------------------------------------------------------
    // Condition variable, wrapper over std::condition_variable_any. Lets threads wait until
    // they are notified by another thread, releasing the associated UniqueLock while waiting.
    // condition_variable_any is used (instead of condition_variable) so it works with the
    // Tracy-instrumented mutex the UniqueLock wraps when profiling is enabled
    // -----------------------------------------------------------------------------------
    class ConditionVariable
    {
    public:
        ConditionVariable() = default;
        ConditionVariable(const ConditionVariable& other) = delete;
        ConditionVariable& operator=(const ConditionVariable& other) = delete;

        // Atomically releases the lock and blocks the thread until notified
        void Wait(UniqueLock& lock) { mConditionVariable.wait(lock.Base()); }

        // Blocks the thread until the predicate returns true, guarding against spurious wakeups
        template<typename _predicate>
        void Wait(UniqueLock& lock, _predicate predicate) { mConditionVariable.wait(lock.Base(), predicate); }

        // Blocks until the predicate is true or the timeout expires; returns the final predicate value
        template<typename _predicate>
        bool WaitFor(UniqueLock& lock, float seconds, _predicate predicate)
        {
            return mConditionVariable.wait_for(lock.Base(), std::chrono::duration<float>(seconds), predicate);
        }

        // Wakes up one thread waiting on this condition variable
        void NotifyOne() { mConditionVariable.notify_one(); }

        // Wakes up all threads waiting on this condition variable
        void NotifyAll() { mConditionVariable.notify_all(); }

    protected:
        std::condition_variable_any mConditionVariable; // Wrapped standard condition variable
    };
}
