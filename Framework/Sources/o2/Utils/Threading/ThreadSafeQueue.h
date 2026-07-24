#pragma once

#include <deque>
#include <utility>
#include "o2/Utils/Threading/ConditionVariable.h"
#include "o2/Utils/Threading/Mutex.h"
#include "o2/Utils/Threading/ScopeLock.h"

namespace o2
{
    // -------------------------------------------------------------------------------------
    // Thread-safe multi-producer multi-consumer FIFO queue. Consumers can block until an item
    // is available. Closing the queue wakes all blocked consumers so worker threads can exit
    // -------------------------------------------------------------------------------------
    template<typename _type>
    class ThreadSafeQueue
    {
    public:
        ThreadSafeQueue() = default;
        ThreadSafeQueue(const ThreadSafeQueue& other) = delete;
        ThreadSafeQueue& operator=(const ThreadSafeQueue& other) = delete;

        // Pushes a copy of the value to the back of the queue and wakes one waiting consumer
        void Push(const _type& value)
        {
            {
                UniqueLock lock(mMutex);
                mQueue.push_back(value);
            }
            mConditionVariable.NotifyOne();
        }

        // Moves the value to the back of the queue and wakes one waiting consumer
        void Push(_type&& value)
        {
            {
                UniqueLock lock(mMutex);
                mQueue.push_back(std::move(value));
            }
            mConditionVariable.NotifyOne();
        }

        // Tries to pop the front value without blocking. Returns false if the queue is empty
        bool TryPop(_type& outValue)
        {
            UniqueLock lock(mMutex);
            if (mQueue.empty())
                return false;

            outValue = std::move(mQueue.front());
            mQueue.pop_front();
            return true;
        }

        // Blocks until an item is available and pops it, or until the queue is closed. Returns
        // false only when the queue is closed and empty
        bool WaitAndPop(_type& outValue)
        {
            UniqueLock lock(mMutex);
            mConditionVariable.Wait(lock, [&] { return !mQueue.empty() || mClosed; });

            if (mQueue.empty())
                return false;

            outValue = std::move(mQueue.front());
            mQueue.pop_front();
            return true;
        }

        // Blocks until an item is available, the timeout expires, or the queue is closed. Returns
        // true if an item was popped
        bool WaitAndPopFor(_type& outValue, float seconds)
        {
            UniqueLock lock(mMutex);
            if (!mConditionVariable.WaitFor(lock, seconds, [&] { return !mQueue.empty() || mClosed; }))
                return false;

            if (mQueue.empty())
                return false;

            outValue = std::move(mQueue.front());
            mQueue.pop_front();
            return true;
        }

        // Closes the queue and wakes all waiting consumers. They receive whatever is queued, then
        // WaitAndPop starts returning false
        void Close()
        {
            {
                UniqueLock lock(mMutex);
                mClosed = true;
            }
            mConditionVariable.NotifyAll();
        }

        // Reopens a previously closed queue
        void Reopen()
        {
            UniqueLock lock(mMutex);
            mClosed = false;
        }

        // Returns true if the queue has been closed
        bool IsClosed() const
        {
            UniqueLock lock(mMutex);
            return mClosed;
        }

        // Removes all queued items
        void Clear()
        {
            UniqueLock lock(mMutex);
            mQueue.clear();
        }

        // Returns count of queued items
        int Count() const
        {
            UniqueLock lock(mMutex);
            return (int)mQueue.size();
        }

        // Returns true if the queue is empty
        bool IsEmpty() const
        {
            UniqueLock lock(mMutex);
            return mQueue.empty();
        }

    protected:
        mutable Mutex     mMutex;             // Guards the queue and closed flag
        ConditionVariable mConditionVariable; // Signals waiting consumers
        std::deque<_type> mQueue;             // Underlying FIFO storage
        bool              mClosed = false;    // True when closed, wakes consumers to let them exit
    };
}
