#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2AssetBuilder/AstcCompressor.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Bitmap/DdsFormat.h"

using namespace o2;

// The whole compressed-texture path on this platform: encode a bitmap with the built-in
// encoders, load through Texture::Create, draw and check pixels come out in the right
// colors and orientation. Runs for every supported block compression.
namespace
{
    // Quadrant colors in bitmap data space: rows are stored bottom-up, so y < 32 is the
    // bottom half of the image. Pixels are written directly in the RGBA byte order the
    // encoders expect (the same order png-loaded bitmaps use; Bitmap::Fill writes BGRA)
    Bitmap MakeQuadrantBitmap()
    {
        Bitmap bitmap(PixelFormat::R8G8B8A8, Vec2I(64, 64));
        UInt8* pixels = bitmap.GetData();
        for (int y = 0; y < 64; y++)
        {
            for (int x = 0; x < 64; x++)
            {
                UInt8* p = pixels + (y*64 + x)*4;
                bool top = y >= 32, right = x >= 32;
                p[0] = (!top && !right) || (top && right) ? 255 : 0; // red in bottom-left and top-right
                p[1] = (!top && right) || (top && right) ? 255 : 0;  // green in bottom-right, top-right
                p[2] = top && !right ? 255 : 0;                      // blue in top-left
                p[3] = 255;
            }
        }
        // resulting quadrants: bottom-left red, bottom-right green, top-left blue, top-right yellow
        return bitmap;
    }

    void CheckTextureDrawsQuadrants(const TextureRef& texture, TextureFormat expectedFormat)
    {
        ASSERT_TRUE(texture.IsValid());
        EXPECT_EQ(texture->GetFormat(), expectedFormat);
        EXPECT_EQ(texture->GetSize(), Vec2I(64, 64));

        Ref<Bitmap> captured;
        o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

        o2Render.Begin();
        o2Render.Clear(Color4::Black());
        o2Render.SetCamera(Camera());

        Sprite sprite(texture, RectI(0, 0, 64, 64));
        sprite.SetSize(Vec2F(256.0f, 256.0f));
        sprite.SetPosition(Vec2F(0.0f, 0.0f));
        sprite.Draw();

        o2Render.SetCamera(Camera());
        o2Render.End();

        ASSERT_TRUE(captured);

        Vec2I size = captured->GetSize();
        const UInt8* data = captured->GetData();
        auto pixel = [&](int x, int y) { return data + (y*size.x + x)*4; };

        // Captured bitmap rows are bottom-up too: y below the centre is the bottom of the screen.
        // Sample quadrant centres, far from the block boundaries; block codecs are near-exact on
        // flat colors, the tolerance covers BC7/ASTC rounding.
        int cx = size.x/2, cy = size.y/2, q = 64;

        const UInt8* bottomLeft = pixel(cx - q, cy - q);
        EXPECT_GT(bottomLeft[0], 215); EXPECT_LT(bottomLeft[1], 40); EXPECT_LT(bottomLeft[2], 40);

        const UInt8* bottomRight = pixel(cx + q, cy - q);
        EXPECT_LT(bottomRight[0], 40); EXPECT_GT(bottomRight[1], 215); EXPECT_LT(bottomRight[2], 40);

        const UInt8* topLeft = pixel(cx - q, cy + q);
        EXPECT_LT(topLeft[0], 40); EXPECT_LT(topLeft[1], 40); EXPECT_GT(topLeft[2], 215);

        const UInt8* topRight = pixel(cx + q, cy + q);
        EXPECT_GT(topRight[0], 215); EXPECT_GT(topRight[1], 215); EXPECT_LT(topRight[2], 40);
    }

    void CheckDdsFormatEndToEnd(TextureFormat format, int quality)
    {
        Bitmap bitmap = MakeQuadrantBitmap();

        // Unique file per format: the render caches textures by filename
        String ddsPath = String("dds_texture_test_tmp_") + (String)(int)format + ".dds";
        ASSERT_TRUE(SaveDds(bitmap, ddsPath, format, quality));

        TextureRef texture(ddsPath);
        remove(ddsPath.Data());

        CheckTextureDrawsQuadrants(texture, format);
    }
}

// One suite per format: ctest runs each suite in its own process, giving every test a fresh
// metal drawable for the frame capture
TEST(DdsTextureDxt5, DrawsWithCorrectColorsAndOrientation)
{
    CheckDdsFormatEndToEnd(TextureFormat::DXT5, 100);
}

TEST(DdsTextureDxt1, DrawsWithCorrectColorsAndOrientation)
{
    CheckDdsFormatEndToEnd(TextureFormat::DXT1, 100);
}

TEST(DdsTextureBc7, DrawsWithCorrectColorsAndOrientation)
{
    CheckDdsFormatEndToEnd(TextureFormat::BC7, 50);
}

TEST(AstcTexture, EncodedFileDrawsWithCorrectColorsAndOrientation)
{
    Bitmap bitmap = MakeQuadrantBitmap();

    const char* astcPath = "astc_texture_test_tmp.astc";
    ASSERT_TRUE(SaveAstc4x4(bitmap, astcPath, 60));

    // .astc container: magic and 4x4 block dims
    {
        FILE* f = fopen(astcPath, "rb");
        ASSERT_NE(f, nullptr);
        UInt8 header[16];
        fread(header, 1, 16, f);
        fclose(f);

        EXPECT_EQ(header[0], 0x13); EXPECT_EQ(header[1], 0xAB);
        EXPECT_EQ(header[2], 0xA1); EXPECT_EQ(header[3], 0x5C);
        EXPECT_EQ(header[4], 4); EXPECT_EQ(header[5], 4); EXPECT_EQ(header[6], 1);
    }

    String astcPathStr(astcPath);
    TextureRef texture(astcPathStr);
    remove(astcPath);

    // Intel macs have no ASTC support in Metal; skip there instead of failing
    if (!texture.IsValid() || !texture->IsReady())
        GTEST_SKIP() << "ASTC textures are not supported by this GPU";

    CheckTextureDrawsQuadrants(texture, TextureFormat::ASTC4x4);
}
