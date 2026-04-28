#include "EditorTestLogger.h"

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)

#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/FileLogStream.h"
#include "o2/Utils/FileSystem/FileSystem.h"

#include <iostream>

namespace Editor::Tests
{
    EditorTestLogger::EditorTestLogger() = default;

    EditorTestLogger::~EditorTestLogger()
    {
        Shutdown();
    }

    bool EditorTestLogger::Init(bool verbose, const o2::String& logFilePath)
    {
        mVerbose = verbose;
        mLogFilePath = logFilePath;

        o2::String dir = o2FileSystem.ExtractPathStr(logFilePath);
        if (!dir.IsEmpty())
            o2FileSystem.FolderCreate(dir, true);

        // Standalone FileLogStream — not bound to o2Debug. Test-specific messages
        // go only to this file (no duplicate stdout output through the main log).
        // Engine logs continue to print to stdout normally.
        mFileStream = mmake<o2::FileLogStream>(o2::WString(), logFilePath);

        mInitialized = true;
        Verbose(o2::String("Test logger initialized, file: ") + logFilePath);
        return true;
    }

    void EditorTestLogger::Shutdown()
    {
        if (!mInitialized)
            return;
        mFileStream = nullptr;
        mInitialized = false;
    }

    // Write directly to the test file stream — avoids the duplicate stdout output
    // that o2Debug's main log would produce.
    void EditorTestLogger::Verbose(const o2::String& msg)
    {
        if (mFileStream)
            mFileStream->OutStr(o2::WString(o2::String("[V] ") + msg));
        if (mVerbose)
            std::cout << "[V] " << msg.Data() << std::endl;
    }

    void EditorTestLogger::Info(const o2::String& msg)
    {
        if (mFileStream)
            mFileStream->OutStr(o2::WString(o2::String("[I] ") + msg));
        std::cout << "[I] " << msg.Data() << std::endl;
    }

    void EditorTestLogger::Warn(const o2::String& msg)
    {
        if (mFileStream)
            mFileStream->WarningStr(o2::WString(msg));
        std::cout << "[WARN] " << msg.Data() << std::endl;
    }

    void EditorTestLogger::Err(const o2::String& msg)
    {
        if (mFileStream)
            mFileStream->ErrorStr(o2::WString(msg));
        std::cerr << "[ERROR] " << msg.Data() << std::endl;
    }

    void EditorTestLogger::Result(bool passed, const o2::String& testName, const o2::String& reasonIfFailed)
    {
        o2::String line = (passed ? "[PASS] " : "[FAIL] ") + testName;
        if (!passed && !reasonIfFailed.IsEmpty())
            line += o2::String(": ") + reasonIfFailed;
        if (mFileStream)
            mFileStream->OutStr(o2::WString(line));
        std::cout << line.Data() << std::endl;
    }

    void EditorTestLogger::Summary(int total, int passed, int failed)
    {
        o2::String line = o2::String("Tests: total=") + (o2::String)total +
                          " passed=" + (o2::String)passed +
                          " failed=" + (o2::String)failed;
        if (mFileStream)
            mFileStream->OutStr(o2::WString(line));
        std::cout << "\n==================================" << std::endl;
        std::cout << line.Data() << std::endl;
        std::cout << "==================================" << std::endl;
    }
}

#endif
