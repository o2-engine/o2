#pragma once
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    // ---------------------------------------------------------------------------------------------
    // Function holder delegate, used to wrap function and destroy it when subscription is destroyed
    // ---------------------------------------------------------------------------------------------
    template<typename _res_type, typename ... _args>
    class SubscriptionWrapper: public IFunction<_res_type(_args ...)>
    {
        Function<_res_type(_args ...)> mFunction;
        Function<void()> mOnDestroy;
        int* mRefs = mnew int;

    public:
        // Constructor
        SubscriptionWrapper(const Function<_res_type(_args ...)>& function, const Function<void()>& onDestroy) :
            mFunction(function), mOnDestroy(onDestroy)
        {
            (*mRefs) = 1;
        }

        // Copy-constructor
        SubscriptionWrapper(const SubscriptionWrapper& other) :
            mFunction(other.mFunction), mOnDestroy(other.mOnDestroy), mRefs(other.mRefs)
        {
            (*mRefs)++;
        }

        ~SubscriptionWrapper()
        {
            DecreaseRefs();
        }

        // Copy-operator
        SubscriptionWrapper& operator=(const SubscriptionWrapper& other)
        {
            DecreaseRefs();

            mFunction = other.mFunction;
            mOnDestroy = other.mOnDestroy;
            mRefs = other.mRefs;
            (*mRefs)++;

            return *this;
        }

        // Equal operator
        bool operator==(const SubscriptionWrapper& other) const
        {
            return mFunction == other.mFunction;
        }

        // Not equal operator
        bool operator!=(const SubscriptionWrapper& other) const
        {
            return mFunction != other.mFunction;
        }

        // Returns cloned copy of this
        IFunction<_res_type(_args ...)>* MakeClone() const override
        {
            return mnew SubscriptionWrapper(*this);
        }

        // Returns cloned emplace copy of this in memory
        IFunction<_res_type(_args ...)>* MakeClone(void* memory) const override
        {
            return new (memory) SubscriptionWrapper(*this);
        }

        // Invokes function with arguments as functor
        _res_type Invoke(_args ... args) const override
        {
            return mFunction.Invoke(args ...);
        }

        // Returns true if functions is equal
        bool Equals(const IFunction<_res_type(_args ...)>* other) const override
        {
            const SubscriptionWrapper* otherFuncPtr = dynamic_cast<const SubscriptionWrapper*>(other);
            if (otherFuncPtr)
                return *otherFuncPtr == *this;

            return mFunction.Equals(other);
        }

        // Returns size of function
        UInt GetSizeOf() const override
        {
            return sizeof(*this);
        }

    protected:
        void DecreaseRefs()
        {
            (*mRefs)--;
            if ((*mRefs) == 0)
            {
                mOnDestroy.Invoke();
                delete mRefs;
            }
        }
    };

    // -------------------------------------------------------------------------------------
    // Subscription holder, used to unsubscribe from function when subscription is destroyed
    // -------------------------------------------------------------------------------------
    class SubscriptionHolder: public RefCounterable
    {
    public:
        // Default constructor
        SubscriptionHolder(const Function<void()>& unsubscribe):
            mUnsubscribe(unsubscribe)
        {}

        // Destructor, unsubscribe from function
        ~SubscriptionHolder()
        {
            mUnsubscribe(); 
        }

        // Unsubscribe from function
        void Unsubscribe() 
        { 
            mUnsubscribe.Clear(); 
        }

    private:
        Function<void()> mUnsubscribe; // Function to unsubscribe from
    };

    // -----------------------------------------------------------------------------------------
    // Subscription, used automatically unsubscribe from function when subscription is destroyed
    // -----------------------------------------------------------------------------------------
    class Subscription
    {
    public:
        // Default constructor
        Subscription() = default;

        // Move constructor
        Subscription(Subscription&& other) noexcept:
            mSubscriptionHolder(std::move(other.mSubscriptionHolder)) 
        {}

        // Constructor from subscription holder
        Subscription(const Ref<SubscriptionHolder>& subscriptionHolder):
            mSubscriptionHolder(subscriptionHolder) 
        {}

        // Copy constructor
        Subscription(const Subscription& other):
            mSubscriptionHolder(other.mSubscriptionHolder) 
        {}

        // Destructor, resets subscription holder, which will unsubscribe from function
        ~Subscription()
        {
            mSubscriptionHolder = nullptr;
        }

        // Copy operator
        Subscription& operator=(const Subscription& other)
        {
            mSubscriptionHolder = other.mSubscriptionHolder;
            return *this;
        }

        // Move operator
        Subscription& operator=(Subscription&& other) noexcept
        {
            mSubscriptionHolder = std::move(other.mSubscriptionHolder);
            return *this;
        }

        // Equality operator
        bool operator==(const Subscription& other) const
        {
            return mSubscriptionHolder == other.mSubscriptionHolder;
        }
        
        // Inequality operator
        bool operator!=(const Subscription& other) const
        {
            return mSubscriptionHolder != other.mSubscriptionHolder;
        }

        // Clears subscription holder, which will unsubscribe from function
        void Clear()
        {
            mSubscriptionHolder = nullptr;
        }

    private:
        Ref<SubscriptionHolder> mSubscriptionHolder; // Subscription holder
    };

    // Makes subscription for function
    template<typename _res_type, typename ... _args>
    Subscription MakeSubscription(Function<_res_type(_args ...)>& subscribeTo, Function<_res_type(_args ...)> function)
    {
        auto subscriptionHolder = mmake<SubscriptionHolder>([&subscribeTo, function]() 
            {
                 subscribeTo -= function; 
            }
        );

        subscribeTo += SubscriptionWrapper<_res_type, _args...>(function, [subscriptionHolderWeak = WeakRef<SubscriptionHolder>(subscriptionHolder)]() 
            {
                if (auto subscriptionHolder = subscriptionHolderWeak.Lock())
                    subscriptionHolder->Unsubscribe(); 
            }
        );

        return Subscription(subscriptionHolder);
    }
}
