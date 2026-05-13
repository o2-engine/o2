#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Actions/CreateAssetFromScene.h"

#include "support/AssetsActionsFixture.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(CreateAssetFromSceneAction, CtorCapturesActorIdsAndDest)
{
    Vector<SceneUID> ids;
    ids.Add(42);
    ids.Add(99);

    auto action = mmake<CreateAssetFromSceneAction>(ids, String("Prefabs"));

    ASSERT_EQ(action->entries.Count(), 2);
    EXPECT_EQ(action->entries[0].actorId, (SceneUID)42);
    EXPECT_EQ(action->entries[1].actorId, (SceneUID)99);
    EXPECT_EQ(action->destFolder, String("Prefabs"));
}

TEST(CreateAssetFromSceneAction, DefaultCtorWorks)
{
    auto action = mmake<CreateAssetFromSceneAction>();
    EXPECT_EQ(action->entries.Count(), 0);
}

TEST(CreateAssetFromSceneAction, GetNameIsHumanReadable)
{
    auto action = mmake<CreateAssetFromSceneAction>();
    EXPECT_FALSE(action->GetName().IsEmpty());
}

namespace
{
    void SeedAsCreated(const Ref<CreateAssetFromSceneAction>& action,
                       const String& sandboxRoot, int entryIdx, const String& relPath)
    {
        action->entries[entryIdx].createdPath = relPath;
        FileSystem::WriteFile(sandboxRoot + relPath, "fake-proto-bytes");
    }
}

TEST_F(AssetsActionsFixture, CreateAssetFromSceneAction_UndoStashesCreatedAsset)
{
    SceneCleanGuard sceneGuard;
    auto actor = MakeActor();
    TickScene();

    Vector<SceneUID> ids;
    ids.Add(actor->GetID());
    auto action = mmake<CreateAssetFromSceneAction>(ids, String(""));
    SeedAsCreated(action, mSandboxRoot, 0, "Actor.proto");
    ASSERT_TRUE(o2FileSystem.IsFileExist(SandboxPath("Actor.proto")));

    action->Undo();

    EXPECT_FALSE(o2FileSystem.IsFileExist(SandboxPath("Actor.proto")));
    ASSERT_FALSE(action->entries[0].trashPath.IsEmpty());
    EXPECT_TRUE(o2FileSystem.IsFileExist(action->entries[0].trashPath + "Actor.proto"));
}

TEST_F(AssetsActionsFixture, CreateAssetFromSceneAction_UndoToleratesMissingActor)
{
    SceneCleanGuard sceneGuard;

    Vector<SceneUID> ids;
    ids.Add((SceneUID)0xDEAD);
    auto action = mmake<CreateAssetFromSceneAction>(ids, String(""));
    SeedAsCreated(action, mSandboxRoot, 0, "Orphan.proto");

    EXPECT_NO_THROW(action->Undo());
    EXPECT_FALSE(o2FileSystem.IsFileExist(SandboxPath("Orphan.proto")));
    EXPECT_FALSE(action->entries[0].trashPath.IsEmpty());
}

TEST_F(AssetsActionsFixture, CreateAssetFromSceneAction_ReRedoRestoresFromTrash)
{
    SceneCleanGuard sceneGuard;
    auto actor = MakeActor();
    TickScene();

    Vector<SceneUID> ids;
    ids.Add(actor->GetID());
    auto action = mmake<CreateAssetFromSceneAction>(ids, String(""));
    SeedAsCreated(action, mSandboxRoot, 0, "Cycle.proto");

    action->Undo();
    ASSERT_FALSE(o2FileSystem.IsFileExist(SandboxPath("Cycle.proto")));

    action->Redo();

    EXPECT_TRUE(o2FileSystem.IsFileExist(SandboxPath("Cycle.proto")));
    EXPECT_TRUE(action->entries[0].trashPath.IsEmpty());
}

TEST_F(AssetsActionsFixture, CreateAssetFromSceneAction_MultiActorBatch)
{
    SceneCleanGuard sceneGuard;
    auto a = MakeActor();
    auto b = MakeActor();
    auto c = MakeActor();
    TickScene();

    Vector<SceneUID> ids;
    ids.Add(a->GetID());
    ids.Add(b->GetID());
    ids.Add(c->GetID());
    auto action = mmake<CreateAssetFromSceneAction>(ids, String(""));

    SeedAsCreated(action, mSandboxRoot, 0, "A.proto");
    SeedAsCreated(action, mSandboxRoot, 1, "B.proto");
    SeedAsCreated(action, mSandboxRoot, 2, "C.proto");

    action->Undo();
    EXPECT_FALSE(o2FileSystem.IsFileExist(SandboxPath("A.proto")));
    EXPECT_FALSE(o2FileSystem.IsFileExist(SandboxPath("B.proto")));
    EXPECT_FALSE(o2FileSystem.IsFileExist(SandboxPath("C.proto")));

    action->Redo();
    EXPECT_TRUE(o2FileSystem.IsFileExist(SandboxPath("A.proto")));
    EXPECT_TRUE(o2FileSystem.IsFileExist(SandboxPath("B.proto")));
    EXPECT_TRUE(o2FileSystem.IsFileExist(SandboxPath("C.proto")));
}
