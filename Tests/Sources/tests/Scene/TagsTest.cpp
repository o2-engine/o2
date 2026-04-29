#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2/Scene/Tags.h"
#include "tests/Scene/SceneTestHelpers.h"

using namespace o2;

// ===== Tag =====

TEST(Tag, DefaultConstructionHasEmptyName)
{
    Tag t;
    EXPECT_TRUE(t.GetName().IsEmpty());
}

TEST(Tag, NameConstructorStores)
{
    Tag t("loot");
    EXPECT_EQ(t.GetName(), "loot");
}

TEST(Tag, SetNameRoundTrip)
{
    Tag t;
    t.SetName("enemy");
    EXPECT_EQ(t.GetName(), "enemy");
}

// ===== TagGroup =====

TEST(TagGroup, DefaultIsEmpty)
{
    TagGroup g;
    EXPECT_EQ(g.GetTags().Count(), 0);
    EXPECT_EQ(g.GetTagsNames().Count(), 0);
    EXPECT_FALSE(g.IsHaveTag("anything"));
}

TEST(TagGroup, AddTagByRefStores)
{
    TagGroup g;
    auto tag = mmake<Tag>("npc");
    g.AddTag(tag);
    EXPECT_EQ(g.GetTags().Count(), 1);
    EXPECT_TRUE(g.IsHaveTag(tag));
    EXPECT_TRUE(g.IsHaveTag("npc"));
}

TEST(TagGroup, DuplicateAddTagIgnored)
{
    TagGroup g;
    auto tag = mmake<Tag>("npc");
    g.AddTag(tag);
    g.AddTag(tag);
    EXPECT_EQ(g.GetTags().Count(), 1);
}

TEST(TagGroup, RemoveTagByRefRemoves)
{
    TagGroup g;
    auto tag = mmake<Tag>("npc");
    g.AddTag(tag);
    g.RemoveTag(tag);
    EXPECT_EQ(g.GetTags().Count(), 0);
    EXPECT_FALSE(g.IsHaveTag(tag));
}

TEST(TagGroup, ClearRemovesAllTags)
{
    TagGroup g;
    g.AddTag(mmake<Tag>("a"));
    g.AddTag(mmake<Tag>("b"));
    g.Clear();
    EXPECT_EQ(g.GetTags().Count(), 0);
}

TEST(TagGroup, OnTagAddedCallbackFires)
{
    TagGroup g;
    int addedCount = 0;
    g.onTagAdded = [&](const Ref<Tag>&) { addedCount++; };
    g.AddTag(mmake<Tag>("alpha"));
    EXPECT_EQ(addedCount, 1);
}

TEST(TagGroup, OnTagRemovedCallbackFires)
{
    TagGroup g;
    auto tag = mmake<Tag>("beta");
    g.AddTag(tag);
    int removedCount = 0;
    g.onTagRemoved = [&](const Ref<Tag>&) { removedCount++; };
    g.RemoveTag(tag);
    EXPECT_EQ(removedCount, 1);
}

TEST(TagGroup, GetTagsNamesReturnsAllNames)
{
    TagGroup g;
    g.AddTag(mmake<Tag>("a"));
    g.AddTag(mmake<Tag>("b"));
    auto names = g.GetTagsNames();
    EXPECT_EQ(names.Count(), 2);
    EXPECT_TRUE(names.Contains("a"));
    EXPECT_TRUE(names.Contains("b"));
}
