#include "o2/stdafx.h"

#if defined(SCRIPTING_BACKEND_JERRYSCRIPT)
#include "o2/Scripts/ScriptValue.h"

namespace o2
{
    ScriptValueBase::~ScriptValueBase()
    {
        jerry_release_value(jvalue);
    }

    void ScriptValueBase::AcquireValue(jerry_value_t v)
    {
        jerry_release_value(jvalue);
        jvalue = jerry_acquire_value(v);
    }

    ScriptValue ScriptValue::EmptyObject()
    {
        ScriptValue res;
        res.Accept(jerry_create_object());
        return res;
    }

    ScriptValue ScriptValue::EmptyArray()
    {
        ScriptValue res;
        res.Accept(jerry_create_array(0));
        return res;
    }

    void ScriptValueBase::Accept(jerry_value_t v)
    {
        jerry_release_value(jvalue);
        jvalue = v;
    }

    ScriptValue::ScriptValue()
    {
        jvalue = jerry_create_undefined();
    }

    ScriptValue::ScriptValue(const ScriptValue& other)
    {
        jvalue = jerry_acquire_value(other.jvalue);
    }

    ScriptValue ScriptValue::operator[](const ScriptValue& name) const
    {
        return GetProperty(name);
    }

    ScriptValue ScriptValue::operator[](int idx) const
    {
        ScriptValue res;
        res.Accept(jerry_get_property_by_index(jvalue, idx));
        return res;
    }

    bool ScriptValue::operator!=(const ScriptValue& other) const
    {
        return !operator==(other);
    }

    bool ScriptValue::operator==(const ScriptValue& other) const
    {
        ScriptValue res;
        res.Accept(jerry_binary_operation(JERRY_BIN_OP_EQUAL, jvalue, other.jvalue));
        return res.ToBool();
    }

    ScriptValue& ScriptValue::operator=(const ScriptValue& other)
    {
        jerry_release_value(jvalue);
        jvalue = jerry_acquire_value(other.jvalue);
        return *this;
    }

    ScriptValue::ValueType ScriptValue::GetValueType() const
    {
        if (jerry_value_is_array(jvalue))
            return ValueType::Array;

        return (ValueType)jerry_value_get_type(jvalue);
    }

    bool ScriptValue::IsConstructor() const
    {
        return jerry_value_is_constructor(jvalue);
    }

    bool ScriptValue::IsUndefined() const
    {
        return GetValueType() == ValueType::Undefined;
    }

    ScriptValue ScriptValue::Copy() const
    {
        auto type = GetValueType();
        if (type == ValueType::Array) 
        {
            ScriptValue res = EmptyArray();
            for (int i = 0; i < GetLength(); i++)
                res.AddElement(GetElement(i).Copy());

            return res;
        }
        else if (type == ValueType::Object)
        {
            ScriptValue res = EmptyObject();
            ForEachProperties([&](const ScriptValue& name, const ScriptValue& value)
                              {
                                  res[name] = value.Copy();
                                  return true;
                              });
            return res;
        }

        return *this;
    }

    int ScriptValue::GetLength() const
    {
        if (!jerry_value_is_array(jvalue))
            return 0;

        return jerry_get_array_length(jvalue);
    }

    String ScriptValue::GetError() const
    {
        if (GetValueType() != ValueType::Error)
            return String();

        auto errorJValue = jerry_get_value_from_error(jvalue, true);

        ScriptValue errorValue;
        errorValue.Accept(jerry_value_to_string(errorJValue));

        jerry_release_value(errorJValue);

        return errorValue.GetValue<String>();
    }

    bool ScriptValue::IsObjectContainer() const
    {
        if (!IsObject())
            return false;

        void* dataPtr = nullptr;
        jerry_get_object_native_pointer(jvalue, &dataPtr, &GetDataDeleter().info);
        return dataPtr != nullptr;
    }

    const Type* ScriptValue::GetObjectContainerType() const
    {
        if (!IsObject())
            return nullptr;

        void* dataPtr = nullptr;
        jerry_get_object_native_pointer(jvalue, &dataPtr, &GetDataDeleter().info);
        auto dataContainer = (IDataContainer*)dataPtr;
        if (dataContainer)
            return dataContainer->GetType();

        return nullptr;
    }

    void* ScriptValue::GetContainingObject() const
    {
        if (!IsObject())
            return nullptr;

        void* dataPtr = nullptr;
        jerry_get_object_native_pointer(jvalue, &dataPtr, &GetDataDeleter().info);
        auto dataContainer = (IDataContainer*)dataPtr;
        if (dataContainer)
            return dataContainer->GetData();

        return nullptr;
    }

    bool ScriptValue::IsObjectContainerOwner() const
    {
        void* dataPtr = nullptr;
        jerry_get_object_native_pointer(jvalue, &dataPtr, &GetDataDeleter().info);
        auto dataContainer = (IDataContainer*)dataPtr;
        if (dataContainer)
            return dataContainer->isDataOwner;

        return false;
    }

    void ScriptValue::SetObjectOwnership(bool own)
    {
        void* dataPtr = nullptr;
        jerry_get_object_native_pointer(jvalue, &dataPtr, &GetDataDeleter().info);
        auto dataContainer = (IDataContainer*)dataPtr;
        if (dataContainer)
            dataContainer->isDataOwner = own;
    }

    ScriptValue ScriptValue::Construct(const Vector<ScriptValue>& args)
    {
        const int maxParameters = 16;
        jerry_value_t valuesBuf[maxParameters];
        for (int i = 0; i < args.Count() && i < maxParameters; i++)
            valuesBuf[i] = args[i].jvalue;

        ScriptValue res;
        res.Accept(jerry_construct_object(jvalue, valuesBuf, args.Count()));
        return res;
    }

    void ScriptValue::ForEachProperties(const Function<bool(const ScriptValue& name, const ScriptValue& value)>& func, 
                                        bool withPrototypes /*= true*/) const
    {
        struct Helper
        {
            static bool IterateFunc(const jerry_value_t property_name, const jerry_value_t property_value, void* user_data_p)
            {
                auto func = (Function<bool(const ScriptValue&, const ScriptValue&)>*)user_data_p;
                ScriptValue name, value;
                name.AcquireValue(property_name);
                value.AcquireValue(property_value);
                return (*func)(name, value);
            }
        };

        if (GetValueType() != ValueType::Object)
            return;

        jerry_foreach_object_property(jvalue, &Helper::IterateFunc, (void*)&func);

        if (withPrototypes)
            GetPrototype().ForEachProperties(func, withPrototypes);
    }

    ScriptValue ScriptValue::GetProperty(const ScriptValue& name) const
    {
        if (GetValueType() != ValueType::Object)
        {
            jerry_release_value(jvalue);
            jvalue = jerry_create_object();
        }

        ScriptValue res;
        res.Accept(jerry_get_property(jvalue, name.jvalue));
        return res;
    }

    ScriptValue ScriptValue::GetInternalProperty(const ScriptValue& name) const
    {
        ScriptValue res;
        res.Accept(jerry_get_internal_property(jvalue, name.jvalue));
        return res;
    }

    ScriptValue ScriptValue::GetOwnProperty(const ScriptValue& name) const
    {
        ScriptValue res;

        jerry_property_descriptor_t descr;
        jerry_init_property_descriptor_fields(&descr);

        if (jerry_get_own_property_descriptor(jvalue, name.jvalue, &descr))
            res.AcquireValue(descr.value);

        jerry_free_property_descriptor_fields(&descr);

        return res;
    }

    ScriptValue ScriptValue::GetPropertyNames() const
    {
        ScriptValue res;

        res.AcquireValue(jerry_object_get_property_names(jvalue, JERRY_PROPERTY_FILTER_TRAVERSE_PROTOTYPE_CHAIN));

        return res;
    }

    void ScriptValue::SetInternalProperty(const ScriptValue& name, const ScriptValue& value)
    {
        jerry_set_internal_property(jvalue, name.jvalue, value.jvalue);
    }

    void ScriptValue::SetProperty(const ScriptValue& name, const ScriptValue& value)
    {
        if (GetValueType() != ValueType::Object)
        {
            jerry_release_value(jvalue);
            jvalue = jerry_create_object();
        }

        jerry_set_property(jvalue, name.jvalue, value.jvalue);
    }

    void ScriptValue::RemoveProperty(const ScriptValue& name)
    {
        jerry_delete_property(jvalue, name.jvalue);
    }

    void ScriptValue::SetPrototype(const ScriptValue& proto)
    {
        ScriptValue res;
        res.Accept(jerry_set_prototype(jvalue, proto.jvalue));
    }

    ScriptValue ScriptValue::GetPrototype() const
    {
        ScriptValue res;
        res.Accept(jerry_get_prototype(jvalue));
        return res;
    }

    void ScriptValue::SetElement(const ScriptValue& value, int idx)
    {
        if (GetValueType() != ValueType::Array)
        {
            jerry_release_value(jvalue);
            jvalue = jerry_create_array(0);
        }

        jerry_set_property_by_index(jvalue, idx, value.jvalue);
    }

    ScriptValue ScriptValue::GetElement(int idx) const
    {
        ScriptValue res;
        res.Accept(jerry_get_property_by_index(jvalue, idx));
        return res;
    }

    void ScriptValue::AddElement(const ScriptValue& value)
    {
        SetElement(value, GetLength());
    }

    void ScriptValue::RemoveElement(int idx)
    {
        jerry_delete_property_by_index(jvalue, idx);
    }

    bool ScriptValue::ToBool() const
    {
        return jerry_value_to_boolean(jvalue);
    }

    float ScriptValue::ToNumber() const
    {
        if (GetValueType() != ValueType::Number)
        {
            auto prev = jvalue;
            jvalue = jerry_value_to_number(jvalue);
            jerry_release_value(prev);
        }

        return (float)jerry_get_number_value(jvalue);
    }

    String ScriptValue::ToString() const
    {
        if (GetValueType() != ValueType::String)
        {
            auto prev = jvalue;
            jvalue = jerry_value_to_string(jvalue);
            jerry_release_value(prev);
        }

        String res;
        res.resize(jerry_get_string_length(jvalue));
        jerry_string_to_char_buffer(jvalue, (jerry_char_t*)res.Data(), res.Capacity());
        return res;
    }

    ScriptValue ScriptValue::InvokeRaw(const Vector<ScriptValue>& args) const
    {
        return InvokeRaw(ScriptValue(), args);
    }

    ScriptValue ScriptValue::InvokeRaw(const ScriptValue& thisValue, const Vector<ScriptValue>& args) const
    {
        if (IsFunction())
        {
            const int maxParameters = 16;
            jerry_value_t valuesBuf[maxParameters];
            for (int i = 0; i < args.Count() && i < maxParameters; i++)
                valuesBuf[i] = args[i].jvalue;

            auto res = jerry_call_function(jvalue, thisValue.jvalue, valuesBuf, args.Count());

            ScriptValue resValue;
            jerry_release_value(resValue.jvalue);
            resValue.jvalue = res;

            return resValue;
        }

        return {};
    }

    ScriptValueBase::DataContainerDeleter& ScriptValueBase::GetDataDeleter()
    {
        static DataContainerDeleter deleter;
        return deleter;
    }

    ScriptValueBase::DataContainerDeleter::DataContainerDeleter()
    {
        info.free_cb = &Free;
    }

    void ScriptValueBase::DataContainerDeleter::Free(void* ptr)
    {
        delete (IDataContainer*)ptr;
    }

    jerry_value_t ScriptValueBase::CallFunction(const jerry_value_t function_obj,
                                                const jerry_value_t this_val,
                                                const jerry_value_t args_p[], const jerry_length_t args_count)
    {
        void* ptr = nullptr;
        jerry_get_object_native_pointer(function_obj, &ptr, &GetDataDeleter().info);

        IFunctionContainer* container = dynamic_cast<IFunctionContainer*>((IDataContainer*)ptr);
        return container->Invoke(this_val, (jerry_value_t*)args_p, args_count);
    }

    jerry_value_t ScriptValueBase::DescriptorSetter(const jerry_value_t function_obj,
                                                    const jerry_value_t this_val,
                                                    const jerry_value_t args_p[],
                                                    const jerry_length_t args_count)
    {
        void* ptr = nullptr;
        jerry_get_object_native_pointer(function_obj, &ptr, &GetDataDeleter().info);

        ISetterWrapperContainer* container = dynamic_cast<ISetterWrapperContainer*>((IDataContainer*)ptr);
        container->Set(args_p[0]);

        return jerry_create_undefined();
    }

    jerry_value_t ScriptValueBase::DescriptorGetter(const jerry_value_t function_obj,
                                                    const jerry_value_t this_val,
                                                    const jerry_value_t args_p[],
                                                    const jerry_length_t args_count)
    {
        void* ptr = nullptr;
        jerry_get_object_native_pointer(function_obj, &ptr, &GetDataDeleter().info);

        IGetterWrapperContainer* container = dynamic_cast<IGetterWrapperContainer*>((IDataContainer*)ptr);
        return container->Get();
    }

    ScriptValue*& ScriptValuePrototypes::GetVec2Prototype()
    {
        static ScriptValue* value;
        return value;
    }

    ScriptValue*& ScriptValuePrototypes::GetRectPrototype()
    {
        static ScriptValue* value;
        return value;
    }

    ScriptValue*& ScriptValuePrototypes::GetBorderPrototype()
    {
        static ScriptValue* value;
        return value;
    }

    ScriptValue*& ScriptValuePrototypes::GetColor4Prototype()
    {
        static ScriptValue* value;
        return value;
    }

    void FixNamespace(String& path)
    {
        path.ReplaceAll("::", "");
        path.ReplaceAll("<", "_");
        path.ReplaceAll(">", "");
        path.ReplaceAll("_o2", "");
    }

    int FindNamespaceDel(const String& path)
    {
        int fnd = -1;
        int braces = 0;
        for (int i = 0; i < path.Length(); i++)
        {
            if (path[i] == '<')
                braces++;

            if (path[i] == '>')
                braces--;

            if (i > 0 && path[i] == ':' && path[i - 1] == ':' && braces == 0)
            {
                fnd = i - 1;
                break;
            }
        }

        return fnd;
    }

    ScriptValue GetNameSpace(ScriptValue base, const String& path)
    {
        int fnd = FindNamespaceDel(path);

        auto subPath = fnd >= 0 ? path.SubStr(0, fnd) : path;
        FixNamespace(subPath);

        ScriptValue subPathValue(subPath);
        ScriptValue subPathProp = base.GetProperty(subPathValue);
        if (subPathProp.GetValueType() == ScriptValue::ValueType::Undefined)
        {
            subPathProp = ScriptValue::EmptyObject();
            base.SetProperty(ScriptValue(subPath), subPathProp);
        }

        if (fnd < 0)
            return subPathProp;

        return GetNameSpace(subPathProp, path.SubStr(fnd + 2));
    }

    ScriptValue GetNameSpaceAndConstructor(ScriptValue base, const String& path, String& constructor)
    {
        int fnd = FindNamespaceDel(path);

        auto subPath = fnd >= 0 ? path.SubStr(0, fnd) : path;
        FixNamespace(subPath);

        if (fnd < 0)
        {
            constructor = subPath;
            return base;
        }

        ScriptValue subPathValue(subPath);
        ScriptValue subPathProp = base.GetProperty(subPathValue);
        if (subPathProp.GetValueType() == ScriptValue::ValueType::Undefined)
        {
            subPathProp = ScriptValue::EmptyObject();
            base.SetProperty(ScriptValue(subPath), subPathProp);
        }

        return GetNameSpaceAndConstructor(subPathProp, path.SubStr(fnd + 2), constructor);
    }

    void ScriptPrototypeProcessor::RegisterTypeConstructor(Type* type, ScriptValue& constructorFunc)
    {
        String constructor;
        auto nspace = GetNameSpaceAndConstructor(o2Scripts.GetGlobal(), type->GetName(), constructor);
        auto t1 = constructorFunc.GetValueType();
        ScriptValue proto;
        proto.SetProperty("type", ScriptValue(type));
        constructorFunc.SetPrototype(proto);
        auto t2 = proto.GetValueType();
        nspace.SetProperty(constructor.Data(), constructorFunc);
    }

    void ScriptPrototypeProcessor::RegisterTypeStaticFunction(Type* type, const char* name, const ScriptValue& func)
    {
        GetNameSpace(o2Scripts.GetGlobal(), type->GetName()).SetProperty(name, func);
    }
}

#endif
