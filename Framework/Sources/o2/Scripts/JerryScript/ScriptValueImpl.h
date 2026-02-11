#pragma once

#include "o2/Utils/Reflection/Type.h"
#include "o2/Utils/Reflection/TypeTraits.h"
#include "o2/Utils/Debug/Debug.h"
#include <type_traits>
#include <functional>

#if defined(SCRIPTING_BACKEND_JERRYSCRIPT)

#if __cplusplus < 202002L
namespace std
{
    template<typename F, typename... FRONT_ARGS>
    auto bind_front(F&& f, FRONT_ARGS&&... front_args)
    {
        // front_args are copied because multiple invocations of this closure are possible
        return [captured_f = std::forward<F>(f), front_args...](auto&&... back_args) {
            return std::invoke(captured_f, front_args...,
                               std::forward<decltype(back_args)>(back_args)...);
        };
    }
}
#endif

namespace o2
{
    // ------------------------------
    // ScriptValueBase implementation
    // ------------------------------

    template<typename _type>
    ScriptValueBase::DataContainer<_type>::DataContainer(_type* data) :
        data(data)
    {
    }

    template<typename _type>
    ScriptValueBase::DataContainer<_type>::~DataContainer()
    {
        if (isDataOwner && data)
            delete data;
    }

    template<typename _type>
    void* ScriptValueBase::DataContainer<_type>::GetData() const
    {
        if constexpr (std::is_const<_type>::value)
            return nullptr;
        else
            return const_cast<_type*>(data);
    }

    template<typename _type>
    IObject* ScriptValueBase::DataContainer<_type>::TryCastToIObject() const
    {
        if constexpr (std::is_base_of<IObject, _type>::value)
            return dynamic_cast<IObject*>(data);

        return nullptr;
    }

    template<typename _type>
    const Type* ScriptValueBase::DataContainer<_type>::GetType() const
    {
        return &TypeOf(_type);
    }

    template<typename _type>
    ScriptValueBase::IDataContainer* ScriptValueBase::DataContainer<_type>::Clone() const
    {
        if (!data)
            return nullptr;

        _type* clonedData = nullptr;    
        if constexpr (std::is_copy_constructible<_type>::value)
            clonedData = mnew _type(*data);

        auto clonedContainer = mnew DataContainer<_type>(clonedData);
        clonedContainer->isDataOwner = true;
        return clonedContainer;
    }

    template<typename _invocable_type, typename _res_type, typename ... _args>
    struct ScriptFunctionContainer :
        public ScriptValueBase::DataContainer<_invocable_type>,
        public ScriptValueBase::TFunctionContainer<_res_type, int, _args ...>
    {
        ScriptFunctionContainer(_invocable_type* function) :
            ScriptValueBase::DataContainer<_invocable_type>(function)
        {
        }

        _res_type InvokeT(int, _args ... args) const override
        {
            if constexpr (sizeof...(_args) > 0)
                return (*this->data)(args...);
            else
                return (*this->data)();
        }
    };

    template<typename _invocable_type, typename _res_type, typename ... _args>
    struct ScriptThisFunctionContainer :
        public ScriptValueBase::DataContainer<_invocable_type>,
        public ScriptValueBase::TFunctionContainer<_res_type, ScriptValue, _args ...>
    {
        ScriptThisFunctionContainer(_invocable_type* function) :
            ScriptValueBase::DataContainer<_invocable_type>(function)
        {
        }

        _res_type InvokeT(ScriptValue this_, _args ... args) const override
        {
            return (*this->data)(this_, args ...);
        }
    };

    template<bool isConst, typename _class_type, typename _res_type, typename ... _args>
    struct ScriptClassFunction
    {
        using type = typename std::conditional<isConst, _res_type(_class_type::*)(_args ... args) const,
                                                        _res_type(_class_type::*)(_args ... args)>::type;
    };

    template<bool isConst, typename _class_type, typename _res_type, typename ... _args>
    struct ScriptClassFunctionContainer :
        public ScriptValueBase::DataContainer<typename ScriptClassFunction<isConst, _class_type, _res_type, _args ...>::type>,
        public ScriptValueBase::TFunctionContainer<_res_type, _class_type*, _args ...>
    {
        using FuncType = typename ScriptClassFunction<isConst, _class_type, _res_type, _args ...>::type;

        ScriptClassFunctionContainer(FuncType functionPtr) :
            ScriptValueBase::DataContainer<FuncType>(mnew FuncType(functionPtr))
        {
        }

        _res_type InvokeT(_class_type* this_, _args ... args) const override
        {
            return (this_->*(*this->data))(args ...);
        }
    };

    template<typename _res_type, typename _this_type, typename ... _args>
    jerry_value_t ScriptValueBase::TFunctionContainer<_res_type, _this_type, _args ...>::Invoke(jerry_value_t thisValue,
                                                                                                jerry_value_t* args, int argsCount)
    {
        using ThisClass = ScriptValueBase::TFunctionContainer<_res_type, _this_type, _args ...>;

        ScriptValue thisValueObj;
        thisValueObj.AcquireValue(thisValue);

        if constexpr (sizeof...(_args) > 0)
        {
            std::tuple<_this_type, typename RemoveConstAndRef<_args>::type...> argst;

            if constexpr (!std::is_void<_this_type>::value)
                std::get<0>(argst) = thisValueObj;

            UnpackArgs<1, 0, _this_type, typename RemoveConstAndRef<_args>::type...>(argst, args, argsCount);

            if constexpr (std::is_void<_res_type>::value)
            {
                std::apply(std::bind_front(&ThisClass::InvokeT, this), argst);
                return jerry_create_undefined();
            }
            else
            {
                ScriptValue res(std::apply(std::bind_front(&ThisClass::InvokeT, this), argst));
                return jerry_acquire_value(res.jvalue);
            }
        }
        else
        {
            if constexpr (std::is_void<_res_type>::value)
            {
                InvokeT(thisValueObj);
                return jerry_create_undefined();
            }
            else
            {
                ScriptValue res(InvokeT(thisValueObj));
                return jerry_acquire_value(res.jvalue);
            }
        }
    }

    template<typename _type>
    jerry_value_t ScriptValueBase::PointerGetterWrapperContainer<_type>::Get()
    {
        ScriptValue tmp;
        tmp.SetValue<_type>(*dataPtr);
        return jerry_acquire_value(tmp.jvalue);
    }

    template<typename _type>
    void ScriptValueBase::PointerSetterWrapperContainer<_type>::Set(jerry_value_t value)
    {
        ScriptValue tmp;
        tmp.AcquireValue(value);
        *dataPtr = tmp.GetValue<_type>();
    }

    template<typename _property_type>
    jerry_value_t ScriptValueBase::PropertyGetterWrapperContainer<_property_type>::Get()
    {
        ScriptValue tmp;
        tmp.SetValue<typename ExtractPropertyValueType<_property_type>::type>(propertyPtr->Get());
        return jerry_acquire_value(tmp.jvalue);
    }

    template<typename _property_type>
    void ScriptValueBase::PropertySetterWrapperContainer<_property_type>::Set(jerry_value_t value)
    {
        ScriptValue tmp;
        tmp.AcquireValue(value);
        propertyPtr->Set(tmp.GetValue<typename ExtractPropertyValueType<_property_type>::type>());
    }

    template<typename _type>
    jerry_value_t ScriptValueBase::FunctionalGetterWrapperContainer<_type>::Get()
    {
        ScriptValue tmp;
        tmp.SetValue<_type>(getter());
        return jerry_acquire_value(tmp.jvalue);
    }

    template<typename _type>
    void ScriptValueBase::FunctionalSetterWrapperContainer<_type>::Set(jerry_value_t value)
    {
        ScriptValue tmp;
        tmp.AcquireValue(value);
        setter(tmp.GetValue<_type>());
    }

    template<size_t _i /*= 0*/, size_t _j /*= 0*/, typename... _args>
    void ScriptValueBase::UnpackArgs(std::tuple<_args ...>& argst, jerry_value_t* args, int argsCount)
    {
        if (_j < argsCount)
        {
            ScriptValue tmp;
            tmp.AcquireValue(args[_j]);
            using x = typename std::remove_reference<decltype(std::get<_i>(argst))>::type;
            std::get<_i>(argst) = tmp.GetValue<x>();

            if constexpr (_i + 1 != sizeof...(_args))
                UnpackArgs<_i + 1, _j + 1>(argst, args, argsCount);
        }
    }

    // --------------------------
    // ScriptValue implementation
    // --------------------------

    template<typename _type>
    ScriptValue::ScriptValue(const _type& value)
    {
        Converter<_type>::Write(value, *this);
    }

    template<typename _type>
    ScriptValue::operator _type() const
    {
        _type res;
        Converter<_type>::Read(res, *this);
        return std::move(res);
    }

    template<typename _type>
    ScriptValue& ScriptValue::operator=(const _type& value)
    {
        SetValue(value);
        return *this;
    }

    template<typename _type>
    void ScriptValue::SetValue(const _type& value)
    {
        jerry_release_value(jvalue);
        Converter<_type>::Write(value, *this);
    }

    template<typename _type>
    _type ScriptValue::GetValue() const
    {
        _type res;
        Converter<_type>::Read(res, *this);
        return std::move(res);
    }

    template<typename ... _args>
    ScriptValue ScriptValue::Construct(_args ... args) const
    {
        Vector<ScriptValue> argsValues;

        if constexpr (sizeof...(_args) > 0)
            PackArgs(argsValues, args ...);

        return Construct(argsValues);
    }

    template<typename _type>
    void ScriptValue::SetContainingObject(_type* object, bool owner /*= true*/)
    {
        if (!object)
        {
            *this = ScriptValue();
            return;
        }

        auto dataContainer = mnew DataContainer<_type>(object);
        dataContainer->isDataOwner = owner;
        jerry_set_object_native_pointer(jvalue, (IDataContainer*)dataContainer, &GetDataDeleter().info);

        if constexpr (std::is_base_of<IObject, _type>::value)
        {
            object->ReflectIntoScriptValue(*this);
            SetPrototype(_type::GetScriptPrototype());
        }

        SetProperty("FreeOwnership", Function<ScriptValue()>(
            [d = dataContainer, j = jvalue]() {
                d->isDataOwner = false;
                ScriptValue th; th.AcquireValue(j);
                return th;
            }));
    }


    template<typename _type>
    void ScriptValue::SetProperty(const char* name, const _type& value)
    {
        SetProperty(ScriptValue(name), ScriptValue(value));
    }

    template<typename _class_type, typename _res_type, typename ... _args>
    void ScriptValue::SetProperty(const char* name, _class_type* object, _res_type(_class_type::* functionPtr)(_args ... args))
    {
        if constexpr (std::is_same<void, _res_type>::value)
        {
            SetProperty(name, std::function<void(_args ...)>(
                [object, functionPtr](_args ... args)
                {
                    (object->*functionPtr)(args ...);
                }));
        }
        else
        {
            using __res_type = typename std::remove_const<typename std::remove_reference<_res_type>::type>::type;
            SetProperty(name, std::function<__res_type(_args ...)>(
                [object, functionPtr](_args ... args)
                {
                    __res_type res = (object->*functionPtr)(args ...);
                    return res;
                }));
        }
    }

    template<typename _class_type, typename _res_type, typename ... _args>
    void ScriptValue::SetProperty(const char* name, _class_type* object, _res_type(_class_type::* functionPtr)(_args ... args) const)
    {
        if constexpr (std::is_same<void, _res_type>::value)
        {
            SetProperty(name, std::function<void(_args ...)>(
                [object, functionPtr](_args ... args)
                {
                    (object->*functionPtr)(args ...);
                }));
        }
        else
        {
            using __res_type = typename std::remove_const<typename std::remove_reference<_res_type>::type>::type;
            SetProperty(name, std::function<__res_type(_args ...)>(
                [object, functionPtr](_args ... args)
                {
                    __res_type res = (object->*functionPtr)(args ...);
                    return res;
                }));
        }
    }

    template<typename _type>
    void ScriptValue::SetPropertyWrapper(const ScriptValue& name, _type& value)
    {
        if (GetValueType() != ValueType::Object)
        {
            jerry_release_value(jvalue);
            jvalue = jerry_create_object();
        }

        jerry_property_descriptor_t propertyDescriptor;
        jerry_init_property_descriptor_fields(&propertyDescriptor);

        propertyDescriptor.is_enumerable = true;
        propertyDescriptor.is_enumerable_defined = true;

        propertyDescriptor.is_get_defined = true;
        propertyDescriptor.getter = jerry_create_external_function(DescriptorGetter);

        if constexpr (IsProperty<_type>::value)
        {
            auto getterWrapperContainer = new PropertyGetterWrapperContainer<_type>();
            getterWrapperContainer->propertyPtr = &value;
            jerry_set_object_native_pointer(propertyDescriptor.getter, getterWrapperContainer, &GetDataDeleter().info);
        }
        else
        {
            auto getterWrapperContainer = new PointerGetterWrapperContainer<_type>();
            getterWrapperContainer->dataPtr = &value;
            jerry_set_object_native_pointer(propertyDescriptor.getter, getterWrapperContainer, &GetDataDeleter().info);
        }

        propertyDescriptor.is_set_defined = true;
        propertyDescriptor.setter = jerry_create_external_function(DescriptorSetter);

        if constexpr (IsProperty<_type>::value)
        {
            auto setterWrapperContainer = new PropertySetterWrapperContainer<_type>();
            setterWrapperContainer->propertyPtr = &value;
            jerry_set_object_native_pointer(propertyDescriptor.setter, setterWrapperContainer, &GetDataDeleter().info);
        }
        else
        {
            auto setterWrapperContainer = new PointerSetterWrapperContainer<_type>();
            setterWrapperContainer->dataPtr = &value;
            jerry_set_object_native_pointer(propertyDescriptor.setter, setterWrapperContainer, &GetDataDeleter().info);
        }

        jerry_value_t newPropertyValue = jerry_define_own_property(jvalue, name.jvalue, &propertyDescriptor);
        jerry_release_value(newPropertyValue);

        jerry_free_property_descriptor_fields(&propertyDescriptor);
    }

    template<typename _type>
    void ScriptValue::SetPropertyWrapper(const ScriptValue& name, const Function<void(const _type& value)>& setter,
                                         const Function<_type()>& getter)
    {
        if (GetValueType() != ValueType::Object)
        {
            jerry_release_value(jvalue);
            jvalue = jerry_create_object();
        }

        jerry_property_descriptor_t propertyDescriptor;
        jerry_init_property_descriptor_fields(&propertyDescriptor);

        propertyDescriptor.is_enumerable = true;
        propertyDescriptor.is_enumerable_defined = true;

        propertyDescriptor.is_get_defined = true;
        propertyDescriptor.getter = jerry_create_external_function(DescriptorGetter);
        auto getterWrapperContainer = new FunctionalGetterWrapperContainer<_type>();
        getterWrapperContainer->getter = getter;
        jerry_set_object_native_pointer(propertyDescriptor.getter, getterWrapperContainer, &GetDataDeleter().info);

        propertyDescriptor.is_set_defined = true;
        propertyDescriptor.setter = jerry_create_external_function(DescriptorSetter);
        auto setterWrapperContainer = new FunctionalSetterWrapperContainer<_type>();
        setterWrapperContainer->setter = setter;
        jerry_set_object_native_pointer(propertyDescriptor.setter, setterWrapperContainer, &GetDataDeleter().info);

        jerry_value_t newPropertyValue = jerry_define_own_property(jvalue, name.jvalue, &propertyDescriptor);
        jerry_release_value(newPropertyValue);

        jerry_free_property_descriptor_fields(&propertyDescriptor);
    }

    template<typename _res_type, typename ... _args>
    _res_type ScriptValue::Invoke(_args ... args) const
    {
        return Invoke<_res_type, _args ...>(ScriptValue(), args ...);
    }

    template<typename _res_type, typename ... _args>
    _res_type ScriptValue::Invoke(const ScriptValue& thisValue, _args ... args) const
    {
        Vector<ScriptValue> argsValues;

        if constexpr (sizeof...(_args) > 0)
            PackArgs(argsValues, args ...);

        if constexpr (std::is_same<_res_type, void>::value)
            InvokeRaw(thisValue, argsValues);
        else
            return InvokeRaw(thisValue, argsValues).GetValue<_res_type>();
    }

    template<typename _res_type, typename ... _args>
    void ScriptValue::SetThisFunction(const Function<_res_type(ScriptValue, _args ...)>& func)
    {
        Accept(jerry_create_external_function(&CallFunction));

        IDataContainer* funcContainer = mnew ScriptThisFunctionContainer<Function<_res_type(ScriptValue, _args ...)>, _res_type, _args ...>(
            mnew Function<_res_type(ScriptValue, _args ...)>(func));

        jerry_set_object_native_pointer(jvalue, funcContainer, &GetDataDeleter().info);
    }

    template<typename _class_type, typename _res_type, typename ... _args>
    void ScriptValue::SetClassFunction(_res_type(_class_type::* functionPtr)(_args ... args))
    {
        Accept(jerry_create_external_function(&CallFunction));

        IDataContainer* funcContainer = mnew ScriptClassFunctionContainer<false, _class_type, _res_type, _args ...>(functionPtr);

        jerry_set_object_native_pointer(jvalue, funcContainer, &GetDataDeleter().info);
    }

    template<typename _class_type, typename _res_type, typename ... _args>
    void ScriptValue::SetClassFunction(_res_type(_class_type::* functionPtr)(_args ... args) const)
    {
        Accept(jerry_create_external_function(&CallFunction));

        IDataContainer* funcContainer = mnew ScriptClassFunctionContainer<true, _class_type, _res_type, _args ...>(functionPtr);

        jerry_set_object_native_pointer(jvalue, funcContainer, &GetDataDeleter().info);
    }

    template<typename _class_type, typename _res_type, typename ... _args>
    ScriptValue ScriptValue::ClassFunction(_res_type(_class_type::* functionPtr)(_args ... args))
    {
        ScriptValue res;
        res.SetClassFunction(functionPtr);
        return res;
    }

    template<typename _class_type, typename _res_type, typename ... _args>
    ScriptValue ScriptValue::ClassFunction(_res_type(_class_type::* functionPtr)(_args ... args) const)
    {
        ScriptValue res;
        res.SetClassFunction(functionPtr);
        return res;
    }


    class Type;

    struct ScriptPrototypeProcessor : public BaseTypeProcessor
    {
        ScriptValue proto = ScriptValue::EmptyObject();
        bool hasBaseClass = false;

    public:
        struct BaseFunctionProcessor : public BaseTypeProcessor::FunctionProcessor
        {
            ScriptPrototypeProcessor& processor;

        public:
            BaseFunctionProcessor(ScriptPrototypeProcessor& processor) : processor(processor) {}

            template<typename _attribute_type, typename ... _args>
            auto AddAttribute(_args ... args);

            BaseFunctionProcessor& SetProtectSection(ProtectSection section) { return *this; }
        };

        struct FunctionProcessor : public BaseFunctionProcessor
        {
            FunctionProcessor(const BaseFunctionProcessor& processor) :BaseFunctionProcessor(processor) {}

            template<typename _object_type, typename ... _args>
            void Constructor(_object_type* object, Type* type);

            template<typename _object_type, typename _res_type, typename ... _args>
            void Signature(_object_type* object, Type* type, const char* name,
                           _res_type(_object_type::* pointer)(_args ...));

            template<typename _object_type, typename _res_type, typename ... _args>
            void Signature(_object_type* object, Type* type, const char* name,
                           _res_type(_object_type::* pointer)(_args ...) const);

            template<typename _object_type, typename _res_type, typename ... _args>
            void SignatureStatic(_object_type* object, Type* type, const char* name,
                                 _res_type(*pointer)(_args ...));
        };

        BaseFunctionProcessor StartFunction() { return BaseFunctionProcessor(*this); }

        template<typename _object_type, typename _base_type>
        void BaseType(_object_type* object, Type* type, const char* name)
        {
            if (hasBaseClass)
                return;

            if constexpr (std::is_base_of<ISerializable, _base_type>::value && !std::is_same<ISerializable, _base_type>::value)
            {
                _object_type::GetScriptPrototype().SetPrototype(_base_type::GetScriptPrototype());
                hasBaseClass = true;
            }
        }

        static void RegisterTypeConstructor(Type* type, ScriptValue& constructorFunc);
        static void RegisterTypeStaticFunction(Type* type, const char* name, const ScriptValue& func);
    };

    template<typename _attribute_type, typename ... _args>
    auto ScriptPrototypeProcessor::BaseFunctionProcessor::AddAttribute(_args ... args)
    {
        if constexpr (std::is_same<ScriptableAttribute, _attribute_type>::value)
            return ScriptPrototypeProcessor::FunctionProcessor(*this);
        else
            return *this;
    }

    template<typename _object_type, typename ... _args>
    void ScriptPrototypeProcessor::FunctionProcessor::Constructor(_object_type* object, Type* type)
    {
        ScriptValue thisFunc;
        thisFunc.SetThisFunction<void, _args ...>(Function<void(ScriptValue thisValue, _args ...)>(
            [](ScriptValue thisValue, _args ... args)
            {
                _object_type* sample = mnew _object_type(args ...);
                thisValue.SetContainingObject(sample, true);
                thisValue.SetPrototype(_object_type::GetScriptPrototype());
            }));

        ScriptPrototypeProcessor::RegisterTypeConstructor(type, thisFunc);
    }

    template<typename _object_type, typename _res_type, typename ... _args>
    void ScriptPrototypeProcessor::FunctionProcessor::Signature(_object_type* object, Type* type, const char* name,
                                                                      _res_type(_object_type::* pointer)(_args ...))
    {
        _object_type::GetScriptPrototype().SetProperty(name, ScriptValue::ClassFunction<_object_type, _res_type, _args ...>(pointer));
    }

    template<typename _object_type, typename _res_type, typename ... _args>
    void ScriptPrototypeProcessor::FunctionProcessor::Signature(_object_type* object, Type* type, const char* name,
                                                                      _res_type(_object_type::* pointer)(_args ...) const)
    {
        _object_type::GetScriptPrototype().SetProperty(name, ScriptValue::ClassFunction<_object_type, _res_type, _args ...>(pointer));
    }

    template<typename _object_type, typename _res_type, typename ... _args>
    void ScriptPrototypeProcessor::FunctionProcessor::SignatureStatic(_object_type* object, Type* type,
                                                                            const char* name, _res_type(*pointer)(_args ...))
    {
        ScriptPrototypeProcessor::RegisterTypeStaticFunction(type, name, ScriptValue(Function<_res_type(_args ...)>(
            [=](_args ... args) {
                return (*pointer)(args ...);
            })));
    }

    template<>
    struct DataValue::Converter<ScriptValue>
    {
        static constexpr bool isSupported = true;

        static void Write(const ScriptValue& value, DataValue& data)
        {
            auto type = value.GetValueType();
            if (type == ScriptValue::ValueType::Number)
                data.Set(value.ToNumber());
            else if (type == ScriptValue::ValueType::String)
                data.Set(value.ToString());
            else if (type == ScriptValue::ValueType::Bool)
                data.Set(value.ToBool());
            else if (type == ScriptValue::ValueType::Array)
            {
                int length = value.GetLength();
                for (int i = 0; i < length; i++)
                    data.AddElement().Set(value.GetElement(i));
            }
            else if (type == ScriptValue::ValueType::Object)
            {
                if (value.IsObjectContainer())
                {
                    if (auto objType = dynamic_cast<const ObjectType*>(value.GetObjectContainerType()))
                    {
                        IObject* object = objType->DynamicCastToIObject(value.GetContainingObject());
                        if (auto serializable = dynamic_cast<ISerializable*>(object))
                            data = serializable;
                    }
                }
                else
                {
                    value.ForEachProperties(
                        [&](const ScriptValue& name, const ScriptValue& vvalue)
                        {
                            auto nameStr = name.ToString();
                            if (nameStr[0] != '_')
                                data[nameStr].Set(vvalue);
                            return true;
                        });
                }
            }
            else
                data.SetNull();
        }

        static void Read(ScriptValue& value, const DataValue& data)
        {
            if (data.IsNull())
                value = ScriptValue();
            else if (data.IsNumber())
                value = (float)data;
            else if (data.IsBoolean())
                value = (bool)data;
            else if (data.IsString())
                value = String(data.GetString());
            else if (data.IsArray())
            {
                for (auto& element : data)
                {
                    ScriptValue newElement;
                    element.Get(newElement);
                    value.AddElement(newElement);
                }
            }
            else if (data.IsObject())
            {
                if (auto typeMember = data.FindMember("Type"))
                {
                    if (ISerializable* object = data)
                        value = object->GetScriptValue();
                }
                else
                {
                    for (auto it = data.BeginMember(); it != data.EndMember(); ++it)
                    {
                        ScriptValue itName;
                        it->name.Get(itName);

                        ScriptValue itValue;
                        it->value.Get(itValue);

                        auto oldProp = value.GetProperty(itName);
                        if (oldProp.IsObject())
                        {
                            auto oldPropProto = oldProp.GetPrototype();
                            if (oldPropProto.IsObject())
                                itValue.SetPrototype(oldPropProto);
                        }

                        value.SetProperty(itName, itValue);
                    }
                }
            }
        }
    };
}

#endif // SCRIPTING_BACKEND_JERRYSCRIPT
