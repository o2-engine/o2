#include "o2/stdafx.h"
#include "o2/O2.h"
#include "o2/Application/Application.h"
#include "o2/EngineSettings.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include <gtest/gtest.h>

#include "Support/RenderedTestAssets.h"

#include <filesystem>
#ifdef _WIN32
#include <process.h>
#define o2_test_getpid _getpid
#else
#include <unistd.h>
#define o2_test_getpid getpid
#endif

extern void InitializeTypeso2TestsSupport();
extern void InitializeTypeso2RenderTests();

using namespace o2;

namespace o2::RenderedTests
{
    namespace
    {
        String& MutableTestAssetsRoot()
        {
            static String path;
            return path;
        }

        // Lays down a small fixed asset tree so file/asset tests have a known starting
        // shape. Tests that mutate the tree are expected to clean up after themselves.
        void SeedTestAssets(const String& root)
        {
            o2FileSystem.FolderRemove(root);
            o2FileSystem.FolderCreate(root, true);

            o2FileSystem.FolderCreate(root + "Textures", true);
            o2FileSystem.FolderCreate(root + "Data", true);
            o2FileSystem.FolderCreate(root + "Data/Sub", true);

            FileSystem::WriteFile(root + "readme.txt", "rendered tier test fixture");
            FileSystem::WriteFile(root + "Data/sample.json", "{\"key\":42}");
            FileSystem::WriteFile(root + "Data/Sub/nested.txt", "deep");
            FileSystem::WriteFile(root + "Textures/empty.bin", "");
        }

        String DeriveTestAssetsRoot(const char* argv0)
        {
            std::filesystem::path exe = argv0 ? std::filesystem::path(argv0)
                                              : std::filesystem::current_path() / "o2RenderTests";
            std::filesystem::path dir = exe.has_parent_path() ? exe.parent_path()
                                                              : std::filesystem::current_path();
            // Per-PID leaf so concurrent ctest subprocesses (each gtest case in its own
            // subprocess under --parallel N) get isolated sandboxes that can't race on
            // each other's seed/teardown.
            std::string leaf = "RenderTestAssets_" + std::to_string(o2_test_getpid());
            std::filesystem::path root = dir / leaf;
            // generic_string() forces forward-slash separators. o2FileSystem helpers
            // (ExtractPathStr, FolderCreate's recursive parent-walk) only split on '/',
            // so a backslash-only path breaks the recursive create on Windows.
            String s(root.generic_string());
            if (!s.IsEmpty() && s[s.Length() - 1] != '/')
                s += "/";
            return s;
        }
    }

    const String& GetTestAssetsRoot()
    {
        return MutableTestAssetsRoot();
    }
}

// Rendered-tier test runner. Full Application::Initialize — window, render device,
// FreeType, UI styles. Use for Render/Sprite/Camera/Material tests and UI widget
// tests that require the styles asset to be loaded.
//
// Unlike the headless Systems runner, this binary points o2 at a dedicated sandbox
// assets directory ("RenderTestAssets/" next to the executable) via
// SetAssetsPathOverride(). Tests can use o2Assets.GetAssetsPath() to read or write
// files in that sandbox without touching the project's real Assets/.
int main(int argc, char** argv)
{
    // Builtin-asset paths (GetBuiltinAssetsPath() = "../../BuiltAssets/Windows/FrameworkData/")
    // are resolved relative to the cwd. ctest sets WORKING_DIRECTORY to the exe folder, but a
    // direct invocation from any other cwd breaks shader/font/Math.js loading. Pin cwd here so
    // both paths behave identically.
    if (argc > 0 && argv[0])
    {
        std::filesystem::path exe(argv[0]);
        if (exe.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::current_path(exe.parent_path(), ec);
        }
    }

    InitializeTypeso2TestsSupport();
    InitializeTypeso2RenderTests();
    INITIALIZE_O2;

    ::testing::InitGoogleTest(&argc, argv);

    bool listOnly = ::testing::GTEST_FLAG(list_tests);

    Ref<Application> app;
    if (!listOnly)
    {
        String root = o2::RenderedTests::DeriveTestAssetsRoot(argc > 0 ? argv[0] : nullptr);
        o2::RenderedTests::MutableTestAssetsRoot() = root;

        // Seed before Initialize so the sandbox dir exists by the time anything
        // downstream observes the override path. o2FileSystem is a static singleton
        // (CREATE_SINGLETON in Integration.cpp), so it's safe to use this early.
        o2::RenderedTests::SeedTestAssets(root);
        SetAssetsPathOverride(root.Data());

        app = mmake<Application>();
        app->Initialize();
        // Multithreaded rendering is enabled by default (Integration::InitiazeRender) on supported
        // platforms, so the whole rendered suite — including pixel/screenshot checks — validates it
    }

    int result = RUN_ALL_TESTS();

    if (app)
    {
        app->Deinitialize();
        o2FileSystem.FolderRemove(o2::RenderedTests::GetTestAssetsRoot());
        SetAssetsPathOverride(nullptr);
    }

    return result;
}
