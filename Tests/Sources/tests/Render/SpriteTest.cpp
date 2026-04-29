#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Sprite.h"

using namespace o2;

TEST(Sprite, DefaultConstructionIsValid)
{
    Sprite s;
    EXPECT_EQ(s.GetMode(), SpriteMode::Default);
}

TEST(Sprite, SetCornerColorsIndependently)
{
    Sprite s;
    s.SetLeftTopColor(Color4(255, 0, 0, 255));
    s.SetRightBottomColor(Color4(0, 0, 255, 255));
    EXPECT_EQ(s.GetLeftTopCorner(), Color4(255, 0, 0, 255));
    EXPECT_EQ(s.GetRightBottomCorner(), Color4(0, 0, 255, 255));
}

TEST(Sprite, SetModeRoundTrip)
{
    Sprite s;
    s.SetMode(SpriteMode::Sliced);
    EXPECT_EQ(s.GetMode(), SpriteMode::Sliced);
    s.SetMode(SpriteMode::Tiled);
    EXPECT_EQ(s.GetMode(), SpriteMode::Tiled);
    s.SetMode(SpriteMode::FixedAspect);
    EXPECT_EQ(s.GetMode(), SpriteMode::FixedAspect);
}

TEST(Sprite, SetFillRoundTrip)
{
    Sprite s;
    s.SetFill(0.7f);
    EXPECT_NEAR(s.GetFill(), 0.7f, 0.001f);
}

TEST(Sprite, SetTileScaleRoundTrip)
{
    Sprite s;
    s.SetTileScale(2.5f);
    EXPECT_FLOAT_EQ(s.GetTileScale(), 2.5f);
}

TEST(Sprite, SetSliceBorderRoundTrip)
{
    Sprite s;
    s.SetSliceBorder(BorderI(1, 2, 3, 4));
    EXPECT_EQ(s.GetSliceBorder(), BorderI(1, 2, 3, 4));
}

TEST(Sprite, AssignmentOperatorClonesProperties)
{
    Sprite a;
    a.SetMode(SpriteMode::Sliced);
    a.SetFill(0.4f);

    Sprite b;
    b = a;
    EXPECT_EQ(b.GetMode(), SpriteMode::Sliced);
    EXPECT_NEAR(b.GetFill(), 0.4f, 0.001f);
}
