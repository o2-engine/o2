#pragma once

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)

#include "o2/Scripts/ScriptValue.h"

namespace Editor::Tests
{
    enum class TestStepType
    {
        Function,    // Run JS function once.
        WaitFrames,  // Skip N frames before next step.
        WaitTime     // Skip frames until <seconds> elapsed.
    };

    struct TestStep
    {
        TestStepType type = TestStepType::Function;

        // For Function: the JS callable.
        o2::ScriptValue function;

        // For WaitFrames.
        int frames = 0;

        // For WaitTime.
        float seconds = 0.0f;

        static TestStep MakeFunction(const o2::ScriptValue& fn)
        {
            TestStep s;
            s.type = TestStepType::Function;
            s.function = fn;
            return s;
        }

        static TestStep MakeWaitFrames(int n)
        {
            TestStep s;
            s.type = TestStepType::WaitFrames;
            s.frames = n;
            return s;
        }

        static TestStep MakeWaitTime(float sec)
        {
            TestStep s;
            s.type = TestStepType::WaitTime;
            s.seconds = sec;
            return s;
        }
    };
}

#endif
