#pragma once

#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Types/String.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    // Test class for ScriptValue binding tests
    class TestScriptObject : public IObject, public RefCounterable
    {
    public:
        int value = 0;       // @SCRIPTABLE
        String name = "default";  // @SCRIPTABLE
        float score = 0.0f;  // @SCRIPTABLE

    public:
        TestScriptObject() = default;
        TestScriptObject(int v, const String& n) : value(v), name(n) {}

        int GetValue() const { return value; }
        void SetValue(int v) { value = v; }

        String GetName() const { return name; }
        void SetName(const String& n) { name = n; }

        float GetScore() const { return score; }
        void SetScore(float s) { score = s; }

        int Add(int a, int b) const { return a + b; }
        String Concat(const String& a, const String& b) const { return a + b; }
        float Multiply(float a, float b) const { return a * b; }

        IOBJECT(TestScriptObject);
    };
}
// --- META ---

CLASS_BASES_META(o2::TestScriptObject)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(o2::TestScriptObject)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(value);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().DEFAULT_VALUE("default").NAME(name);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(score);
}
END_META;
CLASS_METHODS_META(o2::TestScriptObject)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(int, const String&);
    FUNCTION().PUBLIC().SIGNATURE(int, GetValue);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValue, int);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, SetName, const String&);
    FUNCTION().PUBLIC().SIGNATURE(float, GetScore);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScore, float);
    FUNCTION().PUBLIC().SIGNATURE(int, Add, int, int);
    FUNCTION().PUBLIC().SIGNATURE(String, Concat, const String&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(float, Multiply, float, float);
}
END_META;
// --- END META ---
