#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Windows/TreeWindow/SceneSearchFilter.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

static Ref<Actor> NamedActor(const String& name)
{
    auto a = MakeActor();
    a->SetName(name);
    return a;
}

TEST(SceneSearchFilter, MatchesByName)
{
    SceneCleanGuard guard;
    auto a = NamedActor("Player");
    auto b = NamedActor("Enemy");
    auto c = NamedActor("Camera");
    TickScene();

    auto result = SceneSearchFilter::Search(AsEditable({a, b, c}), "enemy");

    ASSERT_EQ(result.Count(), 1);
    EXPECT_EQ(result[0], DynamicCast<SceneEditableObject>(b));
}

TEST(SceneSearchFilter, CaseInsensitive)
{
    SceneCleanGuard guard;
    auto a = NamedActor("Player");
    TickScene();

    EXPECT_EQ(SceneSearchFilter::Search(AsEditable({a}), "PLAYER").Count(), 1);
    EXPECT_EQ(SceneSearchFilter::Search(AsEditable({a}), "player").Count(), 1);
    EXPECT_EQ(SceneSearchFilter::Search(AsEditable({a}), "PlAyEr").Count(), 1);
}

TEST(SceneSearchFilter, SubstringNotPrefix)
{
    SceneCleanGuard guard;
    auto a = NamedActor("BackgroundSprite");
    TickScene();

    auto result = SceneSearchFilter::Search(AsEditable({a}), "sprite");

    ASSERT_EQ(result.Count(), 1);
    EXPECT_EQ(result[0], DynamicCast<SceneEditableObject>(a));
}

TEST(SceneSearchFilter, NoMatchReturnsEmpty)
{
    SceneCleanGuard guard;
    auto a = NamedActor("Player");
    auto b = NamedActor("Enemy");
    TickScene();

    auto result = SceneSearchFilter::Search(AsEditable({a, b}), "nothing");

    EXPECT_TRUE(result.IsEmpty());
}

TEST(SceneSearchFilter, RecursesIntoChildren)
{
    SceneCleanGuard guard;
    auto root = NamedActor("Root");
    auto child = NamedActor("Child");
    auto grand = NamedActor("DeepTarget");
    root->AddChild(child);
    child->AddChild(grand);
    TickScene();

    auto result = SceneSearchFilter::Search(AsEditable({root}), "deeptarget");

    ASSERT_EQ(result.Count(), 1);
    EXPECT_EQ(result[0], DynamicCast<SceneEditableObject>(grand));
}

TEST(SceneSearchFilter, PreOrderAndMultiple)
{
    SceneCleanGuard guard;
    auto root = NamedActor("MatchRoot");
    auto child = NamedActor("MatchChild");
    root->AddChild(child);
    TickScene();

    auto result = SceneSearchFilter::Search(AsEditable({root}), "match");

    ASSERT_EQ(result.Count(), 2);
    EXPECT_EQ(result[0], DynamicCast<SceneEditableObject>(root));
    EXPECT_EQ(result[1], DynamicCast<SceneEditableObject>(child));
}

TEST(SceneSearchFilter, EmptySearchReturnsAll)
{
    SceneCleanGuard guard;
    auto root = NamedActor("Root");
    auto child = NamedActor("Child");
    root->AddChild(child);
    TickScene();

    auto result = SceneSearchFilter::Search(AsEditable({root}), "");

    EXPECT_EQ(result.Count(), 2);
}

TEST(SceneSearchFilter, MultipleRoots)
{
    SceneCleanGuard guard;
    auto rootA = NamedActor("AlphaOne");
    auto rootB = NamedActor("AlphaTwo");
    TickScene();

    auto result = SceneSearchFilter::Search(AsEditable({rootA, rootB}), "alpha");

    ASSERT_EQ(result.Count(), 2);
    EXPECT_EQ(result[0], DynamicCast<SceneEditableObject>(rootA));
    EXPECT_EQ(result[1], DynamicCast<SceneEditableObject>(rootB));
}
