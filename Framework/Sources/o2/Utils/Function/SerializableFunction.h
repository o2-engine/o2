#pragma once
#include "o2/Utils/Serialization/DataValue.h"

namespace o2
{
    template <typename UnusedType>
    class SerializableFunction;

    // -------------------------------------------------
    // Serializable function, can add ActorSubscriptions
    // -------------------------------------------------
    template<typename _res_type, typename ... _args>
    class SerializableFunction<_res_type(_args ...)> : public Function<_res_type(_args ...)>
    {
    public:
        using Base = Function<_res_type(_args ...)>;

    public:
        // Constructor
        SerializableFunction() :
            Base()
        {}

        // Copy-constructor
        SerializableFunction(const SerializableFunction& other) :
            Base(other)
        {}

        // Move-constructor
        SerializableFunction(SerializableFunction&& other) :
            Base(other)
        {}

        // Constructor from IFunction
        explicit SerializableFunction(const IFunction<_res_type(_args ...)>& func) :
            Base(func)
        {}

        // Constructor from IFunction
        explicit SerializableFunction(IFunction<_res_type(_args ...)>&& func) :
            Base(std::move(func))
        {}

        // Constructor from static function pointer
        template<typename _static_func_type, typename enable = typename std::enable_if<std::is_function<_static_func_type>::value>::type>
        explicit SerializableFunction(const _static_func_type* func) :
            Base(func)
        {}

        // Constructor from lambda
        template<typename _lambda_type, typename enable = typename std::enable_if<std::is_invocable_r<_res_type, _lambda_type, _args ...>::value && !std::is_base_of<IFunction<_res_type(_args ...)>, _lambda_type>::value>::type>
        explicit SerializableFunction(const _lambda_type& lambda) :
            Base(lambda)
        {}

        // Move-constructor from lambda
        template<typename _lambda_type, typename enable = typename std::enable_if<
            std::is_invocable_r<_res_type, _lambda_type, _args ...>::value &&
            !std::is_base_of<IFunction<_res_type(_args ...)>, typename std::remove_reference<_lambda_type>::type>::value&&
            std::is_rvalue_reference<_lambda_type&&>::value>::type>
        explicit SerializableFunction(_lambda_type&& lambda) :
            Base(lambda)
        {}

        // Constructor from object and his function
        template<typename _class_type>
        SerializableFunction(_class_type* object, _res_type(_class_type::* functionPtr)(_args ... args)) :
            Base(object, functionPtr)
        {}

        // Constructor from object and his function
        template<typename _class_type>
        explicit SerializableFunction(const ObjFunctionPtr<_class_type, _res_type, _args ...>& func) :
            Base(func)
        {}

        // Constructor from object and his function
        template<typename _class_type>
        SerializableFunction(_class_type* object, _res_type(_class_type::* functionPtr)(_args ... args) const) :
            Base(object, functionPtr)
        {}

        // Returns cloned copy of this
        IFunction<_res_type(_args ...)>* MakeClone() const override
        {
            return mnew SerializableFunction(*this);
        }

        // Returns cloned emplace copy of this in memory
        IFunction<_res_type(_args ...)>* MakeClone(void* memory) const override
        {
            return new (memory) SerializableFunction(*this);
        }

        // Invokes function with arguments as functor
        _res_type operator()(_args ... args) const
        {
            return Base::Invoke(args ...);
        }

        // Copy operator
        SerializableFunction<_res_type(_args ...)>& operator=(const IFunction<_res_type(_args ...)>& func)
        {
            Base::operator=(func);
            return *this;
        }

        // Copy operator
        SerializableFunction<_res_type(_args ...)>& operator=(IFunction<_res_type(_args ...)>&& func)
        {
            Base::operator=(func);
            return *this;
        }

		// Copy operator
		SerializableFunction<_res_type(_args ...)>& operator=(const Function<_res_type(_args ...)>& other)
		{
			Base::operator=(other);
			return *this;
		}

		// Move operator
		SerializableFunction<_res_type(_args ...)>& operator=(Function<_res_type(_args ...)>&& other)
		{
			Base::operator=(other);
			return *this;
		}

        // Copy operator
        SerializableFunction<_res_type(_args ...)>& operator=(const SerializableFunction<_res_type(_args ...)>& other)
        {
            Base::operator=(other);
            return *this;
        }

        // Move operator
        SerializableFunction<_res_type(_args ...)>& operator=(SerializableFunction<_res_type(_args ...)>&& other)
        {
            Base::operator=(other);
            return *this;
        }

        // Equal operator
        bool operator==(const Function<_res_type(_args ...)>& other) const
        {
            return Base::operator==(other);
        }

        // Not equal operator
        bool operator!=(const Function<_res_type(_args ...)>& other) const
        {
            return !(*this == other);
        }

        // Equal operator
        bool operator==(const IFunction<_res_type(_args ...)>& func) const
        {
            return Base::operator==(func);
        }

        // Not equal operator
        bool operator!=(const IFunction<_res_type(_args ...)>& func) const
        {
            return !(*this == func);
        }

        // Returns true, when delegates list isn't empty
        operator bool() const
        {
            return Base::IsEmpty();
        }

        // Add delegate to inside list
        SerializableFunction<_res_type(_args ...)> operator+(const IFunction<_res_type(_args ...)>& func) const
        {
            return Base::operator+(func);
        }

        // Add delegate to inside list
        SerializableFunction<_res_type(_args ...)>& operator+=(const IFunction<_res_type(_args ...)>& func)
        {
            Base::operator+=(func);
            return *this;
        }

        // Add delegate to inside list
        SerializableFunction<_res_type(_args ...)> operator+(const Function<_res_type(_args ...)>& other) const
        {
            return Base::operator+(other);
        }

        // Add delegate to inside list
        SerializableFunction<_res_type(_args ...)>& operator+=(const Function<_res_type(_args ...)>& other)
        {
            Base::operator+=(other);
            return *this;
        }

        // Removes delegate from list
        SerializableFunction<_res_type(_args ...)> operator-(const IFunction<_res_type(_args ...)>& func) const
        {
            return Base::operator-(func);
        }

        // Removes delegate from list
        SerializableFunction<_res_type(_args ...)>& operator-=(const IFunction<_res_type(_args ...)>& func)
        {
            Base::operator-=(func);
            return *this;
        }

        // Removes delegate from list
        SerializableFunction<_res_type(_args ...)> operator-(const Function<_res_type(_args ...)>& other) const
        {
            return Base::operator-(other);
        }

        // Removes delegate from list
        SerializableFunction<_res_type(_args ...)>& operator-=(const Function<_res_type(_args ...)>& other)
        {
            Base::operator-=(other);
            return *this;
        }

        // Returns size of function
        UInt GetSizeOf() const override
        {
            return sizeof(*this);
        }

        // Adds function to list
        void AddActorSubscription() override;
    };
}
