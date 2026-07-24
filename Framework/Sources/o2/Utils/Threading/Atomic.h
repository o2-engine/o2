#pragma once

#include <atomic>

namespace o2
{
    // -----------------------------------------------------------------------------------
    // Atomic value, wrapper over std::atomic. Provides lock-free thread-safe operations on
    // a value with CamelCase o2 API. Non-copyable, like the underlying std::atomic
    // -----------------------------------------------------------------------------------
    template<typename _type>
    class Atomic
    {
    public:
        // Default constructor, value-initializes the atomic
        Atomic() : mValue(_type()) {}

        // Constructor from initial value
        Atomic(_type value) : mValue(value) {}

        Atomic(const Atomic& other) = delete;
        Atomic& operator=(const Atomic& other) = delete;

        // Atomically loads and returns the current value
        _type Load(std::memory_order order = std::memory_order_seq_cst) const { return mValue.load(order); }

        // Atomically stores a new value
        void Store(_type value, std::memory_order order = std::memory_order_seq_cst) { mValue.store(value, order); }

        // Atomically replaces the value and returns the previous one
        _type Exchange(_type value, std::memory_order order = std::memory_order_seq_cst) { return mValue.exchange(value, order); }

        // Atomically compares the value with expected and, if equal, sets it to desired. Strong version:
        // won't fail spuriously. Returns true if the exchange happened; otherwise writes the actual value into expected
        bool CompareExchange(_type& expected, _type desired, std::memory_order order = std::memory_order_seq_cst)
        {
            return mValue.compare_exchange_strong(expected, desired, order);
        }

        // Weak compare-and-exchange, may fail spuriously but is cheaper in loops
        bool CompareExchangeWeak(_type& expected, _type desired, std::memory_order order = std::memory_order_seq_cst)
        {
            return mValue.compare_exchange_weak(expected, desired, order);
        }

        // Atomically adds the value and returns the previous value
        _type FetchAdd(_type value, std::memory_order order = std::memory_order_seq_cst) { return mValue.fetch_add(value, order); }

        // Atomically subtracts the value and returns the previous value
        _type FetchSub(_type value, std::memory_order order = std::memory_order_seq_cst) { return mValue.fetch_sub(value, order); }

        // Blocks the calling thread while the value stays equal to `old`, until another thread
        // changes it and calls NotifyOne/NotifyAll. Cheap alternative to a condition variable
        void WaitWhileEquals(_type old, std::memory_order order = std::memory_order_seq_cst) const { mValue.wait(old, order); }

        // Wakes one thread blocked in WaitWhileEquals
        void NotifyOne() { mValue.notify_one(); }

        // Wakes all threads blocked in WaitWhileEquals
        void NotifyAll() { mValue.notify_all(); }

        // Implicit conversion loads the current value
        operator _type() const { return Load(); }

        // Assignment stores the value
        _type operator=(_type value) { Store(value); return value; }

        // Pre-increment, returns the new value
        _type operator++() { return mValue.fetch_add(1) + 1; }

        // Post-increment, returns the previous value
        _type operator++(int) { return mValue.fetch_add(1); }

        // Pre-decrement, returns the new value
        _type operator--() { return mValue.fetch_sub(1) - 1; }

        // Post-decrement, returns the previous value
        _type operator--(int) { return mValue.fetch_sub(1); }

    protected:
        std::atomic<_type> mValue; // Wrapped standard atomic
    };
}
