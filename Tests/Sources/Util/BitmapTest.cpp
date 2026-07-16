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

TEST(Bitmap, LoadExpandsPalettePngToRgba)
{
	// 2x2 8-bit palette PNG with tRNS: top row red, green; bottom row blue, transparent
	static const UInt8 palettePng[] = {
		0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
		0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x0F, 0xD8, 0xE5,
		0xB7, 0x00, 0x00, 0x00, 0x0C, 0x50, 0x4C, 0x54, 0x45, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00,
		0x00, 0xFF, 0x00, 0x00, 0x00, 0xFB, 0xBE, 0x46, 0xE4, 0x00, 0x00, 0x00, 0x04, 0x74, 0x52, 0x4E,
		0x53, 0xFF, 0xFF, 0xFF, 0x00, 0x40, 0x2A, 0xA9, 0xF4, 0x00, 0x00, 0x00, 0x0C, 0x49, 0x44, 0x41,
		0x54, 0x78, 0x9C, 0x63, 0x10, 0x60, 0xD8, 0x00, 0x00, 0x00, 0xE4, 0x00, 0xC1, 0x27, 0xA8, 0xE8,
		0x57, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
	};

	const char* path = "bitmap_palette_test_tmp.png";
	FILE* f = fopen(path, "wb");
	ASSERT_NE(f, nullptr);
	fwrite(palettePng, 1, sizeof(palettePng), f);
	fclose(f);

	Bitmap bm;
	bool loaded = bm.Load(path, Bitmap::ImageType::Png);
	remove(path);

	ASSERT_TRUE(loaded);
	ASSERT_EQ(bm.GetSize(), Vec2I(2, 2));
	ASSERT_EQ(bm.GetFormat(), PixelFormat::R8G8B8A8);

	// rows are stored bottom-up: bitmap row 0 is the PNG's bottom row
	auto pixel = [&](int x, int y) { return bm.GetData() + (y * 2 + x) * 4; };

	EXPECT_EQ(pixel(0, 0)[2], 0xFF); // blue, opaque
	EXPECT_EQ(pixel(0, 0)[3], 0xFF);
	EXPECT_EQ(pixel(1, 0)[3], 0x00); // transparent
	EXPECT_EQ(pixel(0, 1)[0], 0xFF); // red, opaque
	EXPECT_EQ(pixel(0, 1)[3], 0xFF);
	EXPECT_EQ(pixel(1, 1)[1], 0xFF); // green, opaque
	EXPECT_EQ(pixel(1, 1)[3], 0xFF);
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

// Outline draws an anti-aliased stroke ring around opaque pixels: full stroke alpha inside the
// radius, fading to zero over one pixel past it, glyph pixels composited on top of the stroke
TEST(Bitmap, OutlineFadesStrokePastRadiusAndKeepsGlyphPixels)
{
    auto pixelAt = [](const Bitmap& bm, int x, int y) {
        Color4 c;
        c.SetABGR(*(const Color32Bit*)(bm.GetData() + (y * bm.GetSize().x + x) * 4));
        return c;
    };

    Bitmap bm(PixelFormat::R8G8B8A8, Vec2I(15, 15));
    bm.Fill(Color4(0, 0, 0, 0));

    // single opaque pixel: every distance below is exact Euclidean, whatever the row order
    Color32Bit white = Color4(255, 255, 255, 255).ABGR();
    memcpy(bm.GetData() + (7 * 15 + 7) * 4, &white, 4);

    const Color4 stroke(255, 0, 0, 200);
    bm.Outline(2.0f, stroke, 100);

    EXPECT_EQ(pixelAt(bm, 7, 7), Color4(255, 255, 255, 255)); // glyph pixel stays on top

    Color4 insideRing = pixelAt(bm, 9, 7); // distance 2 = radius -> full stroke alpha
    EXPECT_GT(insideRing.a, 190);
    EXPECT_GT(insideRing.r, 150);
    EXPECT_EQ(insideRing.g, 0);

    Color4 fadeRing = pixelAt(bm, 9, 8); // distance ~2.24 -> stroke alpha faded by ~24%
    EXPECT_GT(fadeRing.a, 100);
    EXPECT_LT(fadeRing.a, 190);

    EXPECT_EQ(pixelAt(bm, 12, 7).a, 0); // out of the stroke reach, untouched
}
