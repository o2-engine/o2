#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Scripts/ScriptValue.h"
#include "o2/Scripts/ScriptEngine.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "tests/TestScriptObject.h"

#if IS_SCRIPTING_SUPPORTED
#include "o2/Scene/Actor.h"
#include "o2/Scene/UI/Widgets/Image.h"
#endif

using namespace o2;

#if IS_SCRIPTING_SUPPORTED

namespace
{
    struct LargeNativePayload
    {
        int  value = 0;
        char padding[1024] = {};
    };
}

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

// JS function invoked from C++; argument is another native object (ScriptValue / Jerry native pointer).
// Using a scriptable method on the argument checks it is the real binding, not a plain object.
TEST(ScriptValue, ScriptFunctionWithNativeObjectArgument) {
    auto other = mmake<TestScriptObject>();
    other->value = 21;
    other->name = "arg";

    o2Scripts.Eval("function readNativeOther(other) { return other.GetDoubleValue(); }");
    o2Scripts.CollectGarbage();

    ScriptValue fn = o2Scripts.GetGlobal().GetProperty("readNativeOther");
    EXPECT_TRUE(fn.IsFunction());

    ScriptValue otherSv = other->GetScriptValue();
    EXPECT_TRUE(otherSv.IsObject());
    EXPECT_TRUE(otherSv.IsObjectContainer());

    int result = fn.Invoke<int>(ScriptValue(), otherSv);
    EXPECT_EQ(result, 42);

    // Same flow from pure script: pass global holding native container into a function
    o2Scripts.GetGlobal().SetProperty("scriptArgObj", otherSv);
    o2Scripts.CollectGarbage();
    o2Scripts.Eval("function sumWithOther(o) { return o.value + o.GetDoubleValue(); } var scriptPassSum = sumWithOther(scriptArgObj);");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("scriptPassSum").GetValue<int>(), 63);
}

TEST(ScriptValue, NativeMethodTakesNativeObjectFromScript) {
    auto a = mmake<TestScriptObject>();
    a->value = 10;
    a->name = "left";
    auto b = mmake<TestScriptObject>();
    b->value = 32;
    b->name = "right";

    o2Scripts.GetGlobal().SetProperty("sumLeft", a->GetScriptValue());
    o2Scripts.GetGlobal().SetProperty("sumRight", b->GetScriptValue());
    o2Scripts.CollectGarbage();

    o2Scripts.Eval("var sumNativePair = sumLeft.SumValueWith(sumRight);");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("sumNativePair").GetValue<int>(), 42);

    o2Scripts.Eval("var sumNativeRev = sumRight.SumValueWith(sumLeft);");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("sumNativeRev").GetValue<int>(), 42);

    o2Scripts.Eval("var newPair = new o2.TestScriptObject(3, 'x'); var sumConstructed = newPair.SumValueWith(sumLeft);");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("sumConstructed").GetValue<int>(), 13);
}

TEST(ScriptValue, NativeMethodTakesNativeObjectFromCpp) {
    auto a = mmake<TestScriptObject>();
    a->value = 7;
    auto b = mmake<TestScriptObject>();
    b->value = 5;

    ScriptValue svA = a->GetScriptValue();
    ScriptValue sumFn = svA.GetProperty("SumValueWith");
    EXPECT_TRUE(sumFn.IsFunction());

    int r = sumFn.Invoke<int>(svA, b);
    EXPECT_EQ(r, 12);
}

TEST(ScriptValue, NativeStoresRefPassedFromScript) {
    auto host = mmake<TestScriptObject>();
    host->value = 100;
    auto partner = mmake<TestScriptObject>();
    partner->value = 5;

    o2Scripts.GetGlobal().SetProperty("refHost", host->GetScriptValue());
    o2Scripts.GetGlobal().SetProperty("refPartner", partner->GetScriptValue());
    o2Scripts.CollectGarbage();

    o2Scripts.Eval("refHost.SetLinkedPartner(refPartner); var sumLinked = refHost.SumWithLinkedPartner();");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("sumLinked").GetValue<int>(), 105);

    partner->value = 10;
    o2Scripts.Eval("var sumLinked2 = refHost.SumWithLinkedPartner();");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("sumLinked2").GetValue<int>(), 110);
}

// Mirrors Assets/Scripts/Reel.js CreateImages loop body: AssetRefImageAsset, new Image, imageAsset, SetParent(Actor).
TEST(ScriptValue, ReelJsStyleImageWidgetCreateAndParent) {
    auto container = mmake<Actor>();
    o2Scripts.GetGlobal().SetProperty("reelImagesContainer", container->GetScriptValue());
    o2Scripts.CollectGarbage();

    o2Scripts.Eval(
        "var rotatingImage = { info: { regularImage: new o2.AssetRefImageAsset() }, image: null };"
        "var imageAsset = rotatingImage.info.regularImage;"
        "var img = new o2.Image();"
        "img.imageAsset = imageAsset;"
        "img.SetParent(reelImagesContainer, false);"
        "rotatingImage.image = img;"
    );

    EXPECT_EQ(container->GetChildren().Count(), 1);
    Ref<Image> imageChild = DynamicCast<Image>(container->GetChildren()[0]);
    EXPECT_TRUE(imageChild);
    EXPECT_EQ(imageChild->GetParent().Lock(), container);
}

TEST(ScriptValue, GetTransformReturnsLiveNativeObject) {
    auto image = mmake<Image>();
    o2Scripts.GetGlobal().SetProperty("liveTransformImage", image->GetScriptValue());
    o2Scripts.CollectGarbage();

    o2Scripts.Eval(
        "liveTransformImage.GetTransform().SetPivotX(0.25);"
        "liveTransformImage.GetTransform().SetPivotY(0.75);"
    );

    auto pivot = image.Get()->o2::Actor::GetTransform()->GetPivot();
    EXPECT_FLOAT_EQ(pivot.x, 0.25f);
    EXPECT_FLOAT_EQ(pivot.y, 0.75f);
}

TEST(ScriptValue, LargeOwnedContainerFallback) {
    LargeNativePayload payload;
    payload.value = 77;
    payload.padding[0] = 'A';
    payload.padding[std::size(payload.padding) - 1] = 'Z';

    ScriptValue value(payload);
    LargeNativePayload restored = value.GetValue<LargeNativePayload>();

    EXPECT_EQ(restored.value, 77);
    EXPECT_EQ(restored.padding[0], 'A');
    EXPECT_EQ(restored.padding[std::size(restored.padding) - 1], 'Z');
}

TEST(ScriptValue, NativeContainerPoolStressAndGarbageCollection) {
    const int objectsCount = 512;

    Vector<Ref<TestScriptObject>> objects;
    ScriptValue values = ScriptValue::EmptyArray();

    for (int i = 0; i < objectsCount; i++) {
        auto object = mmake<TestScriptObject>();
        object->value = i;
        object->score = (float)i*0.5f;
        objects.Add(object);

        values.AddElement(object->GetScriptValue());
        values.AddElement(ScriptValue(Function<int(int)>([base = i](int arg) {
            return base + arg;
        })));
    }

    o2Scripts.GetGlobal().SetProperty("pooledStressValues", values);
    o2Scripts.CollectGarbage();

    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("pooledStressValues").GetElement(0).GetProperty("value").GetValue<int>(), 0);
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("pooledStressValues").GetElement(1).Invoke<int>(5), 5);

    o2Scripts.Eval(
        "var pooledStressSum = 0;"
        "for (var i = 0; i < pooledStressValues.length; i += 2)"
        "    pooledStressSum += pooledStressValues[i].value;"
    );

    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("pooledStressSum").GetValue<int>(), objectsCount*(objectsCount - 1)/2);

    o2Scripts.GetGlobal().RemoveProperty(ScriptValue("pooledStressValues"));
    o2Scripts.GetGlobal().RemoveProperty(ScriptValue("pooledStressSum"));

    values = ScriptValue();
    objects.Clear();

    for (int i = 0; i < 5; i++)
        o2Scripts.CollectGarbage();

    SUCCEED();
}

#else

TEST(ScriptValue, ScriptingDisabled) {
    SUCCEED() << "Scripting is disabled in this build";
}

#endif // IS_SCRIPTING_SUPPORTED
