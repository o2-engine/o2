#include <gtest/gtest.h>

#include "o2/Assets/Assets.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Actions/AssetsTrash.h"

#include "support/AssetsActionsFixture.h"

using namespace o2;
using namespace Editor::Tests;

namespace
{
    void WriteText(const String& path, const String& content)
    {
        FileSystem::WriteFile(path, content);
    }
}

TEST_F(AssetsActionsFixture, AssetsTrash_GetRootIsSiblingOfAssets)
{
    String trash = Editor::AssetsTrash::GetRoot();
    EXPECT_FALSE(trash.IsEmpty());
    EXPECT_TRUE(trash.EndsWith(".editor-trash/"));
    EXPECT_FALSE(trash == mSandboxRoot);
}

TEST_F(AssetsActionsFixture, AssetsTrash_StashMovesFileOut)
{
    WriteText(SandboxPath("note.txt"), "hello");
    ASSERT_TRUE(o2FileSystem.IsFileExist(SandboxPath("note.txt")));

    String stash = Editor::AssetsTrash::StashAsset("note.txt");
    ASSERT_FALSE(stash.IsEmpty());
    EXPECT_FALSE(o2FileSystem.IsFileExist(SandboxPath("note.txt")));
    EXPECT_TRUE(o2FileSystem.IsFileExist(stash + "note.txt"));
}

TEST_F(AssetsActionsFixture, AssetsTrash_StashMovesMetaAlongWithAsset)
{
    WriteText(SandboxPath("image.png"), "raw-bytes");
    WriteText(SandboxPath("image.png.meta"), "{ \"uid\": 1 }");
    ASSERT_TRUE(o2FileSystem.IsFileExist(SandboxPath("image.png")));

    String stash = Editor::AssetsTrash::StashAsset("image.png");
    ASSERT_FALSE(stash.IsEmpty());

    EXPECT_FALSE(o2FileSystem.IsFileExist(SandboxPath("image.png")));
    EXPECT_FALSE(o2FileSystem.IsFileExist(SandboxPath("image.png.meta")));
    EXPECT_TRUE(o2FileSystem.IsFileExist(stash + "image.png"));
    EXPECT_TRUE(o2FileSystem.IsFileExist(stash + "image.png.meta"));
}

TEST_F(AssetsActionsFixture, AssetsTrash_RestoreRoundTrip)
{
    WriteText(SandboxPath("data.json"), "{\"k\":1}");
    WriteText(SandboxPath("data.json.meta"), "{ \"uid\": 1 }");

    String stash = Editor::AssetsTrash::StashAsset("data.json");
    ASSERT_FALSE(stash.IsEmpty());
    ASSERT_FALSE(o2FileSystem.IsFileExist(SandboxPath("data.json")));

    EXPECT_TRUE(Editor::AssetsTrash::RestoreAsset(stash, "data.json"));
    EXPECT_TRUE(o2FileSystem.IsFileExist(SandboxPath("data.json")));
    EXPECT_TRUE(o2FileSystem.IsFileExist(SandboxPath("data.json.meta")));
    EXPECT_EQ(FileSystem::ReadFile(SandboxPath("data.json")), String("{\"k\":1}"));

    EXPECT_FALSE(o2FileSystem.IsFolderExist(stash));
}

TEST_F(AssetsActionsFixture, AssetsTrash_StashIntoNestedRelPath)
{
    o2FileSystem.FolderCreate(SandboxPath("Sub/Deep"), true);
    WriteText(SandboxPath("Sub/Deep/leaf.txt"), "nested");

    String stash = Editor::AssetsTrash::StashAsset("Sub/Deep/leaf.txt");
    ASSERT_FALSE(stash.IsEmpty());
    EXPECT_FALSE(o2FileSystem.IsFileExist(SandboxPath("Sub/Deep/leaf.txt")));
    EXPECT_TRUE(o2FileSystem.IsFileExist(stash + "leaf.txt"));

    EXPECT_TRUE(Editor::AssetsTrash::RestoreAsset(stash, "Sub/Deep/leaf.txt"));
    EXPECT_TRUE(o2FileSystem.IsFileExist(SandboxPath("Sub/Deep/leaf.txt")));
    EXPECT_EQ(FileSystem::ReadFile(SandboxPath("Sub/Deep/leaf.txt")), String("nested"));
}

TEST_F(AssetsActionsFixture, AssetsTrash_StashFolder)
{
    o2FileSystem.FolderCreate(SandboxPath("MyFolder"), true);
    WriteText(SandboxPath("MyFolder/inside.txt"), "x");

    String stash = Editor::AssetsTrash::StashAsset("MyFolder");
    ASSERT_FALSE(stash.IsEmpty());
    EXPECT_FALSE(o2FileSystem.IsFolderExist(SandboxPath("MyFolder")));
    EXPECT_TRUE(o2FileSystem.IsFolderExist(stash + "MyFolder"));
    EXPECT_TRUE(o2FileSystem.IsFileExist(stash + "MyFolder/inside.txt"));

    EXPECT_TRUE(Editor::AssetsTrash::RestoreAsset(stash, "MyFolder"));
    EXPECT_TRUE(o2FileSystem.IsFolderExist(SandboxPath("MyFolder")));
    EXPECT_TRUE(o2FileSystem.IsFileExist(SandboxPath("MyFolder/inside.txt")));
}

TEST_F(AssetsActionsFixture, AssetsTrash_StashOfMissingAssetReturnsEmpty)
{
    String stash = Editor::AssetsTrash::StashAsset("does_not_exist.png");
    EXPECT_TRUE(stash.IsEmpty());
}

TEST_F(AssetsActionsFixture, AssetsTrash_RestoreOfEmptyPathFails)
{
    EXPECT_FALSE(Editor::AssetsTrash::RestoreAsset(String(), "anything.txt"));
}

TEST_F(AssetsActionsFixture, AssetsTrash_ClearAllOnStartupWipesRoot)
{
    WriteText(SandboxPath("a.txt"), "x");
    String stash = Editor::AssetsTrash::StashAsset("a.txt");
    ASSERT_FALSE(stash.IsEmpty());

    String root = Editor::AssetsTrash::GetRoot();
    EXPECT_TRUE(o2FileSystem.IsFolderExist(root));

    Editor::AssetsTrash::ClearAllOnStartup();
    EXPECT_FALSE(o2FileSystem.IsFolderExist(root));
}
