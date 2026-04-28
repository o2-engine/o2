#pragma once

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)

#include "EditorTestArgs.h"
#include "EditorTestContext.h"
#include "EditorTestLogger.h"

#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

namespace Editor::Tests
{
    // Orchestrates the integration test run.
    // Lifecycle:
    //   1. construct with args
    //   2. Initialize() — set up logger, output dir, scan tests dir.
    //   3. Schedule() — register a per-frame task on TaskManager.
    //      The host should then proceed to app->Launch() so the regular
    //      main loop drives our task.
    //   4. The task drives the tests step by step and finally calls exit(code).
    class EditorTestRunner
    {
    public:
        explicit EditorTestRunner(const EditorTestArgs& args);
        ~EditorTestRunner();

        // Returns false if no tests were found.
        bool Initialize();

        // Registers a per-frame TaskManager task that drives the test run.
        // After this returns, the caller should hand control to app->Launch().
        void Schedule();

        EditorTestLogger& GetLogger() { return mLogger; }

        // Save a screenshot tagged with current test name + label.
        bool TakeScreenshot(const o2::String& label);

        // Output directory resolved at Initialize-time.
        const o2::String& GetOutputDir() const { return mOutputDir; }

    private:
        enum class Phase
        {
            Warmup,
            LoadNextTest,
            RunStep,
            Finished
        };

        void Tick(float dt);

        void LoadNextTest();
        void FinalizeCurrentTest();
        void ScheduleExitOnNextTick();

        void ScanTestsDir();
        bool MatchesFilter(const o2::String& fileName) const;

    private:
        EditorTestArgs   mArgs;
        EditorTestLogger mLogger;
        o2::String       mTestsDir;
        o2::String       mOutputDir;

        o2::Vector<o2::String> mTestFiles;     // queued absolute paths to .js files
        int                    mNextFileIdx = 0;

        TestContext* mCurrentTest = nullptr;
        o2::Vector<TestContext*> mAllTests;     // owned

        Phase mPhase           = Phase::Warmup;
        int   mWarmupRemaining = 0;
        bool  mExitScheduled   = false;
        int   mExitCode        = 0;
    };
}

#endif
