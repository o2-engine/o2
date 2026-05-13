#pragma once

#include <gtest/gtest.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#include <process.h>
#define o2_editor_fixture_getpid _getpid
#else
#include <unistd.h>
#define o2_editor_fixture_getpid getpid
#endif

#include "o2/EngineSettings.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Types/String.h"
#include "o2/Utils/Types/UID.h"
#include "o2Editor/Actions/AssetsTrash.h"

namespace Editor::Tests
{
    // Per-test fixture for asset-action tests. Redirects o2Assets to a private
    // absolute sandbox folder so actions can touch the filesystem safely.
    // Existing tests that don't inherit this fixture see the default assets path
    // and are unaffected.
    class AssetsActionsFixture: public ::testing::Test
    {
    protected:
        o2::String mSandboxRoot;

        void SetUp() override
        {
            namespace fs = std::filesystem;
            // Different test processes share rand() seeds, so UID() can collide
            // across processes when run in parallel. Seed once per fixture.
            static bool seeded = false;
            if (!seeded)
            {
                auto nowMs = std::chrono::duration_cast<std::chrono::microseconds>(
                                 std::chrono::system_clock::now().time_since_epoch())
                                 .count();
                std::srand(static_cast<unsigned>(nowMs ^ o2_editor_fixture_getpid()));
                seeded = true;
            }

            o2::String uniqLeaf = o2::String("EditorAssetsActions_") + (o2::String)o2::UID();
            fs::path leaf = fs::current_path() / uniqLeaf.Data();
            o2::String s(leaf.generic_string());
            if (!s.IsEmpty() && s[s.Length() - 1] != '/')
                s += "/";
            mSandboxRoot = s;

            std::error_code ec;
            std::filesystem::remove_all(mSandboxRoot.Data(), ec);
            std::filesystem::create_directories(mSandboxRoot.Data(), ec);
            SetAssetsPathOverride(mSandboxRoot.Data());
            Editor::AssetsTrash::SetRebuildAssetsAfterMutation(false);
        }

        void TearDown() override
        {
            Editor::AssetsTrash::SetRebuildAssetsAfterMutation(true);
            SetAssetsPathOverride(nullptr);

            std::error_code ec;
            std::filesystem::remove_all(mSandboxRoot.Data(), ec);
        }

        o2::String SandboxPath(const o2::String& leaf) const
        {
            return mSandboxRoot + leaf;
        }
    };
}
