#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"
#include "o2/Scene/Tags.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Singleton =====

TEST(Scene, SingletonAccessorReturnsInstance)
{
    EXPECT_EQ(&Scene::Instance(), &o2Scene);
}

// ===== Root and All actors =====

TEST(Scene, NewActorAppearsInRootActors)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    a->SetName("rootCandidate_Scene_42");
    TickFrame();

    bool found = false;
    for (auto& root : o2Scene.GetRootActors())
        if (root == a) { found = true; break; }

    EXPECT_TRUE(found);
}

TEST(Scene, NewActorAppearsInAllActors)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    TickFrame();

    bool found = false;
    for (auto& weak : o2Scene.GetAllActors())
        if (weak.Lock() == a) { found = true; break; }

    EXPECT_TRUE(found);
}

TEST(Scene, ChildActorIsInAllActorsButNotInRoots)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);
    TickFrame();

    bool inRoots = false;
    for (auto& root : o2Scene.GetRootActors())
        if (root == child) { inRoots = true; break; }
    EXPECT_FALSE(inRoots);

    bool inAll = false;
    for (auto& weak : o2Scene.GetAllActors())
        if (weak.Lock() == child) { inAll = true; break; }
    EXPECT_TRUE(inAll);
}

TEST(Scene, ReparentToNullPromotesActorToRoot)
{
    SceneCleanGuard guard;
    auto parent = mmake<Actor>(ActorCreateMode::InScene);
    auto child = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(child);
    TickFrame();

    child->SetParent(nullptr);
    TickFrame();

    bool inRoots = false;
    for (auto& root : o2Scene.GetRootActors())
        if (root == child) { inRoots = true; break; }

    EXPECT_TRUE(inRoots);
}

// ===== Lookup =====

TEST(Scene, GetActorByIdReturnsActor)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    TickFrame();

    auto found = o2Scene.GetActorByID(a->GetID());
    EXPECT_EQ(found, a);
}

TEST(Scene, GetActorByIdReturnsNullForUnknown)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    TickFrame();

    auto found = o2Scene.GetActorByID(SceneUID(999999));
    EXPECT_FALSE(found);
}

TEST(Scene, FindActorResolvesByPath)
{
    SceneCleanGuard guard;
    auto root = mmake<Actor>(ActorCreateMode::InScene);
    root->SetName("worldRoot_unique_42");
    auto leaf = mmake<Actor>(ActorCreateMode::InScene);
    leaf->SetName("leaf_unique_42");
    root->AddChild(leaf);
    TickFrame();

    auto found = o2Scene.FindActor("worldRoot_unique_42/leaf_unique_42");
    EXPECT_EQ(found, leaf);
}

// ===== Tags =====

TEST(Scene, AddTagAndGetByName)
{
    SceneCleanGuard guard;
    auto tag = o2Scene.AddTag("test_tag_unique_42");
    ASSERT_TRUE(tag);
    EXPECT_EQ(o2Scene.GetTag("test_tag_unique_42"), tag);
}

TEST(Scene, RemoveTagByName)
{
    SceneCleanGuard guard;
    auto tag = o2Scene.AddTag("test_tag_remove_42");
    EXPECT_TRUE(o2Scene.GetTag("test_tag_remove_42"));

    o2Scene.RemoveTag("test_tag_remove_42");
    EXPECT_FALSE(o2Scene.GetTag("test_tag_remove_42"));
}

// ===== Update tick =====

TEST(Scene, UpdateRunsComponentUpdate)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();
    TickFrame();

    int before = comp->onUpdateCount;
    TickFrame();
    EXPECT_GT(comp->onUpdateCount, before);
}

TEST(Scene, FixedUpdateRunsComponentFixedUpdate)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto comp = a->AddComponent<TestComponent>();
    TickFrame();

    int before = comp->onFixedUpdateCount;
    o2Scene.FixedUpdate(0.016f);
    EXPECT_GT(comp->onFixedUpdateCount, before);
}

TEST(Scene, IsUpdatingFalseOutsideTick)
{
    EXPECT_FALSE(o2Scene.IsUpdating());
}

// ===== Destroy and Clear =====

TEST(Scene, DestroyActorMarkAndCleanup)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto id = a->GetID();
    TickFrame();
    EXPECT_TRUE(o2Scene.GetActorByID(id));

    o2Scene.DestroyActor(a);
    a = nullptr;
    TickFrames(2);
    EXPECT_FALSE(o2Scene.GetActorByID(id));
}

TEST(Scene, ClearKeepsDefaultLayer)
{
    SceneCleanGuard guard;
    auto layer = o2Scene.AddLayer("temp_layer_for_clear");
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    TickFrame();

    o2Scene.Clear(true);

    EXPECT_TRUE(o2Scene.GetDefaultLayer());
}

TEST(Scene, ClearRemovesRootActors)
{
    SceneCleanGuard guard;
    auto a = mmake<Actor>(ActorCreateMode::InScene);
    auto b = mmake<Actor>(ActorCreateMode::InScene);
    TickFrame();
    int countBefore = o2Scene.GetRootActors().Count();
    EXPECT_GE(countBefore, 2);

    a = nullptr;
    b = nullptr;
    o2Scene.Clear(true);
    TickFrames(3);

    EXPECT_LT(o2Scene.GetRootActors().Count(), countBefore);
}
