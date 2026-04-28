#include "EditorTestJSBindings.h"

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)

#include "EditorTestContext.h"
#include "EditorTestLogger.h"
#include "EditorTestRunner.h"
#include "EditorTestScreenshot.h"
#include "EditorTestStep.h"
#include "EditorTestUIActions.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scripts/ScriptEngine.h"
#include "o2/Scripts/ScriptValue.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Function/Function.h"

namespace Editor::Tests
{
    using namespace o2;

    static const char* kStepTypeKey   = "__o2TestStepType";
    static const char* kFramesKey     = "frames";
    static const char* kSecondsKey    = "seconds";
    static const char* kWaitFramesTag = "waitFrames";
    static const char* kWaitTimeTag   = "waitTime";

    static TestStep ParseStep(const ScriptValue& v)
    {
        if (v.IsFunction())
            return TestStep::MakeFunction(v);

        if (v.IsObject())
        {
            ScriptValue typeMarker = v.GetProperty(kStepTypeKey);
            String t = typeMarker.IsUndefined() ? String() : typeMarker.ToString();
            if (t == kWaitFramesTag)
            {
                int n = (int)v.GetProperty(kFramesKey).ToNumber();
                return TestStep::MakeWaitFrames(n);
            }
            if (t == kWaitTimeTag)
            {
                float s = v.GetProperty(kSecondsKey).ToNumber();
                return TestStep::MakeWaitTime(s);
            }
        }

        TestStep step;
        step.type = TestStepType::Function;
        return step;
    }

    void EditorTestJSBindings::Register(EditorTestRunner& runner, TestContext& ctx)
    {
        ScriptValue global = o2Scripts.GetGlobal();

        TestContext* ctxPtr = &ctx;
        EditorTestRunner* runnerPtr = &runner;

        // ---- Test object ----
        ScriptValue test = ScriptValue::EmptyObject();

        test.SetProperty("register",
            Function<void(String, ScriptValue)>(
                [ctxPtr](String name, ScriptValue steps) {
                    if (!name.IsEmpty())
                        ctxPtr->name = name;
                    if (!steps.IsArray())
                    {
                        ctxPtr->failReason = "Test.register: second argument must be an array";
                        ctxPtr->status = TestStatus::Failed;
                        return;
                    }
                    int n = steps.GetLength();
                    for (int i = 0; i < n; ++i)
                    {
                        ScriptValue elem = steps.GetElement(i);
                        ctxPtr->steps.Add(ParseStep(elem));
                    }
                }));

        test.SetProperty("waitFrames",
            Function<ScriptValue(int)>(
                [](int frames) {
                    ScriptValue m = ScriptValue::EmptyObject();
                    m.SetProperty(kStepTypeKey, ScriptValue(String(kWaitFramesTag)));
                    m.SetProperty(kFramesKey, ScriptValue(frames));
                    return m;
                }));

        test.SetProperty("waitTime",
            Function<ScriptValue(float)>(
                [](float seconds) {
                    ScriptValue m = ScriptValue::EmptyObject();
                    m.SetProperty(kStepTypeKey, ScriptValue(String(kWaitTimeTag)));
                    m.SetProperty(kSecondsKey, ScriptValue(seconds));
                    return m;
                }));

        test.SetProperty("log",
            Function<void(String)>(
                [runnerPtr](String msg) {
                    runnerPtr->GetLogger().Verbose(String("[js] ") + msg);
                }));

        test.SetProperty("info",
            Function<void(String)>(
                [runnerPtr](String msg) {
                    runnerPtr->GetLogger().Info(String("[js] ") + msg);
                }));

        test.SetProperty("warn",
            Function<void(String)>(
                [runnerPtr](String msg) {
                    runnerPtr->GetLogger().Warn(String("[js] ") + msg);
                }));

        test.SetProperty("error",
            Function<void(String)>(
                [runnerPtr](String msg) {
                    runnerPtr->GetLogger().Err(String("[js] ") + msg);
                }));

        test.SetProperty("assert",
            Function<void(ScriptValue, String)>(
                [ctxPtr, runnerPtr](ScriptValue cond, String msg) {
                    if (!cond.ToBool())
                    {
                        ctxPtr->failReason = msg.IsEmpty() ? String("assertion failed") : msg;
                        ctxPtr->status = TestStatus::Failed;
                        runnerPtr->GetLogger().Err(String("[assert] ") + ctxPtr->failReason);
                    }
                }));

        test.SetProperty("assertEqual",
            Function<void(ScriptValue, ScriptValue, String)>(
                [ctxPtr, runnerPtr](ScriptValue a, ScriptValue b, String msg) {
                    bool eq = (a == b) || (a.ToString() == b.ToString());
                    if (!eq)
                    {
                        String reason = String("assertEqual: ") + a.ToString() + " != " + b.ToString();
                        if (!msg.IsEmpty())
                            reason = msg + " (" + reason + ")";
                        ctxPtr->failReason = reason;
                        ctxPtr->status = TestStatus::Failed;
                        runnerPtr->GetLogger().Err(String("[assert] ") + reason);
                    }
                }));

        test.SetProperty("assertNotNull",
            Function<void(ScriptValue, String)>(
                [ctxPtr, runnerPtr](ScriptValue v, String msg) {
                    auto t = v.GetValueType();
                    if (t == ScriptValue::ValueType::Null || t == ScriptValue::ValueType::Undefined)
                    {
                        ctxPtr->failReason = msg.IsEmpty() ? String("value is null/undefined") : msg;
                        ctxPtr->status = TestStatus::Failed;
                        runnerPtr->GetLogger().Err(String("[assert] ") + ctxPtr->failReason);
                    }
                }));

        test.SetProperty("fail",
            Function<void(String)>(
                [ctxPtr, runnerPtr](String msg) {
                    ctxPtr->failReason = msg.IsEmpty() ? String("Test.fail()") : msg;
                    ctxPtr->status = TestStatus::Failed;
                    runnerPtr->GetLogger().Err(String("[fail] ") + ctxPtr->failReason);
                }));

        test.SetProperty("screenshot",
            Function<bool(String)>(
                [runnerPtr](String label) -> bool {
                    return runnerPtr->TakeScreenshot(label);
                }));

        global.SetProperty("Test", test);

        // ---- EditorUI object ----
        ScriptValue editorUI = ScriptValue::EmptyObject();

        editorUI.SetProperty("find",
            Function<Ref<Widget>(String)>(
                [](String path) {
                    return FindWidgetByPath(path);
                }));

        editorUI.SetProperty("findByName",
            Function<Ref<Widget>(String)>(
                [](String name) {
                    return FindWidgetByName(name);
                }));

        editorUI.SetProperty("findByType",
            Function<Ref<Widget>(String)>(
                [](String typeName) {
                    auto vec = FindWidgetsByType(typeName);
                    return vec.IsEmpty() ? nullptr : vec[0];
                }));

        editorUI.SetProperty("findCount",
            Function<int(String)>(
                [](String typeName) {
                    return FindWidgetsByType(typeName).Count();
                }));

        editorUI.SetProperty("dump",
            Function<String()>(
                [runnerPtr]() {
                    String d = DumpUITree();
                    runnerPtr->GetLogger().Verbose(d);
                    return d;
                }));

        editorUI.SetProperty("click",
            Function<bool(Ref<Widget>)>(
                [](Ref<Widget> w) {
                    return ClickWidget(w);
                }));

        editorUI.SetProperty("beginClick",
            Function<bool(Ref<Widget>)>(
                [](Ref<Widget> w) {
                    return BeginClickWidget(w);
                }));

        editorUI.SetProperty("endClick",
            Function<void()>(
                []() { EndClickWidget(); }));

        editorUI.SetProperty("mouseMove",
            Function<void(float, float)>(
                [](float x, float y) {
                    MouseMoveTo(Vec2F(x, y));
                }));

        editorUI.SetProperty("mouseDown",
            Function<void(float, float)>(
                [](float x, float y) {
                    MouseDown(Vec2F(x, y));
                }));

        editorUI.SetProperty("mouseUp",
            Function<void()>(
                []() { MouseUp(); }));

        editorUI.SetProperty("typeText",
            Function<void(String)>(
                [](String text) { TypeText(text); }));

        editorUI.SetProperty("keyPress",
            Function<void(int)>(
                [](int code) { KeyPress(code); }));

        global.SetProperty("EditorUI", editorUI);

        // ---- Scene object ----
        ScriptValue scene = ScriptValue::EmptyObject();

        scene.SetProperty("find",
            Function<Ref<Actor>(String)>(
                [](String path) {
                    return o2Scene.FindActor(path);
                }));

        scene.SetProperty("dump",
            Function<String()>(
                [runnerPtr]() {
                    String d = DumpSceneTree();
                    runnerPtr->GetLogger().Verbose(d);
                    return d;
                }));

        global.SetProperty("Scene", scene);
    }
}

#endif
