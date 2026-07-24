#pragma once

#include <utility>
#include "o2/Utils/Memory/MemoryManager.h"
#include "o2/Utils/Threading/Atomic.h"

namespace o2
{
    template<typename _type>
    class SharedRef;

    // ---------------------------------------------------------------------------------------------
    // Base class for objects whose references are shared across threads. Unlike o2::RefCounterable /
    // o2::Ref (whose counter is non-atomic and single-thread only), this uses an atomic reference
    // count, so a SharedRef to it may be safely copied and destroyed on any thread. Use it for the
    // handles that legitimately cross the main/worker boundary: jobs and coroutine state
    // ---------------------------------------------------------------------------------------------
    class ThreadSafeRefCounterable
    {
    public:
        virtual ~ThreadSafeRefCounterable() = default;

        // Returns current shared reference count
        int GetSharedReferencesCount() const { return mSharedRefCount.Load(); }

    protected:
        mutable Atomic<int> mSharedRefCount{ 0 }; // Atomic strong reference count

        // Atomically increments the reference count
        void AddSharedRef() const { mSharedRefCount.FetchAdd(1); }

        // Atomically decrements the reference count, returns true if this was the last reference
        bool ReleaseSharedRef() const { return mSharedRefCount.FetchSub(1) == 1; }

        template<typename _type>
        friend class SharedRef;
    };

    // ---------------------------------------------------------------------------------------------
    // Thread-safe intrusive shared reference, analogous to std::shared_ptr but atomically ref-counted
    // through ThreadSafeRefCounterable. Safe to copy/move/destroy across threads. Objects are created
    // with MakeShared and deleted when the last SharedRef drops
    // ---------------------------------------------------------------------------------------------
    template<typename _type>
    class SharedRef
    {
    public:
        // Default constructor, holds nothing
        SharedRef() = default;

        // Null constructor
        SharedRef(std::nullptr_t) {}

        // Constructor from a raw pointer, takes a shared reference to it
        explicit SharedRef(_type* ptr): mPtr(ptr)
        {
            if (mPtr)
                mPtr->AddSharedRef();
        }

        // Copy constructor
        SharedRef(const SharedRef& other): mPtr(other.mPtr)
        {
            if (mPtr)
                mPtr->AddSharedRef();
        }

        // Converting copy constructor (e.g. derived to base)
        template<typename _other_type, typename = std::enable_if_t<std::is_convertible_v<_other_type*, _type*>>>
        SharedRef(const SharedRef<_other_type>& other): mPtr(other.Get())
        {
            if (mPtr)
                mPtr->AddSharedRef();
        }

        // Move constructor
        SharedRef(SharedRef&& other) noexcept: mPtr(other.mPtr)
        {
            other.mPtr = nullptr;
        }

        // Destructor, releases the reference and deletes the object if it was the last one
        ~SharedRef() { Release(); }

        // Copy assign operator
        SharedRef& operator=(const SharedRef& other)
        {
            if (mPtr != other.mPtr)
            {
                Release();
                mPtr = other.mPtr;
                if (mPtr)
                    mPtr->AddSharedRef();
            }
            return *this;
        }

        // Move assign operator
        SharedRef& operator=(SharedRef&& other) noexcept
        {
            if (this != &other)
            {
                Release();
                mPtr = other.mPtr;
                other.mPtr = nullptr;
            }
            return *this;
        }

        // Null assign operator
        SharedRef& operator=(std::nullptr_t)
        {
            Release();
            mPtr = nullptr;
            return *this;
        }

        // Returns raw pointer
        _type* Get() const { return mPtr; }

        // Member access operator
        _type* operator->() const { return mPtr; }

        // Dereference operator
        _type& operator*() const { return *mPtr; }

        // Returns true if it holds an object
        bool IsValid() const { return mPtr != nullptr; }

        // Bool conversion, true if it holds an object
        explicit operator bool() const { return mPtr != nullptr; }

        // Equality operators
        bool operator==(const SharedRef& other) const { return mPtr == other.mPtr; }
        bool operator!=(const SharedRef& other) const { return mPtr != other.mPtr; }
        bool operator==(std::nullptr_t) const { return mPtr == nullptr; }
        bool operator!=(std::nullptr_t) const { return mPtr != nullptr; }

    protected:
        _type* mPtr = nullptr; // Referenced object

        // Releases the current reference, deleting the object if it was the last one
        void Release()
        {
            if (mPtr && mPtr->ReleaseSharedRef())
                delete mPtr;
        }

        template<typename _other_type>
        friend class SharedRef;
    };

    // Creates a new object managed by a SharedRef, allocated through the o2 memory manager
    template<typename _type, typename ... _args>
    SharedRef<_type> MakeShared(_args&& ... args)
    {
        _type* ptr = mnew _type(std::forward<_args>(args)...);
        return SharedRef<_type>(ptr);
    }

    // Static cast between shared reference types
    template<typename _to_type, typename _from_type>
    SharedRef<_to_type> StaticCastShared(const SharedRef<_from_type>& from)
    {
        return SharedRef<_to_type>(static_cast<_to_type*>(from.Get()));
    }
}
