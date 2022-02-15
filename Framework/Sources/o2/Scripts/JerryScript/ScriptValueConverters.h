#pragma once

#if defined(SCRIPTING_BACKEND_JERRYSCRIPT)

#include "o2/Utils/Function.h"

namespace o2
{
	template<typename _type, typename _enable /*= void*/>
	void ScriptValue::Converter<_type, _enable>::Read(__type& value, const ScriptValue& data)
	{}

	template<typename _type, typename _enable /*= void*/>
	void ScriptValue::Converter<_type, _enable>::Write(const __type& value, ScriptValue& data)
	{
		data.jvalue = jerry_create_undefined();
	}

	template<>
	struct ScriptValue::Converter<char*>
	{
		static constexpr bool isSupported = true;

		using charPtr = char*;

		static void Write(const charPtr& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_string((jerry_char_t*)(value));
		}

		static void Read(charPtr& value, const ScriptValue& data)
		{
			jerry_string_to_char_buffer(data.jvalue, (jerry_char_t*)value, 256);
		}
	};

	template<UInt _size>
	struct ScriptValue::Converter<char[_size]>
	{
		static constexpr bool isSupported = true;

		using charPtr = char[_size];

		static void Write(const charPtr& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_string((jerry_char_t*)(value));
		}

		static void Read(charPtr& value, const ScriptValue& data)
		{
			jerry_string_to_char_buffer(data.jvalue, (jerry_char_t*)value, _size);
		}
	};

	template<>
	struct ScriptValue::Converter<const char*>
	{
		static constexpr bool isSupported = true;

		using charPtr = const char*;

		static void Write(const charPtr& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_string((jerry_char_t*)(value));
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
			data.jvalue = jerry_create_boolean(value);
		}

		static void Read(bool& value, const ScriptValue& data)
		{
			value = jerry_value_to_boolean(data.jvalue);
		}
	};

	template<>
	struct ScriptValue::Converter<int>
	{
		static constexpr bool isSupported = true;

		static void Write(const int& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_number((double)value);
		}

		static void Read(int& value, const ScriptValue& data)
		{
			value = (int)jerry_value_as_integer(data.jvalue);
		}
	};

	template<>
	struct ScriptValue::Converter<UInt>
	{
		static constexpr bool isSupported = true;

		static void Write(const UInt& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_number((double)value);
		}

		static void Read(UInt& value, const ScriptValue& data)
		{
			value = (UInt)jerry_value_as_integer(data.jvalue);
		}
	};

	template<>
	struct ScriptValue::Converter<Int64>
	{
		static constexpr bool isSupported = true;

		static void Write(const Int64& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_number((double)value);
		}

		static void Read(Int64& value, const ScriptValue& data)
		{
			value = (Int64)jerry_value_as_integer(data.jvalue);
		}
	};

	template<>
	struct ScriptValue::Converter<UInt64>
	{
		static constexpr bool isSupported = true;

		static void Write(const UInt64& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_number((double)value);
		}

		static void Read(UInt64& value, const ScriptValue& data)
		{
			value = (Int64)jerry_value_as_integer(data.jvalue);
		}
	};

	template<>
	struct ScriptValue::Converter<double>
	{
		static constexpr bool isSupported = true;

		static void Write(const double& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_number(value);
		}

		static void Read(double& value, const ScriptValue& data)
		{
			value = jerry_get_number_value(data.jvalue);
		}
	};

	template<>
	struct ScriptValue::Converter<float>
	{
		static constexpr bool isSupported = true;

		static void Write(const float& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_number((double)value);
		}

		static void Read(float& value, const ScriptValue& data)
		{
			value = (float)jerry_get_number_value(data.jvalue);
		}
	};

	template<>
	struct ScriptValue::Converter<String>
	{
		static constexpr bool isSupported = true;

		static void Write(const String& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_string((jerry_char_t*)value.Data());
		}

		static void Read(String& value, const ScriptValue& data)
		{
			value.resize(jerry_get_string_length(data.jvalue) + 1);
			jerry_string_to_char_buffer(data.jvalue, (jerry_char_t*)value.Data(), value.Capacity());
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
			data.jvalue = jerry_create_object();
			data.SetProperty(ScriptValue("x"), ScriptValue(value.x));
			data.SetProperty(ScriptValue("y"), ScriptValue(value.y));
		}

		static void Read(Vec2F& value, const ScriptValue& data)
		{
			value.x = data.GetProperty(ScriptValue("x"));
			value.y = data.GetProperty(ScriptValue("y"));
		}
	};

	template<>
	struct ScriptValue::Converter<Vec2I>
	{
		static constexpr bool isSupported = true;

		static void Write(const Vec2I& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_object();
			data.SetProperty(ScriptValue("x"), ScriptValue(value.x));
			data.SetProperty(ScriptValue("y"), ScriptValue(value.y));
		}

		static void Read(Vec2I& value, const ScriptValue& data)
		{
			value.x = data.GetProperty(ScriptValue("x"));
			value.y = data.GetProperty(ScriptValue("y"));
		}
	};

	template<>
	struct ScriptValue::Converter<RectF>
	{
		static constexpr bool isSupported = true;

		static void Write(const RectF& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_object();
			data.SetProperty(ScriptValue("left"), ScriptValue(value.left));
			data.SetProperty(ScriptValue("bottom"), ScriptValue(value.bottom));
			data.SetProperty(ScriptValue("right"), ScriptValue(value.right));
			data.SetProperty(ScriptValue("top"), ScriptValue(value.top));
		}

		static void Read(RectF& value, const ScriptValue& data)
		{
			value.left = data.GetProperty(ScriptValue("left"));
			value.bottom = data.GetProperty(ScriptValue("bottom"));
			value.right = data.GetProperty(ScriptValue("right"));
			value.top = data.GetProperty(ScriptValue("top"));
		}
	};

	template<>
	struct ScriptValue::Converter<RectI>
	{
		static constexpr bool isSupported = true;

		static void Write(const RectI& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_object();
			data.SetProperty(ScriptValue("left"), ScriptValue(value.left));
			data.SetProperty(ScriptValue("bottom"), ScriptValue(value.bottom));
			data.SetProperty(ScriptValue("right"), ScriptValue(value.right));
			data.SetProperty(ScriptValue("top"), ScriptValue(value.top));
		}

		static void Read(RectI& value, const ScriptValue& data)
		{
			value.left = data.GetProperty(ScriptValue("left"));
			value.bottom = data.GetProperty(ScriptValue("bottom"));
			value.right = data.GetProperty(ScriptValue("right"));
			value.top = data.GetProperty(ScriptValue("top"));
		}
	};

	template<>
	struct ScriptValue::Converter<BorderF>
	{
		static constexpr bool isSupported = true;

		static void Write(const BorderF& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_object();
			data.SetProperty(ScriptValue("left"), ScriptValue(value.left));
			data.SetProperty(ScriptValue("bottom"), ScriptValue(value.bottom));
			data.SetProperty(ScriptValue("right"), ScriptValue(value.right));
			data.SetProperty(ScriptValue("top"), ScriptValue(value.top));
		}

		static void Read(BorderF& value, const ScriptValue& data)
		{
			value.left = data.GetProperty(ScriptValue("left"));
			value.bottom = data.GetProperty(ScriptValue("bottom"));
			value.right = data.GetProperty(ScriptValue("right"));
			value.top = data.GetProperty(ScriptValue("top"));
		}
	};

	template<>
	struct ScriptValue::Converter<BorderI>
	{
		static constexpr bool isSupported = true;

		static void Write(const BorderI& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_object();
			data.SetProperty(ScriptValue("left"), ScriptValue(value.left));
			data.SetProperty(ScriptValue("bottom"), ScriptValue(value.bottom));
			data.SetProperty(ScriptValue("right"), ScriptValue(value.right));
			data.SetProperty(ScriptValue("top"), ScriptValue(value.top));
		}

		static void Read(BorderI& value, const ScriptValue& data)
		{
			value.left = data.GetProperty(ScriptValue("left"));
			value.bottom = data.GetProperty(ScriptValue("bottom"));
			value.right = data.GetProperty(ScriptValue("right"));
			value.top = data.GetProperty(ScriptValue("top"));
		}
	};

	template<>
	struct ScriptValue::Converter<Color4>
	{
		static constexpr bool isSupported = true;

		static void Write(const Color4& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_object();
			data.SetProperty(ScriptValue("r"), ScriptValue(value.r));
			data.SetProperty(ScriptValue("g"), ScriptValue(value.g));
			data.SetProperty(ScriptValue("b"), ScriptValue(value.b));
			data.SetProperty(ScriptValue("a"), ScriptValue(value.a));
		}

		static void Read(Color4& value, const ScriptValue& data)
		{
			value.r = data.GetProperty(ScriptValue("r"));
			value.g = data.GetProperty(ScriptValue("g"));
			value.b = data.GetProperty(ScriptValue("b"));
			value.a = data.GetProperty(ScriptValue("a"));
		}
	};

	template<typename T>
	struct ScriptValue::Converter<T, typename std::enable_if<std::is_pointer<T>::value && !std::is_const<T>::value&&
		std::is_base_of<o2::IObject, typename std::remove_pointer<T>::type>::value>::type>
	{
		static constexpr bool isSupported = true;

		static void Write(const T& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_object();
		}

		static void Read(T& value, const ScriptValue& data)
		{
		}
	};

	template<typename T>
	struct ScriptValue::Converter<Vector<T>>
	{
		static constexpr bool isSupported = true;

		static void Write(const Vector<T>& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_array();

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
			data.jvalue = jerry_create_object();

			for (auto& kv : value)
				data.SetProperty(ScriptValue(kv.first), ScriptValue(kv.second));
		}

		static void Read(Map<_key, _value>& value, const ScriptValue& data)
		{
			if (data.GetValueType() == ValueType::Object)
			{
				value.Clear();
				data.ForEachProperties([&](const ScriptValue& name, const ScriptValue& value) {
					value[name.GetValue<_key>()] = name.GetValue<_value>();
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
			data.jvalue = jerry_create_undefined();
			data.SetValue(Reflection::GetEnumName<T>(value));
		}

		static void Read(T& value, const ScriptValue& data)
		{
			value = Reflection::GetEnumValue<T>(data.GetValue<String>(buf));
		}
	};

	template<typename T>
	struct ScriptValue::Converter<T, typename std::enable_if<IsProperty<T>::value>::type>
	{
		static constexpr bool isSupported = ScriptValue::IsSupports<typename T::valueType>::value;
		using TValueType = typename T::valueType;

		static void Write(const T& value, ScriptValue& data)
		{
			data.jvalue = jerry_create_undefined();
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
			data.jvalue = jerry_create_external_function(&CallFunction);
			
 			IDataContainer* funcContainer = new FunctionContainer<Function<_res_type(_args ...)>, _res_type, _args ...>(value);
 			jerry_set_object_native_pointer(data.jvalue, funcContainer, &GetDataDeleter().info);
		}

		static void Read(Function<_res_type(_args ...)>& value, const ScriptValue& data)
		{
		}
	};
}

#endif // SCRIPTING_BACKEND_JERRYSCRIPT
