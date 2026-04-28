#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Reflection/TypeTraits.h"
#include "o2/Utils/Property.h"

using namespace o2;

namespace
{
    class PropertyTestHost
    {
    public:
        PROPERTIES(PropertyTestHost);
        PROPERTY(int, Value, SetValue, GetValue);
        GETTER(int, ReadOnly, GetReadOnly);
        SETTER(int, WriteOnly, SetWriteOnly);

        int mValue = 0;
        int mWriteOnly = 0;
        int mSetterCalls = 0;
        mutable int mGetterCalls = 0;

        int GetValue() const { mGetterCalls++; return mValue; }
        void SetValue(int v) { mValue = v; mSetterCalls++; }
        int GetReadOnly() const { return 42; }
        void SetWriteOnly(int v) { mWriteOnly = v; }
    };
}

TEST(Property, AssignmentInvokesSetter) {
    PropertyTestHost h;
    h.Value = 5;

    EXPECT_EQ(h.mValue, 5);
    EXPECT_EQ(h.mSetterCalls, 1);
}

TEST(Property, ImplicitConversionInvokesGetter) {
    PropertyTestHost h;
    h.mValue = 17;

    int x = h.Value;
    EXPECT_EQ(x, 17);
    EXPECT_GE(h.mGetterCalls, 1);
}

TEST(Property, GetSetExplicitMethods) {
    PropertyTestHost h;
    h.Value.Set(99);
    EXPECT_EQ(h.mValue, 99);
    EXPECT_EQ(h.Value.Get(), 99);
}

TEST(Property, AssignmentBetweenInstances) {
    PropertyTestHost a;
    PropertyTestHost b;

    a.Value = 100;
    b.Value = a.Value;

    EXPECT_EQ(b.mValue, 100);
    EXPECT_EQ(b.mSetterCalls, 1);
}

TEST(Property, OffsetTrickCorrectAcrossArrayInstances) {
    // Property uses offsetof to find its owner. With multiple host instances in an
    // array, writing through one property must only modify that instance.
    PropertyTestHost arr[3];

    arr[0].Value = 10;
    arr[1].Value = 20;
    arr[2].Value = 30;

    EXPECT_EQ(arr[0].mValue, 10);
    EXPECT_EQ(arr[1].mValue, 20);
    EXPECT_EQ(arr[2].mValue, 30);

    EXPECT_EQ(arr[0].mSetterCalls, 1);
    EXPECT_EQ(arr[1].mSetterCalls, 1);
    EXPECT_EQ(arr[2].mSetterCalls, 1);

    int v0 = arr[0].Value;
    int v1 = arr[1].Value;
    int v2 = arr[2].Value;
    EXPECT_EQ(v0, 10);
    EXPECT_EQ(v1, 20);
    EXPECT_EQ(v2, 30);
}

TEST(Property, PlusEqualsCombinesGetAndSet) {
    PropertyTestHost h;
    h.Value = 10;
    h.mSetterCalls = 0;
    h.mGetterCalls = 0;

    h.Value += 5;
    EXPECT_EQ(h.mValue, 15);
    EXPECT_EQ(h.mSetterCalls, 1);
    EXPECT_GE(h.mGetterCalls, 1);
}

TEST(Property, MinusEqualsAndMultiplyEquals) {
    PropertyTestHost h;
    h.Value = 20;

    h.Value -= 5;
    EXPECT_EQ(h.mValue, 15);

    h.Value *= 2;
    EXPECT_EQ(h.mValue, 30);

    h.Value /= 3;
    EXPECT_EQ(h.mValue, 10);
}

TEST(Property, ArithmeticReturnsValueWithoutMutating) {
    PropertyTestHost h;
    h.Value = 7;
    int oldSetters = h.mSetterCalls;

    int sum = h.Value + 3;
    EXPECT_EQ(sum, 10);
    EXPECT_EQ(h.mValue, 7);
    EXPECT_EQ(h.mSetterCalls, oldSetters);
}

TEST(Property, EqualityOperator) {
    PropertyTestHost h;
    h.Value = 42;

    EXPECT_TRUE(h.Value == 42);
    EXPECT_FALSE(h.Value == 100);
    EXPECT_TRUE(h.Value != 100);
}

TEST(Property, GetterIsReadOnly) {
    PropertyTestHost h;
    int v = h.ReadOnly;
    EXPECT_EQ(v, 42);
    EXPECT_TRUE(h.ReadOnly == 42);
}

TEST(Property, SetterIsWriteOnly) {
    PropertyTestHost h;
    h.WriteOnly = 77;
    EXPECT_EQ(h.mWriteOnly, 77);
}

TEST(Property, SetterCalledOncePerAssignment) {
    PropertyTestHost h;
    h.mSetterCalls = 0;

    h.Value = 1;
    h.Value = 2;
    h.Value = 3;
    EXPECT_EQ(h.mSetterCalls, 3);

    int x = h.Value;
    int y = h.Value;
    (void)x; (void)y;
    EXPECT_EQ(h.mSetterCalls, 3); // reads do not increment
}

TEST(Property, IsPropertyReturnsTrue) {
    PropertyTestHost h;
    EXPECT_TRUE(h.Value.IsProperty());
}
