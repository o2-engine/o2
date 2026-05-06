#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Assets.h"
#include "o2/Utils/FileSystem/File.h"
#include "o2/Utils/FileSystem/FileInfo.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Types/String.h"

#include "Support/RenderedTestAssets.h"

using namespace o2;

namespace
{
    String Join(const String& root, const String& tail)
    {
        if (root.IsEmpty())
            return tail;
        char back = root[root.Length() - 1];
        if (back == '/' || back == '\\')
            return root + tail;
        return root + "/" + tail;
    }

    // Files created by tests get a per-test prefix to avoid stepping on the seed
    // tree set up by TestsMain or on each other when gtest is run with -shuffle.
    String ScratchPath(const String& leaf)
    {
        return Join(o2::RenderedTests::GetTestAssetsRoot(), leaf);
    }
}

// ===== Sandbox plumbing =====

TEST(RenderedAssetsSandbox, RootIsConfigured)
{
    const String& root = o2::RenderedTests::GetTestAssetsRoot();
    ASSERT_FALSE(root.IsEmpty()) << "Test assets root must be initialized by TestsMain";
    EXPECT_TRUE(o2FileSystem.IsFolderExist(root));
}

TEST(RenderedAssetsSandbox, AssetsSingletonReportsSandboxPath)
{
    EXPECT_EQ(o2Assets.GetAssetsPath(), o2::RenderedTests::GetTestAssetsRoot());
}

// ===== Seeded fixture is visible =====

TEST(RenderedAssetsSandbox, SeedFilesExist)
{
    EXPECT_TRUE(o2FileSystem.IsFileExist(ScratchPath("readme.txt")));
    EXPECT_TRUE(o2FileSystem.IsFileExist(ScratchPath("Data/sample.json")));
    EXPECT_TRUE(o2FileSystem.IsFileExist(ScratchPath("Data/Sub/nested.txt")));
    EXPECT_TRUE(o2FileSystem.IsFolderExist(ScratchPath("Textures")));
}

TEST(RenderedAssetsSandbox, SeedFileContentsRoundTrip)
{
    EXPECT_EQ(FileSystem::ReadFile(ScratchPath("readme.txt")), "rendered tier test fixture");
    EXPECT_EQ(FileSystem::ReadFile(ScratchPath("Data/sample.json")), "{\"key\":42}");
    EXPECT_EQ(FileSystem::ReadFile(ScratchPath("Data/Sub/nested.txt")), "deep");
}

TEST(RenderedAssetsSandbox, EmptySeedFileExistsButHasZeroSize)
{
    String path = ScratchPath("Textures/empty.bin");
    ASSERT_TRUE(o2FileSystem.IsFileExist(path));
    FileInfo info = o2FileSystem.GetFileInfo(path);
    EXPECT_NE(info.path, "invalid_file");
    EXPECT_EQ(info.size, 0);
}

TEST(RenderedAssetsSandbox, FolderInfoEnumeratesSeedTree)
{
    FolderInfo info = o2FileSystem.GetFolderInfo(o2::RenderedTests::GetTestAssetsRoot());

    EXPECT_GE(info.files.Count(), 1);
    EXPECT_GE(info.folders.Count(), 2);

    bool dataFound = false, texturesFound = false;
    for (auto& sub : info.folders)
    {
        if (sub.path.Contains("Data"))
            dataFound = true;
        if (sub.path.Contains("Textures"))
            texturesFound = true;
    }

    EXPECT_TRUE(dataFound);
    EXPECT_TRUE(texturesFound);
}

TEST(RenderedAssetsSandbox, FolderInfoFindsSeededFileRecursively)
{
    FolderInfo info = o2FileSystem.GetFolderInfo(o2::RenderedTests::GetTestAssetsRoot());
    EXPECT_TRUE(info.IsFileExist(ScratchPath("Data/Sub/nested.txt")));
}

// ===== Mutation against the sandbox =====

TEST(RenderedAssetsSandbox, WriteThenReadInSandbox)
{
    String path = ScratchPath("scratch_write.txt");
    String payload = "rendered tier mutation";

    FileSystem::WriteFile(path, payload);
    EXPECT_TRUE(o2FileSystem.IsFileExist(path));
    EXPECT_EQ(FileSystem::ReadFile(path), payload);

    o2FileSystem.FileDelete(path);
    EXPECT_FALSE(o2FileSystem.IsFileExist(path));
}

TEST(RenderedAssetsSandbox, FolderCreateRecursiveBelowSandbox)
{
    String nested = ScratchPath("scratch_dir/a/b/c");

    EXPECT_TRUE(o2FileSystem.FolderCreate(nested, true));
    EXPECT_TRUE(o2FileSystem.IsFolderExist(nested));

    o2FileSystem.FolderRemove(ScratchPath("scratch_dir"));
    EXPECT_FALSE(o2FileSystem.IsFolderExist(ScratchPath("scratch_dir")));
}

TEST(RenderedAssetsSandbox, FileCopyAndMoveInsideSandbox)
{
    String src = ScratchPath("scratch_copy_src.txt");
    String copy = ScratchPath("scratch_copy_dst.txt");
    String moved = ScratchPath("scratch_moved/leaf.txt");

    FileSystem::WriteFile(src, "rendered-tier-payload");

    EXPECT_TRUE(o2FileSystem.FileCopy(src, copy));
    EXPECT_TRUE(o2FileSystem.IsFileExist(copy));
    EXPECT_EQ(FileSystem::ReadFile(copy), "rendered-tier-payload");

    EXPECT_TRUE(o2FileSystem.FileMove(copy, moved));
    EXPECT_FALSE(o2FileSystem.IsFileExist(copy));
    EXPECT_TRUE(o2FileSystem.IsFileExist(moved));
    EXPECT_EQ(FileSystem::ReadFile(moved), "rendered-tier-payload");

    o2FileSystem.FileDelete(src);
    o2FileSystem.FolderRemove(ScratchPath("scratch_moved"));
}

TEST(RenderedAssetsSandbox, OutFileInFileRoundTripInSandbox)
{
    String path = ScratchPath("scratch_io.bin");

    {
        OutFile out(path);
        ASSERT_TRUE(out.IsOpened());
        const char payload[] = "rendered-bytes";
        out.WriteData(payload, sizeof(payload) - 1);
    }

    {
        InFile in(path);
        ASSERT_TRUE(in.IsOpened());
        EXPECT_EQ(in.GetDataSize(), 14u);

        char buf[32] = {};
        in.ReadData(buf, 14);
        EXPECT_STREQ(buf, "rendered-bytes");
    }

    o2FileSystem.FileDelete(path);
}

TEST(RenderedAssetsSandbox, MutationsDoNotEscapeSandbox)
{
    // Sanity: a path computed via GetAssetsPath() must stay rooted at the sandbox,
    // not at the project's real Assets/. If this regresses the override is broken.
    String assetsPath = o2Assets.GetAssetsPath();
    EXPECT_TRUE(assetsPath.Contains("RenderTestAssets"));
    EXPECT_FALSE(assetsPath == "../../Assets/");
}
