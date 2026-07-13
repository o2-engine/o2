#pragma once

#if defined(SCRIPTING_BACKEND_QUICKJS)
#include "o2/Scripts/QuickJS/QuickJSCore.h"
#include "o2/Utils/Reflection/Type.h"

#include <cstring>
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    template<typename _type, typename _enable /*= void*/>
    void ScriptValue::Converter<_type, _enable>::Read(__type& value, const ScriptValue& data)
    {
        if constexpr (std::is_pointer<__type>::value)
            value = (__type)data.GetContainingObject();
        else if (auto objPtr = (__type*)data.GetContainingObject())
            value = *objPtr;
    }

    template<typename _type, typename _enable /*= void*/>
    void ScriptValue::Converter<_type, _enable>::Write(const __type& value, ScriptValue& data)
    {
        data.mValue = JS_NewObject(QuickJs::Ctx());
        if constexpr (std::is_pointer<__type>::value)
            data.SetContainingObject(value);
        else
        {
            auto dataContainer = ScriptValueBase::CreateContainer<ScriptValueBase::DataContainer<__type>>(value);
            QuickJs::SetNativePointer(data.mValue, (ScriptValueBase::IDataContainer*)dataContainer,
                                      &ScriptValueBase::FreeDataContainer);
        }
    }

    template<>
    struct ScriptValue::Converter<ScriptValue>
    {
        static constexpr bool isSupported = true;

        static void Write(const ScriptValue& value, ScriptValue& data)
        {
            data = value;
        }

        static void Read(ScriptValue& value, const ScriptValue& data)
        {
            value = data;
        }
    };

    template<>
    struct ScriptValue::Converter<char*>
    {
        static constexpr bool isSupported = true;

        using charPtr = char*;

        static void Write(const charPtr& value, ScriptValue& data)
        {
            data.mValue = JS_NewString(QuickJs::Ctx(), value);
        }

        static void Read(charPtr& value, const ScriptValue& data)
        {
            JSContext* ctx = QuickJs::Ctx();
            size_t len = 0;
            if (const char* cstr = JS_ToCStringLen(ctx, &len, data.mValue))
            {
                size_t count = len < 255 ? len : 255;
                memcpy(value, cstr, count);
                value[count] = 0;
                JS_FreeCString(ctx, cstr);
            }
        }
    };

    template<UInt _size>
    struct ScriptValue::Converter<char[_size]>
    {
        static constexpr bool isSupported = true;

        using charPtr = char[_size];

        static void Write(const charPtr& value, ScriptValue& data)
        {
            data.mValue = JS_NewString(QuickJs::Ctx(), value);
        }

        static void Read(charPtr& value, const ScriptValue& data)
        {
            JSContext* ctx = QuickJs::Ctx();
            size_t len = 0;
            if (const char* cstr = JS_ToCStringLen(ctx, &len, data.mValue))
            {
                size_t count = len < _size - 1 ? len : _size - 1;
                memcpy(value, cstr, count);
                value[count] = 0;
                JS_FreeCString(ctx, cstr);
            }
        }
    };

    template<>
    struct ScriptValue::Converter<const char*>
    {
        static constexpr bool isSupported = true;

        using charPtr = const char*;

        static void Write(const charPtr& value, ScriptValue& data)
        {
            data.mValue = JS_NewString(QuickJs::Ctx(), value);
        }

        static void Read(charPtr& value, const ScriptValue& data)
        {}
    };

    template<>
    struct ScriptValue::Converter<bool>
    {
        static constexpr bool isSupported = true;

        static void Write(const bool& value, ScriptValue& data)
        {
            data.mValue = JS_NewBool(QuickJs::Ctx(), value);
        }

        static void Read(bool& value, const ScriptValue& data)
        {
            value = JS_ToBool(QuickJs::Ctx(), data.mValue) > 0;
        }
    };

    template<>
    struct ScriptValue::Converter<int>
    {
        static constexpr bool isSupported = true;

        static void Write(const int& value, ScriptValue& data)
        {
            data.mValue = JS_NewFloat64(QuickJs::Ctx(), (double)value);
        }

        static void Read(int& value, const ScriptValue& data)
        {
            value = (int)QuickJs::ToInteger(data.mValue);
        }
    };

    template<>
    struct ScriptValue::Converter<UInt>
    {
        static constexpr bool isSupported = true;

        static void Write(const UInt& value, ScriptValue& data)
        {
            data.mValue = JS_NewFloat64(QuickJs::Ctx(), (double)value);
        }

        static void Read(UInt& value, const ScriptValue& data)
        {
            value = (UInt)QuickJs::ToInteger(data.mValue);
        }
    };

    template<>
    struct ScriptValue::Converter<Int64>
    {
        static constexpr bool isSupported = true;

        static void Write(const Int64& value, ScriptValue& data)
        {
            data.mValue = JS_NewFloat64(QuickJs::Ctx(), (double)value);
        }

        static void Read(Int64& value, const ScriptValue& data)
        {
            value = (Int64)QuickJs::ToInteger(data.mValue);
        }
    };

    template<>
    struct ScriptValue::Converter<UInt64>
    {
        static constexpr bool isSupported = true;

        static void Write(const UInt64& value, ScriptValue& data)
        {
            data.mValue = JS_NewFloat64(QuickJs::Ctx(), (double)value);
        }

        static void Read(UInt64& value, const ScriptValue& data)
        {
            value = (UInt64)QuickJs::ToInteger(data.mValue);
        }
    };

    template<>
    struct ScriptValue::Converter<double>
    {
        static constexpr bool isSupported = true;

        static void Write(const double& value, ScriptValue& data)
        {
            data.mValue = JS_NewFloat64(QuickJs::Ctx(), value);
        }

        static void Read(double& value, const ScriptValue& data)
        {
            double res = 0;
            if (JS_IsNumber(data.mValue))
                JS_ToFloat64(QuickJs::Ctx(), &res, data.mValue);

            value = res;
        }
    };

    template<>
    struct ScriptValue::Converter<float>
    {
        static constexpr bool isSupported = true;

        static void Write(const float& value, ScriptValue& data)
        {
            data.mValue = JS_NewFloat64(QuickJs::Ctx(), (double)value);
        }

        static void Read(float& value, const ScriptValue& data)
        {
            double res = 0;
            if (JS_IsNumber(data.mValue))
                JS_ToFloat64(QuickJs::Ctx(), &res, data.mValue);

            value = (float)res;
        }
    };

    template<>
    struct ScriptValue::Converter<String>
    {
        static constexpr bool isSupported = true;

        static void Write(const String& value, ScriptValue& data)
        {
            data.mValue = JS_NewString(QuickJs::Ctx(), value.Data());
        }

        static void Read(String& value, const ScriptValue& data)
        {
            JSContext* ctx = QuickJs::Ctx();
            if (const char* cstr = JS_ToCString(ctx, data.mValue))
            {
                value = cstr;
                JS_FreeCString(ctx, cstr);
            }
            else
            {
                QuickJs::ClearThrown();
                value = String();
            }
        }
    };

    template<>
    struct ScriptValue::Converter<UID>
    {
        static constexpr bool isSupported = true;

        static void Write(const UID& value, ScriptValue& data)
        {
            data.SetValue((String)value);
        }

        static void Read(UID& value, const ScriptValue& data)
        {
            String buf = data.GetValue<String>();
            value = buf;
        }
    };

    template<>
    struct ScriptValue::Converter<Vec2F>
    {
        static constexpr bool isSupported = true;

        static void Write(const Vec2F& value, ScriptValue& data)
        {
            data.mValue = JS_NewObject(QuickJs::Ctx());
            data.SetPrototype(*ScriptValuePrototypes::GetVec2Prototype());
            data.SetProperty(ScriptValuePropertyKeys::GetX(), ScriptValue(value.x));
            data.SetProperty(ScriptValuePropertyKeys::GetY(), ScriptValue(value.y));
        }

        static void Read(Vec2F& value, const ScriptValue& data)
        {
            value.x = data.GetProperty(ScriptValuePropertyKeys::GetX()).GetValue<float>();
            value.y = data.GetProperty(ScriptValuePropertyKeys::GetY()).GetValue<float>();
        }
    };

    template<>
    struct ScriptValue::Converter<Vec2I>
    {
        static constexpr bool isSupported = true;

        static void Write(const Vec2I& value, ScriptValue& data)
        {
            data.mValue = JS_NewObject(QuickJs::Ctx());
            data.SetPrototype(*ScriptValuePrototypes::GetVec2Prototype());
            data.SetProperty(ScriptValuePropertyKeys::GetX(), ScriptValue(value.x));
            data.SetProperty(ScriptValuePropertyKeys::GetY(), ScriptValue(value.y));
        }

        static void Read(Vec2I& value, const ScriptValue& data)
        {
            value.x = data.GetProperty(ScriptValuePropertyKeys::GetX()).GetValue<int>();
            value.y = data.GetProperty(ScriptValuePropertyKeys::GetY()).GetValue<int>();
        }
    };

    template<>
    struct ScriptValue::Converter<RectF>
    {
        static constexpr bool isSupported = true;

        static void Write(const RectF& value, ScriptValue& data)
        {
            data.mValue = JS_NewObject(QuickJs::Ctx());
            data.SetPrototype(*ScriptValuePrototypes::GetRectPrototype());
            data.SetProperty(ScriptValuePropertyKeys::GetLeft(), ScriptValue(value.left));
            data.SetProperty(ScriptValuePropertyKeys::GetBottom(), ScriptValue(value.bottom));
            data.SetProperty(ScriptValuePropertyKeys::GetRight(), ScriptValue(value.right));
            data.SetProperty(ScriptValuePropertyKeys::GetTop(), ScriptValue(value.top));
        }

        static void Read(RectF& value, const ScriptValue& data)
        {
            value.left = data.GetProperty(ScriptValuePropertyKeys::GetLeft()).GetValue<float>();
            value.bottom = data.GetProperty(ScriptValuePropertyKeys::GetBottom()).GetValue<float>();
            value.right = data.GetProperty(ScriptValuePropertyKeys::GetRight()).GetValue<float>();
            value.top = data.GetProperty(ScriptValuePropertyKeys::GetTop()).GetValue<float>();
        }
    };

    template<>
    struct ScriptValue::Converter<RectI>
    {
        static constexpr bool isSupported = true;

        static void Write(const RectI& value, ScriptValue& data)
        {
            data.mValue = JS_NewObject(QuickJs::Ctx());
            data.SetPrototype(*ScriptValuePrototypes::GetRectPrototype());
            data.SetProperty(ScriptValuePropertyKeys::GetLeft(), ScriptValue(value.left));
            data.SetProperty(ScriptValuePropertyKeys::GetBottom(), ScriptValue(value.bottom));
            data.SetProperty(ScriptValuePropertyKeys::GetRight(), ScriptValue(value.right));
            data.SetProperty(ScriptValuePropertyKeys::GetTop(), ScriptValue(value.top));
        }

        static void Read(RectI& value, const ScriptValue& data)
        {
            value.left = data.GetProperty(ScriptValuePropertyKeys::GetLeft()).GetValue<int>();
            value.bottom = data.GetProperty(ScriptValuePropertyKeys::GetBottom()).GetValue<int>();
            value.right = data.GetProperty(ScriptValuePropertyKeys::GetRight()).GetValue<int>();
            value.top = data.GetProperty(ScriptValuePropertyKeys::GetTop()).GetValue<int>();
        }
    };

    template<>
    struct ScriptValue::Converter<BorderF>
    {
        static constexpr bool isSupported = true;

        static void Write(const BorderF& value, ScriptValue& data)
        {
            data.mValue = JS_NewObject(QuickJs::Ctx());
            data.SetPrototype(*ScriptValuePrototypes::GetBorderPrototype());
            data.SetProperty(ScriptValuePropertyKeys::GetLeft(), ScriptValue(value.left));
            data.SetProperty(ScriptValuePropertyKeys::GetBottom(), ScriptValue(value.bottom));
            data.SetProperty(ScriptValuePropertyKeys::GetRight(), ScriptValue(value.right));
            data.SetProperty(ScriptValuePropertyKeys::GetTop(), ScriptValue(value.top));
        }

        static void Read(BorderF& value, const ScriptValue& data)
        {
            value.left = data.GetProperty(ScriptValuePropertyKeys::GetLeft()).GetValue<float>();
            value.bottom = data.GetProperty(ScriptValuePropertyKeys::GetBottom()).GetValue<float>();
            value.right = data.GetProperty(ScriptValuePropertyKeys::GetRight()).GetValue<float>();
            value.top = data.GetProperty(ScriptValuePropertyKeys::GetTop()).GetValue<float>();
        }
    };

    template<>
    struct ScriptValue::Converter<BorderI>
    {
        static constexpr bool isSupported = true;

        static void Write(const BorderI& value, ScriptValue& data)
        {
            data.mValue = JS_NewObject(QuickJs::Ctx());
            data.SetPrototype(*ScriptValuePrototypes::GetBorderPrototype());
            data.SetProperty(ScriptValuePropertyKeys::GetLeft(), ScriptValue(value.left));
            data.SetProperty(ScriptValuePropertyKeys::GetBottom(), ScriptValue(value.bottom));
            data.SetProperty(ScriptValuePropertyKeys::GetRight(), ScriptValue(value.right));
            data.SetProperty(ScriptValuePropertyKeys::GetTop(), ScriptValue(value.top));
        }

        static void Read(BorderI& value, const ScriptValue& data)
        {
            value.left = data.GetProperty(ScriptValuePropertyKeys::GetLeft()).GetValue<int>();
            value.bottom = data.GetProperty(ScriptValuePropertyKeys::GetBottom()).GetValue<int>();
            value.right = data.GetProperty(ScriptValuePropertyKeys::GetRight()).GetValue<int>();
            value.top = data.GetProperty(ScriptValuePropertyKeys::GetTop()).GetValue<int>();
        }
    };

    template<>
    struct ScriptValue::Converter<Color4>
    {
        static constexpr bool isSupported = true;

        static void Write(const Color4& value, ScriptValue& data)
        {
            data.mValue = JS_NewObject(QuickJs::Ctx());
            data.SetPrototype(*ScriptValuePrototypes::GetColor4Prototype());
            data.SetProperty(ScriptValuePropertyKeys::GetR(), ScriptValue(value.r));
            data.SetProperty(ScriptValuePropertyKeys::GetG(), ScriptValue(value.g));
            data.SetProperty(ScriptValuePropertyKeys::GetB(), ScriptValue(value.b));
            data.SetProperty(ScriptValuePropertyKeys::GetA(), ScriptValue(value.a));
        }

        static void Read(Color4& value, const ScriptValue& data)
        {
            value.r = data.GetProperty(ScriptValuePropertyKeys::GetR()).GetValue<int>();
            value.g = data.GetProperty(ScriptValuePropertyKeys::GetG()).GetValue<int>();
            value.b = data.GetProperty(ScriptValuePropertyKeys::GetB()).GetValue<int>();
            value.a = data.GetProperty(ScriptValuePropertyKeys::GetA()).GetValue<int>();
        }
    };

    template<typename _ptr_type>
    struct ScriptValue::Converter<_ptr_type, typename std::enable_if<std::is_pointer<_ptr_type>::value && !std::is_const<_ptr_type>::value &&
        std::is_base_of<o2::IObject, typename std::remove_pointer<_ptr_type>::type>::value>::type>
    {
        static constexpr bool isSupported = true;

        typedef typename std::remove_const<typename std::remove_pointer<typename std::remove_reference<_ptr_type>::type>::type>::type _non_ptr_type;

        static void Write(const _ptr_type& value, ScriptValue& data)
        {
            data.mValue = JS_UNDEFINED;
            if (value)
            {
                data = ScriptValue::EmptyObject();
                data.SetContainingObject(const_cast<_non_ptr_type*>(value));
            }
        }

        static void Read(_ptr_type& value, const ScriptValue& data)
        {
            auto dataContainer = GetNativeContainer(data.mValue);
            if (dataContainer)
            {
                auto object = dataContainer->TryCastToIObject();
                value = dynamic_cast<_ptr_type>(object);
            }
            else
                value = nullptr;
        }
    };

    template<typename T>
    struct ScriptValue::Converter<Ref<T>, void_t<decltype(std::declval<T*>()->GetScriptValue())>>
    {
        static constexpr bool isSupported = true;

        static void Write(const Ref<T>& value, ScriptValue& data)
        {
            data.mValue = JS_UNDEFINED;
            if (value)
                data = value->GetScriptValue();
        }

        static void Read(Ref<T>& value, const ScriptValue& data)
        {
            auto dataContainer = GetNativeContainer(data.mValue);
            if (dataContainer)
            {
                auto* object = dataContainer->TryCastToIObject();
                T* typed = dynamic_cast<T*>(object);
                value = typed ? Ref<T>(typed) : Ref<T>();
            }
            else
                value = Ref<T>();
        }
    };

    template<typename T>
    struct ScriptValue::Converter<Vector<T>>
    {
        static constexpr bool isSupported = true;

        static void Write(const Vector<T>& value, ScriptValue& data)
        {
            data.mValue = JS_NewArray(QuickJs::Ctx());

            for (auto& v : value)
                data.AddElement(ScriptValue(v));
        }

        static void Read(Vector<T>& value, const ScriptValue& data)
        {
            if (data.GetValueType() == ValueType::Array)
            {
                value.Clear();
                for (int i = 0; i < data.GetLength(); i++)
                    value.Add(data[i].GetValue<T>());
            }
        }
    };

    template<typename _key, typename _value>
    struct ScriptValue::Converter<Map<_key, _value>>
    {
        static constexpr bool isSupported = true;

        static void Write(const Map<_key, _value>& value, ScriptValue& data)
        {
            data.mValue = JS_NewObject(QuickJs::Ctx());

            for (auto& kv : value)
                data.SetProperty(ScriptValue(kv.first), ScriptValue(kv.second));
        }

        static void Read(Map<_key, _value>& value, const ScriptValue& data)
        {
            if (data.GetValueType() == ValueType::Object)
            {
                value.Clear();
                data.ForEachProperties([&](const ScriptValue& propName, const ScriptValue& propValue) {
                    value[propName.GetValue<_key>()] = propValue.GetValue<_value>();
                    return true;
                });
            }
        }
    };

    template<typename T>
    struct ScriptValue::Converter<T, typename std::enable_if<std::is_enum<T>::value>::type>
    {
        static constexpr bool isSupported = true;

        static void Write(const T& value, ScriptValue& data)
        {
            data.mValue = JS_UNDEFINED;
            data.SetValue(Reflection::GetEnumName<T>(value));
        }

        static void Read(T& value, const ScriptValue& data)
        {
            if (data.GetValueType() == ValueType::Number)
                value = (T)data.GetValue<int>();
            else
                value = Reflection::GetEnumValue<T>(data.GetValue<String>());
        }
    };

    template<typename T>
    struct ScriptValue::Converter<T, typename std::enable_if<IsProperty<T>::value>::type>
    {
        static constexpr bool isSupported = ScriptValue::Converter<typename T::valueType>::isSupported;
        using TValueType = typename T::valueType;

        static void Write(const T& value, ScriptValue& data)
        {
            data.mValue = JS_UNDEFINED;
            data.SetValue(value.Get());
        }

        static void Read(T& value, const ScriptValue& data)
        {
            value.Set(data.GetValue<TValueType>());
        }
    };

    template<typename _res_type, typename ... _args>
    struct ScriptValue::Converter<Function<_res_type(_args ...)>>
    {
        static constexpr bool isSupported = true;

        static void Write(const Function<_res_type(_args ...)>& value, ScriptValue& data)
        {
            data.mValue = QuickJs::NewExternalFunction(&CallFunction);

            IDataContainer* funcContainer = ScriptValueBase::CreateContainer<ScriptFunctionContainer<Function<_res_type(_args ...)>, _res_type, _args ...>>(value);

            QuickJs::SetNativePointer(data.mValue, funcContainer, &FreeDataContainer);
        }

        static void Read(Function<_res_type(_args ...)>& value, const ScriptValue& data)
        {
            value = [dataCopy = data](_args ... args)
            {
                return dataCopy.Invoke<_res_type, _args ...>(args ...);
            };
        }
    };
}

#endif // SCRIPTING_BACKEND_QUICKJS
