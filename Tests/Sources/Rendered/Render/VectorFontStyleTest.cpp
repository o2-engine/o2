#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/FontStyleAsset.h"
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
}

TEST(VectorFontStyle, StylesShareAtlasWithSeparateCharacters)
{
    auto font = LoadTestFont();
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    auto strokeStyle = mmake<FontStyle>();
    strokeStyle->AddEffect<FontStrokeEffect>(3.0f, Color4(255, 0, 0, 255), 100);

    auto shadowStyle = mmake<FontStyle>();
    shadowStyle->AddEffect<FontShadowEffect>(4.0f, Vec2I(4, 4), Color4(0, 0, 0, 150));

    font->CheckCharacters("A", 20, nullptr);
    font->CheckCharacters("A", 20, strokeStyle);
    font->CheckCharacters("A", 20, shadowStyle);

    auto& plain = font->GetCharacter('A', 20, nullptr);
    auto& stroked = font->GetCharacter('A', 20, strokeStyle);
    auto& shadowed = font->GetCharacter('A', 20, shadowStyle);

    ASSERT_GT(plain.mSize.x, 0.0f);
    ASSERT_GT(stroked.mSize.x, 0.0f);
    ASSERT_GT(shadowed.mSize.x, 0.0f);

    EXPECT_EQ(plain.mStyleId, 0);
    EXPECT_NE(stroked.mStyleId, 0);
    EXPECT_NE(shadowed.mStyleId, 0);
    EXPECT_NE(stroked.mStyleId, shadowed.mStyleId);

    // all styles are rendered into the same atlas texture, at different places
    EXPECT_NE(plain.mTexSrc, stroked.mTexSrc);
    EXPECT_NE(plain.mTexSrc, shadowed.mTexSrc);
    EXPECT_NE(stroked.mTexSrc, shadowed.mTexSrc);

    // stroke extends glyph, so the stroked character is bigger than the plain one
    EXPECT_GT(stroked.mSize.x, plain.mSize.x);
}

TEST(VectorFontStyle, EqualContentStylesShareGlyphs)
{
    auto font = LoadTestFont();
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    auto a = mmake<FontStyle>();
    a->AddEffect<FontStrokeEffect>(2.0f, Color4(0, 255, 0, 255), 100);

    auto b = mmake<FontStyle>();
    b->AddEffect<FontStrokeEffect>(2.0f, Color4(0, 255, 0, 255), 100);

    font->CheckCharacters("B", 16, a);
    font->CheckCharacters("B", 16, b);

    auto& charA = font->GetCharacter('B', 16, a);
    auto& charB = font->GetCharacter('B', 16, b);

    ASSERT_GT(charA.mSize.x, 0.0f);
    EXPECT_EQ(charA.mStyleId, charB.mStyleId);
    EXPECT_EQ(charA.mTexSrc, charB.mTexSrc);
}

TEST(VectorFontStyle, EmptyStyleFallsBackToPlainCharacters)
{
    auto font = LoadTestFont();
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    auto emptyStyle = mmake<FontStyle>();

    font->CheckCharacters("C", 18, nullptr);
    font->CheckCharacters("C", 18, emptyStyle);

    auto& plain = font->GetCharacter('C', 18, nullptr);
    auto& styled = font->GetCharacter('C', 18, emptyStyle);

    ASSERT_GT(plain.mSize.x, 0.0f);
    EXPECT_EQ(styled.mStyleId, 0);
    EXPECT_EQ(plain.mTexSrc, styled.mTexSrc);
}

TEST(VectorFontStyle, StyleChangeRendersFreshGlyphs)
{
    auto font = LoadTestFont();
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    auto style = mmake<FontStyle>();
    style->AddEffect<FontStrokeEffect>(2.0f, Color4(255, 255, 0, 255), 100);

    font->CheckCharacters("D", 22, style);
    RectF oldTexSrc = font->GetCharacter('D', 22, style).mTexSrc;
    int oldStyleId = font->GetCharacter('D', 22, style).mStyleId;

    style->SetEffects({ mmake<FontShadowEffect>(5.0f, Vec2I(6, 6), Color4(0, 0, 0, 200)) });

    font->CheckCharacters("D", 22, style);
    auto& fresh = font->GetCharacter('D', 22, style);

    ASSERT_GT(fresh.mSize.x, 0.0f);
    EXPECT_NE(fresh.mStyleId, oldStyleId);
    EXPECT_NE(fresh.mTexSrc, oldTexSrc);
}

TEST(VectorFontStyle, DifferentHeightsOfSameStyleAreSeparate)
{
    auto font = LoadTestFont();
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    auto style = mmake<FontStyle>();
    style->AddEffect<FontStrokeEffect>(2.0f, Color4(0, 0, 255, 255), 100);

    font->CheckCharacters("E", 12, style);
    font->CheckCharacters("E", 24, style);

    auto& small = font->GetCharacter('E', 12, style);
    auto& big = font->GetCharacter('E', 24, style);

    ASSERT_GT(small.mSize.x, 0.0f);
    ASSERT_GT(big.mSize.x, 0.0f);
    EXPECT_EQ(small.mStyleId, big.mStyleId);
    EXPECT_NE(small.mTexSrc, big.mTexSrc);
    EXPECT_GT(big.mSize.x, small.mSize.x);
}

TEST(TextFontStyle, TextUsesStyleForMeshAndRebuildsOnStyleChange)
{
    auto font = LoadTestFont();
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    auto text = mmake<Text>(Ref<Font>(font));
    text->SetHeight(20);
    text->SetText("Hi");

    Vec2F plainSize = text->GetRealSize();
    ASSERT_GT(plainSize.x, 0.0f);

    RectF plainTexSrc = text->GetSymbolsSet().mLines[0].mSymbols[0].mTexSrc;

    auto style = mmake<FontStyle>();
    style->AddEffect<FontStrokeEffect>(3.0f, Color4(255, 0, 0, 255), 100);
    text->SetFontStyle(style);

    EXPECT_EQ(text->GetFontStyle(), style);

    RectF styledTexSrc = text->GetSymbolsSet().mLines[0].mSymbols[0].mTexSrc;
    EXPECT_NE(styledTexSrc, plainTexSrc);

    // mutating the style regenerates glyphs and rebuilds the mesh through onChanged
    style->SetEffects({ mmake<FontShadowEffect>(4.0f, Vec2I(5, 5), Color4(0, 0, 0, 150)) });

    RectF changedTexSrc = text->GetSymbolsSet().mLines[0].mSymbols[0].mTexSrc;
    EXPECT_NE(changedTexSrc, styledTexSrc);
}

TEST(TextFontStyle, FontStyleAssetInstanceOnText)
{
    auto font = LoadTestFont();
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    auto text = mmake<Text>(Ref<Font>(font));
    text->SetHeight(18);
    text->SetText("Ok");

    AssetRef<FontStyleAsset> styleAsset;
    styleAsset.CreateInstance();
    styleAsset->AddEffect<FontStrokeEffect>(2.0f, Color4(0, 255, 255, 255), 100);

    text->SetFontStyleAsset(styleAsset);

    EXPECT_EQ(text->GetFontStyleAsset(), styleAsset);
    EXPECT_EQ(text->GetFontStyle(), Ref<FontStyle>(styleAsset.GetRef()));

    auto& styled = font->GetCharacter('O', 18, styleAsset.GetRef());
    EXPECT_GT(styled.mSize.x, 0.0f);
    EXPECT_NE(styled.mStyleId, 0);
}

TEST(TextFontStyle, SetFontStyleWithAssetFillsAssetRef)
{
    auto font = LoadTestFont();

    auto text = mmake<Text>(Ref<Font>(font));

    AssetRef<FontStyleAsset> styleAsset;
    styleAsset.CreateInstance();
    styleAsset->AddEffect<FontStrokeEffect>();

    text->SetFontStyle(styleAsset.GetRef());
    EXPECT_EQ(text->GetFontStyleAsset().Get(), styleAsset.Get());

    auto rawStyle = mmake<FontStyle>();
    text->SetFontStyle(rawStyle);
    EXPECT_FALSE(text->GetFontStyleAsset().IsValid());
    EXPECT_EQ(text->GetFontStyle(), rawStyle);
}
