#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Bitmap/BitmapCompare.h"

using namespace o2;

namespace
{
    Bitmap Filled(const Vec2I& size, const Color4& color)
    {
        Bitmap bitmap(PixelFormat::R8G8B8A8, size);
        UInt8* data = bitmap.GetData();
        for (int i = 0; i < size.x*size.y; i++)
        {
            data[i*4] = (UInt8)color.r;
            data[i*4 + 1] = (UInt8)color.g;
            data[i*4 + 2] = (UInt8)color.b;
            data[i*4 + 3] = (UInt8)color.a;
        }

        return bitmap;
    }

    void SetPixel(Bitmap& bitmap, int idx, const Color4& color)
    {
        UInt8* pixel = bitmap.GetData() + idx*4;
        pixel[0] = (UInt8)color.r;
        pixel[1] = (UInt8)color.g;
        pixel[2] = (UInt8)color.b;
        pixel[3] = (UInt8)color.a;
    }
}

TEST(BitmapCompare, IdenticalBitmapsAreFullySimilar)
{
    Bitmap a = Filled(Vec2I(4, 4), Color4(10, 20, 30, 200));
    BitmapCompareResult result = BitmapCompare::Compare(a, a, 0);
    EXPECT_TRUE(result.comparable);
    EXPECT_FLOAT_EQ(result.similarity, 1.0f);
    EXPECT_FLOAT_EQ(result.meanDifference, 0.0f);
    EXPECT_EQ(result.maxDifference, 0);
}

TEST(BitmapCompare, DifferentSizesAreNotComparable)
{
    Bitmap a = Filled(Vec2I(4, 4), Color4(0, 0, 0, 255));
    Bitmap b = Filled(Vec2I(4, 5), Color4(0, 0, 0, 255));
    BitmapCompareResult result = BitmapCompare::Compare(a, b);
    EXPECT_FALSE(result.comparable);
    EXPECT_FLOAT_EQ(result.similarity, 0.0f);

    Bitmap empty;
    EXPECT_FALSE(BitmapCompare::Compare(empty, empty).comparable);
}

TEST(BitmapCompare, ToleranceIsInclusiveMaxChannelDifference)
{
    Bitmap a = Filled(Vec2I(2, 2), Color4(100, 100, 100, 255));
    Bitmap b = Filled(Vec2I(2, 2), Color4(100, 108, 97, 255));

    EXPECT_FLOAT_EQ(BitmapCompare::GetSimilarity(a, b, 8), 1.0f);
    EXPECT_FLOAT_EQ(BitmapCompare::GetSimilarity(a, b, 7), 0.0f);

    BitmapCompareResult result = BitmapCompare::Compare(a, b, 8);
    EXPECT_EQ(result.maxDifference, 8);
    EXPECT_FLOAT_EQ(result.meanDifference, 8.0f);
}

TEST(BitmapCompare, SimilarityIsPartOfMatchedPixels)
{
    Bitmap a = Filled(Vec2I(4, 5), Color4(0, 0, 0, 255));
    Bitmap b = a;
    SetPixel(b, 3, Color4(40, 0, 0, 255));
    SetPixel(b, 7, Color4(0, 0, 200, 255));
    SetPixel(b, 9, Color4(0, 4, 0, 255));

    BitmapCompareResult result = BitmapCompare::Compare(a, b, 8);
    EXPECT_FLOAT_EQ(result.similarity, 18.0f/20.0f);
    EXPECT_EQ(result.maxDifference, 200);
    EXPECT_FLOAT_EQ(result.meanDifference, 244.0f/20.0f);
}

TEST(BitmapCompare, ColorUnderZeroAlphaDoesNotMatter)
{
    Bitmap a = Filled(Vec2I(2, 2), Color4(255, 0, 0, 0));
    Bitmap b = Filled(Vec2I(2, 2), Color4(0, 255, 0, 0));
    EXPECT_FLOAT_EQ(BitmapCompare::GetSimilarity(a, b, 0), 1.0f);

    Bitmap opaqueWhite = Filled(Vec2I(2, 2), Color4(255, 255, 255, 255));
    EXPECT_FLOAT_EQ(BitmapCompare::GetSimilarity(a, opaqueWhite, 0, Color4::White()), 1.0f);
    EXPECT_FLOAT_EQ(BitmapCompare::GetSimilarity(a, opaqueWhite, 0, Color4::Black()), 0.0f);
}

TEST(BitmapCompare, AlphaIsComparedOverBackground)
{
    Bitmap half = Filled(Vec2I(2, 2), Color4(0, 0, 0, 128));
    Bitmap gray = Filled(Vec2I(2, 2), Color4(127, 127, 127, 255));
    EXPECT_FLOAT_EQ(BitmapCompare::GetSimilarity(half, gray, 0, Color4::White()), 1.0f);
    EXPECT_EQ(BitmapCompare::Compare(half, gray, 0, Color4::Black()).maxDifference, 127);
}

TEST(BitmapCompare, DifferenceBitmapMarksMismatches)
{
    Bitmap a = Filled(Vec2I(3, 1), Color4(255, 255, 255, 255));
    Bitmap b = a;
    SetPixel(b, 1, Color4(0, 0, 0, 255));

    Bitmap difference;
    BitmapCompare::Compare(a, b, 8, Color4::White(), &difference);
    ASSERT_EQ(difference.GetSize(), Vec2I(3, 1));

    const UInt8* data = difference.GetData();
    EXPECT_EQ(data[0], data[1]);
    EXPECT_EQ(data[1], data[2]);
    EXPECT_EQ(data[4], 255);
    EXPECT_LT(data[5], 64);
    EXPECT_EQ(data[7], 255);
}

TEST(BitmapCompare, CompositeOver)
{
    Bitmap straight = Filled(Vec2I(2, 2), Color4(200, 100, 0, 128));
    BitmapCompare::CompositeOver(straight, Color4(255, 255, 255, 255));
    EXPECT_EQ(straight.GetData()[0], 227);
    EXPECT_EQ(straight.GetData()[1], 177);
    EXPECT_EQ(straight.GetData()[2], 127);
    EXPECT_EQ(straight.GetData()[3], 255);

    Bitmap premultiplied = Filled(Vec2I(2, 2), Color4(100, 50, 0, 128));
    BitmapCompare::CompositeOver(premultiplied, Color4(255, 255, 255, 255), true);
    EXPECT_EQ(premultiplied.GetData()[0], 227);
    EXPECT_EQ(premultiplied.GetData()[1], 177);
    EXPECT_EQ(premultiplied.GetData()[2], 127);
}
