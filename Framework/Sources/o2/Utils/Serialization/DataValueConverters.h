#pragma once

#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    template<>
    struct DataValue::Converter<std::nullptr_t>
    {
        static constexpr bool isSupported = false;
    };

    template<>
    struct DataValue::Converter<char*>
    {
        static constexpr bool isSupported = true;

        using charPtr = char*;

        static void Write(const charPtr& value, DataValue& data)
        {
            data.SetString(value, (int)strlen(value), true);
        }

        static void Read(charPtr& value, const DataValue& data)
        {
            Assert(data.mData.flagsData.Is(DataValue::Flags::String), "Trying get string from not string value");

            if (data.mData.flagsData.Is(DataValue::Flags::ShortString))
            {
                strcpy(value, data.mData.shortStringData.stringValue);
            }
            else
            {
                auto length = data.mData.stringPtrData.stringLength;
                memcpy(value, data.mData.stringPtrData.stringPtr, sizeof(char) * length);
                value[length] = '\0';
            }
        }
    };

    template<UInt _size>
    struct DataValue::Converter<char[_size]>
    {
        static constexpr bool isSupported = true;

        using charPtr = char[_size];

        static void Write(const charPtr& value, DataValue& data)
        {
            data.SetString(value, _size, true);
        }

        static void Read(charPtr& value, const DataValue& data)
        {
        }
    };

    template<>
    struct DataValue::Converter<const char*>
    {
        static constexpr bool isSupported = true;

        using charPtr = const char*;

        static void Write(const charPtr& value, DataValue& data)
        {
            data.SetString(value, (int)strlen(value), true);
        }

        static void Read(charPtr& value, const DataValue& data)
        {
        }
    };

    template<>
    struct DataValue::Converter<wchar_t*>
    {
        static constexpr bool isSupported = true;

        using wcharPtr = wchar_t*;

        static void Write(const wcharPtr& value, DataValue& data)
        {
            rapidjson::GenericStringBuffer<rapidjson::UTF8<>> target;
            if (Transcode(target, value))
                data.Set(target.GetString());
        }

        static void Read(wcharPtr value, const DataValue& data)
        {
            rapidjson::GenericStringBuffer<rapidjson::UTF16<>> target;
            if (Transcode(target, data.GetString()))
                wcscpy(value, target.GetString());
        }
    };

    template<>
    struct DataValue::Converter<const wchar_t*>
    {
        static constexpr bool isSupported = true;

        using wcharPtr = const wchar_t*;

        static void Write(const wcharPtr& value, DataValue& data)
        {
            rapidjson::GenericStringBuffer<rapidjson::UTF8<>> target;
            if (Transcode(target, value))
                data.Set(target.GetString());
        }

        static void Read(wcharPtr value, const DataValue& data)
        {
        }
    };

    template<>
    struct DataValue::Converter<bool>
    {
        static constexpr bool isSupported = true;

        static void Write(const bool& value, DataValue& data)
        {
            if (value)
                data.mData.flagsData.flags = DataValue::Flags::Bool | DataValue::Flags::BoolTrue;
            else
                data.mData.flagsData.flags = DataValue::Flags::Bool | DataValue::Flags::BoolFalse;
        }

        static void Read(bool& value, const DataValue& data)
        {
            value = data.mData.flagsData.Is(DataValue::Flags::Bool) && data.mData.flagsData.Is(DataValue::Flags::BoolTrue);
        }
    };

    template<>
    struct DataValue::Converter<int>
    {
        static constexpr bool isSupported = true;

        static void Write(const int& value, DataValue& data)
        {
            data.mData.flagsData.flags = Flags::Int | Flags::Value;
            data.mData.intData.intValue = value;
        }

        static void Read(int& value, const DataValue& data)
        {
            if (data.mData.flagsData.Is(Flags::Int))
                value = data.mData.intData.intValue;
            else if (data.mData.flagsData.Is(Flags::UInt))
                value = (int)data.mData.intData.uintValue;
            else if (data.mData.flagsData.Is(Flags::Int64))
                value = (int)data.mData.int64Data.intValue;
            else if (data.mData.flagsData.Is(Flags::UInt64))
                value = (int)data.mData.int64Data.uintValue;
            else if (data.mData.flagsData.Is(Flags::Double))
                value = (int)data.mData.doubleData.value;
            else
                Assert(false, "Trying to get int from not number value");
        }
    };

    template<>
    struct DataValue::Converter<UInt>
    {
        static constexpr bool isSupported = true;

        static void Write(const UInt& value, DataValue& data)
        {
            data.mData.flagsData.flags = Flags::UInt | Flags::Value;
            data.mData.intData.uintValue = value;
        }

        static void Read(UInt& value, const DataValue& data)
        {
            if (data.mData.flagsData.Is(Flags::UInt))
                value = data.mData.intData.uintValue;
            else if (data.mData.flagsData.Is(Flags::Int))
                value = (UInt)data.mData.intData.intValue;
            else if (data.mData.flagsData.Is(Flags::Int64))
                value = (UInt)data.mData.int64Data.intValue;
            else if (data.mData.flagsData.Is(Flags::UInt64))
                value = (UInt)data.mData.int64Data.uintValue;
            else if (data.mData.flagsData.Is(Flags::Double))
                value = (UInt)data.mData.doubleData.value;
            else
                Assert(false, "Trying to get unsigned int from not number value");
        }
    };

    template<>
    struct DataValue::Converter<Int64>
    {
        static constexpr bool isSupported = true;

        static void Write(const Int64& value, DataValue& data)
        {
            data.mData.flagsData.flags = Flags::Int64 | Flags::Value;
            data.mData.int64Data.intValue = value;
        }

        static void Read(Int64& value, const DataValue& data)
        {
            if (data.mData.flagsData.Is(Flags::Int64))
                value = data.mData.int64Data.intValue;
            else if (data.mData.flagsData.Is(Flags::UInt64))
                value = (Int64)data.mData.int64Data.uintValue;
            else if (data.mData.flagsData.Is(Flags::Int))
                value = (Int64)data.mData.intData.intValue;
            else if (data.mData.flagsData.Is(Flags::UInt))
                value = (Int64)data.mData.intData.uintValue;
            else if (data.mData.flagsData.Is(Flags::Double))
                value = (Int64)data.mData.doubleData.value;
            else
                Assert(false, "Trying to get int64 from not number value");
        }
    };

    template<>
    struct DataValue::Converter<UInt64>
    {
        static constexpr bool isSupported = true;

        static void Write(const UInt64& value, DataValue& data)
        {
            data.mData.flagsData.flags = Flags::UInt64 | Flags::Value;
            data.mData.int64Data.uintValue = value;
        }

        static void Read(UInt64& value, const DataValue& data)
        {
            if (data.mData.flagsData.Is(Flags::UInt64))
                value = data.mData.int64Data.uintValue;
            else if (data.mData.flagsData.Is(Flags::Int64))
                value = (UInt64)data.mData.int64Data.intValue;
            else if (data.mData.flagsData.Is(Flags::Int))
                value = (UInt64)data.mData.intData.intValue;
            else if (data.mData.flagsData.Is(Flags::UInt))
                value = (UInt64)data.mData.intData.uintValue;
            else if (data.mData.flagsData.Is(Flags::Double))
                value = (UInt64)data.mData.doubleData.value;
            else
                Assert(false, "Trying to get unsigned int64 from not number value");
        }
    };

    template<>
    struct DataValue::Converter<double>
    {
        static constexpr bool isSupported = true;

        static void Write(const double& value, DataValue& data)
        {
            data.mData.flagsData.flags = Flags::Double | Flags::Value;
            data.mData.doubleData.value = value;
        }

        static void Read(double& value, const DataValue& data)
        {
            if (data.mData.flagsData.Is(Flags::Double))
                value = data.mData.doubleData.value;
            else if (data.mData.flagsData.Is(Flags::Int))
                value = (double)data.mData.intData.intValue;
            else if (data.mData.flagsData.Is(Flags::UInt))
                value = (double)data.mData.intData.uintValue;
            else if (data.mData.flagsData.Is(Flags::Int64))
                value = (double)data.mData.int64Data.intValue;
            else if (data.mData.flagsData.Is(Flags::UInt64))
                value = (double)data.mData.int64Data.uintValue;
        }
    };

    template<>
    struct DataValue::Converter<float>
    {
        static constexpr bool isSupported = true;

        static void Write(const float& value, DataValue& data)
        {
            data.mData.flagsData.flags = Flags::Double | Flags::Value;
            data.mData.doubleData.value = (double)value;
        }

        static void Read(float& value, const DataValue& data)
        {
            if (data.mData.flagsData.Is(Flags::Double))
                value = (float)data.mData.doubleData.value;
            else if (data.mData.flagsData.Is(Flags::Int))
                value = (float)data.mData.intData.intValue;
            else if (data.mData.flagsData.Is(Flags::UInt))
                value = (float)data.mData.intData.uintValue;
            else if (data.mData.flagsData.Is(Flags::Int64))
                value = (float)data.mData.int64Data.intValue;
            else if (data.mData.flagsData.Is(Flags::UInt64))
                value = (float)data.mData.int64Data.uintValue;
            else
                Assert(false, "Trying to get float from not number value");
        }
    };

    template<>
    struct DataValue::Converter<String>
    {
        static constexpr bool isSupported = true;

        static void Write(const String& value, DataValue& data)
        {
            data.SetString(value.Data(), value.Length(), true);
        }

        static void Read(String& value, const DataValue& data)
        {
            Assert(data.mData.flagsData.Is(DataValue::Flags::String), "Trying get string from not string value");

            if (data.mData.flagsData.Is(DataValue::Flags::ShortString))
                value = data.mData.shortStringData.stringValue;
            else
                value = data.mData.stringPtrData.stringPtr;
        }
    };

    template<>
    struct DataValue::Converter<WString>
    {
        static constexpr bool isSupported = true;

        static void Write(const WString& value, DataValue& data)
        {
            data.Set(value.Data());
        }

        static void Read(WString& value, const DataValue& data)
        {
            rapidjson::GenericStringBuffer<rapidjson::UTF16<>> target;
            if (Transcode(target, data.GetString()))
                value = target.GetString();
        }
    };

    template<>
    struct DataValue::Converter<UID>
    {
        static constexpr bool isSupported = true;

        static void Write(const UID& value, DataValue& data)
        {
            data.Set((WString)value);
        }

        static void Read(UID& value, const DataValue& data)
        {
            WString buf;
            data.Get(buf);
            value = buf;
        }
    };

    template<>
    struct DataValue::Converter<Vec2F>
    {
        static constexpr bool isSupported = true;

        static void Write(const Vec2F& value, DataValue& data)
        {
            data.AddMember("x") = value.x;
            data.AddMember("y") = value.y;
        }

        static void Read(Vec2F& value, const DataValue& data)
        {
            value.x = data.GetMember("x");
            value.y = data.GetMember("y");
        }
    };

    template<>
    struct DataValue::Converter<Vec2I>
    {
        static constexpr bool isSupported = true;

        static void Write(const Vec2I& value, DataValue& data)
        {
            data.AddMember("x") = value.x;
            data.AddMember("y") = value.y;
        }

        static void Read(Vec2I& value, const DataValue& data)
        {
            value.x = data.GetMember("x");
            value.y = data.GetMember("y");
        }
    };

    template<>
    struct DataValue::Converter<RectF>
    {
        static constexpr bool isSupported = true;

        static void Write(const RectF& value, DataValue& data)
        {
            data.AddMember("left") = value.left;
            data.AddMember("bottom") = value.bottom;
            data.AddMember("right") = value.right;
            data.AddMember("top") = value.top;
        }

        static void Read(RectF& value, const DataValue& data)
        {
            value.left = data.GetMember("left");
            value.bottom = data.GetMember("bottom");
            value.right = data.GetMember("right");
            value.top = data.GetMember("top");
        }
    };

    template<>
    struct DataValue::Converter<RectI>
    {
        static constexpr bool isSupported = true;

        static void Write(const RectI& value, DataValue& data)
        {
            data.AddMember("left") = value.left;
            data.AddMember("bottom") = value.bottom;
            data.AddMember("right") = value.right;
            data.AddMember("top") = value.top;
        }

        static void Read(RectI& value, const DataValue& data)
        {
            value.left = data.GetMember("left");
            value.bottom = data.GetMember("bottom");
            value.right = data.GetMember("right");
            value.top = data.GetMember("top");
        }
    };

    template<>
    struct DataValue::Converter<BorderF>
    {
        static constexpr bool isSupported = true;

        static void Write(const BorderF& value, DataValue& data)
        {
            data.AddMember("left") = value.left;
            data.AddMember("bottom") = value.bottom;
            data.AddMember("right") = value.right;
            data.AddMember("top") = value.top;
        }

        static void Read(BorderF& value, const DataValue& data)
        {
            value.left = data.GetMember("left");
            value.bottom = data.GetMember("bottom");
            value.right = data.GetMember("right");
            value.top = data.GetMember("top");
        }
    };

    template<>
    struct DataValue::Converter<BorderI>
    {
        static constexpr bool isSupported = true;

        static void Write(const BorderI& value, DataValue& data)
        {
            data.AddMember("left") = value.left;
            data.AddMember("bottom") = value.bottom;
            data.AddMember("right") = value.right;
            data.AddMember("top") = value.top;
        }

        static void Read(BorderI& value, const DataValue& data)
        {
            value.left = data.GetMember("left");
            value.bottom = data.GetMember("bottom");
            value.right = data.GetMember("right");
            value.top = data.GetMember("top");
        }
    };

    template<>
    struct DataValue::Converter<Color4>
    {
        static constexpr bool isSupported = true;

        static void Write(const Color4& value, DataValue& data)
        {
            data.AddMember("r") = value.r;
            data.AddMember("g") = value.g;
            data.AddMember("b") = value.b;
            data.AddMember("a") = value.a;
        }

        static void Read(Color4& value, const DataValue& data)
        {
            value.r = data.GetMember("r");
            value.g = data.GetMember("g");
            value.b = data.GetMember("b");
            value.a = data.GetMember("a");
        }
    };

    template<>
    struct DataValue::Converter<Vec3F>
    {
        static constexpr bool isSupported = true;

        static void Write(const Vec3F& value, DataValue& data)
        {
            data.AddMember("x") = value.x;
            data.AddMember("y") = value.y;
            data.AddMember("z") = value.z;
        }

        static void Read(Vec3F& value, const DataValue& data)
        {
            value.x = data.GetMember("x");
            value.y = data.GetMember("y");
            value.z = data.GetMember("z");
        }
    };

    template<>
    struct DataValue::Converter<Vec3I>
    {
        static constexpr bool isSupported = true;

        static void Write(const Vec3I& value, DataValue& data)
        {
            data.AddMember("x") = value.x;
            data.AddMember("y") = value.y;
            data.AddMember("z") = value.z;
        }

        static void Read(Vec3I& value, const DataValue& data)
        {
            value.x = data.GetMember("x");
            value.y = data.GetMember("y");
            value.z = data.GetMember("z");
        }
    };

    template<>
    struct DataValue::Converter<Quat>
    {
        static constexpr bool isSupported = true;

        static void Write(const Quat& value, DataValue& data)
        {
            data.AddMember("x") = value.x;
            data.AddMember("y") = value.y;
            data.AddMember("z") = value.z;
            data.AddMember("w") = value.w;
        }

        static void Read(Quat& value, const DataValue& data)
        {
            value.x = data.GetMember("x");
            value.y = data.GetMember("y");
            value.z = data.GetMember("z");
            value.w = data.GetMember("w");
        }
    };

    template<>
    struct DataValue::Converter<Mat4>
    {
        static constexpr bool isSupported = true;

        static void Write(const Mat4& value, DataValue& data)
        {
            data.mData.flagsData.flags = Flags::Array;
            data.mData.arrayData.elements = nullptr;
            data.mData.arrayData.count = 0;
            data.mData.arrayData.capacity = 0;

            for (int i = 0; i < 16; i++)
                data.AddElement() = DataValue(value.m[i], *data.mDocument);
        }

        static void Read(Mat4& value, const DataValue& data)
        {
            if (data.IsArray())
            {
                int count = Math::Min(16, data.GetElementsCount());
                for (int i = 0; i < count; i++)
                    data.GetElement(i).Get(value.m[i]);
            }
        }
    };

    template<>
    struct DataValue::Converter<AABB>
    {
        static constexpr bool isSupported = true;

        static void Write(const AABB& value, DataValue& data)
        {
            data.AddMember("min").Set(value.min);
            data.AddMember("max").Set(value.max);
        }

        static void Read(AABB& value, const DataValue& data)
        {
            data.GetMember("min").Get(value.min);
            data.GetMember("max").Get(value.max);
        }
    };

    template<>
    struct DataValue::Converter<Basis3D>
    {
        static constexpr bool isSupported = true;

        static void Write(const Basis3D& value, DataValue& data)
        {
            data.AddMember("xv").Set(value.xv);
            data.AddMember("yv").Set(value.yv);
            data.AddMember("zv").Set(value.zv);
            data.AddMember("origin").Set(value.origin);
        }

        static void Read(Basis3D& value, const DataValue& data)
        {
            data.GetMember("xv").Get(value.xv);
            data.GetMember("yv").Get(value.yv);
            data.GetMember("zv").Get(value.zv);
            data.GetMember("origin").Get(value.origin);
        }
    };

    template<typename TKey, typename TValue>
    struct DataValue::Converter<Pair<TKey, TValue>>
    {
        static constexpr bool isSupported = true;

        static void Write(const Pair<TKey, TValue>& value, DataValue& data)
        {
            data.AddMember("f") = value.first;
            data.AddMember("s") = value.second;
        }

        static void Read(Pair<TKey, TValue>& value, const DataValue& data)
        {
            data.GetMember("f").Get(value.first);
            data.GetMember("s").Get(value.second);
        }
    };

    // ---------------------
    // Ref<Actor> types converter
    // ---------------------
    template<typename T>
    struct DataValue::Converter<T, typename std::enable_if<IsRef<T>::value && !std::is_const<T>::value &&
        !std::is_base_of<ISerializable, T>::value&&
        std::is_base_of<o2::IObject, typename ExtractRefType<T>::type>::value>::type>
    {
        static constexpr bool isSupported = true;

        using _ref_type = typename ExtractRefType<T>::type;

        static void Write(const T& value, DataValue& data)
        {
            if (value)
            {
                data.AddMember("Type").Set(value->GetType().GetName());
                data.AddMember("Value").Set(*value);
            }
        }

        static void Read(T& value, const DataValue& data)
        {
            if (auto typeNode = data.FindMember("Type"))
            {
                if (auto valueNode = data.FindMember("Value"))
                {
                    String typeName = *typeNode;
                    auto type = Reflection::GetType(typeName);
                    if (!type)
                    {
                        printf("Failed to deserialize unknown type: %s\n", typeName.Data());
                        return;
                    }

                    auto sample = type->CreateSampleRef();
                    value = DynamicCast<_ref_type>(sample);

                    if (value)
                        valueNode->Get(*value);
                }
            }
            else
                value = nullptr;
        }
    };

    // ----------------------
    // Raw pointers converter
    // ----------------------
    template<typename T>
    struct DataValue::Converter<T, typename std::enable_if<std::is_pointer<T>::value && !std::is_const<T>::value &&
        std::is_base_of<o2::IObject, typename std::remove_pointer<T>::type>::value && !std::is_same<T, std::nullptr_t>::value>::type>
    {
        static constexpr bool isSupported = true;

        static void Write(const T& value, DataValue& data)
        {
            if (value)
            {
                data.AddMember("Type").Set(value->GetType().GetName());
                if (auto serializable = dynamic_cast<const ISerializable*>(value))
                    data.AddMember("Value").Set(*serializable);
                else
                    data.AddMember("Value").Set(*value);
            }
        }

        static void Read(T& value, const DataValue& data)
        {
            if (auto typeNode = data.FindMember("Type"))
            {
                if (auto valueNode = data.FindMember("Value"))
                {
                    if (value)
                        delete value;

                    String typeName = *typeNode;
                    auto type = Reflection::GetType( typeName );
                    if (!type)
                    {
                        printf("Failed to deserialize unknown type: %s\n", typeName.Data());
                        return;
                    }

                    void* sample = type->CreateSample();
                    if (type->GetUsage() == Type::Usage::Object)
                    {
                        auto objectType = dynamic_cast<const ObjectType*>(type);
                        value = dynamic_cast<T>(objectType->DynamicCastToIObject(sample));
                    }
                    else
                        value = static_cast<T>(sample);

                    if (auto serializable = dynamic_cast<ISerializable*>(value))
                        valueNode->Get(*serializable);
                    else if (value)
                        valueNode->Get(*value);
                }
            }
        }
    };

    template<typename T>
    struct DataValue::Converter<Vector<T>>
    {
        static constexpr bool isSupported = true;

        static void Write(const Vector<T>& value, DataValue& data)
        {
            data.mData.flagsData.flags = Flags::Array;
            data.mData.arrayData.elements = nullptr;
            data.mData.arrayData.count = 0;
            data.mData.arrayData.capacity = 0;

            for (auto& v : value)
                data.AddElement() = DataValue(v, *data.mDocument);
        }

        static void Read(Vector<T>& value, const DataValue& data)
        {
            if (data.IsArray())
            {
                value.Clear();
                for (auto& element : data)
                {
                    T v = T();
                    element.Get(v);
                    value.Add(v);
                }
            }
        }
    };

    template<typename _key, typename _value>
    struct DataValue::Converter<Map<_key, _value>>
    {
        static constexpr bool isSupported = true;

        static void Write(const Map<_key, _value>& value, DataValue& data)
        {
            data.mData.flagsData.flags = Flags::Array;
            data.mData.arrayData.elements = nullptr;
            data.mData.arrayData.count = 0;
            data.mData.arrayData.capacity = 0;

            for (auto& kv : value)
            {
                DataValue& child = data.AddElement();
                child.AddMember("Key").Set(kv.first);
                child.AddMember("Value").Set(kv.second);
            }
        }

        static void Read(Map<_key, _value>& value, const DataValue& data)
        {
            if (data.IsArray())
            {
                value.Clear();
                for (auto& childNode : data)
                {
                    auto keyNode = childNode.FindMember("Key");
                    auto valueNode = childNode.FindMember("Value");

                    if (keyNode && valueNode)
                    {
                        _value v = _value();
                        _key k = _key();
                        keyNode->Get(k);
                        valueNode->Get(v);
                        value.Add(k, v);
                    }
                }
            }
        }
    };

    // ---------------
    // Enums converter
    // ---------------
    template<typename T>
    struct DataValue::Converter<T, std::enable_if_t<std::is_enum_v<T>>>
    {
        static constexpr bool isSupported = true;

        static void Write(const T& value, DataValue& data)
        {
            data.Set(Reflection::GetEnumName<T>(value));
        }

        static void Read(T& value, const DataValue& data)
        {
            String buf;
            data.Get(buf);
            value = Reflection::GetEnumValue<T>(buf);
        }
    };

    template<typename T>
    struct DataValue::Converter<T, std::enable_if_t<IsProperty<T>::value>>
    {
        static constexpr bool isSupported = DataValue::IsSupports<typename T::valueType>::value;
        using TValueType = typename T::valueType;

        static void Write(const T& value, DataValue& data)
        {
            data.Set(value.Get());
        }

        static void Read(T& value, const DataValue& data)
        {
            TValueType val;
            data.Get(val);
            value.Set(val);
        }
    };

    template<typename _type, typename _enable /*= void*/>
    void DataValue::DeltaConverter<_type, _enable>::Write(const __type& value, const __type& origin, DataValue& data)
    {
        if (!EqualsForDeltaSerialize(value, origin))
            data.Set(value);
    }

    template<typename _type, typename _enable /*= void*/>
    void DataValue::DeltaConverter<_type, _enable>::Read(__type& value, const __type& origin, const DataValue& data)
    {
        data.Get(value);
    }

    template<typename T>
    struct DataValue::DeltaConverter<T, std::enable_if_t<std::is_pointer_v<T> && !std::is_const_v<T> &&
                                                         std::is_base_of_v<o2::IObject, std::remove_pointer_t<T>>>>
    {
        static constexpr bool isSupported = true;

        static void Write(const T& value, const T& origin, DataValue& data)
        {
            if (value)
            {
                if (!origin)
                    DataValue::Converter<T>::Write(value, data);
                else
                {
                    if (!EqualsForDeltaSerialize(*value, *origin))
                        data.SetDelta(*value, *origin);
                }
            }
        }

        static void Read(T& value, const T& origin, const DataValue& data)
        {
            if (!origin)
                DataValue::Converter<T>::Read(value, data);
            else
            {
                value = origin->template CloneAs<typename std::remove_pointer<T>::type>();
                data.GetDelta(*value, *origin);
            }
        }
    };

    template<typename T>
    struct DataValue::DeltaConverter<Vector<T>>
    {
        static constexpr bool isSupported = true;

        static void Write(const Vector<T>& value, const Vector<T>& origin, DataValue& data)
        {
            data.mData.flagsData.flags = Flags::Array;
            data.mData.arrayData.elements = nullptr;
            data.mData.arrayData.count = 0;
            data.mData.arrayData.capacity = 0;

            for (int i = 0; i < value.Count(); i++)
            {
                if (i < origin.Count())
                {
                    if (EqualsForDeltaSerialize(value[i], origin[i]))
                        data.AddElement();
                    else
                        data.AddElement().SetDelta(value[i], origin[i]);
                }
                else
                    data.AddElement().SetDelta(value[i], origin[i]);
            }
        }

        static void Read(Vector<T>& value, const Vector<T>& origin, const DataValue& data)
        {
            if (data.IsArray())
            {
                value.Clear();
                int i = 0;
                for (auto& element : data)
                {
                    T v = T();
                    if (element.IsNull() && i < origin.Count())
                        v = origin[i];
                    else
                        element.GetDelta(v, origin[i]);

                    value.Add(v);

                    i++;
                }
            }
        }
    };
}
