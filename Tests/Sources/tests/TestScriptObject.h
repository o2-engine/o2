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
        // @SCRIPTABLE
        TestScriptObject(RefCounter* refCounter, int v, const String& n);
        // @SCRIPTABLE
        TestScriptObject(int v, const String& n);

        int GetValue() const;
        void SetValue(int v);

        String GetName() const;
        void SetName(const String& n);

        float GetScore() const;
        void SetScore(float s);

        // @SCRIPTABLE
        int Add(int a, int b) const;
        // @SCRIPTABLE
        String Concat(const String& a, const String& b) const;
        // @SCRIPTABLE
        float Multiply(float a, float b) const;

        // @SCRIPTABLE
        void SetAll(int v, const String& n, float s);
        // @SCRIPTABLE
        int GetDoubleValue() const;
        // @SCRIPTABLE
        String GetDescription() const;
        // @SCRIPTABLE
        void AddToScore(float delta);

        // @SCRIPTABLE
        int SumValueWith(const Ref<TestScriptObject>& other) const;

        // @SCRIPTABLE
        void SetLinkedPartner(const Ref<TestScriptObject>& other);
        // @SCRIPTABLE
        int SumWithLinkedPartner() const;

        IOBJECT(TestScriptObject);

    private:
        Ref<TestScriptObject> linkedPartner;
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
    FIELD().PRIVATE().NAME(linkedPartner);
}
END_META;
CLASS_METHODS_META(o2::TestScriptObject)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(RefCounter*, int, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(int, const String&);
    FUNCTION().PUBLIC().SIGNATURE(int, GetValue);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValue, int);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, SetName, const String&);
    FUNCTION().PUBLIC().SIGNATURE(float, GetScore);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScore, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(int, Add, int, int);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(String, Concat, const String&, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, Multiply, float, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetAll, int, const String&, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(int, GetDoubleValue);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(String, GetDescription);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, AddToScore, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(int, SumValueWith, const Ref<TestScriptObject>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetLinkedPartner, const Ref<TestScriptObject>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(int, SumWithLinkedPartner);
}
END_META;
// --- END META ---
