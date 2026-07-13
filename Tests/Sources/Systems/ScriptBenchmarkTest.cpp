#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scripts/ScriptEngine.h"
#include "o2/Scripts/ScriptValue.h"
#include "o2/Utils/Math/Vector2.h"

#include <chrono>
#include <cstdio>

using namespace o2;

#if IS_SCRIPTING_SUPPORTED

namespace
{
    double MeasureMs(int repeats, const std::function<void()>& body)
    {
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < repeats; i++)
            body();

        auto end = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }

    void Report(const char* name, double ms)
    {
        printf("BENCH | %-38s | %10.2f ms\n", name, ms);
    }
}

// Opt-in backend benchmark: run with O2_SCRIPT_BENCHMARK=1, skipped otherwise
TEST(ScriptBenchmark, Backends)
{
    if (!getenv("O2_SCRIPT_BENCHMARK"))
        GTEST_SKIP() << "set O2_SCRIPT_BENCHMARK=1 to run";

    printf("BENCH | backend workload, lower is better\n");

    // Pure script execution
    o2Scripts.Eval("function benchFib(n) { return n < 2 ? n : benchFib(n - 1) + benchFib(n - 2); }");
    Report("js: fib(24) recursion x5", MeasureMs(5, [] {
        o2Scripts.Eval("benchFib(24)");
    }));

    Report("js: arithmetic loop 1M x5", MeasureMs(5, [] {
        o2Scripts.Eval("var s = 0; for (var i = 0; i < 1000000; i++) s += i * 3 % 7; s");
    }));

    Report("js: array push/index 200k x5", MeasureMs(5, [] {
        o2Scripts.Eval("var a = []; for (var i = 0; i < 200000; i++) a.push(i); var t = 0;"
                       "for (var i = 0; i < 200000; i++) t += a[i]; t");
    }));

    Report("js: object create/props 100k x5", MeasureMs(5, [] {
        o2Scripts.Eval("var t = 0; for (var i = 0; i < 100000; i++) { var o = { x: i, y: i * 2 }; t += o.x + o.y; } t");
    }));

    Report("js: string concat 20k x5", MeasureMs(5, [] {
        o2Scripts.Eval("var s = ''; for (var i = 0; i < 20000; i++) s += 'a'; s.length");
    }));

    // C++ <-> JS boundary
    auto global = o2Scripts.GetGlobal();

    Report("boundary: Set/GetProperty 20k", MeasureMs(20000, [&] {
        global.SetProperty("benchProp", 42);
        volatile int v = global.GetProperty("benchProp").GetValue<int>();
        (void)v;
    }));

    ScriptValue cachedKey("benchProp");
    ScriptValue cachedValue(42);
    Report("boundary: Set/GetProperty cached key 20k", MeasureMs(20000, [&] {
        global.SetProperty(cachedKey, cachedValue);
        volatile int v = global.GetProperty(cachedKey).GetValue<int>();
        (void)v;
    }));

    o2Scripts.Eval("function benchAdd(a, b) { return a + b; }");
    ScriptValue addFunc = global.GetProperty("benchAdd");
    Report("boundary: C++ invokes JS fn 20k", MeasureMs(20000, [&] {
        volatile float v = addFunc.Invoke<float>(10, 32.0f);
        (void)v;
    }));

    ScriptValue noThis;
    Vector<ScriptValue> prebuiltArgs { ScriptValue(10), ScriptValue(32.0f) };
    Report("boundary: InvokeRaw prebuilt args 20k", MeasureMs(20000, [&] {
        volatile float v = addFunc.InvokeRaw(noThis, prebuiltArgs).GetValue<float>();
        (void)v;
    }));

    global.SetProperty("benchNative", Function<float(int, float)>([](int a, float b) { return a + b; }));
    Report("boundary: JS invokes C++ fn 20k x5", MeasureMs(5, [] {
        o2Scripts.Eval("var t = 0; for (var i = 0; i < 20000; i++) t += benchNative(i, 0.5); t");
    }));

    Report("boundary: Vec2F converter 20k", MeasureMs(20000, [] {
        ScriptValue v(Vec2F(1.5f, 2.5f));
        volatile float x = v.GetValue<Vec2F>().x;
        (void)x;
    }));

    // Engine services
    const String mediumScript =
        "var benchAcc = 0;"
        "function benchHelper(v) { return v * 2 + 1; }"
        "for (var i = 0; i < 100; i++) benchAcc += benchHelper(i);";

    Report("engine: parse+run medium script x500", MeasureMs(500, [&] {
        auto parsed = o2Scripts.Parse(mediumScript);
        o2Scripts.Run(parsed);
    }));

    o2Scripts.Eval("var benchGarbage = []; for (var i = 0; i < 50000; i++) benchGarbage.push({ v: i });"
                   "benchGarbage = null;");
    Report("engine: CollectGarbage x10", MeasureMs(10, [] {
        o2Scripts.CollectGarbage();
    }));

    printf("BENCH | used memory: %d bytes\n", o2Scripts.GetUsedMemory());
}

#endif // IS_SCRIPTING_SUPPORTED
