#pragma once

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)

#include "EditorTestStep.h"

#include "o2/Scripts/ScriptValue.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

namespace Editor::Tests
{
    enum class TestStatus
    {
        Pending,
        Running,
        Passed,
        Failed
    };

    struct TestContext
    {
        o2::String fileName;       // .js file name (no extension), used as fallback test id.
        o2::String name;           // Name from Test.register; falls back to fileName.
        o2::String filePath;       // Absolute path to the .js file.

        o2::Vector<TestStep> steps;
        int                  currentStep = 0;

        // Progress within the current step (frames left or seconds left).
        int   framesLeft  = 0;
        float secondsLeft = 0.0f;

        TestStatus status     = TestStatus::Pending;
        o2::String failReason;

        // Per-test JS realm (so globals are isolated between tests).
        o2::ScriptValue realm;

        bool screenshotTaken = false;
    };
}

#endif
