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

TEST(ScriptValue, ObjectMethodCallFromCpp) {
    auto obj = mmake<TestScriptObject>();

    ScriptValue sv = obj->GetScriptValue();
    EXPECT_TRUE(sv.IsObject());

    ScriptValue addFunc = sv.GetProperty("Add");
    EXPECT_TRUE(addFunc.IsFunction());
    int addResult = addFunc.Invoke<int>(sv, 3, 7);
    EXPECT_EQ(addResult, 10);

    ScriptValue concatFunc = sv.GetProperty("Concat");
    EXPECT_TRUE(concatFunc.IsFunction());
    String concatResult = concatFunc.Invoke<String>(sv, String("Hello, "), String("World!"));
    EXPECT_EQ(concatResult, "Hello, World!");

    ScriptValue multiplyFunc = sv.GetProperty("Multiply");
    EXPECT_TRUE(multiplyFunc.IsFunction());
    float multiplyResult = multiplyFunc.Invoke<float>(sv, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(multiplyResult, 12.0f);
}

TEST(ScriptValue, ConstructFromScript) {
    o2Scripts.Eval("var constructed = new o2.TestScriptObject(42, 'hello');");

    ScriptValue sv = o2Scripts.GetGlobal().GetProperty("constructed");
    EXPECT_TRUE(sv.IsObject());
    EXPECT_EQ(sv.GetProperty("value").GetValue<int>(), 42);
    EXPECT_EQ(sv.GetProperty("name").ToString(), "hello");

    int addRes = sv.GetProperty("Add").Invoke<int>(sv, 1, 2);
    EXPECT_EQ(addRes, 3);
}

TEST(ScriptValue, ObjectMethodCallFromScript) {
    auto obj = mmake<TestScriptObject>();

    o2Scripts.GetGlobal().SetProperty("testObj2", obj->GetScriptValue());
    o2Scripts.CollectGarbage();

    o2Scripts.Eval("var addRes = testObj2.Add(6, 7);");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("addRes").GetValue<int>(), 13);

    o2Scripts.Eval("var concatRes = testObj2.Concat('foo', 'bar');");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("concatRes").ToString(), "foobar");

    o2Scripts.Eval("var mulRes = testObj2.Multiply(2.5, 4.0);");
    EXPECT_FLOAT_EQ(o2Scripts.GetGlobal().GetProperty("mulRes").GetValue<float>(), 10.0f);
}


TEST(ScriptValue, MethodMutatesStateFromScript) {
    auto obj = mmake<TestScriptObject>();
    obj->value = 1;
    obj->name = "old";
    obj->score = 0.0f;

    o2Scripts.GetGlobal().SetProperty("mutObj", obj->GetScriptValue());
    o2Scripts.CollectGarbage();

    o2Scripts.Eval("mutObj.SetAll(99, 'new', 7.5);");
    EXPECT_EQ(obj->value, 99);
    EXPECT_EQ(obj->name, "new");
    EXPECT_FLOAT_EQ(obj->score, 7.5f);

    o2Scripts.Eval("mutObj.AddToScore(2.5);");
    EXPECT_FLOAT_EQ(obj->score, 10.0f);
}

TEST(ScriptValue, MethodReadsStateFromScript) {
    auto obj = mmake<TestScriptObject>();
    obj->value = 21;
    obj->name = "item";

    o2Scripts.GetGlobal().SetProperty("readObj", obj->GetScriptValue());
    o2Scripts.CollectGarbage();

    o2Scripts.Eval("var dv = readObj.GetDoubleValue();");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("dv").GetValue<int>(), 42);

    o2Scripts.Eval("var desc = readObj.GetDescription();");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("desc").ToString(), "item:21");
}

TEST(ScriptValue, FieldReadFromScript) {
    auto obj = mmake<TestScriptObject>();
    obj->value = 77;
    obj->name = "alpha";
    obj->score = 1.5f;

    o2Scripts.GetGlobal().SetProperty("fieldObj", obj->GetScriptValue());
    o2Scripts.CollectGarbage();

    o2Scripts.Eval("var fv = fieldObj.value; var fn = fieldObj.name; var fs = fieldObj.score;");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("fv").GetValue<int>(), 77);
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("fn").ToString(), "alpha");
    EXPECT_FLOAT_EQ(o2Scripts.GetGlobal().GetProperty("fs").GetValue<float>(), 1.5f);
}

TEST(ScriptValue, FieldWriteThenMethodReads) {
    auto obj = mmake<TestScriptObject>();

    o2Scripts.GetGlobal().SetProperty("fmObj", obj->GetScriptValue());
    o2Scripts.CollectGarbage();

    o2Scripts.Eval("fmObj.value = 10; fmObj.name = 'test';");
    EXPECT_EQ(obj->value, 10);
    EXPECT_EQ(obj->name, "test");

    o2Scripts.Eval("var fmDbl = fmObj.GetDoubleValue();");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("fmDbl").GetValue<int>(), 20);

    o2Scripts.Eval("var fmDesc = fmObj.GetDescription();");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("fmDesc").ToString(), "test:10");
}

TEST(ScriptValue, MultipleInstancesSharePrototype) {
    auto obj1 = mmake<TestScriptObject>();
    obj1->value = 10;
    obj1->name = "first";
    obj1->score = 1.0f;

    auto obj2 = mmake<TestScriptObject>();
    obj2->value = 20;
    obj2->name = "second";
    obj2->score = 2.0f;

    o2Scripts.GetGlobal().SetProperty("inst1", obj1->GetScriptValue());
    o2Scripts.GetGlobal().SetProperty("inst2", obj2->GetScriptValue());
    o2Scripts.CollectGarbage();

    o2Scripts.Eval("var v1 = inst1.value; var v2 = inst2.value;");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("v1").GetValue<int>(), 10);
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("v2").GetValue<int>(), 20);

    o2Scripts.Eval("inst1.value = 111; inst2.value = 222;");
    EXPECT_EQ(obj1->value, 111);
    EXPECT_EQ(obj2->value, 222);

    o2Scripts.Eval("var d1 = inst1.GetDoubleValue(); var d2 = inst2.GetDoubleValue();");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("d1").GetValue<int>(), 222);
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("d2").GetValue<int>(), 444);

    o2Scripts.Eval("inst1.AddToScore(5.0); inst2.AddToScore(10.0);");
    EXPECT_FLOAT_EQ(obj1->score, 6.0f);
    EXPECT_FLOAT_EQ(obj2->score, 12.0f);
}

TEST(ScriptValue, ConstructedObjectFieldsAndMethods) {
    o2Scripts.Eval(
        "var cObj = new o2.TestScriptObject(5, 'built');"
        "cObj.score = 9.9;"
        "var cDbl = cObj.GetDoubleValue();"
        "var cDesc = cObj.GetDescription();"
        "cObj.AddToScore(0.1);"
        "var cScore = cObj.score;"
    );

    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("cDbl").GetValue<int>(), 10);
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("cDesc").ToString(), "built:5");
    EXPECT_FLOAT_EQ(o2Scripts.GetGlobal().GetProperty("cScore").GetValue<float>(), 10.0f);
}

#else

TEST(ScriptValue, ScriptingDisabled) {
    SUCCEED() << "Scripting is disabled in this build";
}

#endif // IS_SCRIPTING_SUPPORTED
