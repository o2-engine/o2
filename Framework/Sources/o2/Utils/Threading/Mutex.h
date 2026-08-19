#pragma once

#include <mutex>
#include <shared_mutex>

#ifdef TRACY_ENABLE
#include "tracy/Tracy.hpp"
#endif

namespace o2
{
    // ------------------------------------------------------------
    // Basic mutual exclusion lock, wrapper over std::mutex. Used to
    // protect shared data from being accessed by multiple threads.
    //
    // When Tracy profiling is enabled the underlying mutex is a
    // tracy::Lockable, so lock/unlock/contention show up in the
    // profiler's Locks view — no changes needed at the call sites
    // ------------------------------------------------------------
    class Mutex
    {
    public:
#ifdef TRACY_ENABLE
        using NativeType = tracy::Lockable<std::mutex>;
#else
        using NativeType = std::mutex;
#endif

    public:
        Mutex()
#ifdef TRACY_ENABLE
            : mMutex(SourceLocation())
#endif
        {}

        Mutex(const Mutex& other) = delete;
        Mutex& operator=(const Mutex& other) = delete;

        // Locks the mutex, blocks until the lock is acquired
        void Lock() { mMutex.lock(); }

        // Tries to lock the mutex, returns true if the lock was acquired without blocking
        bool TryLock() { return mMutex.try_lock(); }

        // Unlocks the mutex
        void Unlock() { mMutex.unlock(); }

        // Returns the underlying (possibly Tracy-instrumented) mutex, for use with condition variables
        // and unique locks
        NativeType& Base() { return mMutex; }

    protected:
        NativeType mMutex;

#ifdef TRACY_ENABLE
        static const tracy::SourceLocationData* SourceLocation()
        {
            static constexpr tracy::SourceLocationData srcloc{ nullptr, "o2::Mutex", __FILE__, __LINE__, 0 };
            return &srcloc;
        }
#endif
    };

    // ---------------------------------------------------------------------------
    // Recursive mutex, wrapper over std::recursive_mutex. Can be locked repeatedly
    // by the same thread without deadlocking; must be unlocked the same number of
    // times it was locked. Not Tracy-instrumented — Tracy's lock model assumes
    // single ownership, which recursive locking would confuse
    // ---------------------------------------------------------------------------
    class RecursiveMutex
    {
    public:
        RecursiveMutex() = default;
        RecursiveMutex(const RecursiveMutex& other) = delete;
        RecursiveMutex& operator=(const RecursiveMutex& other) = delete;

        // Locks the mutex, blocks until the lock is acquired
        void Lock() { mMutex.lock(); }

        // Tries to lock the mutex, returns true if the lock was acquired without blocking
        bool TryLock() { return mMutex.try_lock(); }

        // Unlocks the mutex
        void Unlock() { mMutex.unlock(); }

        // Returns the underlying std::recursive_mutex
        std::recursive_mutex& Base() { return mMutex; }

    protected:
        std::recursive_mutex mMutex; // Wrapped standard recursive mutex
    };

    // -------------------------------------------------------------------------------
    // Shared mutex, wrapper over std::shared_mutex. Allows multiple concurrent readers
    // (shared lock) or a single exclusive writer (exclusive lock). Tracy-instrumented
    // when profiling is enabled
    // -------------------------------------------------------------------------------
    class SharedMutex
    {
    public:
#ifdef TRACY_ENABLE
        using NativeType = tracy::SharedLockable<std::shared_mutex>;
#else
        using NativeType = std::shared_mutex;
#endif

    public:
        SharedMutex()
#ifdef TRACY_ENABLE
            : mMutex(SourceLocation())
#endif
        {}

        SharedMutex(const SharedMutex& other) = delete;
        SharedMutex& operator=(const SharedMutex& other) = delete;

        // Locks the mutex exclusively (for writing)
        void Lock() { mMutex.lock(); }

        // Tries to lock the mutex exclusively, returns true on success
        bool TryLock() { return mMutex.try_lock(); }

        // Unlocks the exclusive lock
        void Unlock() { mMutex.unlock(); }

        // Locks the mutex in shared mode (for reading)
        void LockShared() { mMutex.lock_shared(); }

        // Tries to lock the mutex in shared mode, returns true on success
        bool TryLockShared() { return mMutex.try_lock_shared(); }

        // Unlocks the shared lock
        void UnlockShared() { mMutex.unlock_shared(); }

        // Returns the underlying (possibly Tracy-instrumented) shared mutex
        NativeType& Base() { return mMutex; }

    protected:
        NativeType mMutex;

#ifdef TRACY_ENABLE
        static const tracy::SourceLocationData* SourceLocation()
        {
            static constexpr tracy::SourceLocationData srcloc{ nullptr, "o2::SharedMutex", __FILE__, __LINE__, 0 };
            return &srcloc;
        }
#endif
    };
}
