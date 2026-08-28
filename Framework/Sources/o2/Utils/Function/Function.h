#pragma once

#include <algorithm>

#include <string>
#include <functional>
#include <vector>
#include <cstring>
#include "o2/Utils/Memory/MemoryManager.h"

namespace o2
{
    class IAbstractValueProxy;

    // ---------------------------
    // Abstract function interface
    // ---------------------------
    class IAbstractFunction
    {
    public:
        // Calls a function passing parameters through a proxies, writes the value to result Proxy
        virtual void InvokeRaw(IAbstractValueProxy* resultProxy, IAbstractValueProxy** parameters, int parametersCount) {}
    };

    template <typename UnusedType>
    class IFunction;

    // ------------------------
    // Basic delegate interface
    // ------------------------
    template<typename _res_type, typename ... _args>
    class IFunction<_res_type(_args ...)>: public IAbstractFunction
    {
    public:
        // Virtual destructor
        virtual ~IFunction() {}

        // Returns cloned copy of this
        virtual IFunction* MakeClone() const = 0;

        // Returns cloned emplace copy of this in memory
        virtual IFunction* MakeClone(void* memory) const = 0;

        // Invokes function with arguments
        virtual _res_type Invoke(_args ... args) const = 0;

        // Returns true if other functions is equal
        virtual bool Equals(const IFunction<_res_type(_args ...)>* other) const = 0;

        // Calls a function passing parameters through a proxies, writes the value to result Proxy
        void InvokeRaw(IAbstractValueProxy* resultProxy, IAbstractValueProxy** parameters, int parametersCount) override {}

        // Returns size of function
        virtual UInt GetSizeOf() const = 0;

        // Invokes function with arguments as functor
        _res_type operator()(_args ... args) const
        {
            return Invoke(args ...);
        }
    };

    // ------------------------
    // Static function delegate
    // ------------------------
    template<typename _res_type, typename ... _args>
    class FunctionPtr : public IFunction<_res_type(_args ...)>
    {
        _res_type(*mFunctionPtr)(_args ... args); // Pointer to static function

    public:
        // Constructor
        FunctionPtr(_res_type(*functionPtr)(_args ... args)) :
            mFunctionPtr(functionPtr)
        {
        }

        // Copy-constructor
        FunctionPtr(const FunctionPtr& other) :
            mFunctionPtr(other.mFunctionPtr)
        {
        }

        // Copy-operator
        FunctionPtr& operator=(const FunctionPtr& other)
        {
            mFunctionPtr = other.mFunctionPtr;
            return *this;
        }

        // Equal operator
        bool operator==(const FunctionPtr& other) const
        {
            return mFunctionPtr == other.mFunctionPtr;
        }

        // Not equal operator
        bool operator!=(const FunctionPtr& other) const
        {
            return mFunctionPtr != other.mFunctionPtr;
        }

        // Returns cloned copy of this
        IFunction<_res_type(_args ...)>* MakeClone() const
        {
            return mnew FunctionPtr(*this);
        }

        // Returns cloned emplace copy of this in memory
        IFunction<_res_type(_args ...)>* MakeClone(void* memory) const
        {
            return new (memory) FunctionPtr(*this);
        }

        // Invokes function with arguments as functor
        _res_type Invoke(_args ... args) const
        {
            return mFunctionPtr(args ...);
        }

        // Returns true if functions is equal
        bool Equals(const IFunction<_res_type(_args ...)>* other) const
        {
            const FunctionPtr* otherFuncPtr = dynamic_cast<const FunctionPtr*>(other);
            if (otherFuncPtr)
                return *otherFuncPtr == *this;

            return false;
        }

        // Returns size of function
        UInt GetSizeOf() const override
        {
            return sizeof(*this);
        }
    };

    // ------------------------
    // Object function delegate
    // ------------------------
    template<typename _class_type, typename _res_type, typename ... _args>
    class ObjFunctionPtr : public IFunction<_res_type(_args ...)>
    {
        _res_type(_class_type::* mFunctionPtr)(_args ... args); // Pointer to function
        _class_type* mObject;                                  // Pointer to function's owner object

    public:
        // Constructor
        ObjFunctionPtr(_class_type* object, _res_type(_class_type::* functionPtr)(_args ... args)) :
            mFunctionPtr(functionPtr), mObject(object)
        {
        }

        // Copy-constructor
        ObjFunctionPtr(const ObjFunctionPtr& other) :
            mFunctionPtr(other.mFunctionPtr), mObject(other.mObject)
        {
        }

        // Copy-operator
        ObjFunctionPtr& operator=(const ObjFunctionPtr& other)
        {
            mFunctionPtr = other.mFunctionPtr;
            mObject = other.mObject;
            return *this;
        }

        // Equals operator
        bool operator==(const ObjFunctionPtr& other) const
        {
            return mObject == other.mObject && mFunctionPtr == other.mFunctionPtr;
        }

        // Not equals operator
        bool operator!=(const ObjFunctionPtr& other) const
        {
            return mObject != other.mObject || mFunctionPtr != other.mFunctionPtr;
        }

        // Returns cloned copy of this
        IFunction<_res_type(_args ...)>* MakeClone() const override
        {
            return mnew ObjFunctionPtr(*this);
        }

        // Returns cloned emplace copy of this in memory
        IFunction<_res_type(_args ...)>* MakeClone(void* memory) const override
        {
            return new (memory) ObjFunctionPtr(*this);
        }

        // Invokes function with arguments as functor
        _res_type Invoke(_args ... args) const override
        {
            return (mObject->*mFunctionPtr)(args ...);
        }

        // Returns true if functions is equal
        bool Equals(const IFunction<_res_type(_args ...)>* other) const override
        {
            const ObjFunctionPtr* otherFuncPtr = dynamic_cast<const ObjFunctionPtr*>(other);
            if (otherFuncPtr)
                return *otherFuncPtr == *this;

            return false;
        }

        // Returns size of function
        UInt GetSizeOf() const override
        {
            return sizeof(*this);
        }
    };

    // ---------------------------------
    // Object constant function delegate
    // ---------------------------------
    template<typename _class_type, typename _res_type, typename ... _args>
    class ObjConstFunctionPtr : public IFunction<_res_type(_args ...)>
    {
        _res_type(_class_type::* mFunctionPtr)(_args ... args) const; // Pointer to const function
        _class_type* mObject;                                        // Pointer to function's owner object

    public:
        // Constructor
        ObjConstFunctionPtr(_class_type* object, _res_type(_class_type::* functionPtr)(_args ... args) const) :
            mFunctionPtr(functionPtr), mObject(object)
        {
        }

        // Copy-constructor
        ObjConstFunctionPtr(const ObjConstFunctionPtr& other) :
            mFunctionPtr(other.mFunctionPtr), mObject(other.mObject)
        {
        }

        // Copy-operator
        ObjConstFunctionPtr& operator=(const ObjConstFunctionPtr& other)
        {
            mFunctionPtr = other.mFunctionPtr;
            mObject = other.mObject;
            return *this;
        }

        // Equals operator
        bool operator==(const ObjConstFunctionPtr& other) const
        {
            return mObject == other.mObject && mFunctionPtr == other.mFunctionPtr;
        }

        // Not equals operator
        bool operator!=(const ObjConstFunctionPtr& other) const
        {
            return mObject != other.mObject || mFunctionPtr != other.mFunctionPtr;
        }

        // Returns cloned copy of this
        IFunction<_res_type(_args ...)>* MakeClone() const
        {
            return mnew ObjConstFunctionPtr(*this);
        }

        // Returns cloned emplace copy of this in memory
        IFunction<_res_type(_args ...)>* MakeClone(void* memory) const
        {
            return new (memory) ObjConstFunctionPtr(*this);
        }

        // Invokes function with arguments as functor
        _res_type Invoke(_args ... args) const
        {
            return (mObject->*mFunctionPtr)(args ...);
        }

        // Returns true if functions is equal
        bool Equals(const IFunction<_res_type(_args ...)>* other) const
        {
            const ObjConstFunctionPtr* otherFuncPtr = dynamic_cast<const ObjConstFunctionPtr*>(other);
            if (otherFuncPtr)
                return *otherFuncPtr == *this;

            return false;
        }

        // Returns size of function
        UInt GetSizeOf() const override
        {
            return sizeof(*this);
        }
    };

    // ----------------------
    // Shared lambda delegate
    // ----------------------
    template<typename _lambda_type, typename _res_type, typename ... _args>
    class SharedLambda : public IFunction<_res_type(_args ...)>
    {
        _lambda_type mLambda;

    public:
        // Constructor
        SharedLambda(const _lambda_type& lambda) :
            mLambda(lambda)
        {
        }

        // Constructor
        SharedLambda(_lambda_type&& lambda) :
            mLambda(std::forward<_lambda_type>(lambda))
        {
        }

        // Copy-constructor
        SharedLambda(const SharedLambda& other) :
            mLambda(other.mLambda)
        {
        }

        // Move-constructor
        SharedLambda(SharedLambda&& other) :
            mLambda(std::forward<_lambda_type>(other.mLambda))
        {
        }

        // Copy-operator
        SharedLambda& operator=(const SharedLambda& other)
        {
            mLambda = other.mLambda;
            return *this;
        }

        // Returns cloned copy of this
        IFunction<_res_type(_args ...)>* MakeClone() const override
        {
            return mnew SharedLambda(*this);
        }

        // Returns cloned emplace copy of this in memory
        IFunction<_res_type(_args ...)>* MakeClone(void* memory) const override
        {
            return new (memory) SharedLambda(*this);
        }

        // Invokes function with arguments as functor
        _res_type Invoke(_args ... args) const override
        {
            return mLambda(args ...);
        }

        // Equal operator
        bool operator==(const SharedLambda& other) const
        {
            return true;
        }

        // Not equal operator
        bool operator!=(const SharedLambda& other) const
        {
            return false;
        }

        // Returns true if functions is equals
        bool Equals(const IFunction<_res_type(_args ...)>* other) const override
        {
            const SharedLambda* otherFuncPtr = dynamic_cast<const SharedLambda*>(other);
            if (otherFuncPtr)
                return *otherFuncPtr == *this;

            return false;
        }

        // Returns size of function
        UInt GetSizeOf() const override
        {
            return sizeof(*this);
        }
    };

    class ISerializableFunction;

    // -----------------------------------------------------
    // Basic interface for enumeration of internal functions
    // -----------------------------------------------------
    class AbstractFunction
    {
    public:
        // Iterates over functions over an abstract interface
        virtual void ForEachAbstract(const IFunction<void(const IAbstractFunction*)>& func) {}

        // Adds function to list
        virtual void AddActorSubscription() {}

        // Removes abstract function
        virtual void RemoveFunction(const IAbstractFunction* func) {}
    };

    template <typename UnusedType>
    class Function;

    // --------------------------------------------------
    // Combined delegate. Can contain many other functors
    // --------------------------------------------------
    template<typename _res_type, typename ... _args>
    class Function<_res_type(_args ...)> : public IFunction<_res_type(_args ...)>, public AbstractFunction
    {
    protected:
        enum class DataType { Empty, OneFunction, CoupleOfFunctions };

        static constexpr UInt extraPayload = 40;
        static constexpr UInt payloadSize = sizeof(std::vector<void*>) + extraPayload;
        static constexpr UInt dataSize = payloadSize + sizeof(DataType);

        struct TypeData
        {
            Byte padding[sizeof(std::vector<void*>)];
            int  dispatchDepth; // Nested Invoke depth of the functions list: removals inside are tombstoned
            Byte padding2[payloadSize - sizeof(std::vector<void*>) - sizeof(int)];
            DataType type;
        };

        struct OneFunctionData
        {
            static constexpr UInt capacity = payloadSize - sizeof(void*);

            Byte functionData[capacity];
            void(*destructor)(IFunction<_res_type(_args ...)>*) = nullptr;
        };

        union Data
        {
            std::vector<IFunction<_res_type(_args ...)>*> functions;

            OneFunctionData oneFunctionData;

            TypeData typeData;

            Data() { typeData.type = DataType::Empty; }
            ~Data() {}
        };

    protected:
        Data mData;

    protected:
        IFunction<_res_type(_args ...)>& OneFunctionRef() const
        {
            return *const_cast<IFunction<_res_type(_args ...)>*>(reinterpret_cast<const IFunction<_res_type(_args ...)>*>(mData.oneFunctionData.functionData));
        }

        void DestroyOneFunction()
        {
            if (mData.oneFunctionData.destructor)
                (*mData.oneFunctionData.destructor)(&OneFunctionRef());
        }

        void ConvertToCoupleFunctionsType()
        {
            if (mData.typeData.type == DataType::OneFunction)
            {
                IFunction<_res_type(_args ...)>* firstFunction = OneFunctionRef().MakeClone();
                DestroyOneFunction();

                new (&mData.functions) std::vector<IFunction<_res_type(_args ...)>*>();
                mData.typeData.dispatchDepth = 0;
                mData.functions.push_back(firstFunction);
            }
            else if (mData.typeData.type == DataType::Empty)
                new (&mData.functions) std::vector<IFunction<_res_type(_args ...)>*>();
                mData.typeData.dispatchDepth = 0;

            mData.typeData.type = DataType::CoupleOfFunctions;
        }

    public:
        static const Function<_res_type(_args ...)> empty;

    public:
        // Constructor
        Function()
            : mData()
        {
        }

        // Copy-constructor
        Function(const Function& other)
        {
            if (other.mData.typeData.type == DataType::Empty)
                return;

            if (other.mData.typeData.type == DataType::OneFunction)
            {
                Add(other.OneFunctionRef());
                return;
            }

            for (auto& func : other.mData.functions)
            {
                if (func)
                    Add(*func);
            }
        }

        // Move-constructor
        Function(Function&& other)
        {
            memcpy(&mData, &other.mData, sizeof(Data));
            other.mData.typeData.type = DataType::Empty;
        }

        // Constructor from IFunction
        Function(const IFunction<_res_type(_args ...)>& func)
            : mData()
        {
            Add(func);
        }

        // Constructor from IFunction
        Function(IFunction<_res_type(_args ...)>&& func)
            : mData()
        {
            Emplace(func);
        }

        // Constructor from static function pointer
        template<typename _static_func_type, typename enable = typename std::enable_if<std::is_function<_static_func_type>::value>::type>
        Function(const _static_func_type* func) :
            Function()
        {
            Emplace(FunctionPtr<_res_type, _args ...>(func));
        }

        // Constructor from lambda
        template<typename _lambda_type, typename enable = typename std::enable_if<std::is_invocable_r<_res_type, _lambda_type, _args ...>::value && !std::is_base_of<IFunction<_res_type(_args ...)>, _lambda_type>::value>::type>
        Function(const _lambda_type& lambda) :
            Function()
        {
            Emplace(SharedLambda<_lambda_type, _res_type, _args ...>(lambda));
        }

        // Move-constructor from lambda
        template<typename _lambda_type, typename enable = typename std::enable_if<
            std::is_invocable_r<_res_type, _lambda_type, _args ...>::value &&
            !std::is_base_of<IFunction<_res_type(_args ...)>, typename std::remove_reference<_lambda_type>::type>::value&&
            std::is_rvalue_reference<_lambda_type&&>::value
        >::type>
            Function(_lambda_type&& lambda) :
            Function()
        {
            Emplace(std::forward<SharedLambda<_lambda_type, _res_type, _args ...>>(SharedLambda<_lambda_type, _res_type, _args ...>(std::forward<_lambda_type>(lambda))));
        }

        // Constructor from object and his function
        template<typename _class_type>
        Function(_class_type* object, _res_type(_class_type::* functionPtr)(_args ... args))
            : Function()
        {
            Emplace(std::forward<ObjFunctionPtr<_class_type, _res_type, _args ...>>(ObjFunctionPtr<_class_type, _res_type, _args ...>(object, functionPtr)));
        }

        // Constructor from object and his function
        template<typename _class_type>
        Function(const ObjFunctionPtr<_class_type, _res_type, _args ...>& func) :
            Function()
        {
            Emplace(std::forward<ObjFunctionPtr<_class_type, _res_type, _args ...>>(ObjFunctionPtr<_class_type, _res_type, _args ...>(func)));
        }

        // Constructor from object and his function
        template<typename _class_type>
        Function(_class_type* object, _res_type(_class_type::* functionPtr)(_args ... args) const) :
            Function()
        {
            Emplace(std::forward<ObjConstFunctionPtr<_class_type, _res_type, _args ...>>(ObjConstFunctionPtr<_class_type, _res_type, _args ...>(object, functionPtr)));
        }

        // Destructor
        ~Function()
        {
            Clear();
        }

        // Returns cloned copy of this
        IFunction<_res_type(_args ...)>* MakeClone() const override
        {
            return mnew Function(*this);
        }

        // Returns cloned emplace copy of this in memory
        IFunction<_res_type(_args ...)>* MakeClone(void* memory) const override
        {
            return new (memory) Function(*this);
        }

        // Removing all inside functions
        void Clear()
        {
            if (mData.typeData.type == DataType::CoupleOfFunctions)
            {
                for (auto& func : mData.functions)
                {
                    delete func;
                    func = nullptr;
                }

                if (mData.typeData.dispatchDepth == 0)
                    mData.functions.clear();
            }
            else if (mData.typeData.type == DataType::OneFunction)
            {
                DestroyOneFunction();
                mData.typeData.type = DataType::Empty;
            }
        }

        // Returns true when function is empty
        bool IsEmpty() const
        {
            return mData.typeData.type == DataType::Empty ||
                (mData.typeData.type == DataType::CoupleOfFunctions && mData.functions.empty());
        }

        // Emplace function
        template<typename _function_type, typename enabled = typename std::enable_if<std::is_base_of<IFunction<_res_type(_args ...)>, _function_type>::value>::type>
        void Emplace(_function_type&& func)
        {
            auto size = sizeof(_function_type);
            if (mData.typeData.type == DataType::Empty && size <= OneFunctionData::capacity)
            {
                new (mData.oneFunctionData.functionData) _function_type(std::forward<_function_type>(func));
                mData.oneFunctionData.destructor = [](IFunction<_res_type(_args ...)>* f) { f->~IFunction<_res_type(_args ...)>(); };
                mData.typeData.type = DataType::OneFunction;
            }
            else
            {
                ConvertToCoupleFunctionsType();
                mData.functions.push_back(mnew _function_type(func));
            }
        }

        // Add function
        void Add(const IFunction<_res_type(_args ...)>& func)
        {
            auto size = func.GetSizeOf();
            if (mData.typeData.type == DataType::Empty && size <= OneFunctionData::capacity)
            {
                func.MakeClone(mData.oneFunctionData.functionData);
                mData.oneFunctionData.destructor = [](IFunction<_res_type(_args ...)>* f) { f->~IFunction<_res_type(_args ...)>(); };
                mData.typeData.type = DataType::OneFunction;
            }
            else
            {
                ConvertToCoupleFunctionsType();
                mData.functions.push_back(func.MakeClone());
            }
        }

        // Add function pointer
        void Add(IFunction<_res_type(_args ...)>* func)
        {
            if (mData.typeData.type != DataType::CoupleOfFunctions)
                ConvertToCoupleFunctionsType();

            mData.functions.push_back(func);
        }

        // Removes function
        void Remove(const IFunction<_res_type(_args ...)>& function)
        {
            if (mData.typeData.type == DataType::OneFunction)
            {
                if (OneFunctionRef().Equals(&function))
                {
                    DestroyOneFunction();
                    mData.typeData.type = DataType::Empty;
                }
            }
            else if (mData.typeData.type == DataType::CoupleOfFunctions)
            {
                for (auto funcIt = mData.functions.begin(); funcIt != mData.functions.end(); ++funcIt)
                {
                    if (*funcIt && (*funcIt)->Equals(&function))
                    {
                        delete* funcIt;
                        EraseFunction(funcIt);
                        break;
                    }
                }
            }
        }

        // Erases a functions list slot; while dispatching the slot is only tombstoned
        void EraseFunction(typename std::vector<IFunction<_res_type(_args ...)>*>::iterator it)
        {
            if (mData.typeData.dispatchDepth > 0)
                *it = nullptr;
            else
                mData.functions.erase(it);
        }

        // Removes function pointer
        void Remove(const IFunction<_res_type(_args ...)>* function)
        {
            if (mData.typeData.type == DataType::CoupleOfFunctions)
            {
                auto fnd = std::find(mData.functions.begin(), mData.functions.end(), function);
                if (fnd != mData.functions.end())
                    EraseFunction(fnd);

                delete function;
            }
        }

        // Removes abstract function
        void RemoveFunction(const IAbstractFunction* func) override 
        {
            if (auto casted = dynamic_cast<const IFunction<_res_type(_args ...)>*>(func))
                Remove(*casted);
        }

        // Add delegate to inside list
        template<typename _class_type>
        void Add(_class_type* object, _res_type(_class_type::* functionPtr)(_args ... args))
        {
            Emplace(std::forward<ObjFunctionPtr<_class_type, _res_type, _args ...>>(ObjFunctionPtr<_class_type, _res_type, _args ...>(object, functionPtr)));
        }

        // Add delegate to inside list
        template<typename _class_type>
        void Add(_class_type* object, _res_type(_class_type::* functionPtr)(_args ... args) const)
        {
            Emplace(std::forward<ObjConstFunctionPtr<_class_type, _res_type, _args ...>>(ObjConstFunctionPtr<_class_type, _res_type, _args ...>(object, functionPtr)));
        }

        // Add delegate to inside list
        void Add(const Function& func)
        {
            if (func.mData.typeData.type == DataType::OneFunction)
                Add(func.OneFunctionRef());
            else if (func.mData.typeData.type == DataType::CoupleOfFunctions)
            {
                for (auto& x : func.mData.functions)
                {
                    if (x)
                        Add(*x);
                }
            }
        }

        // Remove delegate from list
        void Remove(const Function& func)
        {
            if (func.mData.typeData.type == DataType::OneFunction)
                Remove(func.OneFunctionRef());
            else if (func.mData.typeData.type == DataType::CoupleOfFunctions)
            {
                for (auto& x : func.mData.functions)
                {
                    if (x)
                        Remove(*x);
                }
            }
        }

        // Remove delegate from list
        template<typename _class_type>
        void Remove(_class_type* object, _res_type(_class_type::* functionPtr)(_args ... args))
        {
            Remove(ObjFunctionPtr<_class_type, _res_type, _args ...>(object, functionPtr));
        }

        // Remove delegate from list
        template<typename _class_type>
        void Remove(_class_type* object, _res_type(_class_type::* functionPtr)(_args ... args) const)
        {
            Remove(ObjConstFunctionPtr<_class_type, _res_type, _args ...>(object, functionPtr));
        }

        // Returns true, if this contains the delegate
        bool Contains(const IFunction<_res_type(_args ...)>& func) const
        {
            if (mData.typeData.type == DataType::OneFunction)
                return OneFunctionRef().Equals(&func);
            else if (mData.typeData.type == DataType::CoupleOfFunctions)
            {
                for (auto& x : mData.functions)
                {
                    if (x && x->Equals(&func))
                        return true;
                }
            }

            return false;
        }

        // Invokes function with arguments as functor
        _res_type operator()(_args ... args) const
        {
            return Invoke(args ...);
        }

        // Invokes function with arguments
        _res_type Invoke(_args ... args) const override
        {
            if (mData.typeData.type == DataType::OneFunction)
                return OneFunctionRef().Invoke(args ...);
            else if (mData.typeData.type == DataType::CoupleOfFunctions)
            {
                // handlers may unsubscribe (tombstoned while dispatching) or subscribe (appended, not called now)
                auto& depth = const_cast<int&>(mData.typeData.dispatchDepth);
                depth++;

                size_t count = mData.functions.size();
                auto stillDispatching = [&](size_t i) {
                    return mData.typeData.type == DataType::CoupleOfFunctions && i < mData.functions.size();
                };
                auto finish = [&]() {
                    if (mData.typeData.type != DataType::CoupleOfFunctions || depth <= 0)
                        return;

                    if (--depth == 0)
                    {
                        auto& functions = const_cast<std::vector<IFunction<_res_type(_args ...)>*>&>(mData.functions);
                        functions.erase(std::remove(functions.begin(), functions.end(), nullptr), functions.end());
                    }
                };

                if constexpr (std::is_void<_res_type>::value)
                {
                    for (size_t i = 0; i < count && stillDispatching(i); i++)
                    {
                        if (auto function = mData.functions[i])
                            function->Invoke(args ...);
                    }

                    finish();
                }
                else
                {
                    _res_type result = _res_type();
                    for (size_t i = 0; i < count && stillDispatching(i); i++)
                    {
                        if (auto function = mData.functions[i])
                            result = function->Invoke(args ...);
                    }

                    finish();
                    return result;
                }
            }

            return _res_type();
        }

        // Copy operator
        Function<_res_type(_args ...)>& operator=(const IFunction<_res_type(_args ...)>& func)
        {
            Clear();
            Add(func);
            return *this;
        }

        // Copy operator
        Function<_res_type(_args ...)>& operator=(IFunction<_res_type(_args ...)>&& func)
        {
            Clear();
            Emplace(func);
            return *this;
        }

        // Copy operator
        Function<_res_type(_args ...)>& operator=(const Function& other)
        {
            Clear();
            Add(other);
            return *this;
        }

        // Move operator
        Function<_res_type(_args ...)>& operator=(Function&& other)
        {
            Clear();
            memcpy(&mData, &other.mData, sizeof(Data));
            other.mData.typeData.type = DataType::Empty;

            return *this;
        }

        // Equal operator
        bool operator==(const Function& other) const
        {
            if (mData.typeData.type == DataType::Empty)
                return other.mData.typeData.type == DataType::Empty;

            if (mData.typeData.type == DataType::OneFunction)
            {
                if (other.mData.typeData.type == DataType::OneFunction)
                    return OneFunctionRef().Equals(&other.OneFunctionRef());
                else if (other.mData.typeData.type == DataType::CoupleOfFunctions)
                {
                    if (other.mData.functions.size() != 1 || !other.mData.functions[0])
                        return false;

                    return OneFunctionRef().Equals(other.mData.functions[0]);
                }

                return false;
            }

            // CoupleOfFunctions

            if (other.mData.typeData.type == DataType::Empty)
                return mData.functions.empty();

            if (other.mData.typeData.type == DataType::OneFunction)
            {
                if (mData.functions.size() != 1 || !mData.functions[0])
                    return false;

                return mData.functions[0]->Equals(&other.OneFunctionRef());
            }

            for (auto& func : mData.functions)
            {
                if (!func)
                    continue;

                bool found = false;
                for (auto& otherFunc : other.mData.functions)
                {
                    if (otherFunc && func->Equals(otherFunc))
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                    return false;
            }

            return true;
        }

        // Not equal operator
        bool operator!=(const Function& other) const
        {
            return !(*this == other);
        }

        // Equal operator
        bool operator==(const IFunction<_res_type(_args ...)>& func) const
        {
            if (mData.typeData.type == DataType::Empty)
                return false;

            if (mData.typeData.type == DataType::OneFunction)
                return OneFunctionRef().Equals(&func);

            if (mData.functions.size() != 1 || !mData.functions[0])
                return false;

            return mData.functions[0]->Equals(&func);
        }

        // Not equal operator
        bool operator!=(const IFunction<_res_type(_args ...)>& func) const
        {
            return !(*this == func);
        }

        // Returns true, when delegates list isn't empty
        operator bool() const
        {
            return !IsEmpty();
        }

        // Returns true when functions is equal
        bool Equals(const IFunction<_res_type(_args ...)>* other) const override
        {
            const Function* otherFuncPtr = dynamic_cast<const Function*>(other);
            if (otherFuncPtr)
                return *otherFuncPtr == *this;

            if (mData.typeData.type == DataType::OneFunction)
                return OneFunctionRef().Equals(other);

            return false;
        }

        // Add delegate to inside list
        Function<_res_type(_args ...)> operator+(const IFunction<_res_type(_args ...)>& func) const
        {
            Function<_res_type(_args ...)> res(*this);
            res.Add(func);
            return res;
        }

        // Add delegate to inside list
        Function<_res_type(_args ...)>& operator+=(const IFunction<_res_type(_args ...)>& func)
        {
            Add(func);
            return *this;
        }

        // Add delegate to inside list
        Function<_res_type(_args ...)> operator+(const Function& other) const
        {
            Function<_res_type(_args ...)> res(*this);
            res.Add(other);
            return res;
        }

        // Add delegate to inside list
        Function<_res_type(_args ...)>& operator+=(const Function& other)
        {
            Add(other);
            return *this;
        }

        // Removes delegate from list
        Function<_res_type(_args ...)> operator-(const IFunction<_res_type(_args ...)>& func) const
        {
            Function<_res_type(_args ...)> res(*this);
            res.Remove(func);
            return res;
        }

        // Removes delegate from list
        Function<_res_type(_args ...)>& operator-=(const IFunction<_res_type(_args ...)>& func)
        {
            Remove(func);
            return *this;
        }

        // Removes delegate from list
        Function<_res_type(_args ...)> operator-(const Function& other) const
        {
            Function<_res_type(_args ...)> res(*this);
            res.Remove(other);
            return res;
        }

        // Removes delegate from list
        Function<_res_type(_args ...)>& operator-=(const Function& other)
        {
            Remove(other);
            return *this;
        }

        // Iterating over functions
        template<typename _invocable>
        void ForEach(const _invocable& func) const
        {
            if (mData.typeData.type == DataType::OneFunction)
                func(&OneFunctionRef());
            else if (mData.typeData.type == DataType::CoupleOfFunctions)
            {
                for (auto& it : mData.functions)
                {
                    if (it)
                        func(it);
                }
            }
        }

        // Iterates over functions over an abstract interface
        void ForEachAbstract(const IFunction<void(const IAbstractFunction*)>& func) override
        {
            ForEach([&](const IFunction<_res_type(_args ...)>* x) { func(dynamic_cast<const IAbstractFunction*>(x)); });
        }

        // Returns size of function
        UInt GetSizeOf() const override
        {
            return sizeof(*this);
        }
    };

    template<typename _res_type, typename ... _args>
    const Function<_res_type(_args ...)> Function<_res_type(_args ...)>::empty;

    template<typename _class_type, typename _res_type, typename ... _args>
    Function<_res_type(_args ...)> MakeFunction(_class_type* object, _res_type(_class_type::* functionPtr)(_args ... args))
    {
        return Function<_res_type(_args ...)>(object, functionPtr);
    }

#define THIS_FUNC(NAME) MakeFunction(this, &thisclass::NAME)
}
