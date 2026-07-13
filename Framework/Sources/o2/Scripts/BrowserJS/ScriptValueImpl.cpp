#include "o2/stdafx.h"

#include "o2/Scripts/ScriptValueDef.h"
#include "o2/Scripts/ScriptValueContainerAllocator.h"
#include "o2/Utils/Debug/Debug.h"

#if defined(SCRIPTING_BACKEND_BROWSERJS)
#include "o2/Scripts/BrowserJS/BrowserJSCore.h"
#include "o2/Scripts/ScriptValue.h"

namespace o2
{
    ScriptValueBase::~ScriptValueBase()
    {
        o2js_release(mValue);
    }

    void ScriptValueBase::AcquireValue(o2js_value_t v)
    {
        o2js_release(mValue);
        mValue = o2js_acquire(v);
    }

    ScriptValue ScriptValue::EmptyObject()
    {
        ScriptValue res;
        res.Accept(o2js_object());
        return res;
    }

    ScriptValue ScriptValue::EmptyArray()
    {
        ScriptValue res;
        res.Accept(o2js_array(0));
        return res;
    }

    void ScriptValueBase::Accept(o2js_value_t v)
    {
        o2js_release(mValue);
        mValue = v;
    }

    ScriptValue::ScriptValue()
    {
        mValue = o2js_undefined();
    }

    ScriptValue::ScriptValue(const ScriptValue& other)
    {
        mValue = o2js_acquire(other.mValue);
    }

    ScriptValue::ScriptValue(ScriptValue&& other) noexcept
    {
        mValue = other.mValue;
        other.mValue = o2js_undefined();
    }

    ScriptValue ScriptValue::operator[](const ScriptValue& name) const
    {
        return GetProperty(name);
    }

    ScriptValue ScriptValue::operator[](int idx) const
    {
        ScriptValue res;
        res.Accept(o2js_get_property_by_index(mValue, idx));
        return res;
    }

    bool ScriptValue::operator!=(const ScriptValue& other) const
    {
        return !operator==(other);
    }

    bool ScriptValue::operator==(const ScriptValue& other) const
    {
        return o2js_equals(mValue, other.mValue);
    }

    ScriptValue& ScriptValue::operator=(const ScriptValue& other)
    {
        o2js_release(mValue);
        mValue = o2js_acquire(other.mValue);
        return *this;
    }

    ScriptValue& ScriptValue::operator=(ScriptValue&& other) noexcept
    {
        if (this != &other)
        {
            o2js_release(mValue);
            mValue = other.mValue;
            other.mValue = o2js_undefined();
        }
        return *this;
    }

    ScriptValue::ValueType ScriptValue::GetValueType() const
    {
        if (o2js_is_array(mValue))
            return ValueType::Array;

        return (ValueType)o2js_get_value_type(mValue);
    }

    bool ScriptValue::IsConstructor() const
    {
        return o2js_is_constructor(mValue);
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
            int length = GetLength();
            for (int i = 0; i < length; i++)
                res.AddElement(GetElement(i).Copy());

            return res;
        }
        else if (type == ValueType::Object)
        {    
            ScriptValue res;
            ForEachProperties([&](const ScriptValue &name, const ScriptValue &value) {
                res.SetProperty(name, value.Copy());
                return true;
            });

            res.SetPrototype(GetPrototype());

            auto dataContainer = GetNativeContainer(mValue);
            if (dataContainer)
            {
                auto clonedDataContainer = dataContainer->Clone();
                o2js_set_native_pointer(res.mValue, clonedDataContainer, &FreeDataContainer);
            }

            return res;
        }

        return *this;
    }

    int ScriptValue::GetLength() const
    {
        if (!o2js_is_array(mValue))
            return 0;

        return o2js_get_array_length(mValue);
    }

    String ScriptValue::GetError() const
    {
        if (GetValueType() != ValueType::Error)
            return String();

        auto thrownValue = o2js_get_error_value(mValue);

        ScriptValue errorValue;
        errorValue.Accept(o2js_to_string(thrownValue));

        o2js_release(thrownValue);

        return errorValue.GetValue<String>();
    }

    bool ScriptValue::IsObjectContainer() const
    {
        if (!IsObject())
            return false;

        return o2js_get_native_pointer(mValue, &FreeDataContainer) != nullptr;
    }

    const Type* ScriptValue::GetObjectContainerType() const
    {
        if (!IsObject())
            return nullptr;

        auto dataContainer = (IDataContainer*)o2js_get_native_pointer(mValue, &FreeDataContainer);
        if (dataContainer)
            return dataContainer->GetType();

        return nullptr;
    }

    void* ScriptValue::GetContainingObject() const
    {
        if (!IsObject())
            return nullptr;

        auto dataContainer = (IDataContainer*)o2js_get_native_pointer(mValue, &FreeDataContainer);
        if (dataContainer)
            return dataContainer->GetData();

        return nullptr;
    }

    ScriptValue ScriptValue::Construct(const Vector<ScriptValue>& args)
    {
        const int maxParameters = 16;
        o2js_value_t valuesBuf[maxParameters];
        for (int i = 0; i < args.Count() && i < maxParameters; i++)
            valuesBuf[i] = args[i].mValue;

        ScriptValue res;
        res.Accept(o2js_construct(mValue, valuesBuf, args.Count()));
        return res;
    }

    void ScriptValue::ForEachProperties(const Function<bool(const ScriptValue& name, const ScriptValue& value)>& func, 
                                        bool withPrototypes /*= true*/) const
    {
        if (GetValueType() != ValueType::Object)
            return;

        auto allProperties = GetPropertyNames();
        int length = allProperties.GetLength();
        for (int i = 0; i < length; i++)
        {
            ScriptValue key = allProperties.GetElement(i);
            ScriptValue value = GetProperty(key);
            if (!func(key, value))
                return;
        }

        if (withPrototypes)
            GetPrototype().ForEachProperties(func, withPrototypes);
    }

    ScriptValue ScriptValue::GetProperty(const ScriptValue& name) const
    {
        if (GetValueType() != ValueType::Object)
        {
            o2js_release(mValue);
            mValue = o2js_object();
        }

        ScriptValue res;
        res.Accept(o2js_get_property(mValue, name.mValue));
        return res;
    }

    ScriptValue ScriptValue::GetInternalProperty(const ScriptValue& name) const
    {
        ScriptValue res;
        res.Accept(o2js_get_internal_property(mValue, name.mValue));
        return res;
    }

    ScriptValue ScriptValue::GetOwnProperty(const ScriptValue& name) const
    {
        ScriptValue res;
        res.Accept(o2js_get_own_property(mValue, name.mValue));
        return res;
    }

    ScriptValue ScriptValue::GetPropertyNames() const
    {
        ScriptValue res;
        res.Accept(o2js_get_property_names(mValue));
        return res;
    }

    void ScriptValue::SetInternalProperty(const ScriptValue& name, const ScriptValue& value)
    {
        o2js_set_internal_property(mValue, name.mValue, value.mValue);
    }

    void ScriptValue::SetProperty(const ScriptValue& name, const ScriptValue& value)
    {
        if (GetValueType() != ValueType::Object)
        {
            o2js_release(mValue);
            mValue = o2js_object();
        }

        o2js_set_property(mValue, name.mValue, value.mValue);
    }

    void ScriptValue::RemoveProperty(const ScriptValue& name)
    {
        o2js_delete_property(mValue, name.mValue);
    }

    void ScriptValue::SetPrototype(const ScriptValue& proto)
    {
        ScriptValue res;
        res.Accept(o2js_set_prototype(mValue, proto.mValue));
    }

    ScriptValue ScriptValue::GetPrototype() const
    {
        ScriptValue res;
        res.Accept(o2js_get_prototype(mValue));
        return res;
    }

    void ScriptValue::SetElement(const ScriptValue& value, int idx)
    {
        if (GetValueType() != ValueType::Array)
        {
            o2js_release(mValue);
            mValue = o2js_array(0);
        }

        o2js_set_property_by_index(mValue, idx, value.mValue);
    }

    ScriptValue ScriptValue::GetElement(int idx) const
    {
        ScriptValue res;
        res.Accept(o2js_get_property_by_index(mValue, idx));
        return res;
    }

    void ScriptValue::AddElement(const ScriptValue& value)
    {
        SetElement(value, GetLength());
    }

    void ScriptValue::RemoveElement(int idx)
    {
        o2js_delete_property_by_index(mValue, idx);
    }

    bool ScriptValue::ToBool() const
    {
        return o2js_to_boolean(mValue);
    }

    float ScriptValue::ToNumber() const
    {
        if (GetValueType() != ValueType::Number)
        {
            auto prev = mValue;
            mValue = o2js_to_number(mValue);
            o2js_release(prev);
        }

        return (float)o2js_get_number(mValue);
    }

    String ScriptValue::ToString() const
    {
        if (GetValueType() != ValueType::String)
        {
            auto prev = mValue;
            mValue = o2js_to_string(mValue);
            o2js_release(prev);
        }

        String res;
        res.resize(o2js_get_string_length(mValue));
        o2js_string_to_buffer(mValue, (char*)res.Data(), res.Capacity());
        return res;
    }

    ScriptValue ScriptValue::InvokeRaw(const Vector<ScriptValue>& args) const
    {
        return InvokeRaw(ScriptValue(), args);
    }

    ScriptValue ScriptValue::InvokeRaw(const ScriptValue& thisValue, const Vector<ScriptValue>& args) const
    {
        return InvokeRaw(thisValue, args.Count() > 0 ? &args[0] : nullptr, args.Count());
    }

    ScriptValue ScriptValue::InvokeRaw(const ScriptValue& thisValue, const ScriptValue* args, int argsCount) const
    {
        if (IsFunction())
        {
            const int maxParameters = 16;
            o2js_value_t valuesBuf[maxParameters];
            for (int i = 0; i < argsCount && i < maxParameters; i++)
                valuesBuf[i] = args[i].mValue;

            auto res = o2js_call_function(mValue, thisValue.mValue, valuesBuf, argsCount);

            ScriptValue resValue;
            resValue.Accept(res);

            return resValue;
        }

        return {};
    }

    void ScriptValueBase::FreeDataContainer(void* ptr)
    {
        auto* container = static_cast<IDataContainer*>(ptr);
        if (container)
            container->Destroy();
    }

    void* ScriptValueBase::AllocateContainerMemory(size_t size, size_t alignment)
    {
        return ScriptContainerAllocator::GetInstance().Allocate(size, alignment);
    }

    void ScriptValueBase::FreeContainerMemory(void* ptr)
    {
        ScriptContainerAllocator::GetInstance().Free(ptr);
    }

    ScriptValueBase::IDataContainer* ScriptValueBase::GetNativeContainer(o2js_value_t jval)
    {
        return (IDataContainer*)o2js_get_native_pointer(jval, &FreeDataContainer);
    }

    o2js_value_t ScriptValueBase::CallFunction(const o2js_value_t function_obj,
                                                const o2js_value_t this_val,
                                                const o2js_value_t args_p[], const int args_count)
    {
        auto container = static_cast<IFunctionContainer*>(GetNativeContainer(function_obj));
        return container->Invoke(this_val, (o2js_value_t*)args_p, args_count);
    }

    o2js_value_t ScriptValueBase::DescriptorSetter(const o2js_value_t function_obj,
                                                    const o2js_value_t this_val,
                                                    const o2js_value_t args_p[],
                                                    const int args_count)
    {
        auto container = static_cast<ISetterWrapperContainer*>(GetNativeContainer(function_obj));
        container->Set(args_p[0]);

        return o2js_undefined();
    }

    o2js_value_t ScriptValueBase::DescriptorGetter(const o2js_value_t function_obj,
                                                    const o2js_value_t this_val,
                                                    const o2js_value_t args_p[],
                                                    const int args_count)
    {
        auto container = static_cast<IGetterWrapperContainer*>(GetNativeContainer(function_obj));
        return container->Get();
    }

    o2js_value_t ScriptValueBase::PrototypeDescriptorGetter(const o2js_value_t function_obj,
                                                             const o2js_value_t this_val,
                                                             const o2js_value_t args_p[],
                                                             const int args_count)
    {
        auto container = static_cast<IPrototypeGetter*>(GetNativeContainer(function_obj));
        return container->GetFrom(this_val);
    }

    o2js_value_t ScriptValueBase::PrototypeDescriptorSetter(const o2js_value_t function_obj,
                                                             const o2js_value_t this_val,
                                                             const o2js_value_t args_p[],
                                                             const int args_count)
    {
        auto container = static_cast<IPrototypeSetter*>(GetNativeContainer(function_obj));
        container->SetTo(this_val, args_p[0]);
        return o2js_undefined();
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

    namespace
    {
        const ScriptValue& GetCachedPropertyKey(ScriptValue*& storage, const char* name)
        {
            if (!storage)
                storage = mnew ScriptValue(name);

            return *storage;
        }

        void ReleaseCachedScriptValue(ScriptValue*& value)
        {
            delete value;
            value = nullptr;
        }
    }

    void ScriptValuePropertyKeys::Initialize()
    {
        GetX();
        GetY();
        GetLeft();
        GetBottom();
        GetRight();
        GetTop();
        GetR();
        GetG();
        GetB();
        GetA();
    }

    void ScriptValuePropertyKeys::Deinitialize()
    {
        ReleaseCachedScriptValue(XStorage());
        ReleaseCachedScriptValue(YStorage());
        ReleaseCachedScriptValue(LeftStorage());
        ReleaseCachedScriptValue(BottomStorage());
        ReleaseCachedScriptValue(RightStorage());
        ReleaseCachedScriptValue(TopStorage());
        ReleaseCachedScriptValue(RStorage());
        ReleaseCachedScriptValue(GStorage());
        ReleaseCachedScriptValue(BStorage());
        ReleaseCachedScriptValue(AStorage());
    }

    const ScriptValue& ScriptValuePropertyKeys::GetX()
    {
        return GetCachedPropertyKey(XStorage(), "x");
    }

    const ScriptValue& ScriptValuePropertyKeys::GetY()
    {
        return GetCachedPropertyKey(YStorage(), "y");
    }

    const ScriptValue& ScriptValuePropertyKeys::GetLeft()
    {
        return GetCachedPropertyKey(LeftStorage(), "left");
    }

    const ScriptValue& ScriptValuePropertyKeys::GetBottom()
    {
        return GetCachedPropertyKey(BottomStorage(), "bottom");
    }

    const ScriptValue& ScriptValuePropertyKeys::GetRight()
    {
        return GetCachedPropertyKey(RightStorage(), "right");
    }

    const ScriptValue& ScriptValuePropertyKeys::GetTop()
    {
        return GetCachedPropertyKey(TopStorage(), "top");
    }

    const ScriptValue& ScriptValuePropertyKeys::GetR()
    {
        return GetCachedPropertyKey(RStorage(), "r");
    }

    const ScriptValue& ScriptValuePropertyKeys::GetG()
    {
        return GetCachedPropertyKey(GStorage(), "g");
    }

    const ScriptValue& ScriptValuePropertyKeys::GetB()
    {
        return GetCachedPropertyKey(BStorage(), "b");
    }

    const ScriptValue& ScriptValuePropertyKeys::GetA()
    {
        return GetCachedPropertyKey(AStorage(), "a");
    }

    ScriptValue*& ScriptValuePropertyKeys::XStorage()
    {
        static ScriptValue* value = nullptr;
        return value;
    }

    ScriptValue*& ScriptValuePropertyKeys::YStorage()
    {
        static ScriptValue* value = nullptr;
        return value;
    }

    ScriptValue*& ScriptValuePropertyKeys::LeftStorage()
    {
        static ScriptValue* value = nullptr;
        return value;
    }

    ScriptValue*& ScriptValuePropertyKeys::BottomStorage()
    {
        static ScriptValue* value = nullptr;
        return value;
    }

    ScriptValue*& ScriptValuePropertyKeys::RightStorage()
    {
        static ScriptValue* value = nullptr;
        return value;
    }

    ScriptValue*& ScriptValuePropertyKeys::TopStorage()
    {
        static ScriptValue* value = nullptr;
        return value;
    }

    ScriptValue*& ScriptValuePropertyKeys::RStorage()
    {
        static ScriptValue* value = nullptr;
        return value;
    }

    ScriptValue*& ScriptValuePropertyKeys::GStorage()
    {
        static ScriptValue* value = nullptr;
        return value;
    }

    ScriptValue*& ScriptValuePropertyKeys::BStorage()
    {
        static ScriptValue* value = nullptr;
        return value;
    }

    ScriptValue*& ScriptValuePropertyKeys::AStorage()
    {
        static ScriptValue* value = nullptr;
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
        ScriptValue proto;
        proto.SetProperty("type", ScriptValue(type));
        constructorFunc.SetPrototype(proto);
        nspace.SetProperty(constructor.Data(), constructorFunc);
    }

    void ScriptPrototypeProcessor::RegisterTypeStaticFunction(Type* type, const char* name, const ScriptValue& func)
    {
        GetNameSpace(o2Scripts.GetGlobal(), type->GetName()).SetProperty(name, func);
    }
}

#endif
