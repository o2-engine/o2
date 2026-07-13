#include "o2/stdafx.h"

#include "o2/Scripts/ScriptValueDef.h"
#include "o2/Scripts/ScriptValueContainerAllocator.h"
#include "o2/Utils/Debug/Debug.h"

#if defined(SCRIPTING_BACKEND_QUICKJS)
#include "o2/Scripts/QuickJS/QuickJSCore.h"
#include "o2/Scripts/ScriptValue.h"

namespace o2
{
    using namespace QuickJs;

    ScriptValueBase::~ScriptValueBase()
    {
        JS_FreeValue(Ctx(), mValue);
    }

    void ScriptValueBase::AcquireValue(JSValueConst v)
    {
        JSContext* ctx = Ctx();
        JS_FreeValue(ctx, mValue);
        mValue = JS_DupValue(ctx, v);
        mIsError = false;
    }

    void ScriptValueBase::Accept(JSValue v)
    {
        JS_FreeValue(Ctx(), mValue);
        mValue = v;
        mIsError = false;
    }

    void ScriptValueBase::AcceptThrown()
    {
        JS_FreeValue(Ctx(), mValue);
        mValue = TakeThrown();
        mIsError = true;
    }

    ScriptValue ScriptValue::EmptyObject()
    {
        ScriptValue res;
        res.Accept(JS_NewObject(Ctx()));
        return res;
    }

    ScriptValue ScriptValue::EmptyArray()
    {
        ScriptValue res;
        res.Accept(JS_NewArray(Ctx()));
        return res;
    }

    ScriptValue::ScriptValue()
    {
        mValue = JS_UNDEFINED;
    }

    ScriptValue::ScriptValue(const ScriptValue& other)
    {
        mValue = JS_DupValue(Ctx(), other.mValue);
        mIsError = other.mIsError;
    }

    ScriptValue::ScriptValue(ScriptValue&& other) noexcept
    {
        mValue = other.mValue;
        mIsError = other.mIsError;
        other.mValue = JS_UNDEFINED;
        other.mIsError = false;
    }

    ScriptValue ScriptValue::operator[](const ScriptValue& name) const
    {
        return GetProperty(name);
    }

    ScriptValue ScriptValue::operator[](int idx) const
    {
        return GetElement(idx);
    }

    bool ScriptValue::operator!=(const ScriptValue& other) const
    {
        return !operator==(other);
    }

    bool ScriptValue::operator==(const ScriptValue& other) const
    {
        int res = JS_IsEqual(Ctx(), mValue, other.mValue);
        if (res < 0)
        {
            ClearThrown();
            return false;
        }

        return res > 0;
    }

    ScriptValue& ScriptValue::operator=(const ScriptValue& other)
    {
        AcquireValue(other.mValue);
        mIsError = other.mIsError;
        return *this;
    }

    ScriptValue& ScriptValue::operator=(ScriptValue&& other) noexcept
    {
        if (this != &other)
        {
            JS_FreeValue(Ctx(), mValue);
            mValue = other.mValue;
            mIsError = other.mIsError;
            other.mValue = JS_UNDEFINED;
            other.mIsError = false;
        }
        return *this;
    }

    ScriptValue::ValueType ScriptValue::GetValueType() const
    {
        if (mIsError)
            return ValueType::Error;

        if (JS_IsArray(mValue))
            return ValueType::Array;

        if (JS_IsUndefined(mValue)) return ValueType::Undefined;
        if (JS_IsNull(mValue)) return ValueType::Null;
        if (JS_IsBool(mValue)) return ValueType::Bool;
        if (JS_IsNumber(mValue)) return ValueType::Number;
        if (JS_IsString(mValue)) return ValueType::String;
        if (JS_IsSymbol(mValue)) return ValueType::Symbol;
        if (JS_IsBigInt(mValue)) return ValueType::BigInt;
        if (JS_IsFunction(Ctx(), mValue)) return ValueType::Function;
        if (JS_IsObject(mValue)) return ValueType::Object;

        return ValueType::None;
    }

    bool ScriptValue::IsConstructor() const
    {
        return JS_IsConstructor(Ctx(), mValue);
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
                SetNativePointer(res.mValue, clonedDataContainer, &FreeDataContainer);
            }

            return res;
        }

        return *this;
    }

    int ScriptValue::GetLength() const
    {
        if (!JS_IsArray(mValue))
            return 0;

        JSContext* ctx = Ctx();
        JSValue lengthValue = JS_GetProperty(ctx, mValue, LengthAtom());
        uint32_t length = 0;
        JS_ToUint32(ctx, &length, lengthValue);
        JS_FreeValue(ctx, lengthValue);
        return (int)length;
    }

    String ScriptValue::GetError() const
    {
        if (GetValueType() != ValueType::Error)
            return String();

        JSContext* ctx = Ctx();
        JSValue str = JS_ToString(ctx, mValue);
        if (JS_IsException(str))
        {
            ClearThrown();
            return String();
        }

        const char* cstr = JS_ToCString(ctx, str);
        String res(cstr ? cstr : "");
        if (cstr)
            JS_FreeCString(ctx, cstr);

        JS_FreeValue(ctx, str);
        return res;
    }

    bool ScriptValue::IsObjectContainer() const
    {
        return IsObject() && GetNativeContainer(mValue) != nullptr;
    }

    const Type* ScriptValue::GetObjectContainerType() const
    {
        if (!IsObject())
            return nullptr;

        if (auto dataContainer = GetNativeContainer(mValue))
            return dataContainer->GetType();

        return nullptr;
    }

    void* ScriptValue::GetContainingObject() const
    {
        if (!IsObject())
            return nullptr;

        if (auto dataContainer = GetNativeContainer(mValue))
            return dataContainer->GetData();

        return nullptr;
    }

    ScriptValue ScriptValue::Construct(const Vector<ScriptValue>& args)
    {
        const int maxParameters = 16;
        JSValue valuesBuf[maxParameters];
        for (int i = 0; i < args.Count() && i < maxParameters; i++)
            valuesBuf[i] = args[i].mValue;

        ScriptValue res;
        JSValue constructed = JS_CallConstructor(Ctx(), mValue, args.Count(), valuesBuf);
        if (JS_IsException(constructed))
            res.AcceptThrown();
        else
            res.Accept(constructed);

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
        JSContext* ctx = Ctx();
        if (GetValueType() != ValueType::Object)
        {
            JS_FreeValue(ctx, mValue);
            mValue = JS_NewObject(ctx);
            mIsError = false;
        }

        JSAtom atom = JS_ValueToAtom(ctx, name.mValue);
        JSValue value = JS_GetProperty(ctx, mValue, atom);
        JS_FreeAtom(ctx, atom);

        ScriptValue res;
        if (JS_IsException(value))
            res.AcceptThrown();
        else
            res.Accept(value);

        return res;
    }

    ScriptValue ScriptValue::GetInternalProperty(const ScriptValue& name) const
    {
        JSContext* ctx = Ctx();
        ScriptValue res;
        if (!JS_IsObject(mValue))
            return res;

        JSPropertyDescriptor desc;
        if (JS_GetOwnProperty(ctx, &desc, mValue, InternalAtom()) <= 0)
            return res;

        JSValue dict = desc.value;
        JS_FreeValue(ctx, desc.getter);
        JS_FreeValue(ctx, desc.setter);

        if (const char* key = JS_ToCString(ctx, name.mValue))
        {
            res.Accept(JS_GetPropertyStr(ctx, dict, key));
            JS_FreeCString(ctx, key);
        }

        JS_FreeValue(ctx, dict);
        return res;
    }

    ScriptValue ScriptValue::GetOwnProperty(const ScriptValue& name) const
    {
        JSContext* ctx = Ctx();
        ScriptValue res;
        if (!JS_IsObject(mValue))
            return res;

        JSAtom atom = JS_ValueToAtom(ctx, name.mValue);
        JSPropertyDescriptor desc;
        int found = JS_GetOwnProperty(ctx, &desc, mValue, atom);
        JS_FreeAtom(ctx, atom);

        if (found <= 0)
        {
            if (found < 0)
                ClearThrown();
            return res;
        }

        if (desc.flags & JS_PROP_GETSET)
        {
            JS_FreeValue(ctx, desc.getter);
            JS_FreeValue(ctx, desc.setter);
            return res;
        }

        res.Accept(desc.value);
        return res;
    }

    ScriptValue ScriptValue::GetPropertyNames() const
    {
        JSContext* ctx = Ctx();
        ScriptValue res = EmptyArray();
        if (!JS_IsObject(mValue))
            return res;

        JSPropertyEnum* tab = nullptr;
        uint32_t count = 0;
        if (JS_GetOwnPropertyNames(ctx, &tab, &count, mValue, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0)
        {
            ClearThrown();
            return res;
        }

        // Mirrors the jerry backend filter: own enumerable+configurable+writable data
        // properties, string keys only, array indices excluded
        uint32_t resultIdx = 0;
        for (uint32_t i = 0; i < count; i++)
        {
            JSPropertyDescriptor desc;
            if (JS_GetOwnProperty(ctx, &desc, mValue, tab[i].atom) <= 0)
                continue;

            bool plainDataProperty = (desc.flags & JS_PROP_ENUMERABLE) && (desc.flags & JS_PROP_CONFIGURABLE) &&
                                     (desc.flags & JS_PROP_WRITABLE) && !(desc.flags & JS_PROP_GETSET);

            JS_FreeValue(ctx, desc.value);
            JS_FreeValue(ctx, desc.getter);
            JS_FreeValue(ctx, desc.setter);

            if (!plainDataProperty)
                continue;

            const char* key = JS_AtomToCString(ctx, tab[i].atom);
            if (!key)
                continue;

            bool isIndex = key[0] != 0;
            for (const char* c = key; *c; c++)
            {
                if (*c < '0' || *c > '9')
                {
                    isIndex = false;
                    break;
                }
            }

            if (!isIndex)
                JS_SetPropertyUint32(ctx, res.mValue, resultIdx++, JS_NewString(ctx, key));

            JS_FreeCString(ctx, key);
        }

        JS_FreePropertyEnum(ctx, tab, count);
        return res;
    }

    void ScriptValue::SetInternalProperty(const ScriptValue& name, const ScriptValue& value)
    {
        JSContext* ctx = Ctx();
        if (!JS_IsObject(mValue))
            return;

        JSValue dict;
        JSPropertyDescriptor desc;
        if (JS_GetOwnProperty(ctx, &desc, mValue, InternalAtom()) > 0)
        {
            dict = desc.value;
            JS_FreeValue(ctx, desc.getter);
            JS_FreeValue(ctx, desc.setter);
        }
        else
        {
            dict = JS_NewObject(ctx);
            JS_DefinePropertyValue(ctx, mValue, InternalAtom(), JS_DupValue(ctx, dict), 0);
        }

        if (const char* key = JS_ToCString(ctx, name.mValue))
        {
            JS_SetPropertyStr(ctx, dict, key, JS_DupValue(ctx, value.mValue));
            JS_FreeCString(ctx, key);
        }

        JS_FreeValue(ctx, dict);
    }

    void ScriptValue::SetProperty(const ScriptValue& name, const ScriptValue& value)
    {
        JSContext* ctx = Ctx();
        if (GetValueType() != ValueType::Object)
        {
            JS_FreeValue(ctx, mValue);
            mValue = JS_NewObject(ctx);
            mIsError = false;
        }

        JSAtom atom = JS_ValueToAtom(ctx, name.mValue);
        if (JS_SetProperty(ctx, mValue, atom, JS_DupValue(ctx, value.mValue)) < 0)
            ClearThrown();

        JS_FreeAtom(ctx, atom);
    }

    void ScriptValue::RemoveProperty(const ScriptValue& name)
    {
        JSContext* ctx = Ctx();
        JSAtom atom = JS_ValueToAtom(ctx, name.mValue);
        if (JS_DeleteProperty(ctx, mValue, atom, 0) < 0)
            ClearThrown();

        JS_FreeAtom(ctx, atom);
    }

    void ScriptValue::SetPrototype(const ScriptValue& proto)
    {
        JSValueConst protoValue = JS_IsUndefined(proto.mValue) ? JS_NULL : proto.mValue;
        if (JS_SetPrototype(Ctx(), mValue, protoValue) < 0)
            ClearThrown();
    }

    ScriptValue ScriptValue::GetPrototype() const
    {
        ScriptValue res;
        if (!JS_IsObject(mValue))
        {
            res.Accept(JS_NULL);
            return res;
        }

        JSValue proto = JS_GetPrototype(Ctx(), mValue);
        if (JS_IsException(proto))
            res.AcceptThrown();
        else
            res.Accept(proto);

        return res;
    }

    void ScriptValue::SetElement(const ScriptValue& value, int idx)
    {
        JSContext* ctx = Ctx();
        if (GetValueType() != ValueType::Array)
        {
            JS_FreeValue(ctx, mValue);
            mValue = JS_NewArray(ctx);
            mIsError = false;
        }

        if (JS_SetPropertyUint32(ctx, mValue, (uint32_t)idx, JS_DupValue(ctx, value.mValue)) < 0)
            ClearThrown();
    }

    ScriptValue ScriptValue::GetElement(int idx) const
    {
        ScriptValue res;
        JSValue value = JS_GetPropertyUint32(Ctx(), mValue, (uint32_t)idx);
        if (JS_IsException(value))
            res.AcceptThrown();
        else
            res.Accept(value);

        return res;
    }

    void ScriptValue::AddElement(const ScriptValue& value)
    {
        SetElement(value, GetLength());
    }

    void ScriptValue::RemoveElement(int idx)
    {
        JSContext* ctx = Ctx();
        JSValue indexValue = JS_NewUint32(ctx, (uint32_t)idx);
        JSAtom atom = JS_ValueToAtom(ctx, indexValue);
        JS_FreeValue(ctx, indexValue);
        if (JS_DeleteProperty(ctx, mValue, atom, 0) < 0)
            ClearThrown();

        JS_FreeAtom(ctx, atom);
    }

    bool ScriptValue::ToBool() const
    {
        return JS_ToBool(Ctx(), mValue) > 0;
    }

    float ScriptValue::ToNumber() const
    {
        JSContext* ctx = Ctx();
        double res = 0;
        if (JS_ToFloat64(ctx, &res, mValue) < 0)
        {
            ClearThrown();
            return 0.0f;
        }

        return (float)res;
    }

    String ScriptValue::ToString() const
    {
        JSContext* ctx = Ctx();
        if (GetValueType() != ValueType::String)
        {
            JSValue str = JS_ToString(ctx, mValue);
            if (JS_IsException(str))
            {
                ClearThrown();
                return String();
            }

            JS_FreeValue(ctx, mValue);
            mValue = str;
            mIsError = false;
        }

        const char* cstr = JS_ToCString(ctx, mValue);
        String res(cstr ? cstr : "");
        if (cstr)
            JS_FreeCString(ctx, cstr);

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
            JSValue valuesBuf[maxParameters];
            for (int i = 0; i < argsCount && i < maxParameters; i++)
                valuesBuf[i] = args[i].mValue;

            ScriptValue resValue;
            JSValue res = JS_Call(Ctx(), mValue, thisValue.mValue, argsCount, valuesBuf);
            if (JS_IsException(res))
                resValue.AcceptThrown();
            else
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

    ScriptValueBase::IDataContainer* ScriptValueBase::GetNativeContainer(JSValueConst val)
    {
        return (IDataContainer*)GetNativePointer(val, &FreeDataContainer);
    }

    JSValue ScriptValueBase::CallFunction(JSValueConst function_obj, JSValueConst this_val,
                                          JSValueConst* args_p, int args_count)
    {
        auto container = static_cast<IFunctionContainer*>(GetNativeContainer(function_obj));
        if (!container)
            return JS_UNDEFINED;

        return container->Invoke(this_val, args_p, args_count);
    }

    JSValue ScriptValueBase::DescriptorSetter(JSValueConst function_obj, JSValueConst this_val,
                                              JSValueConst* args_p, int args_count)
    {
        auto container = static_cast<ISetterWrapperContainer*>(GetNativeContainer(function_obj));
        if (container && args_count > 0)
            container->Set(args_p[0]);

        return JS_UNDEFINED;
    }

    JSValue ScriptValueBase::DescriptorGetter(JSValueConst function_obj, JSValueConst this_val,
                                              JSValueConst* args_p, int args_count)
    {
        auto container = static_cast<IGetterWrapperContainer*>(GetNativeContainer(function_obj));
        if (!container)
            return JS_UNDEFINED;

        return container->Get();
    }

    JSValue ScriptValueBase::PrototypeDescriptorGetter(JSValueConst function_obj, JSValueConst this_val,
                                                       JSValueConst* args_p, int args_count)
    {
        auto container = static_cast<IPrototypeGetter*>(GetNativeContainer(function_obj));
        if (!container)
            return JS_UNDEFINED;

        return container->GetFrom(this_val);
    }

    JSValue ScriptValueBase::PrototypeDescriptorSetter(JSValueConst function_obj, JSValueConst this_val,
                                                       JSValueConst* args_p, int args_count)
    {
        auto container = static_cast<IPrototypeSetter*>(GetNativeContainer(function_obj));
        if (container && args_count > 0)
            container->SetTo(this_val, args_p[0]);

        return JS_UNDEFINED;
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
