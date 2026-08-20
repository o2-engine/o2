#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Types/UID.h"

using namespace o2;

// Chunks above 0x7fffffff used to clamp to LONG_MAX on ILP32 platforms (WebAssembly,
// Windows), corrupting ids and colliding assets in caches
TEST(UID, FromStringRoundTripKeepsHighBitChunks)
{
    const char* ids[] = {
        "f5956d168a341163c275681e9efd9e8e",
        "ffffffffffffffffffffffffffffffff",
        "0b3afda71b3222ca780770f786973fa9",
        "00000000000000000000000000000001",
        "e4f6c7ab99ab075d7e2ec03b6bec0dd6"
    };

    for (auto id : ids)
    {
        UID uid;
        uid.FromString(WString(id));
        EXPECT_EQ((String)uid.ToString(), String(id));
    }
}

TEST(UID, DistinctHighBitIdsStayDistinct)
{
    UID first, second;
    first.FromString(WString("f5956d168a341163c275681e9efd9e8e"));
    second.FromString(WString("9efd9e8ec275681e8a341163f5956d16"));

    EXPECT_FALSE(first == second);
    EXPECT_TRUE(first != UID::empty);
}

// Asset ids must stay unique even when someone reseeds the global rand() with a
// fixed seed (particle emitters do) - identical ids in different runs corrupted
// prototype references
TEST(UID, UniqueAcrossGlobalRandReseed)
{
    srand(42);
    UID first;
    srand(42);
    UID second;

    EXPECT_NE(first, second);
}

TEST(UID, RandomizeProducesDifferentIds)
{
    UID a, b;
    EXPECT_NE(a, b);
}
