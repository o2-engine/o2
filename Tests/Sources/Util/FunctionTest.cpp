#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Utils/Function/Function.h"

using namespace o2;

namespace
{
    int sStaticCallSum = 0;

    int FreeAdd(int a, int b)
    {
        sStaticCallSum += a + b;
        return a + b;
    }

    int FreeMul(int a, int b)
    {
        return a * b;
    }

    class FunctionTestObject
    {
    public:
        int callCount = 0;
        int lastArg = 0;

        int Add(int a, int b) { callCount++; lastArg = a + b; return a + b; }
        int Mul(int a, int b) const { return a * b; }
        void NoReturn(int x) { lastArg = x; callCount++; }
    };
}

TEST(Function, EmptyInvokeReturnsDefault) {
    Function<int(int, int)> f;
    EXPECT_TRUE(f.IsEmpty());
    EXPECT_EQ(f.Invoke(3, 4), 0);
}

TEST(Function, FromStaticFunction) {
    Function<int(int, int)> f = &FreeAdd;
    EXPECT_FALSE(f.IsEmpty());
    EXPECT_EQ(f.Invoke(2, 3), 5);
    EXPECT_EQ(f(10, 20), 30);
}

TEST(Function, FromLambda) {
    Function<int(int)> f = [](int x) { return x * x; };
    EXPECT_EQ(f.Invoke(7), 49);
}

TEST(Function, FromObjectMethod) {
    FunctionTestObject obj;
    Function<int(int, int)> f(&obj, &FunctionTestObject::Add);
    EXPECT_EQ(f.Invoke(4, 5), 9);
    EXPECT_EQ(obj.callCount, 1);
    EXPECT_EQ(obj.lastArg, 9);
}

TEST(Function, FromObjectConstMethod) {
    FunctionTestObject obj;
    Function<int(int, int)> f(&obj, &FunctionTestObject::Mul);
    EXPECT_EQ(f.Invoke(6, 7), 42);
}

TEST(Function, MulticastInvokesAll) {
    sStaticCallSum = 0;
    int lambdaCalls = 0;

    Function<int(int, int)> f;
    f.Add(FunctionPtr<int, int, int>(&FreeAdd));
    f.Add(SharedLambda<std::function<int(int, int)>, int, int, int>(
        std::function<int(int, int)>([&](int a, int b) { lambdaCalls++; return a + b + 100; })));

    int result = f.Invoke(3, 4);
    // Multicast invokes all; the LAST registered handler's return value is what comes back.
    EXPECT_EQ(result, 107);
    // The static handler ran at least once (its side-effect proves it).
    EXPECT_EQ(sStaticCallSum, 7);
    EXPECT_EQ(lambdaCalls, 1);
}

TEST(Function, AddObjectMethodAndInvokeMulticast) {
    FunctionTestObject obj1;
    FunctionTestObject obj2;
    Function<int(int, int)> f;
    f.Add(&obj1, &FunctionTestObject::Add);
    f.Add(&obj2, &FunctionTestObject::Add);

    f.Invoke(1, 2);
    EXPECT_EQ(obj1.callCount, 1);
    EXPECT_EQ(obj2.callCount, 1);
    EXPECT_EQ(obj1.lastArg, 3);
    EXPECT_EQ(obj2.lastArg, 3);
}

TEST(Function, RemoveByEqualsRemovesObjectMethod) {
    FunctionTestObject obj1, obj2;
    Function<int(int, int)> f;
    f.Add(&obj1, &FunctionTestObject::Add);
    f.Add(&obj2, &FunctionTestObject::Add);

    f.Remove(ObjFunctionPtr<FunctionTestObject, int, int, int>(&obj1, &FunctionTestObject::Add));

    f.Invoke(5, 6);
    EXPECT_EQ(obj1.callCount, 0);
    EXPECT_EQ(obj2.callCount, 1);
}

TEST(Function, RemoveByEqualsRemovesStaticPtr) {
    sStaticCallSum = 0;
    FunctionTestObject obj;
    Function<int(int, int)> f;
    f.Add(FunctionPtr<int, int, int>(&FreeAdd));
    f.Add(&obj, &FunctionTestObject::Add);

    f.Remove(FunctionPtr<int, int, int>(&FreeAdd));
    f.Invoke(1, 1);
    EXPECT_EQ(sStaticCallSum, 0);
    EXPECT_EQ(obj.callCount, 1);
}

TEST(Function, ClearEmpties) {
    Function<int(int, int)> f = &FreeAdd;
    EXPECT_FALSE(f.IsEmpty());
    f.Clear();
    EXPECT_TRUE(f.IsEmpty());
    EXPECT_EQ(f.Invoke(1, 2), 0);
}

TEST(Function, MoveSemanticsTransfersFunctions) {
    sStaticCallSum = 0;
    Function<int(int, int)> a = &FreeAdd;
    Function<int(int, int)> b = std::move(a);

    EXPECT_TRUE(a.IsEmpty());
    EXPECT_FALSE(b.IsEmpty());
    EXPECT_EQ(b.Invoke(2, 3), 5);
    EXPECT_EQ(sStaticCallSum, 5);
}

TEST(Function, CopyDoesNotShareState) {
    FunctionTestObject obj;
    Function<int(int, int)> a(&obj, &FunctionTestObject::Add);
    Function<int(int, int)> b = a;

    a.Clear();
    b.Invoke(1, 2);
    EXPECT_EQ(obj.callCount, 1);
}

TEST(Function, FunctionPtrEquality) {
    FunctionPtr<int, int, int> p1(&FreeAdd);
    FunctionPtr<int, int, int> p2(&FreeAdd);
    FunctionPtr<int, int, int> p3(&FreeMul);

    EXPECT_TRUE(p1 == p2);
    EXPECT_FALSE(p1 == p3);
    EXPECT_TRUE(p1 != p3);
}

TEST(Function, ObjFunctionPtrEqualityNeedsBothObjectAndMethod) {
    FunctionTestObject a, b;
    ObjFunctionPtr<FunctionTestObject, int, int, int> p1(&a, &FunctionTestObject::Add);
    ObjFunctionPtr<FunctionTestObject, int, int, int> p2(&a, &FunctionTestObject::Add);
    ObjFunctionPtr<FunctionTestObject, int, int, int> p3(&b, &FunctionTestObject::Add);

    EXPECT_TRUE(p1 == p2);
    EXPECT_FALSE(p1 == p3);
}

TEST(Function, SharedLambdaAlwaysEquals) {
    // Documented behaviour: SharedLambda::operator== returns true unconditionally,
    // since C++ lambdas are not comparable. This is load-bearing for Function::Remove.
    SharedLambda<std::function<int(int)>, int, int> l1(std::function<int(int)>([](int x) { return x; }));
    SharedLambda<std::function<int(int)>, int, int> l2(std::function<int(int)>([](int x) { return x + 1; }));

    EXPECT_TRUE(l1 == l2);
}

TEST(Function, IFunctionInvokeViaPointer) {
    FunctionPtr<int, int, int> p(&FreeAdd);
    IFunction<int(int, int)>* iface = &p;
    EXPECT_EQ(iface->Invoke(10, 20), 30);
    EXPECT_EQ((*iface)(5, 5), 10);
}

TEST(Function, AddingTwoLambdasViaFunctionInterface) {
    int callsA = 0, callsB = 0;
    Function<void(int)> f;
    f = [&](int x) { callsA++; };
    f.Add(SharedLambda<std::function<void(int)>, void, int>(
        std::function<void(int)>([&](int x) { callsB++; })));

    f.Invoke(0);
    EXPECT_EQ(callsA, 1);
    EXPECT_EQ(callsB, 1);
}
