#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Scripts/ScriptValue.h"
#include "o2/Scripts/ScriptEngine.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "tests/TestScriptObject.h"

using namespace o2;

#if IS_SCRIPTING_SUPPORTED

TEST(ScriptValue, PrimitiveTypes) {
    ScriptValue vInt(42);
    EXPECT_EQ(vInt.GetValue<int>(), 42);
    EXPECT_EQ(vInt.ToNumber(), 42.0f);

    ScriptValue vFloat(3.14f);
    EXPECT_FLOAT_EQ(vFloat.GetValue<float>(), 3.14f);
    EXPECT_FLOAT_EQ(vFloat.ToNumber(), 3.14f);

    ScriptValue vBool(true);
    EXPECT_TRUE(vBool.ToBool());
    EXPECT_TRUE(vBool.GetValue<bool>());

    ScriptValue vStr("hello");
    EXPECT_EQ(vStr.ToString(), "hello");
}

TEST(ScriptValue, Vec2F) {
    Vec2F v(3.0f, 4.0f);
    ScriptValue sv(v);
    Vec2F back = sv.GetValue<Vec2F>();
    EXPECT_FLOAT_EQ(back.x, 3.0f);
    EXPECT_FLOAT_EQ(back.y, 4.0f);
}

TEST(ScriptValue, Vector) {
    Vector<int> arr({0, 1, 2, 3, 4});
    ScriptValue sv(arr);
    EXPECT_TRUE(sv.IsArray());
    EXPECT_EQ(sv.GetLength(), 5);
    EXPECT_EQ(sv.GetElement(2).GetValue<int>(), 2);
}

TEST(ScriptValue, EmptyObject) {
    ScriptValue obj = ScriptValue::EmptyObject();
    EXPECT_TRUE(obj.IsObject());

    obj.SetProperty("a", 5);
    obj.SetProperty("b", "test");
    EXPECT_EQ(obj.GetProperty("a").GetValue<int>(), 5);
    EXPECT_EQ(obj.GetProperty("b").ToString(), "test");
}

TEST(ScriptValue, EmptyArray) {
    ScriptValue arr = ScriptValue::EmptyArray();
    EXPECT_TRUE(arr.IsArray());
    EXPECT_EQ(arr.GetLength(), 0);

    arr.AddElement(ScriptValue(1));
    arr.AddElement(ScriptValue(2));
    arr.SetElement(ScriptValue(99), 0);
    EXPECT_EQ(arr.GetLength(), 2);
    EXPECT_EQ(arr.GetElement(0).GetValue<int>(), 99);
    EXPECT_EQ(arr.GetElement(1).GetValue<int>(), 2);
}

TEST(ScriptValue, GlobalSetGet) {
    o2Scripts.GetGlobal().SetProperty("testNum", 123);
    o2Scripts.GetGlobal().SetProperty("testStr", "global");
    o2Scripts.CollectGarbage();

    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("testNum").GetValue<int>(), 123);
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("testStr").ToString(), "global");
}

TEST(ScriptValue, Eval) {
    o2Scripts.Eval("var x = 5 + 5;");
    float res = o2Scripts.GetGlobal().GetProperty("x").ToNumber();
    EXPECT_FLOAT_EQ(res, 10.0f);

    ScriptValue evalResult = o2Scripts.Eval("2 * 3");
    EXPECT_FLOAT_EQ(evalResult.ToNumber(), 6.0f);
}

TEST(ScriptValue, FunctionFromCpp) {
    o2Scripts.GetGlobal().SetProperty("add", Function<float(int, float)>([](int a, float b) {
        return a + b;
    }));
    o2Scripts.CollectGarbage();

    ScriptValue addFunc = o2Scripts.GetGlobal().GetProperty("add");
    EXPECT_TRUE(addFunc.IsFunction());
    float result = addFunc.Invoke<float>(10, 2.5f);
    EXPECT_FLOAT_EQ(result, 12.5f);
}

TEST(ScriptValue, FunctionInvokeFromScript) {
    o2Scripts.GetGlobal().SetProperty("myfunc", Function<float(int, float)>([](int a, float b) {
        return a + b + 5.0f;
    }));
    o2Scripts.CollectGarbage();
    o2Scripts.Eval("var r = myfunc(1, 3.2);");

    float r = o2Scripts.GetGlobal().GetProperty("r").ToNumber();
    EXPECT_NEAR(r, 9.2f, 1e-5f);
}

TEST(ScriptValue, Prototype) {
    ScriptValue prot = ScriptValue::EmptyObject();
    prot.SetProperty("a", 42);
    prot.SetProperty("func", Function<void()>([]() {}));

    ScriptValue exm = ScriptValue::EmptyObject();
    exm.SetPrototype(prot);
    o2Scripts.GetGlobal().SetProperty("exm", exm);

    EXPECT_EQ(exm.GetProperty("a").GetValue<int>(), 42);
}

TEST(ScriptValue, ParseAndRun) {
    auto pres = o2Scripts.Parse("var parsed = 100;");
    EXPECT_TRUE(pres.IsOk());
    o2Scripts.Run(pres);

    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("parsed").ToNumber(), 100.0f);
}

TEST(ScriptValue, ObjectBinding) {
    auto obj = mmake<TestScriptObject>();
    obj->value = 42;
    obj->name = "test";
    obj->score = 3.14f;

    ScriptValue sv = obj->GetScriptValue();
    EXPECT_TRUE(sv.IsObject());

    o2Scripts.GetGlobal().SetProperty("testObj", sv);
    o2Scripts.CollectGarbage();

    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("testObj").GetProperty("value").GetValue<int>(), 42);
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("testObj").GetProperty("name").ToString(), "test");
    EXPECT_FLOAT_EQ(o2Scripts.GetGlobal().GetProperty("testObj").GetProperty("score").GetValue<float>(), 3.14f);
}

TEST(ScriptValue, ObjectFieldWriteFromScript) {
    auto obj = mmake<TestScriptObject>();
    obj->value = 10;
    obj->name = "before";

    o2Scripts.GetGlobal().SetProperty("testObj", obj->GetScriptValue());
    o2Scripts.CollectGarbage();

    o2Scripts.Eval("testObj.value = 100; testObj.name = 'after'; testObj.score = 2.5;");

    EXPECT_EQ(obj->value, 100);
    EXPECT_EQ(obj->name, "after");
    EXPECT_FLOAT_EQ(obj->score, 2.5f);
}


#else

TEST(ScriptValue, ScriptingDisabled) {
    SUCCEED() << "Scripting is disabled in this build";
}

#endif // IS_SCRIPTING_SUPPORTED
