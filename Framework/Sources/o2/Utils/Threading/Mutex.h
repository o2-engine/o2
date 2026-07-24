#pragma once

#include <mutex>
#include <shared_mutex>

namespace o2
{
    // ------------------------------------------------------------
    // Basic mutual exclusion lock, wrapper over std::mutex. Used to
    // protect shared data from being accessed by multiple threads
    // ------------------------------------------------------------
    class Mutex
    {
    public:
        Mutex() = default;
        Mutex(const Mutex& other) = delete;
        Mutex& operator=(const Mutex& other) = delete;

        // Locks the mutex, blocks until the lock is acquired
        void Lock() { mMutex.lock(); }

        // Tries to lock the mutex, returns true if the lock was acquired without blocking
        bool TryLock() { return mMutex.try_lock(); }

        // Unlocks the mutex
        void Unlock() { mMutex.unlock(); }

        // Returns the underlying std::mutex, for use with condition variables and unique locks
        std::mutex& Base() { return mMutex; }

    protected:
        std::mutex mMutex; // Wrapped standard mutex
    };

    // ---------------------------------------------------------------------------
    // Recursive mutex, wrapper over std::recursive_mutex. Can be locked repeatedly
    // by the same thread without deadlocking; must be unlocked the same number of
    // times it was locked
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
    // (shared lock) or a single exclusive writer (exclusive lock)
    // -------------------------------------------------------------------------------
    class SharedMutex
    {
    public:
        SharedMutex() = default;
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

        // Returns the underlying std::shared_mutex
        std::shared_mutex& Base() { return mMutex; }

    protected:
        std::shared_mutex mMutex; // Wrapped standard shared mutex
    };
}
