#pragma once

#include <mutex>
#include "o2/Utils/Threading/Mutex.h"

namespace o2
{
    // -------------------------------------------------------------------------------
    // RAII scoped exclusive lock over any o2 mutex (Mutex, RecursiveMutex, SharedMutex).
    // Locks the mutex on construction and unlocks it on destruction
    // -------------------------------------------------------------------------------
    template<typename _mutex_type>
    class ScopeLock
    {
    public:
        // Constructor, locks the mutex
        explicit ScopeLock(_mutex_type& mutex): mMutex(mutex) { mMutex.Lock(); }

        // Destructor, unlocks the mutex
        ~ScopeLock() { mMutex.Unlock(); }

        ScopeLock(const ScopeLock& other) = delete;
        ScopeLock& operator=(const ScopeLock& other) = delete;

    protected:
        _mutex_type& mMutex; // Referenced mutex
    };

    // -----------------------------------------------------------------------
    // RAII scoped shared (read) lock over an o2 SharedMutex. Locks in shared
    // mode on construction and unlocks on destruction, allowing many readers
    // -----------------------------------------------------------------------
    class SharedLock
    {
    public:
        // Constructor, locks the mutex in shared mode
        explicit SharedLock(SharedMutex& mutex): mMutex(mutex) { mMutex.LockShared(); }

        // Destructor, unlocks the shared lock
        ~SharedLock() { mMutex.UnlockShared(); }

        SharedLock(const SharedLock& other) = delete;
        SharedLock& operator=(const SharedLock& other) = delete;

    protected:
        SharedMutex& mMutex; // Referenced shared mutex
    };

    // Tag type to construct a UniqueLock without locking the mutex immediately
    struct DeferLock {};

    // ------------------------------------------------------------------------------
    // Movable, manually controllable exclusive lock over an o2 Mutex. Can be unlocked
    // and re-locked, and passed to a ConditionVariable to wait on
    // ------------------------------------------------------------------------------
    class UniqueLock
    {
    public:
        // Default constructor, holds no lock
        UniqueLock() = default;

        // Constructor, locks the mutex immediately
        explicit UniqueLock(Mutex& mutex): mLock(mutex.Base()) {}

        // Constructor that associates with the mutex but doesn't lock it
        UniqueLock(Mutex& mutex, DeferLock): mLock(mutex.Base(), std::defer_lock) {}

        UniqueLock(UniqueLock&& other) = default;
        UniqueLock& operator=(UniqueLock&& other) = default;

        UniqueLock(const UniqueLock& other) = delete;
        UniqueLock& operator=(const UniqueLock& other) = delete;

        // Locks the associated mutex
        void Lock() { mLock.lock(); }

        // Tries to lock the associated mutex, returns true on success
        bool TryLock() { return mLock.try_lock(); }

        // Unlocks the associated mutex
        void Unlock() { mLock.unlock(); }

        // Returns true if this lock currently owns the mutex
        bool OwnsLock() const { return mLock.owns_lock(); }

        // Returns the underlying std::unique_lock, for use with a ConditionVariable
        std::unique_lock<Mutex::NativeType>& Base() { return mLock; }

    protected:
        std::unique_lock<Mutex::NativeType> mLock; // Wrapped standard unique lock (over the Tracy-aware mutex)
    };
}
