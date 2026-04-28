#include "EditorTestArgs.h"

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)

#include "o2/Integration.h"
#include "o2/Utils/FileSystem/FileSystem.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <cstring>

namespace Editor::Tests
{
    static bool MatchFlag(const char* arg, const char* expected)
    {
        return std::strcmp(arg, expected) == 0;
    }

    static o2::String GetTimestamp()
    {
        std::time_t t = std::time(nullptr);
        std::tm tm{};
#if defined(PLATFORM_WINDOWS)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
        return oss.str().c_str();
    }

    bool EditorTestArgs::ParseFromArgv(int argc, char** argv, EditorTestArgs& out)
    {
        if (argc > 0 && argv[0])
            out.argv0 = argv[0];

        for (int i = 1; i < argc; ++i)
        {
            const char* arg = argv[i];

            if (MatchFlag(arg, "--run-tests"))
            {
                out.runTests = true;
            }
            else if (MatchFlag(arg, "--test") && i + 1 < argc)
            {
                out.testFilter = argv[++i];
                out.runTests = true;
            }
            else if (MatchFlag(arg, "--tests-dir") && i + 1 < argc)
            {
                out.testsDir = argv[++i];
            }
            else if (MatchFlag(arg, "--tests-output") && i + 1 < argc)
            {
                out.outputDir = argv[++i];
            }
            else if (MatchFlag(arg, "--tests-verbose"))
            {
                out.verbose = true;
            }
            else if (MatchFlag(arg, "--tests-no-screenshot-on-fail"))
            {
                out.screenshotOnFail = false;
            }
            else if (MatchFlag(arg, "--tests-warmup") && i + 1 < argc)
            {
                out.warmupFrames = std::atoi(argv[++i]);
                if (out.warmupFrames < 0)
                    out.warmupFrames = 0;
            }
        }
        return out.runTests;
    }

    static o2::String GetExecutableDir(const o2::String& argv0)
    {
        try
        {
            namespace fs = std::filesystem;
            if (argv0.IsEmpty())
                return fs::current_path().string().c_str();

            fs::path p(argv0.Data());
            if (!p.is_absolute())
                p = fs::current_path() / p;
            std::error_code ec;
            fs::path canon = fs::canonical(p, ec);
            if (ec)
                canon = p;
            return canon.parent_path().string().c_str();
        }
        catch (...)
        {
            return o2::String();
        }
    }

    static o2::String AscendToProjectRoot(const o2::String& exeDir)
    {
        // Bin layout: <root>/Bin/<Platform>/Editor — go two levels up.
        if (exeDir.IsEmpty())
            return o2::String();
        namespace fs = std::filesystem;
        try
        {
            fs::path p(exeDir.Data());
            return (p / "../..").lexically_normal().string().c_str();
        }
        catch (...)
        {
            return o2::String();
        }
    }

    o2::String EditorTestArgs::ResolveTestsDir() const
    {
        if (!testsDir.IsEmpty())
            return testsDir;

        // Default: built-in editor sample tests live in <project_root>/o2/Tests/EditorIntegration.
        // The hosting project (e.g. PetStory) can override via --tests-dir.
        o2::String exeDir = GetExecutableDir(argv0);
        o2::String root = AscendToProjectRoot(exeDir);
        if (!root.IsEmpty())
            return root + "/o2/Tests/EditorIntegration";
        return "o2/Tests/EditorIntegration";
    }

    o2::String EditorTestArgs::ResolveOutputDir() const
    {
        if (!outputDir.IsEmpty())
            return outputDir;

        o2::String exeDir = GetExecutableDir(argv0);
        o2::String root = AscendToProjectRoot(exeDir);
        if (!root.IsEmpty())
            return root + "/Tests/Results/" + GetTimestamp();
        return o2::String("Tests/Results/") + GetTimestamp();
    }
}

#endif
