#include <gtest/gtest.h>

#include "o2/Utils/Types/UID.h"
#include "o2Editor/Actions/RenameAsset.h"

using namespace o2;
using namespace Editor;

TEST(RenameAssetAction, CtorCapturesFields)
{
    UID id;
    id.Randomize();

    auto action = mmake<RenameAssetAction>(id, String("old.png"), String("new.png"));

    EXPECT_EQ(action->assetUid, id);
    EXPECT_EQ(action->originalName, String("old.png"));
    EXPECT_EQ(action->newName, String("new.png"));
}

TEST(RenameAssetAction, GetNameIsHumanReadable)
{
    auto action = mmake<RenameAssetAction>(UID(), String("a"), String("b"));
    EXPECT_FALSE(action->GetName().IsEmpty());
}

TEST(RenameAssetAction, DefaultCtorWorks)
{
    auto action = mmake<RenameAssetAction>();
    EXPECT_TRUE(action->originalName.IsEmpty());
    EXPECT_TRUE(action->newName.IsEmpty());
}

