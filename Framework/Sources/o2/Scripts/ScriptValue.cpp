#include "o2/stdafx.h"
#include "ScriptValue.h"

namespace o2
{
    String ScriptValue::Dump(const String& tab /*= ""*/) const
    {
        String res;

        auto valType = GetValueType();
        if (valType == ValueType::Array)
        {
            res += "[ " + (String)GetLength() + "\n";

            for (int i = 0; i < GetLength(); i++)
                res += (*this)[i].Dump(tab + "  ") + ",\n";

            res += tab + "]";
        }
        else if (valType == ValueType::Bool)
        {
            res += (GetValue<bool>() ? String("true") : String("false"));
        }
        else if (valType == ValueType::Error)
        {
            res += "Error: " + GetError();
        }
        else if (valType == ValueType::Function)
        {
#if SCRIPTING_BACKEND_JERRYSCRIPT
            auto container = GetNativeContainer(jvalue);
            res += container ? "c-function" : "function";
#else
            res += "function";
#endif
        }
        else if (valType == ValueType::None)
        {
            res += "none";
        }
        else if (valType == ValueType::Null)
        {
            res += "null";
        }
        else if (valType == ValueType::Number)
        {
            res += (String)GetValue<float>();
        }
        else if (valType == ValueType::String)
        {
            res += GetValue<String>();
        }
        else if (valType == ValueType::Undefined)
        {
            res += "Undefined";
        }
        else if (valType == ValueType::Object)
        {
            res += "\n" + tab + "{\n";

            ForEachProperties([&](const ScriptValue& name, const ScriptValue& value) {
                res += tab + "  " + name.Dump(tab + "  ") + " : " + value.Dump(tab + "  ") + ",\n";
                return true;
            }, false);


            if (GetPrototype().GetValueType() != ValueType::Null)
                res += tab + "  prototype : " + GetPrototype().Dump(tab + "  ") + ",\n";

            res += tab + "}";
        }

        return res;
    }

    ScriptValue ScriptValue::GetProperty(const char* name) const
    {
        return GetProperty(ScriptValue(name));
    }

    void ScriptValue::SetProperty(const char* name, const ScriptValue& value)
    {
        SetProperty(ScriptValue(name), value);
    }

    bool ScriptValue::IsObject() const
    {
        return GetValueType() == ValueType::Object;
    }

    bool ScriptValue::IsArray() const
    {
        return GetValueType() == ValueType::Array;
    }

    bool ScriptValue::IsFunction() const
    {
        return GetValueType() == ValueType::Function;
    }

    bool ScriptValueProperty::operator==(const ScriptValueProperty& other) const
    {
        return object == other.object && name == other.name;
    }

    ScriptValue ScriptValueProperty::Get() const
    {
        return object.GetProperty(name);
    }

    void ScriptValueProperty::Set(const ScriptValue& value)
    {
        object.SetProperty(name, value);
    }

    ScriptValue ScriptValueArrayElement::Get() const
    {
        return object.GetElement(idx);
    }

    void ScriptValueArrayElement::Set(const ScriptValue& value)
    {
        object.SetElement(value, idx);
    }

    bool ScriptValueArrayElement::operator==(const ScriptValueArrayElement& other) const
    {
        return object == other.object && idx == other.idx;
    }

    IScriptValueProperty* ScriptValueProperty::Clone() const
    {
        return mnew ScriptValueProperty(*this);
    }

    IScriptValueProperty* ScriptValueArrayElement::Clone() const
    {
        return mnew ScriptValueArrayElement(*this);
    }
}
