#pragma once

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)

#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

namespace Editor::Tests
{
    // Centralized logger for test runner. Responsibilities:
    //  - Bind a FileLogStream to o2Debug so all engine logs go to a per-run file.
    //  - Print test-specific output to stdout, respecting verbose/brief mode.
    //  - Accumulate the file path for screenshots and reports.
    class EditorTestLogger
    {
    public:
        EditorTestLogger();
        ~EditorTestLogger();

        // verbose=true: prints all messages (Verbose/Info/Warn/Err/Result) to stdout.
        // verbose=false: only Info/Warn/Err/Result.
        // logFilePath is the path of the run-wide log file; engine logs are tee'd here.
        bool Init(bool verbose, const o2::String& logFilePath);

        // Detaches the file stream from o2Debug. Called before destruction or app shutdown.
        void Shutdown();

        bool IsVerbose() const { return mVerbose; }

        // Verbose: only printed in verbose mode (debugging trace).
        void Verbose(const o2::String& msg);

        // Info: always printed (test progress, brief mode).
        void Info(const o2::String& msg);

        // Warn: always printed, prefixed with [WARN].
        void Warn(const o2::String& msg);

        // Err: always printed to stderr, prefixed with [ERROR].
        void Err(const o2::String& msg);

        // Per-test result line. passed=true → "[PASS] <name>"; false → "[FAIL] <name>: <reason>".
        void Result(bool passed, const o2::String& testName, const o2::String& reasonIfFailed = o2::String());

        // Final summary block.
        void Summary(int total, int passed, int failed);

    private:
        bool                mVerbose = false;
        bool                mInitialized = false;
        o2::Ref<o2::LogStream> mFileStream;
        o2::String          mLogFilePath;
    };
}

#endif
