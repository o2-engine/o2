#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Assets.h"
#include "o2/Render/Camera.h"
#include "o2/Render/FontStyle.h"
#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorFont.h"
#include "o2/Render/VectorFontEffects.h"

using namespace o2;

namespace
{
    Ref<VectorFont> LoadTestFont()
    {
        return mmake<VectorFont>(o2Assets.GetBuiltAssetsPath() + "debugFont.ttf");
    }

    Vec2F TexturePixels(const Ref<VectorFont>& font, const RectF& texSrc)
    {
        Vec2F size = texSrc.Size()*(Vec2F)font->GetTexture()->GetSize();
        return Vec2F(Math::Abs(size.x), Math::Abs(size.y));
    }
}

TEST(VectorFontDensity, DenseGlyphHasMorePixelsAndSameLayout)
{
    auto font = LoadTestFont();
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    font->CheckCharacters("A", 20, nullptr);
    font->CheckCharacters("A", 20, nullptr, 2.0f);

    auto plain = font->GetCharacter('A', 20, nullptr);
    auto dense = font->GetCharacter('A', 20, nullptr, 2.0f);

    ASSERT_GT(plain.mSize.x, 0.0f);
    ASSERT_GT(dense.mSize.x, 0.0f);
    EXPECT_NE(plain.mTexSrc, dense.mTexSrc);

    EXPECT_FLOAT_EQ(dense.mAdvance, plain.mAdvance);
    EXPECT_NEAR(dense.mSize.x, plain.mSize.x, 3.0f);
    EXPECT_NEAR(dense.mSize.y, plain.mSize.y, 3.0f);
    EXPECT_NEAR(dense.mOrigin.x, plain.mOrigin.x, 1.5f);
    EXPECT_NEAR(dense.mOrigin.y, plain.mOrigin.y, 1.5f);

    Vec2F plainPixels = TexturePixels(font, plain.mTexSrc);
    Vec2F densePixels = TexturePixels(font, dense.mTexSrc);
    EXPECT_NEAR(densePixels.x, dense.mSize.x*2.0f, 0.01f);
    EXPECT_NEAR(densePixels.y, dense.mSize.y*2.0f, 0.01f);
    EXPECT_GT(densePixels.x, plainPixels.x*1.5f);
}

TEST(VectorFontDensity, StyledGlyphKeepsSingleDensity)
{
    auto font = LoadTestFont();
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    auto style = mmake<FontStyle>();
    style->AddEffect<FontStrokeEffect>(2.0f, Color4(255, 0, 0, 255), 100);

    font->CheckCharacters("A", 20, style);
    font->CheckCharacters("A", 20, style, 2.0f);

    auto plain = font->GetCharacter('A', 20, style);
    auto dense = font->GetCharacter('A', 20, style, 2.0f);

    ASSERT_GT(dense.mSize.x, 0.0f);
    EXPECT_EQ(dense.mSize, plain.mSize);
    EXPECT_EQ(dense.mTexSrc, plain.mTexSrc) << "the styled glyph is rendered once for any density";
}

TEST(VectorFontDensity, TextSizeDoesNotNeedPreparedGlyphs)
{
    auto font = LoadTestFont();
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    Vec2F size = Text::GetTextSize("Hello", font, 14);
    EXPECT_GT(size.x, 10.0f);
    EXPECT_GT(size.y, 5.0f);

    font->CheckCharacters("Hello", 14, nullptr, 2.0f);
    EXPECT_EQ(Text::GetTextSize("Hello", font, 14), size);

    font->CheckCharacters("Wide", 14, nullptr, 2.0f);
    EXPECT_GT(Text::GetTextSize("Wide", font, 14).x, 10.0f) << "glyphs rendered only for the dense screen are measured too";
}

TEST(VectorFontDensity, ZoomedCameraTakesGlyphsOfTheViewScale)
{
    auto font = LoadTestFont();
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    Text text(font);
    text.SetHeight(17);
    text.SetText("Zoom");
    text.SetRect(RectF(-50, -20, 50, 20));

    auto draw = [&](float zoom)
    {
        float density = 0.0f;

        o2Render.Begin();

        Camera camera;
        camera.SetScale(Vec2F(1.0f/zoom, 1.0f/zoom));
        o2Render.SetCamera(camera);
        density = o2Render.GetViewPixelScale().x;
        text.Draw();

        o2Render.SetCamera(Camera());
        o2Render.End();

        return density;
    };

    float plain = draw(1.0f);
    float zoomed = draw(2.0f);
    ASSERT_FLOAT_EQ(zoomed, plain*2.0f);

    auto glyph = font->GetCharacter('Z', 17, nullptr, zoomed);
    ASSERT_GT(glyph.mSize.x, 0.0f) << "glyphs of the zoomed view are rendered";
    EXPECT_FLOAT_EQ(glyph.mPixelDensity, zoomed);
    EXPECT_NEAR(TexturePixels(font, glyph.mTexSrc).x, glyph.mSize.x*zoomed, 0.01f);

    float farView = draw(0.5f);
    EXPECT_GT(font->GetCharacter('Z', 17, nullptr, farView).mSize.x, 0.0f) << "a far view takes smaller glyphs";

    EXPECT_FLOAT_EQ(font->GetCharacter('Z', 17, nullptr, zoomed).mAdvance, font->GetCharacter('Z', 17, nullptr).mAdvance);
}

namespace
{
    // The alpha of the texture pixels a glyph takes, row by row from the texture top (v = 1 - row/height)
    Vector<UInt8> GlyphAlpha(const Ref<VectorFont>& font, const RectF& texSrc)
    {
        auto bitmap = font->GetTexture()->GetData();
        Vec2I size = bitmap->GetSize();
        int left = Math::RoundToInt(texSrc.left*size.x), right = Math::RoundToInt(texSrc.right*size.x);
        int top = Math::RoundToInt((1.0f - texSrc.top)*size.y), bottom = Math::RoundToInt((1.0f - texSrc.bottom)*size.y);

        Vector<UInt8> alpha;
        for (int row = Math::Min(top, bottom); row < Math::Max(top, bottom); row++)
            for (int x = left; x < right; x++)
                alpha.Add(bitmap->GetData()[(row*size.x + x)*4 + 3]);

        return alpha;
    }
}

TEST(VectorFontAtlas, GrowingAtlasKeepsTheGlyphsItHad)
{
    auto font = LoadTestFont();
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    font->CheckCharacters("A", 40, nullptr);
    Vector<UInt8> before = GlyphAlpha(font, font->GetCharacter('A', 40, nullptr).mTexSrc);
    int coverage = 0;
    for (auto a : before)
        coverage += a;
    ASSERT_GT(coverage, 0);

    Vec2I startSize = font->GetTexture()->GetSize();
    WString letters;
    for (wchar_t ch = 'B'; ch <= 'Z'; ch++)
        letters += ch;

    for (int height = 60; height <= 400 && font->GetTexture()->GetSize() == startSize; height += 20)
        font->CheckCharacters(letters, height, nullptr);

    ASSERT_NE(font->GetTexture()->GetSize(), startSize) << "the atlas has grown";
    EXPECT_EQ(GlyphAlpha(font, font->GetCharacter('A', 40, nullptr).mTexSrc), before)
        << "the glyph is where its texture coordinates point in the grown atlas";
}
