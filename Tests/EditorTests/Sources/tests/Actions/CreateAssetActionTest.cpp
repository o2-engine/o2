#include <gtest/gtest.h>

#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/DataAsset.h"
#include "o2/Assets/Types/FolderAsset.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Actions/AssetsTrash.h"
#include "o2Editor/Actions/CreateAsset.h"

#include "support/AssetsActionsFixture.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(CreateAssetAction, CtorCapturesTypeAndPaths)
{
    auto action = mmake<CreateAssetAction>(TypeOf(FolderAsset), String("Sub"), String("MyFolder"));

    EXPECT_FALSE(action->assetTypeName.IsEmpty());
    EXPECT_EQ(action->parentFolderPath, String("Sub"));
    EXPECT_EQ(action->assetName, String("MyFolder"));
    EXPECT_TRUE(action->createdPath.IsEmpty());
    EXPECT_TRUE(action->trashPath.IsEmpty());
}

TEST(CreateAssetAction, DefaultCtorWorks)
{
    auto action = mmake<CreateAssetAction>();
    EXPECT_TRUE(action->assetTypeName.IsEmpty());
}

TEST(CreateAssetAction, GetNameIsHumanReadable)
{
    auto action = mmake<CreateAssetAction>();
    EXPECT_FALSE(action->GetName().IsEmpty());
}

namespace
{
    // Simulates a successful first Redo by seeding the file and createdPath
    // directly. Bypasses Asset::Save which depends on AssetsTree state that
    // can't easily be retargeted to the sandbox mid-flight.
    void SeedAsCreated(const o2::Ref<Editor::CreateAssetAction>& action,
                       const o2::String& sandboxRoot,
                       const o2::String& relPath,
                       bool isFolder)
    {
        action->createdPath = relPath;
        if (isFolder)
            o2FileSystem.FolderCreate(sandboxRoot + relPath, true);
        else
            FileSystem::WriteFile(sandboxRoot + relPath, "seed");
    }
}

TEST_F(AssetsActionsFixture, CreateAssetAction_UndoStashesCreatedAssetToTrash)
{
    auto action = mmake<CreateAssetAction>(TypeOf(FolderAsset), String(""), String("Victim"));
    SeedAsCreated(action, mSandboxRoot, "Victim", true);
    ASSERT_TRUE(o2FileSystem.IsFolderExist(SandboxPath("Victim")));

    action->Undo();

    EXPECT_FALSE(o2FileSystem.IsFolderExist(SandboxPath("Victim")));
    ASSERT_FALSE(action->trashPath.IsEmpty());
    EXPECT_TRUE(o2FileSystem.IsFolderExist(action->trashPath + "Victim"));
}

TEST_F(AssetsActionsFixture, CreateAssetAction_ReRedoRestoresFromTrash)
{
    auto action = mmake<CreateAssetAction>(TypeOf(DataAsset), String(""), String("RoundTrip.data"));
    SeedAsCreated(action, mSandboxRoot, "RoundTrip.data", false);
    action->Undo();
    ASSERT_FALSE(o2FileSystem.IsFileExist(SandboxPath("RoundTrip.data")));
    ASSERT_FALSE(action->trashPath.IsEmpty());

    action->Redo();

    EXPECT_TRUE(o2FileSystem.IsFileExist(SandboxPath("RoundTrip.data")));
    EXPECT_TRUE(action->trashPath.IsEmpty());
}

TEST_F(AssetsActionsFixture, CreateAssetAction_UndoRedoUndoStashesAgain)
{
    auto action = mmake<CreateAssetAction>(TypeOf(FolderAsset), String(""), String("Yo"));
    SeedAsCreated(action, mSandboxRoot, "Yo", true);

    action->Undo();
    o2::String firstTrash = action->trashPath;
    ASSERT_FALSE(firstTrash.IsEmpty());

    action->Redo();
    ASSERT_TRUE(action->trashPath.IsEmpty());
    ASSERT_TRUE(o2FileSystem.IsFolderExist(SandboxPath("Yo")));

    action->Undo();
    EXPECT_FALSE(o2FileSystem.IsFolderExist(SandboxPath("Yo")));
    EXPECT_FALSE(action->trashPath.IsEmpty());
}

TEST_F(AssetsActionsFixture, CreateAssetAction_UndoBeforeRedoIsSafe)
{
    auto action = mmake<CreateAssetAction>(TypeOf(FolderAsset), String(""), String("Never"));

    EXPECT_NO_THROW(action->Undo());
    EXPECT_TRUE(action->trashPath.IsEmpty());
    EXPECT_FALSE(o2FileSystem.IsFolderExist(SandboxPath("Never")));
}

TEST_F(AssetsActionsFixture, CreateAssetAction_UnknownTypeIsNoOp)
{
    auto action = mmake<CreateAssetAction>();
    action->assetTypeName = "NoSuchTypeFooBar";
    action->parentFolderPath = "";
    action->assetName = "Ghost";

    EXPECT_NO_THROW(action->Redo());
    EXPECT_TRUE(action->createdPath.IsEmpty());
    EXPECT_FALSE(o2FileSystem.IsFileExist(SandboxPath("Ghost")));
}
