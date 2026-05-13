#include <gtest/gtest.h>

#include "o2/Utils/Types/UID.h"
#include "o2Editor/Actions/MoveAsset.h"

using namespace o2;
using namespace Editor;

namespace
{
    MoveAssetAction::Entry MakeEntry(const UID& uid, const String& filename, const String& parent)
    {
        MoveAssetAction::Entry e;
        e.uid = uid;
        e.filename = filename;
        e.originalParent = parent;
        return e;
    }
}

TEST(MoveAssetAction, CtorCapturesFields)
{
    UID id;
    id.Randomize();

    Vector<MoveAssetAction::Entry> entries;
    entries.Add(MakeEntry(id, "image.png", "Sprites"));

    auto action = mmake<MoveAssetAction>(entries, String("Sprites/New"));

    ASSERT_EQ(action->entries.Count(), 1);
    EXPECT_EQ(action->entries[0].uid, id);
    EXPECT_EQ(action->entries[0].filename, String("image.png"));
    EXPECT_EQ(action->entries[0].originalParent, String("Sprites"));
    EXPECT_EQ(action->destFolder, String("Sprites/New"));
}

TEST(MoveAssetAction, DefaultCtorWorks)
{
    auto action = mmake<MoveAssetAction>();
    EXPECT_EQ(action->entries.Count(), 0);
    EXPECT_TRUE(action->destFolder.IsEmpty());
}

TEST(MoveAssetAction, GetNameIsHumanReadable)
{
    auto action = mmake<MoveAssetAction>();
    EXPECT_FALSE(action->GetName().IsEmpty());
}

TEST(MoveAssetAction, MultiAssetEntriesPreserved)
{
    UID a, b;
    a.Randomize();
    b.Randomize();

    Vector<MoveAssetAction::Entry> entries;
    entries.Add(MakeEntry(a, "a.png", ""));
    entries.Add(MakeEntry(b, "b.png", "Sub"));

    auto action = mmake<MoveAssetAction>(entries, String("Dst"));
    ASSERT_EQ(action->entries.Count(), 2);
    EXPECT_EQ(action->entries[0].uid, a);
    EXPECT_EQ(action->entries[1].uid, b);
}
