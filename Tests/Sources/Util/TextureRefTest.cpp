#include "o2/stdafx.h"

#include "o2/Render/TextureRef.h"

#include <gtest/gtest.h>

using namespace o2;

TEST(TextureRef, NullRefComparesEqualToNullptr)
{
    TextureRef nullRef;

    EXPECT_TRUE(nullRef == nullptr);
    EXPECT_FALSE(nullRef != nullptr);
    EXPECT_FALSE(nullRef);
}

TEST(TextureRef, NullRefsCompareEqual)
{
    TextureRef a;
    TextureRef b;

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}
