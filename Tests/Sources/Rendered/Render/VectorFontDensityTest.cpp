#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Assets.h"
#include "o2/Render/FontStyle.h"
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
