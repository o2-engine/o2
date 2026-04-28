#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Type.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Reflection/FieldInfo.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

#include "tests/TestScriptObject.h"

using namespace o2;

TEST(TypeReflection, TypeOfReturnsSameTypeInstance) {
    const Type& t1 = TypeOf(int);
    const Type& t2 = TypeOf(int);
    EXPECT_EQ(&t1, &t2);

    const Type& tFloat = TypeOf(float);
    EXPECT_NE(&t1, &tFloat);
}

TEST(TypeReflection, PrimitiveTypesHaveDistinctIds) {
    EXPECT_NE(TypeOf(int).ID(), TypeOf(float).ID());
    EXPECT_NE(TypeOf(int).ID(), TypeOf(double).ID());
    EXPECT_NE(TypeOf(float).ID(), TypeOf(double).ID());
    EXPECT_NE(TypeOf(int).ID(), TypeOf(bool).ID());
}

TEST(TypeReflection, PrimitiveTypeSize) {
    EXPECT_EQ(TypeOf(int).GetSize(), (int)sizeof(int));
    EXPECT_EQ(TypeOf(float).GetSize(), (int)sizeof(float));
    EXPECT_EQ(TypeOf(double).GetSize(), (int)sizeof(double));
    EXPECT_EQ(TypeOf(bool).GetSize(), (int)sizeof(bool));
}

TEST(TypeReflection, PrimitiveTypeNameNonEmpty) {
    EXPECT_FALSE(TypeOf(int).GetName().IsEmpty());
    EXPECT_FALSE(TypeOf(float).GetName().IsEmpty());
}

TEST(TypeReflection, EqualityOperators) {
    const Type& intType = TypeOf(int);
    const Type& floatType = TypeOf(float);

    bool sameEq = (intType == intType);
    bool diffEq = (intType == floatType);
    bool diffNe = (intType != floatType);

    EXPECT_TRUE(sameEq);
    EXPECT_FALSE(diffEq);
    EXPECT_TRUE(diffNe);
}

TEST(TypeReflection, ObjectTypeNameIsClassName) {
    const Type& t = TypeOf(TestScriptObject);
    EXPECT_FALSE(t.GetName().IsEmpty());
    EXPECT_NE((int)t.GetName().find("TestScriptObject"), -1);
}

TEST(TypeReflection, ObjectTypeSizeMatchesSizeof) {
    EXPECT_EQ(TypeOf(TestScriptObject).GetSize(), (int)sizeof(TestScriptObject));
}

TEST(TypeReflection, ObjectGetTypeMatchesTypeOf) {
    Ref<TestScriptObject> obj = mmake<TestScriptObject>();
    EXPECT_EQ(&obj->GetType(), &TypeOf(TestScriptObject));
}

TEST(TypeReflection, IsBasedOnDetectsBaseClasses) {
    const Type& objectType = TypeOf(TestScriptObject);
    const Type& iObjectType = TypeOf(IObject);
    EXPECT_TRUE(objectType.IsBasedOn(iObjectType));
    // IObject is not based on TestScriptObject.
    EXPECT_FALSE(iObjectType.IsBasedOn(objectType));
}

TEST(TypeReflection, GetFieldsContainsDeclaredFields) {
    const Type& t = TypeOf(TestScriptObject);
    const Vector<FieldInfo>& fields = t.GetFields();

    auto hasField = [&](const String& name) {
        for (const FieldInfo& f : fields)
            if (f.GetName() == name) return true;
        return false;
    };

    EXPECT_TRUE(hasField("value"));
    EXPECT_TRUE(hasField("name"));
    EXPECT_TRUE(hasField("score"));
}

TEST(TypeReflection, GetFieldByNameReturnsCorrectFieldInfo) {
    const FieldInfo* valueField = TypeOf(TestScriptObject).GetField("value");
    ASSERT_NE(valueField, nullptr);
    EXPECT_EQ(valueField->GetName(), "value");
    EXPECT_EQ(valueField->GetType(), &TypeOf(int));

    const FieldInfo* nameField = TypeOf(TestScriptObject).GetField("name");
    ASSERT_NE(nameField, nullptr);
    EXPECT_EQ(nameField->GetType(), &TypeOf(String));
}

TEST(TypeReflection, GetFieldReturnsNullForMissingName) {
    EXPECT_EQ(TypeOf(TestScriptObject).GetField("nonExistentField"), nullptr);
}

TEST(TypeReflection, FieldGetValueAndSetValue) {
    Ref<TestScriptObject> obj = mmake<TestScriptObject>();
    obj->value = 42;

    const FieldInfo* valueField = TypeOf(TestScriptObject).GetField("value");
    ASSERT_NE(valueField, nullptr);

    int read = valueField->GetValue<int>(obj.Get());
    EXPECT_EQ(read, 42);

    valueField->SetValue<int>(obj.Get(), 999);
    EXPECT_EQ(obj->value, 999);
}

TEST(TypeReflection, FieldStringGetSet) {
    Ref<TestScriptObject> obj = mmake<TestScriptObject>();
    obj->name = "alpha";

    const FieldInfo* nameField = TypeOf(TestScriptObject).GetField("name");
    ASSERT_NE(nameField, nullptr);

    String read = nameField->GetValue<String>(obj.Get());
    EXPECT_EQ(read, "alpha");

    nameField->SetValue<String>(obj.Get(), String("beta"));
    EXPECT_EQ(obj->name, "beta");
}

TEST(TypeReflection, GetBaseTypesContainsExpectedBases) {
    const auto& bases = TypeOf(TestScriptObject).GetBaseTypes();
    EXPECT_GE(bases.Count(), 1);

    bool foundIObject = false;
    for (const Type::BaseType& b : bases)
    {
        if (b.type == &TypeOf(IObject)) foundIObject = true;
    }
    EXPECT_TRUE(foundIObject);
}

TEST(TypeReflection, GetFunctionByNameReturnsFunctionInfo) {
    const FunctionInfo* addFunc = TypeOf(TestScriptObject).GetFunction("Add");
    EXPECT_NE(addFunc, nullptr);
}

TEST(TypeReflection, GetFunctionReturnsNullForMissingName) {
    EXPECT_EQ(TypeOf(TestScriptObject).GetFunction("NoSuchMethod"), nullptr);
}

TEST(TypeReflection, GetFieldsWithBaseClassesIncludesInherited) {
    const Type& t = TypeOf(TestScriptObject);
    Vector<const FieldInfo*> all = t.GetFieldsWithBaseClasses();

    // At minimum should include all fields from the type itself.
    EXPECT_GE(all.Count(), t.GetFields().Count());
}
