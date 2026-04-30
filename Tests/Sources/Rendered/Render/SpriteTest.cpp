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

TEST(Sprite, LoadMonoColorClearsTextureAndResetsCorners)
{
    Sprite s;
    s.SetCornerColors(Color4::Red(), Color4::Green(), Color4::Blue(), Color4(10, 20, 30, 200));

    s.LoadMonoColor(Color4(99, 88, 77, 66));

    EXPECT_EQ(s.GetColor(), Color4(99, 88, 77, 66));
    EXPECT_FALSE(s.GetTexture().IsValid());
    EXPECT_EQ(s.GetLeftTopCorner(), Color4::White());
    EXPECT_EQ(s.GetRightTopCorner(), Color4::White());
    EXPECT_EQ(s.GetRightBottomCorner(), Color4::White());
    EXPECT_EQ(s.GetLeftBottomCorner(), Color4::White());
}

TEST(Sprite, AllFourCornerColorsRoundTripIndependently)
{
    Sprite s;
    Color4 lt(255, 0, 0, 255);
    Color4 rt(0, 255, 0, 255);
    Color4 rb(0, 0, 255, 255);
    Color4 lb(255, 255, 0, 255);

    s.SetLeftTopColor(lt);
    s.SetRightTopColor(rt);
    s.SetRightBottomColor(rb);
    s.SetLeftBottomColor(lb);

    EXPECT_EQ(s.GetLeftTopCorner(), lt);
    EXPECT_EQ(s.GetRightTopCorner(), rt);
    EXPECT_EQ(s.GetRightBottomCorner(), rb);
    EXPECT_EQ(s.GetLeftBottomCorner(), lb);
}

TEST(Sprite, BatchSetCornerColorsAppliesEachCorner)
{
    Sprite s;
    Color4 lt(1, 2, 3, 200);
    Color4 rt(11, 22, 33, 200);
    Color4 rb(111, 122, 133, 200);
    Color4 lb(50, 60, 70, 200);

    s.SetCornerColors(lt, rt, rb, lb);

    EXPECT_EQ(s.GetLeftTopCorner(), lt);
    EXPECT_EQ(s.GetRightTopCorner(), rt);
    EXPECT_EQ(s.GetRightBottomCorner(), rb);
    EXPECT_EQ(s.GetLeftBottomCorner(), lb);
}

TEST(Sprite, SetTileScaleStoresAbsoluteValue)
{
    Sprite s;
    s.SetTileScale(-3.0f);
    EXPECT_FLOAT_EQ(s.GetTileScale(), 3.0f);

    s.SetTileScale(0.5f);
    EXPECT_FLOAT_EQ(s.GetTileScale(), 0.5f);
}

TEST(Sprite, SetFillClampedToZeroOne)
{
    Sprite s;
    s.SetFill(2.0f);
    EXPECT_FLOAT_EQ(s.GetFill(), 1.0f);

    s.SetFill(-0.5f);
    EXPECT_FLOAT_EQ(s.GetFill(), 0.0f);

    s.SetFill(0.5f);
    EXPECT_NEAR(s.GetFill(), 0.5f, 0.001f);
}

TEST(Sprite, CopyConstructorPreservesAllSpriteState)
{
    Sprite src;
    src.SetMode(SpriteMode::Sliced);
    src.SetFill(0.3f);
    src.SetSliceBorder(BorderI(2, 4, 6, 8));
    src.SetTileScale(2.5f);
    src.SetLeftTopColor(Color4(10, 0, 0, 255));
    src.SetRightTopColor(Color4(0, 20, 0, 255));
    src.SetRightBottomColor(Color4(0, 0, 30, 255));
    src.SetLeftBottomColor(Color4(40, 40, 40, 255));
    src.SetTransparency(0.5f);

    Sprite copy(src);

    EXPECT_EQ(copy.GetMode(), SpriteMode::Sliced);
    EXPECT_NEAR(copy.GetFill(), 0.3f, 0.001f);
    EXPECT_EQ(copy.GetSliceBorder(), BorderI(2, 4, 6, 8));
    EXPECT_FLOAT_EQ(copy.GetTileScale(), 2.5f);
    EXPECT_EQ(copy.GetLeftTopCorner(), Color4(10, 0, 0, 255));
    EXPECT_EQ(copy.GetRightTopCorner(), Color4(0, 20, 0, 255));
    EXPECT_EQ(copy.GetRightBottomCorner(), Color4(0, 0, 30, 255));
    EXPECT_EQ(copy.GetLeftBottomCorner(), Color4(40, 40, 40, 255));
    EXPECT_EQ(copy.GetColor().a, 127);
}

TEST(Sprite, EqualityRespectsModeSliceBorderAndCorners)
{
    Sprite a;
    Sprite b;
    EXPECT_TRUE(a == b);

    b.SetMode(SpriteMode::Tiled);
    EXPECT_FALSE(a == b);
    a.SetMode(SpriteMode::Tiled);
    EXPECT_TRUE(a == b);

    b.SetSliceBorder(BorderI(1, 2, 3, 4));
    EXPECT_FALSE(a == b);
    a.SetSliceBorder(BorderI(1, 2, 3, 4));
    EXPECT_TRUE(a == b);

    b.SetLeftTopColor(Color4(0, 255, 0, 255));
    EXPECT_FALSE(a == b);
}
