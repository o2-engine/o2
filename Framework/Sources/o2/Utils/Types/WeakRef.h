#pragma once

#include "o2/Utils/Types/Ref.h"

namespace o2
{
    // --------------
    // Weak reference
    // --------------
    template<typename _type>
    class WeakRef
    {
    public:
        // Default constructor
        WeakRef();

        // Constructor from nullptr
        WeakRef(std::nullptr_t);

        // Constructor from pointer
        explicit WeakRef(_type* ptr);

        // Copy constructor
        WeakRef(const WeakRef<_type>& other);

        // Move constructor
        WeakRef(WeakRef<_type>&& other) noexcept;

        // Copy constructor from strong reference
        WeakRef(const Ref<_type>& other);

        // Move constructor from strong reference
        WeakRef(Ref<_type>&& other);

        // Destructor
        ~WeakRef();

        // Equality operator
        bool operator==(const WeakRef<_type>& other) const;

        // Equality operator
        bool operator==(const _type* other) const;

        // Inequality operator
        bool operator!=(const WeakRef<_type>& other) const;

        // Inequality operator
        bool operator!=(const _type* other) const;

        // Copy from nullptr operator
        WeakRef& operator=(std::nullptr_t);

        // Copy operator
        WeakRef& operator=(const WeakRef<_type>& other);

        // Move operator
        WeakRef& operator=(WeakRef<_type>&& other);

        // Copy operator from strong reference
        WeakRef& operator=(const Ref<_type>& other);

        // Move operator from strong reference
        WeakRef& operator=(Ref<_type>&& other);

        // Less operator
        bool operator<(const WeakRef<_type>& other) const { return mPtr < other.mPtr; }

        // Returns true if reference is valid
        bool IsValid() const;

        // Returns true if reference is expired
        bool IsExpired() const;

        // Boolean cast operator, true means that reference is valid
        explicit operator bool() const;

        // Returns strong reference to object
        Ref<_type> Lock() const;

    protected:
        _type*      mPtr = nullptr;        // Pointer to object
        RefCounter* mRefCounter = nullptr; // Pointer to reference counter

    protected:
        void CheckRefCounter();  // Gets reference counter if pointer is valid
        void IncrementWeakRef(); // Increments weak reference counter
        void DecrementWeakRef(); // Decrements weak reference counter

        template<typename _other_type>
        friend class Ref;

        template<typename _to_type, typename _from_type>
        friend WeakRef<_to_type> DynamicWeakCast(const WeakRef<_from_type>& from);
    };

    // Dynamic cast from one weak reference type to another
    template<typename _to_type, typename _from_type>
    WeakRef<_to_type> DynamicWeakCast(const WeakRef<_from_type>& from)
    {
        return WeakRef<_to_type>(dynamic_cast<_to_type*>(const_cast<_from_type*>(from.mPtr)));
    }

    template<typename _type>
    WeakRef<_type>::WeakRef() = default;

    template<typename _type>
    WeakRef<_type>::WeakRef(std::nullptr_t)
    {}

    template<typename _type>
    WeakRef<_type>::WeakRef(_type* ptr) :
        mPtr(ptr)
    {
        CheckRefCounter();
        IncrementWeakRef();
    }

    template<typename _type>
    WeakRef<_type>::WeakRef(const WeakRef<_type>& other) :
        mPtr(other.mPtr), mRefCounter(other.mRefCounter)
    {
        IncrementWeakRef();
    }

    template<typename _type>
    WeakRef<_type>::WeakRef(WeakRef<_type>&& other) noexcept :
        mPtr(other.mPtr), mRefCounter(other.mRefCounter)
    {
        other.mPtr = nullptr;
        other.mRefCounter = nullptr;
    }

    template<typename _type>
    WeakRef<_type>::WeakRef(const Ref<_type>& other) :
        mPtr(other.mPtr)
    {
        CheckRefCounter();
        IncrementWeakRef();
    }

    template<typename _type>
    WeakRef<_type>::WeakRef(Ref<_type>&& other) :
        mPtr(other.mPtr)
    {
        // The moved-from strong reference is released, not kept: a weak reference never owns
        CheckRefCounter();
        IncrementWeakRef();
        other = nullptr;
    }

    template<typename _type>
    WeakRef<_type>::~WeakRef()
    {
        DecrementWeakRef();
    }

    template<typename _type>
    bool WeakRef<_type>::operator==(const WeakRef<_type>& other) const
    {
        return mPtr == other.mPtr;
    }

    template<typename _type>
    bool WeakRef<_type>::operator==(const _type* other) const
    {
        return mPtr == other;
    }

    template<typename _type>
    bool WeakRef<_type>::operator!=(const WeakRef<_type>& other) const
    {
        return mPtr != other.mPtr;
    }

    template<typename _type>
    bool WeakRef<_type>::operator!=(const _type* other) const
    {
        return mPtr != other;
    }

    template<typename _type>
    WeakRef<_type>& WeakRef<_type>::operator=(std::nullptr_t)
    {
        DecrementWeakRef();

        mPtr = nullptr;
        mRefCounter = nullptr;

        return *this;
    }

    template<typename _type>
    WeakRef<_type>& WeakRef<_type>::operator=(const WeakRef<_type>& other)
    {
        DecrementWeakRef();

        mPtr = other.mPtr;
        CheckRefCounter();

        IncrementWeakRef();

        return *this;
    }

    template<typename _type>
    WeakRef<_type>& WeakRef<_type>::operator=(WeakRef<_type>&& other)
    {
        DecrementWeakRef();

        mPtr = other.mPtr;
        mRefCounter = other.mRefCounter;
        other.mPtr = nullptr;
        other.mRefCounter = nullptr;

        return *this;
    }

    template<typename _type>
    WeakRef<_type>& WeakRef<_type>::operator=(const Ref<_type>& other)
    {
        DecrementWeakRef();

        mPtr = other.mPtr;
        CheckRefCounter();

        IncrementWeakRef();

        return *this;
    }

    template<typename _type>
    WeakRef<_type>& WeakRef<_type>::operator=(Ref<_type>&& other)
    {
        DecrementWeakRef();

        mPtr = other.mPtr;
        CheckRefCounter();
        IncrementWeakRef();
        other = nullptr;

        return *this;
    }

    template<typename _type>
    bool WeakRef<_type>::IsValid() const
    {
        return !IsExpired();
    }

    template<typename _type>
    bool WeakRef<_type>::IsExpired() const
    {
        return mRefCounter ? mRefCounter->strongReferences == 0 : true;
    }

    template<typename _type>
    WeakRef<_type>::operator bool() const
    {
        return IsValid();
    }

    template<typename _type>
    Ref<_type> WeakRef<_type>::Lock() const
    {
        if (IsExpired())
            return Ref<_type>();

        return Ref<_type>(mPtr);
    }

    template<typename _type>
    void WeakRef<_type>::CheckRefCounter()
    {
        mRefCounter = mPtr ? GetRefCounter(mPtr) : nullptr;
    }

    template<typename _type>
    void WeakRef<_type>::IncrementWeakRef()
    {
        if (mRefCounter)
            mRefCounter->weakReferences++;
    }

    template<typename _type>
    void WeakRef<_type>::DecrementWeakRef()
    {
        if (mRefCounter)
        {
            mRefCounter->weakReferences--;
            if (mRefCounter->strongReferences == 0)
            {
                if (mRefCounter->weakReferences == 0)
                {
                    _mfree(mRefCounter);
                    mRefCounter = nullptr;
                    mPtr = nullptr;
                }
            }
        }
    }
}
