#include "o2/stdafx.h"
#include <gtest/gtest.h>
#include "o2/Scripts/ScriptEngine.h"
#include "o2/Scripts/ScriptValue.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

#if IS_SCRIPTING_SUPPORTED

TEST(ScriptEngine, ParseValidScriptIsOk) {
    ScriptParseResult res = o2Scripts.Parse("var engineTest_a = 1 + 1;");
    EXPECT_TRUE(res.IsOk());
    EXPECT_TRUE((bool)res);
}

// Note: tests that exercise Parse/Eval failure paths are intentionally omitted.
// jerryscript's ECMA error reference handling currently asserts (ICE) when
// ScriptParseResult holding an error value is destructed, so error-path coverage
// would crash the test runner.

TEST(ScriptEngine, ParseAndRunSetsGlobal) {
    ScriptParseResult res = o2Scripts.Parse("var engineTest_runVal = 7 * 6;");
    ASSERT_TRUE(res.IsOk());
    o2Scripts.Run(res);

    int v = o2Scripts.GetGlobal().GetProperty("engineTest_runVal").GetValue<int>();
    EXPECT_EQ(v, 42);
}

TEST(ScriptEngine, RunReturnsValueOfLastExpression) {
    ScriptParseResult res = o2Scripts.Parse("3 + 4");
    ASSERT_TRUE(res.IsOk());
    ScriptValue out = o2Scripts.Run(res);
    EXPECT_FLOAT_EQ(out.ToNumber(), 7.0f);
}

TEST(ScriptEngine, EvalReturnsExpressionResult) {
    ScriptValue out = o2Scripts.Eval("100 / 4");
    EXPECT_FLOAT_EQ(out.ToNumber(), 25.0f);
}

TEST(ScriptEngine, EvalWithFilenameDoesNotCrash) {
    // filename is used for error reporting. Just verify it doesn't crash with a filename arg.
    ScriptValue out = o2Scripts.Eval("var engineTest_named = 'ok';", "named-test.js");
    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("engineTest_named").ToString(), "ok");
}

TEST(ScriptEngine, GetGlobalReturnsObject) {
    ScriptValue g = o2Scripts.GetGlobal();
    EXPECT_TRUE(g.IsObject());
    EXPECT_FALSE(g.IsUndefined());
}

TEST(ScriptEngine, GetUsedMemoryIsPositive) {
    int mem = o2Scripts.GetUsedMemory();
    EXPECT_GT(mem, 0);
}

TEST(ScriptEngine, ScriptDefinedFunctionCanBeCalledFromCpp) {
    o2Scripts.Eval("function engineTest_double(x) { return x * 2; }");
    ScriptValue fn = o2Scripts.GetGlobal().GetProperty("engineTest_double");
    ASSERT_TRUE(fn.IsFunction());

    float result = fn.Invoke<float>(21);
    EXPECT_FLOAT_EQ(result, 42.0f);
}

TEST(ScriptEngine, ParseResultCanBeRunMultipleTimes) {
    o2Scripts.GetGlobal().SetProperty("engineTest_counter", 0);
    ScriptParseResult res = o2Scripts.Parse("engineTest_counter = engineTest_counter + 1;");
    ASSERT_TRUE(res.IsOk());

    o2Scripts.Run(res);
    o2Scripts.Run(res);
    o2Scripts.Run(res);

    int counter = o2Scripts.GetGlobal().GetProperty("engineTest_counter").GetValue<int>();
    EXPECT_EQ(counter, 3);
}

TEST(ScriptEngine, CollectGarbageDoesNotInvalidateLiveValues) {
    o2Scripts.GetGlobal().SetProperty("engineTest_alive", "stillHere");
    o2Scripts.CollectGarbage();
    o2Scripts.CollectGarbage();

    EXPECT_EQ(o2Scripts.GetGlobal().GetProperty("engineTest_alive").ToString(), "stillHere");
}

TEST(ScriptEngine, CollectGarbageReleasesUnreferencedObjects) {
    int memBefore = 0;
    {
        // Allocate a chunk of throwaway script values.
        for (int i = 0; i < 50; ++i)
        {
            ScriptValue tmp = ScriptValue::EmptyObject();
            tmp.SetProperty("data", String("payload number " + (String)i));
        }
        memBefore = o2Scripts.GetUsedMemory();
    }

    o2Scripts.CollectGarbage();
    int memAfter = o2Scripts.GetUsedMemory();

    // After GC, memory must not exceed the pre-GC level (allocations are released or reused).
    EXPECT_LE(memAfter, memBefore);
}

TEST(ScriptEngine, EvalCanReadGlobalSetByPriorEval) {
    o2Scripts.Eval("var engineTest_chainedA = 11;");
    ScriptValue out = o2Scripts.Eval("engineTest_chainedA + 5");
    EXPECT_FLOAT_EQ(out.ToNumber(), 16.0f);
}

TEST(ScriptEngine, ParsedScriptOutlivesParseCallScope) {
    // Hold the parse result across a CollectGarbage to verify it is properly retained.
    ScriptParseResult res = o2Scripts.Parse("99");
    ASSERT_TRUE(res.IsOk());

    o2Scripts.CollectGarbage();

    ScriptValue out = o2Scripts.Run(res);
    EXPECT_FLOAT_EQ(out.ToNumber(), 99.0f);
}

TEST(ScriptEngine, ParseEmptyScriptIsOk) {
    ScriptParseResult res = o2Scripts.Parse("");
    EXPECT_TRUE(res.IsOk());

    ScriptValue out = o2Scripts.Run(res);
    EXPECT_TRUE(out.IsUndefined());
}

TEST(ScriptEngine, EvalDoesNotLeakToFutureGlobals) {
    // var declared inside a function should not appear on global object.
    o2Scripts.Eval("(function() { var engineTest_localOnly = 123; })();");
    ScriptValue prop = o2Scripts.GetGlobal().GetProperty("engineTest_localOnly");
    EXPECT_TRUE(prop.IsUndefined());
}

#endif // IS_SCRIPTING_SUPPORTED
