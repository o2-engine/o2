#include <gtest/gtest.h>

#include "o2/Assets/Assets.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Actions/AssetsTrash.h"
#include "o2Editor/Actions/DeleteAsset.h"

#include "support/AssetsActionsFixture.h"

using namespace o2;
using namespace Editor::Tests;

TEST(DeleteAssetActionUnit, CtorCapturesPaths)
{
    Vector<String> paths;
    paths.Add("a.png");
    paths.Add("Sub/b.png");

    auto action = mmake<Editor::DeleteAssetAction>(paths);
    ASSERT_EQ(action->entries.Count(), 2);
    EXPECT_EQ(action->entries[0].originalPath, String("a.png"));
    EXPECT_EQ(action->entries[1].originalPath, String("Sub/b.png"));
}

TEST(DeleteAssetActionUnit, DefaultCtorWorks)
{
    auto action = mmake<Editor::DeleteAssetAction>();
    EXPECT_EQ(action->entries.Count(), 0);
}

TEST(DeleteAssetActionUnit, GetNameIsHumanReadable)
{
    auto action = mmake<Editor::DeleteAssetAction>();
    EXPECT_FALSE(action->GetName().IsEmpty());
}

TEST_F(AssetsActionsFixture, DeleteAssetAction_RedoStashesAssetToTrash)
{
    FileSystem::WriteFile(SandboxPath("victim.txt"), "x");

    Vector<String> paths;
    paths.Add("victim.txt");
    auto action = mmake<Editor::DeleteAssetAction>(paths);

    action->Redo();

    EXPECT_FALSE(o2FileSystem.IsFileExist(SandboxPath("victim.txt")));
    ASSERT_FALSE(action->entries[0].trashPath.IsEmpty());
    EXPECT_TRUE(o2FileSystem.IsFileExist(action->entries[0].trashPath + "victim.txt"));
}

TEST_F(AssetsActionsFixture, DeleteAssetAction_UndoRestoresAsset)
{
    FileSystem::WriteFile(SandboxPath("victim.txt"), "x");

    Vector<String> paths;
    paths.Add("victim.txt");
    auto action = mmake<Editor::DeleteAssetAction>(paths);

    action->Redo();
    action->Undo();

    EXPECT_TRUE(o2FileSystem.IsFileExist(SandboxPath("victim.txt")));
}

TEST_F(AssetsActionsFixture, DeleteAssetAction_BatchOfMultipleAssets)
{
    FileSystem::WriteFile(SandboxPath("a.txt"), "1");
    FileSystem::WriteFile(SandboxPath("b.txt"), "2");
    FileSystem::WriteFile(SandboxPath("c.txt"), "3");

    Vector<String> paths;
    paths.Add("a.txt");
    paths.Add("b.txt");
    paths.Add("c.txt");

    auto action = mmake<Editor::DeleteAssetAction>(paths);
    action->Redo();

    EXPECT_FALSE(o2FileSystem.IsFileExist(SandboxPath("a.txt")));
    EXPECT_FALSE(o2FileSystem.IsFileExist(SandboxPath("b.txt")));
    EXPECT_FALSE(o2FileSystem.IsFileExist(SandboxPath("c.txt")));

    action->Undo();

    EXPECT_TRUE(o2FileSystem.IsFileExist(SandboxPath("a.txt")));
    EXPECT_TRUE(o2FileSystem.IsFileExist(SandboxPath("b.txt")));
    EXPECT_TRUE(o2FileSystem.IsFileExist(SandboxPath("c.txt")));
}
