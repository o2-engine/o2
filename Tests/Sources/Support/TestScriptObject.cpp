#include "o2/stdafx.h"
#include "TestScriptObject.h"

namespace o2
{
    TestScriptObject::TestScriptObject(RefCounter* refCounter, int v, const String& n) :
        RefCounterable(refCounter), value(v), name(n)
    {}

    TestScriptObject::TestScriptObject(int v, const String& n) : value(v), name(n)
    {}

    int TestScriptObject::GetValue() const
    {
        return value;
    }

    void TestScriptObject::SetValue(int v)
    {
        value = v;
    }

    String TestScriptObject::GetName() const
    {
        return name;
    }

    void TestScriptObject::SetName(const String& n)
    {
        name = n;
    }

    float TestScriptObject::GetScore() const
    {
        return score;
    }

    void TestScriptObject::SetScore(float s)
    {
        score = s;
    }

    int TestScriptObject::Add(int a, int b) const
    {
        return a + b;
    }

    String TestScriptObject::Concat(const String& a, const String& b) const
    {
        return a + b;
    }

    float TestScriptObject::Multiply(float a, float b) const
    {
        return a * b;
    }

    void TestScriptObject::SetAll(int v, const String& n, float s)
    {
        value = v;
        name = n;
        score = s;
    }

    int TestScriptObject::GetDoubleValue() const
    {
        return value * 2;
    }

    String TestScriptObject::GetDescription() const
    {
        return name + ":" + (String)value;
    }

    void TestScriptObject::AddToScore(float delta)
    {
        score += delta;
    }

    int TestScriptObject::SumValueWith(const Ref<TestScriptObject>& other) const
    {
        return value + (other ? other->value : 0);
    }

    void TestScriptObject::SetLinkedPartner(const Ref<TestScriptObject>& other)
    {
        linkedPartner = other;
    }

    int TestScriptObject::SumWithLinkedPartner() const
    {
        return value + (linkedPartner ? linkedPartner->value : 0);
    }
}
// --- META ---

DECLARE_CLASS(o2::TestScriptObject, o2__TestScriptObject);
// --- END META ---
