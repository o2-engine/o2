#include <gtest/gtest.h>

#include "o2Editor/Actions/CopyAssets.h"

using namespace o2;
using namespace Editor;

TEST(CopyAssetsAction, CtorCapturesSourcesAndDest)
{
    Vector<String> sources;
    sources.Add("a.png");
    sources.Add("Sub/b.png");

    auto action = mmake<CopyAssetsAction>(sources, String("DestFolder"));

    ASSERT_EQ(action->entries.Count(), 2);
    EXPECT_EQ(action->entries[0].sourcePath, String("a.png"));
    EXPECT_EQ(action->entries[1].sourcePath, String("Sub/b.png"));
    EXPECT_EQ(action->destFolder, String("DestFolder"));
}

TEST(CopyAssetsAction, DefaultCtorWorks)
{
    auto action = mmake<CopyAssetsAction>();
    EXPECT_EQ(action->entries.Count(), 0);
}

TEST(CopyAssetsAction, GetNameIsHumanReadable)
{
    auto action = mmake<CopyAssetsAction>();
    EXPECT_FALSE(action->GetName().IsEmpty());
}
