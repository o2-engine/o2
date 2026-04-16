#pragma once

#include "o2/Utils/Reflection/Type.h"
#include "o2/Utils/Reflection/TypeTraits.h"
#include "o2/Utils/Reflection/BaseTypeProcessor.h"
#include "o2/Utils/Debug/Debug.h"
#include <type_traits>
#include <functional>

#if defined(SCRIPTING_BACKEND_JERRYSCRIPT)

namespace o2
{
    // Helper to detect Ref<T>
    template<typename T> struct IsRefType : std::false_type {};
    template<typename T> struct IsRefType<Ref<T>> : std::true_type {};

    template<typename T> struct RefInnerType { using type = T; };
    template<typename T> struct RefInnerType<Ref<T>> { using type = T; };

    // Helper to extract types from member function pointers
    template<typename _type> struct MemberFuncTraits;

    template<typename _class_type, typename _res_type, typename ... _args>
    struct MemberFuncTraits<_res_type(_class_type::*)(_args ...)>
    {
        using ClassType = _class_type;
        using ResType = _res_type;
        using ArgsTuple = std::tuple<_args ...>;
        static constexpr size_t Arity = sizeof...(_args);
        static constexpr bool IsConst = false;
        template<size_t _idx> using ArgType = std::tuple_element_t<_idx, ArgsTuple>;
    };

    template<typename _class_type, typename _res_type, typename ... _args>
    struct MemberFuncTraits<_res_type(_class_type::*)(_args ...) const>
    {
        using ClassType = _class_type;
        using ResType = _res_type;
        using ArgsTuple = std::tuple<_args ...>;
        static constexpr size_t Arity = sizeof...(_args);
        static constexpr bool IsConst = true;
        template<size_t _idx> using ArgType = std::tuple_element_t<_idx, ArgsTuple>;
    };

    // ------------------------------
    // DataContainer - stores by value
    // ------------------------------

    template<typename _type>
    ScriptValueBase::DataContainer<_type>::DataContainer(const _type& d) :
        data(d)
    {}

    template<typename _type>
    ScriptValueBase::DataContainer<_type>::DataContainer(_type&& d) :
        data(std::move(d))
    {}

    template<typename _type>
    void* ScriptValueBase::DataContainer<_type>::GetData() const
    {
        if constexpr (IsRefType<_type>::value)
            return data.Get();
        else if constexpr (std::is_const<_type>::value)
            return nullptr;
        else
            return const_cast<_type*>(&data);
    }

    template<typename _type>
    IObject* ScriptValueBase::DataContainer<_type>::TryCastToIObject() const
    {
        if constexpr (IsRefType<_type>::value)
        {
            using inner = typename RefInnerType<_type>::type;
            if constexpr (std::is_base_of<IObject, inner>::value && std::is_convertible_v<inner*, IObject*>)
                return dynamic_cast<IObject*>(data.Get());
        }
        else if constexpr (std::is_base_of<IObject, _type>::value && std::is_convertible_v<_type*, IObject*>)
        {
            return dynamic_cast<IObject*>(const_cast<_type*>(&data));
        }

        return nullptr;
    }

    template<typename _type>
    const Type* ScriptValueBase::DataContainer<_type>::GetType() const
    {
        if constexpr (IsRefType<_type>::value)
            return &TypeOf(typename RefInnerType<_type>::type);
        else
            return &TypeOf(_type);
    }

    template<typename _type>
    ScriptValueBase::IDataContainer* ScriptValueBase::DataContainer<_type>::Clone() const
    {
        if constexpr (std::is_copy_constructible<_type>::value)
            return CreateContainer<DataContainer<_type>>(data);
        else
            return nullptr;
    }

    template<typename _type>
    ScriptValueBase::PointerDataContainer<_type>::PointerDataContainer(_type* d) :
        data(d)
    {}

    template<typename _type>
    void* ScriptValueBase::PointerDataContainer<_type>::GetData() const
    {
        return data;
    }

    template<typename _type>
    IObject* ScriptValueBase::PointerDataContainer<_type>::TryCastToIObject() const
    {
        if constexpr (std::is_base_of<IObject, _type>::value && std::is_convertible_v<_type*, IObject*>)
            return dynamic_cast<IObject*>(data);

        return nullptr;
    }

    template<typename _type>
    const Type* ScriptValueBase::PointerDataContainer<_type>::GetType() const
    {
        return &TypeOf(_type);
    }

    template<typename _type>
    ScriptValueBase::IDataContainer* ScriptValueBase::PointerDataContainer<_type>::Clone() const
    {
        return CreateContainer<PointerDataContainer<_type>>(data);
    }

    // -------------------------------------------------------
    // Function containers - store callable by value, override
    // Invoke directly for minimal overhead
    // -------------------------------------------------------

    template<typename T>
    static auto ConvertJerryArg(jerry_value_t* args, size_t index, int count)
        -> typename RemoveConstAndRef<T>::type
    {
        using CleanT = typename RemoveConstAndRef<T>::type;
        if (index >= (size_t)count) return CleanT{};
        ScriptValue tmp;
        tmp.AcquireValue(args[index]);
        return tmp.GetValue<CleanT>();
    }

    template<typename _invocable_type, typename _res_type, typename ... _args>
    struct ScriptFunctionContainer : public ScriptValueBase::TPoolContainer<ScriptFunctionContainer<_invocable_type, _res_type, _args...>, ScriptValueBase::IFunctionContainer>
    {
        _invocable_type data;

        ScriptFunctionContainer(const _invocable_type& function) : data(function) {}
        ScriptFunctionContainer(_invocable_type&& function) : data(std::move(function)) {}

        jerry_value_t Invoke(jerry_value_t thisValue, jerry_value_t* args, int argsCount) override
        {
            return InvokeImpl(args, argsCount, std::index_sequence_for<_args...>{});
        }

        template<size_t... Is>
        jerry_value_t InvokeImpl(jerry_value_t* args, int argsCount, std::index_sequence<Is...>)
        {
            if constexpr (std::is_void<_res_type>::value)
            {
                data(ConvertJerryArg<_args>(args, Is, argsCount)...);
                return jerry_create_undefined();
            }
            else
            {
                ScriptValue res(data(ConvertJerryArg<_args>(args, Is, argsCount)...));
                return jerry_acquire_value(res.jvalue);
            }
        }
    };

    template<typename _invocable_type, typename _res_type, typename ... _args>
    struct ScriptThisFunctionContainer : public ScriptValueBase::TPoolContainer<ScriptThisFunctionContainer<_invocable_type, _res_type, _args...>, ScriptValueBase::IFunctionContainer>
    {
        _invocable_type data;

        ScriptThisFunctionContainer(const _invocable_type& function) : data(function) {}

        jerry_value_t Invoke(jerry_value_t thisValue, jerry_value_t* args, int argsCount) override
        {
            ScriptValue thisObj;
            thisObj.AcquireValue(thisValue);
            return InvokeImpl(thisObj, args, argsCount, std::index_sequence_for<_args...>{});
        }

        template<size_t... Is>
        jerry_value_t InvokeImpl(ScriptValue& thisObj, jerry_value_t* args, int argsCount, std::index_sequence<Is...>)
        {
            if constexpr (std::is_void<_res_type>::value)
            {
                data(thisObj, ConvertJerryArg<_args>(args, Is, argsCount)...);
                return jerry_create_undefined();
            }
            else
            {
                ScriptValue res(data(thisObj, ConvertJerryArg<_args>(args, Is, argsCount)...));
                return jerry_acquire_value(res.jvalue);
            }
        }
    };

    // -------------------------------------------------------
    // Direct class method handler - one unique C function per
    // member pointer, no heap container on the function object
    // -------------------------------------------------------

    template<auto _func_ptr, size_t ... _idx>
    static jerry_value_t ClassMethodCallImpl(const jerry_value_t this_val,
                                             const jerry_value_t args_p[],
                                             const jerry_length_t args_count,
                                             std::index_sequence<_idx...>)
    {
        using _traits = MemberFuncTraits<decltype(_func_ptr)>;
        using _class_type = typename _traits::ClassType;
        using _res_type = typename _traits::ResType;

        auto container = ScriptValueBase::GetNativeContainer(this_val);
        _class_type* obj = static_cast<_class_type*>(container->GetData());

        auto fn = std::mem_fn(_func_ptr);

        if constexpr (std::is_void_v<_res_type>)
        {
            fn(obj, ConvertJerryArg<typename _traits::template ArgType<_idx>>((jerry_value_t*)args_p, _idx, args_count)...);
            return jerry_create_undefined();
        }
        else
        {
            ScriptValue res(fn(obj, ConvertJerryArg<typename _traits::template ArgType<_idx>>((jerry_value_t*)args_p, _idx, args_count)...));
            return jerry_acquire_value(res.jvalue);
        }
    }

    template<auto _func_ptr>
    static jerry_value_t ClassMethodCallHandler(const jerry_value_t function_obj,
                                                const jerry_value_t this_val,
                                                const jerry_value_t args_p[],
                                                const jerry_length_t args_count)
    {
        using _traits = MemberFuncTraits<decltype(_func_ptr)>;
        return ClassMethodCallImpl<_func_ptr>(this_val, args_p, args_count,
                                              std::make_index_sequence<_traits::Arity>{});
    }

    // -------------------------------------------------------
    // Prototype-based field getter/setter containers
    // -------------------------------------------------------

    template<typename _object_type, typename _field_type>
    struct PrototypeFieldGetter : public ScriptValueBase::TPoolContainer<PrototypeFieldGetter<_object_type, _field_type>, ScriptValueBase::IPrototypeGetter>
    {
        void* (*pointerGetter)(void*) = nullptr;

        jerry_value_t GetFrom(jerry_value_t this_val) override
        {
            auto container = ScriptValueBase::GetNativeContainer(this_val);
            if (!container) return jerry_create_undefined();

            auto objectPtr = static_cast<_object_type*>(container->GetData());
            if (!objectPtr) return jerry_create_undefined();

            auto fieldPtr = static_cast<_field_type*>(pointerGetter(objectPtr));

            ScriptValue tmp;
            if constexpr (IsProperty<_field_type>::value)
                tmp.SetValue<typename ExtractPropertyValueType<_field_type>::type>(fieldPtr->Get());
            else
                tmp.SetValue<_field_type>(*fieldPtr);

            return jerry_acquire_value(tmp.jvalue);
        }
    };

    template<typename _object_type, typename _field_type>
    struct PrototypeFieldSetter : public ScriptValueBase::TPoolContainer<PrototypeFieldSetter<_object_type, _field_type>, ScriptValueBase::IPrototypeSetter>
    {
        void* (*pointerGetter)(void*) = nullptr;

        void SetTo(jerry_value_t this_val, jerry_value_t value) override
        {
            auto container = ScriptValueBase::GetNativeContainer(this_val);
            if (!container) return;

            auto objectPtr = static_cast<_object_type*>(container->GetData());
            if (!objectPtr) return;

            auto fieldPtr = static_cast<_field_type*>(pointerGetter(objectPtr));

            ScriptValue tmp;
            tmp.AcquireValue(value);

            if constexpr (IsProperty<_field_type>::value)
                fieldPtr->Set(tmp.GetValue<typename ExtractPropertyValueType<_field_type>::type>());
            else
                *fieldPtr = tmp.GetValue<_field_type>();
        }
    };

    // -------------------------------------------------------
    // Wrapper container implementations
    // -------------------------------------------------------

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

    template<typename T, typename = void>
    struct HasRefCounterMethod : std::false_type {};

    template<typename T>
    struct HasRefCounterMethod<T, std::void_t<decltype(std::declval<const T&>().GetStrongReferencesCount())>> : std::true_type {};

    template<typename _type>
    void ScriptValue::SetContainingObject(_type* object)
    {
        if (!object)
        {
            *this = ScriptValue();
            return;
        }

        if constexpr (HasRefCounterMethod<_type>::value)
        {
            auto dataContainer = CreateContainer<DataContainer<Ref<_type>>>(Ref<_type>(object));
            jerry_set_object_native_pointer(jvalue, (IDataContainer*)dataContainer, &GetDataDeleter().info);
        }
        else if constexpr (std::is_base_of<IObject, _type>::value)
        {
            auto dataContainer = CreateContainer<PointerDataContainer<_type>>(object);
            jerry_set_object_native_pointer(jvalue, (IDataContainer*)dataContainer, &GetDataDeleter().info);
        }
        else if constexpr (!std::is_abstract<_type>::value && std::is_copy_constructible<_type>::value)
        {
            auto dataContainer = CreateContainer<DataContainer<_type>>(*object);
            jerry_set_object_native_pointer(jvalue, (IDataContainer*)dataContainer, &GetDataDeleter().info);
        }

        if constexpr (std::is_base_of<IObject, _type>::value)
        {
            SetPrototype(_type::GetScriptPrototype());
        }
    }

    template<typename _type>
    void ScriptValue::SetProperty(const char* name, const _type& value)
    {
        SetProperty(ScriptValue(name), ScriptValue(value));
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
            auto getterWrapperContainer = CreateContainer<PropertyGetterWrapperContainer<_type>>();
            getterWrapperContainer->propertyPtr = &value;
            jerry_set_object_native_pointer(propertyDescriptor.getter, getterWrapperContainer, &GetDataDeleter().info);
        }
        else
        {
            auto getterWrapperContainer = CreateContainer<PointerGetterWrapperContainer<_type>>();
            getterWrapperContainer->dataPtr = &value;
            jerry_set_object_native_pointer(propertyDescriptor.getter, getterWrapperContainer, &GetDataDeleter().info);
        }

        propertyDescriptor.is_set_defined = true;
        propertyDescriptor.setter = jerry_create_external_function(DescriptorSetter);

        if constexpr (IsProperty<_type>::value)
        {
            auto setterWrapperContainer = CreateContainer<PropertySetterWrapperContainer<_type>>();
            setterWrapperContainer->propertyPtr = &value;
            jerry_set_object_native_pointer(propertyDescriptor.setter, setterWrapperContainer, &GetDataDeleter().info);
        }
        else
        {
            auto setterWrapperContainer = CreateContainer<PointerSetterWrapperContainer<_type>>();
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
        auto getterWrapperContainer = CreateContainer<FunctionalGetterWrapperContainer<_type>>();
        getterWrapperContainer->getter = getter;
        jerry_set_object_native_pointer(propertyDescriptor.getter, getterWrapperContainer, &GetDataDeleter().info);

        propertyDescriptor.is_set_defined = true;
        propertyDescriptor.setter = jerry_create_external_function(DescriptorSetter);
        auto setterWrapperContainer = CreateContainer<FunctionalSetterWrapperContainer<_type>>();
        setterWrapperContainer->setter = setter;
        jerry_set_object_native_pointer(propertyDescriptor.setter, setterWrapperContainer, &GetDataDeleter().info);

        jerry_value_t newPropertyValue = jerry_define_own_property(jvalue, name.jvalue, &propertyDescriptor);
        jerry_release_value(newPropertyValue);

        jerry_free_property_descriptor_fields(&propertyDescriptor);
    }

    template<typename _object_type, typename _field_type>
    void ScriptValue::SetPrototypePropertyWrapper(const ScriptValue& name, void* (*pointerGetter)(void*))
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
        propertyDescriptor.getter = jerry_create_external_function(PrototypeDescriptorGetter);
        auto getterContainer = CreateContainer<PrototypeFieldGetter<_object_type, _field_type>>();
        getterContainer->pointerGetter = pointerGetter;
        jerry_set_object_native_pointer(propertyDescriptor.getter, getterContainer, &GetDataDeleter().info);

        propertyDescriptor.is_set_defined = true;
        propertyDescriptor.setter = jerry_create_external_function(PrototypeDescriptorSetter);
        auto setterContainer = CreateContainer<PrototypeFieldSetter<_object_type, _field_type>>();
        setterContainer->pointerGetter = pointerGetter;
        jerry_set_object_native_pointer(propertyDescriptor.setter, setterContainer, &GetDataDeleter().info);

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
    void ScriptValue::SetFunction(const Function<_res_type(ScriptValue, _args ...)>& func)
    {
        Accept(jerry_create_external_function(&CallFunction));

        auto funcContainer = CreateContainer<ScriptThisFunctionContainer<Function<_res_type(ScriptValue, _args ...)>, _res_type, _args ...>>(func);

        jerry_set_object_native_pointer(jvalue, (IDataContainer*)funcContainer, &GetDataDeleter().info);
    }

    // -------------------------------------------------------
    // ScriptPrototypeProcessor - handles prototype setup for
    // class methods and fields marked @SCRIPTABLE
    // -------------------------------------------------------

    class Type;

    struct ScriptPrototypeProcessor : public BaseTypeProcessor
    {
        ScriptValue proto = ScriptValue::EmptyObject();
        bool hasBaseClass = false;

    public:
        // --- Function processing ---

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

            template<auto pointer, typename _object_type>
            void Signature(_object_type* object, Type* type, const char* name);

            template<typename _object_type, typename _res_type, typename ... _args>
            void SignatureStatic(_object_type* object, Type* type, const char* name,
                                 _res_type(*pointer)(_args ...));
        };

        BaseFunctionProcessor StartFunction() { return BaseFunctionProcessor(*this); }

        // --- Field processing ---

        struct BaseFieldProcessor
        {
            ScriptPrototypeProcessor& processor;
            ProtectSection section = ProtectSection::Public;

            BaseFieldProcessor(ScriptPrototypeProcessor& proc) : processor(proc) {}

            template<typename _attribute_type, typename ... _args>
            auto AddAttribute(_args ... args);

            template<typename _type>
            BaseFieldProcessor& SetDefaultValue(const _type& value) { return *this; }

            BaseFieldProcessor& SetProtectSection(ProtectSection sect)
            {
                section = sect;
                return *this;
            }

            template<typename _object_type, typename _field_type>
            BaseFieldProcessor& FieldBasics(_object_type* object, Type* type, const char* name,
                                            void* (*pointerGetter)(void*), _field_type& field)
            {
                return *this;
            }
        };

        struct FieldProcessor : public BaseFieldProcessor
        {
            FieldProcessor(ScriptPrototypeProcessor& proc, ProtectSection sect)
                : BaseFieldProcessor(proc) { section = sect; }

            template<typename _attribute_type, typename ... _args>
            FieldProcessor& AddAttribute(_args ... args) { return *this; }

            template<typename _type>
            FieldProcessor& SetDefaultValue(const _type& value) { return *this; }

            template<typename _object_type, typename _field_type>
            FieldProcessor& FieldBasics(_object_type* object, Type* type, const char* name,
                                        void* (*pointerGetter)(void*), _field_type& field)
            {
                if (section != ProtectSection::Public)
                    return *this;

                if constexpr (std::is_copy_constructible<_field_type>::value)
                {
                    _object_type::GetScriptPrototype().template SetPrototypePropertyWrapper<_object_type, _field_type>(
                        ScriptValue(name), pointerGetter);
                }

                return *this;
            }
        };

        BaseFieldProcessor StartField() { return BaseFieldProcessor(*this); }

        // --- Base type and lifecycle ---

        template<typename _object_type>
        void Start(_object_type* object, Type* type) {}

        template<typename _object_type>
        void StartBases(_object_type* object, Type* type) {}

        template<typename _object_type>
        void StartFields(_object_type* object, Type* type) {}

        template<typename _object_type>
        void StartMethods(_object_type* object, Type* type) {}

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

    template<typename _attribute_type, typename ... _args>
    auto ScriptPrototypeProcessor::BaseFieldProcessor::AddAttribute(_args ... args)
    {
        if constexpr (std::is_same<ScriptableAttribute, _attribute_type>::value)
            return ScriptPrototypeProcessor::FieldProcessor(processor, section);
        else
            return *this;
    }

    template<typename _object_type, typename ... _script_args>
    void RegisterScriptConstructor(Type* type)
    {
        ScriptValue thisFunc;
        thisFunc.SetFunction<void, _script_args ...>(Function<void(ScriptValue thisValue, _script_args ...)>(
            [](ScriptValue thisValue, _script_args ... args)
            {
                if constexpr (std::is_base_of<RefCounterable, _object_type>::value)
                {
                    auto sample = mmake<_object_type>(args ...);
                    thisValue.SetContainingObject(sample.Get());
                }
                else
                {
                    _object_type* sample = mnew _object_type(args ...);
                    thisValue.SetContainingObject(sample);
                }
                thisValue.SetPrototype(_object_type::GetScriptPrototype());
            }));

        ScriptPrototypeProcessor::RegisterTypeConstructor(type, thisFunc);
    }

    template<typename _object_type, typename _first, typename ... _rest>
    void RegisterScriptConstructorStripped(Type* type, std::tuple<_first, _rest...>*)
    {
        RegisterScriptConstructor<_object_type, _rest...>(type);
    }

    template<typename _tuple>
    struct FirstArgIsRefCounter : std::false_type {};

    template<typename _first, typename ... _rest>
    struct FirstArgIsRefCounter<std::tuple<_first, _rest...>>
        : std::is_same<_first, RefCounter*> {};

    template<typename _object_type, typename ... _args>
    void ScriptPrototypeProcessor::FunctionProcessor::Constructor(_object_type* object, Type* type)
    {
        if constexpr (FirstArgIsRefCounter<std::tuple<_args...>>::value)
        {
            RegisterScriptConstructorStripped<_object_type>(type, static_cast<std::tuple<_args...>*>(nullptr));
        }
        else
        {
            RegisterScriptConstructor<_object_type, _args...>(type);
        }
    }

    template<auto pointer, typename _object_type>
    void ScriptPrototypeProcessor::FunctionProcessor::Signature(_object_type* object, Type* type, const char* name)
    {
        ScriptValue funcVal;
        funcVal.Accept(jerry_create_external_function(&ClassMethodCallHandler<pointer>));
        _object_type::GetScriptPrototype().SetProperty(name, funcVal);
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
                        extern void ScriptValueImpl_WriteIObjectToDataValue(IObject* object, DataValue& data);
                        ScriptValueImpl_WriteIObjectToDataValue(object, data);
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
                    extern bool ScriptValueImpl_ReadIObjectFromDataValue(const DataValue& data, ScriptValue& value);
                    ScriptValueImpl_ReadIObjectFromDataValue(data, value);
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
