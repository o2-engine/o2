#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/CommonTypes.h"

using namespace o2;

namespace
{
    int BytesPerPixel(PixelFormat fmt)
    {
        return fmt == PixelFormat::R8G8B8A8 ? 4 : 3;
    }

    bool AllBytesEqual(const Bitmap& bm, UInt8 expected)
    {
        const UInt8* d = bm.GetData();
        Vec2I s = bm.GetSize();
        int total = s.x * s.y * BytesPerPixel(bm.GetFormat());
        for (int i = 0; i < total; i++)
            if (d[i] != expected)
                return false;
        return true;
    }
}

TEST(Bitmap, DefaultConstructorHasNoData)
{
    Bitmap bm;
    EXPECT_EQ(bm.GetData(), nullptr);
    EXPECT_EQ(bm.GetFormat(), PixelFormat::R8G8B8A8);
    EXPECT_TRUE(bm.GetFilename().IsEmpty());
}

TEST(Bitmap, SizedConstructorAllocatesData)
{
    Bitmap bm(PixelFormat::R8G8B8A8, Vec2I(8, 4));

    EXPECT_NE(bm.GetData(), nullptr);
    EXPECT_EQ(bm.GetSize(), Vec2I(8, 4));
    EXPECT_EQ(bm.GetFormat(), PixelFormat::R8G8B8A8);
}

TEST(Bitmap, CreateRecreatesWithNewSize)
{
    Bitmap bm;
    bm.Create(PixelFormat::R8G8B8A8, Vec2I(4, 4));
    UInt8* firstData = bm.GetData();
    ASSERT_NE(firstData, nullptr);

    bm.Create(PixelFormat::R8G8B8A8, Vec2I(16, 16));
    EXPECT_EQ(bm.GetSize(), Vec2I(16, 16));
    EXPECT_NE(bm.GetData(), nullptr);
}

TEST(Bitmap, FillWithWhiteSetsAllBytesToFF)
{
    Bitmap bm(PixelFormat::R8G8B8A8, Vec2I(4, 4));
    bm.Fill(Color4::White());
    EXPECT_TRUE(AllBytesEqual(bm, 0xFF));
}

TEST(Bitmap, FillWithFullyTransparentBlackZeroesAllBytes)
{
    Bitmap bm(PixelFormat::R8G8B8A8, Vec2I(4, 4));
    bm.Fill(Color4(0, 0, 0, 0));
    EXPECT_TRUE(AllBytesEqual(bm, 0x00));
}

TEST(Bitmap, ClearMatchesFill)
{
    Bitmap a(PixelFormat::R8G8B8A8, Vec2I(2, 2));
    Bitmap b(PixelFormat::R8G8B8A8, Vec2I(2, 2));

    a.Fill(Color4(123, 45, 67, 89));
    b.Clear(Color4(123, 45, 67, 89));

    int total = 2 * 2 * 4;
    for (int i = 0; i < total; i++)
        EXPECT_EQ(a.GetData()[i], b.GetData()[i]);
}

TEST(Bitmap, FillRectOnlyTouchesRectArea)
{
    Bitmap bm(PixelFormat::R8G8B8A8, Vec2I(4, 4));
    bm.Fill(Color4(0, 0, 0, 0));

    bm.FillRect(1, 3, 3, 1, Color4::White());

    int bpp = 4;
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            UInt8 byte = bm.GetData()[(y * 4 + x) * bpp];
            bool inside = (x >= 1 && x < 3 && y >= 1 && y < 3);
            if (inside)
                EXPECT_EQ(byte, 0xFF) << "x=" << x << " y=" << y;
            else
                EXPECT_EQ(byte, 0x00) << "x=" << x << " y=" << y;
        }
    }
}

TEST(Bitmap, FillRectClampsToBitmapBounds)
{
    Bitmap bm(PixelFormat::R8G8B8A8, Vec2I(4, 4));
    bm.Fill(Color4(0, 0, 0, 0));

    bm.FillRect(-100, 100, 100, -100, Color4::White());

    EXPECT_TRUE(AllBytesEqual(bm, 0xFF));
}

TEST(Bitmap, CopyConstructorClonesData)
{
    Bitmap a(PixelFormat::R8G8B8A8, Vec2I(2, 2));
    a.Fill(Color4::White());

    Bitmap b(a);

    EXPECT_EQ(b.GetSize(), a.GetSize());
    EXPECT_EQ(b.GetFormat(), a.GetFormat());
    EXPECT_NE(b.GetData(), a.GetData());

    int total = 2 * 2 * 4;
    for (int i = 0; i < total; i++)
        EXPECT_EQ(a.GetData()[i], b.GetData()[i]);
}

TEST(Bitmap, AssignmentOperatorClonesData)
{
    Bitmap a(PixelFormat::R8G8B8A8, Vec2I(2, 2));
    a.Fill(Color4(1, 2, 3, 4));

    Bitmap b(PixelFormat::R8G8B8A8, Vec2I(8, 8));
    b = a;

    EXPECT_EQ(b.GetSize(), Vec2I(2, 2));
    EXPECT_NE(b.GetData(), a.GetData());

    int total = 2 * 2 * 4;
    for (int i = 0; i < total; i++)
        EXPECT_EQ(a.GetData()[i], b.GetData()[i]);
}

TEST(Bitmap, CloneProducesIndependentCopy)
{
    Bitmap a(PixelFormat::R8G8B8A8, Vec2I(2, 2));
    a.Fill(Color4::White());

    Bitmap* clone = a.Clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_EQ(clone->GetSize(), a.GetSize());

    clone->Fill(Color4(0, 0, 0, 0));
    EXPECT_TRUE(AllBytesEqual(a, 0xFF));

    delete clone;
}

TEST(Bitmap, LoadOfMissingFileReturnsFalseAndKeepsFilenameEmpty)
{
    Bitmap bm;
    bool loaded = bm.Load("definitely_does_not_exist__o2_test.png", Bitmap::ImageType::Auto);

    EXPECT_FALSE(loaded);
    EXPECT_TRUE(bm.GetFilename().IsEmpty());
}

TEST(Bitmap, ColoriseWithBlackZeroesAllBytes)
{
    Bitmap bm(PixelFormat::R8G8B8A8, Vec2I(4, 4));
    bm.Fill(Color4::White());

    bm.Colorise(Color4(0, 0, 0, 0));

    EXPECT_TRUE(AllBytesEqual(bm, 0x00));
}

TEST(Bitmap, ColoriseWithWhiteIsIdentity)
{
    Bitmap bm(PixelFormat::R8G8B8A8, Vec2I(4, 4));
    bm.Fill(Color4(50, 100, 150, 200));

    UInt8 before[4 * 4 * 4];
    memcpy(before, bm.GetData(), sizeof(before));

    bm.Colorise(Color4::White());

    for (size_t i = 0; i < sizeof(before); i++)
        EXPECT_EQ(bm.GetData()[i], before[i]) << "byte " << i;
}

TEST(Bitmap, CopyImageWithMismatchedFormatIsNoOp)
{
    Bitmap dst(PixelFormat::R8G8B8A8, Vec2I(4, 4));
    dst.Fill(Color4(0, 0, 0, 0));

    Bitmap src(PixelFormat::R8G8B8, Vec2I(4, 4));
    src.Fill(Color4::White());

    dst.CopyImage(src);

    EXPECT_TRUE(AllBytesEqual(dst, 0x00));
}
