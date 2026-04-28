#include "EditorTestRunner.h"

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)

#include "EditorTestJSBindings.h"
#include "EditorTestMacLoopHelper.h"
#include "EditorTestScreenshot.h"

#include "o2/Scripts/ScriptEngine.h"
#include "o2/Scripts/ScriptValue.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Function/Function.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace Editor::Tests
{
    using namespace o2;

    EditorTestRunner::EditorTestRunner(const EditorTestArgs& args):
        mArgs(args)
    {}

    EditorTestRunner::~EditorTestRunner()
    {
        for (auto* t : mAllTests)
            delete t;
        mAllTests.Clear();
    }

    bool EditorTestRunner::Initialize()
    {
        mTestsDir   = mArgs.ResolveTestsDir();
        mOutputDir  = mArgs.ResolveOutputDir();
        o2FileSystem.FolderCreate(mOutputDir, true);

        String logPath = mOutputDir + "/run.log";
        mLogger.Init(mArgs.verbose, logPath);

        mLogger.Info(String("Tests dir:    ") + mTestsDir);
        mLogger.Info(String("Output dir:   ") + mOutputDir);
        mLogger.Info(String("Filter:       ") + (mArgs.testFilter.IsEmpty() ? String("(none)") : mArgs.testFilter));
        mLogger.Info(String("Verbose:      ") + (mArgs.verbose ? "yes" : "no"));
        mLogger.Info(String("Warmup frames: ") + (String)mArgs.warmupFrames);

        if (!o2FileSystem.IsFolderExist(mTestsDir))
        {
            mLogger.Err(String("Tests dir does not exist: ") + mTestsDir);
            return false;
        }

        ScanTestsDir();

        if (mTestFiles.IsEmpty())
        {
            mLogger.Err("No tests matched.");
            return false;
        }

        mLogger.Info(String("Discovered tests: ") + (String)mTestFiles.Count());
        for (auto& p : mTestFiles)
            mLogger.Verbose(String("  ") + p);

        mWarmupRemaining = mArgs.warmupFrames;
        mPhase = Phase::Warmup;
        return true;
    }

    void EditorTestRunner::ScanTestsDir()
    {
        FolderInfo info = o2FileSystem.GetFolderInfo(mTestsDir);
        for (auto& f : info.files)
        {
            String path = f.path;
            if (path.EndsWith(".js"))
            {
                String fileNameNoExt = o2::FileSystem::GetFileNameWithoutExtension(
                    o2::FileSystem::GetPathWithoutDirectories(path));
                if (MatchesFilter(fileNameNoExt))
                    mTestFiles.Add(path);
            }
        }
    }

    bool EditorTestRunner::MatchesFilter(const String& fileName) const
    {
        if (mArgs.testFilter.IsEmpty())
        {
            // Files starting with '_' are skipped unless explicitly selected.
            return !fileName.StartsWith("_");
        }
        return fileName == mArgs.testFilter;
    }

    namespace { EditorTestRunner* g_activeRunner = nullptr; }

    void EditorTestRunner::Schedule()
    {
        // EditorApplication::UpdateTaskManager only runs in Play mode, so we cannot
        // hook test progression via TaskManager. Drive Tick from a platform timer instead.
        g_activeRunner = this;
        StartTestTickTimer([](float dt) {
            if (g_activeRunner)
                g_activeRunner->Tick(dt);
        });
        mLogger.Info("Test runner scheduled. Awaiting first tick...");
    }

    void EditorTestRunner::Tick(float dt)
    {
        if (mPhase == Phase::Finished)
        {
            // Exit was scheduled on previous tick; perform it now so the final
            // log line gets a chance to flush.
            if (mExitScheduled)
                std::exit(mExitCode);
            return;
        }

        if (mPhase == Phase::Warmup)
        {
            if (mWarmupRemaining > 0)
            {
                mWarmupRemaining--;
                return;
            }
            mPhase = Phase::LoadNextTest;
            // fall through
        }

        if (mPhase == Phase::LoadNextTest)
        {
            if (mNextFileIdx >= mTestFiles.Count())
            {
                // All done. Print summary and schedule exit.
                int total = mAllTests.Count();
                int passed = 0, failed = 0;
                for (auto* t : mAllTests)
                {
                    if (t->status == TestStatus::Passed) passed++;
                    else failed++;
                }
                mLogger.Summary(total, passed, failed);
                if (failed > 0)
                {
                    mLogger.Info(String("Failed tests:"));
                    for (auto* t : mAllTests)
                    {
                        if (t->status != TestStatus::Passed)
                            mLogger.Info(String("  ") + t->name + ": " + t->failReason);
                    }
                }
                mExitCode = failed > 0 ? 1 : 0;
                mPhase = Phase::Finished;
                ScheduleExitOnNextTick();
                return;
            }

            LoadNextTest();
            return;
        }

        if (mPhase == Phase::RunStep && mCurrentTest)
        {
            TestContext* t = mCurrentTest;

            if (t->status == TestStatus::Failed)
            {
                FinalizeCurrentTest();
                return;
            }

            if (t->currentStep >= t->steps.Count())
            {
                t->status = TestStatus::Passed;
                FinalizeCurrentTest();
                return;
            }

            TestStep& step = t->steps[t->currentStep];
            switch (step.type)
            {
                case TestStepType::Function:
                {
                    mLogger.Verbose(String("[") + t->name + "] step #" + (String)t->currentStep);
                    Vector<ScriptValue> args;
                    ScriptValue result = step.function.InvokeRaw(args);
                    if (result.GetValueType() == ScriptValue::ValueType::Error)
                    {
                        t->failReason = String("JS error: ") + result.GetError();
                        t->status = TestStatus::Failed;
                        mLogger.Err(t->failReason);
                    }
                    t->currentStep++;
                    break;
                }
                case TestStepType::WaitFrames:
                {
                    if (t->framesLeft <= 0 && step.frames > 0)
                        t->framesLeft = step.frames;
                    t->framesLeft--;
                    if (t->framesLeft <= 0)
                    {
                        t->framesLeft = 0;
                        t->currentStep++;
                    }
                    break;
                }
                case TestStepType::WaitTime:
                {
                    if (t->secondsLeft <= 0.0f && step.seconds > 0.0f)
                        t->secondsLeft = step.seconds;
                    t->secondsLeft -= dt;
                    if (t->secondsLeft <= 0.0f)
                    {
                        t->secondsLeft = 0.0f;
                        t->currentStep++;
                    }
                    break;
                }
            }
        }
    }

    void EditorTestRunner::LoadNextTest()
    {
        String filePath = mTestFiles[mNextFileIdx++];
        String fileNameNoExt = o2::FileSystem::GetFileNameWithoutExtension(
            o2::FileSystem::GetPathWithoutDirectories(filePath));

        TestContext* ctx = mnew TestContext();
        ctx->fileName = fileNameNoExt;
        ctx->name = fileNameNoExt;  // overwritten by Test.register
        ctx->filePath = filePath;
        ctx->status = TestStatus::Running;
        mAllTests.Add(ctx);
        mCurrentTest = ctx;

        mLogger.Info(String("Loading test: ") + filePath);

        // Read file.
        std::ifstream is(filePath.Data());
        if (!is.is_open())
        {
            ctx->failReason = "failed to open file";
            ctx->status = TestStatus::Failed;
            FinalizeCurrentTest();
            return;
        }
        std::stringstream ss;
        ss << is.rdbuf();
        String src = ss.str().c_str();

        // Each test gets a fresh JS realm so globals from one test don't leak.
        ScriptValue prevRealm;
        ScriptValue newRealm = o2Scripts.CreateRealm();
        if (newRealm.GetValueType() != ScriptValue::ValueType::Error)
        {
            prevRealm = o2Scripts.SetCurrentRealm(newRealm);
            ctx->realm = newRealm;
        }
        else
        {
            mLogger.Warn("CreateRealm failed; using shared realm");
        }

        EditorTestJSBindings::Register(*this, *ctx);

        ScriptValue result = o2Scripts.Eval(src, fileNameNoExt + ".js");

        if (prevRealm.GetValueType() != ScriptValue::ValueType::Undefined)
            o2Scripts.SetCurrentRealm(prevRealm);

        if (result.GetValueType() == ScriptValue::ValueType::Error)
        {
            ctx->failReason = String("Eval error: ") + result.GetError();
            ctx->status = TestStatus::Failed;
            FinalizeCurrentTest();
            return;
        }

        if (ctx->steps.IsEmpty() && ctx->status != TestStatus::Failed)
        {
            ctx->failReason = "Test did not call Test.register or registered no steps";
            ctx->status = TestStatus::Failed;
            FinalizeCurrentTest();
            return;
        }

        mLogger.Info(String("Running test: ") + ctx->name + " (steps=" + (String)ctx->steps.Count() + ")");
        mPhase = Phase::RunStep;
    }

    void EditorTestRunner::FinalizeCurrentTest()
    {
        if (!mCurrentTest)
            return;

        TestContext* t = mCurrentTest;

        if (t->status == TestStatus::Failed)
        {
            if (mArgs.screenshotOnFail && !t->screenshotTaken)
                TakeScreenshot("fail");
            mLogger.Result(false, t->name, t->failReason);
        }
        else
        {
            mLogger.Result(true, t->name);
        }

        // Reactivate global realm for the next eval.
        // (LoadNextTest will switch realms again.)

        mCurrentTest = nullptr;
        mPhase = Phase::LoadNextTest;
    }

    void EditorTestRunner::ScheduleExitOnNextTick()
    {
        mExitScheduled = true;
    }

    bool EditorTestRunner::TakeScreenshot(const String& label)
    {
        String testName = mCurrentTest ? mCurrentTest->name : String("global");
        String safeLabel = label;
        // sanitize spaces and slashes
        for (int i = 0; i < safeLabel.Length(); ++i)
        {
            char c = safeLabel[i];
            if (c == ' ' || c == '/' || c == '\\') safeLabel[i] = '_';
        }
        String path = mOutputDir + "/" + testName + "_" + safeLabel + ".png";
        bool ok = SaveScreenshot(path);
        if (ok)
        {
            mLogger.Info(String("Screenshot: ") + path);
            if (mCurrentTest) mCurrentTest->screenshotTaken = true;
        }
        else
        {
            mLogger.Warn(String("Screenshot failed: ") + path);
        }
        return ok;
    }
}

#endif
