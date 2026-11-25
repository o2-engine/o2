#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Assets.h"
#include "o2/Render/Camera.h"
#include "o2/Render/FontStyle.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Render.h"
#include "o2/Render/SdfFont.h"
#include "o2/Render/SdfGlyphGenerator.h"
#include "o2/Render/SdfTextStyle.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorFont.h"
#include "o2/Render/VectorFontEffects.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Bitmap/BitmapCompare.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Types/Containers/Pair.h"

using namespace o2;

// Distance field fonts: the fields, the styles turned into layers, the baked atlas, and the texts drawn by them against
// the ones of the bitmap atlas of the vector font

namespace
{
    const Color4 background(255, 255, 255, 255);

    String TestFontPath()
    {
        return o2Assets.GetBuiltAssetsPath() + "debugFont.ttf";
    }

    Ref<SdfFont> LoadSdfFont(const SdfFontSettings& settings = SdfFontSettings())
    {
        return mmake<SdfFont>(TestFontPath(), settings);
    }

    Ref<VectorFont> LoadVectorFont()
    {
        return mmake<VectorFont>(TestFontPath());
    }

    // An effect without a distance field version
    class InvertEffect: public VectorFont::Effect
    {
    public:
        void Process(Bitmap& bitmap) override {}
    };

    Ref<Bitmap> DrawAndCapture(const Function<void()>& draw, float zoom = 1.0f)
    {
        Ref<Bitmap> captured;
        for (int i = 0; i < 2; i++)
        {
            o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

            o2Render.Begin();

            Camera camera;
            camera.SetScale(Vec2F(1.0f/zoom, 1.0f/zoom));
            o2Render.SetCamera(camera);
            o2Render.Clear(background);
            draw();
            o2Render.SetCamera(Camera());
            o2Render.End();
        }

        return captured;
    }

    Ref<Text> MakeText(const Ref<Font>& font, int height, const Ref<FontStyle>& style = nullptr,
                       const WString& string = "Hamburgefonstiv 0123\nQuick brown fox!")
    {
        auto text = mmake<Text>(font);
        text->SetFontStyle(style);
        text->SetHeight(height);
        text->SetColor(Color4(255, 255, 255, 255));
        text->SetText(string);
        text->SetPosition(Vec2F(-200, 60));
        return text;
    }

    void SaveShot(const Ref<Bitmap>& bitmap, const String& name)
    {
        const char* folder = getenv("O2_TEXT_SHOTS");
        if (folder && bitmap)
            bitmap->Save(String(folder) + "/" + name + ".png", Bitmap::ImageType::Png);
    }

    // Pixels darker than the background: where the text is
    int CountInk(const Bitmap& bitmap)
    {
        int ink = 0;
        Vec2I size = bitmap.GetSize();
        const UInt8* data = bitmap.GetData();
        for (int i = 0; i < size.x*size.y; i++)
            ink += (data[i*4] + data[i*4 + 1] + data[i*4 + 2]) < 600 ? 1 : 0;

        return ink;
    }

    // Pixels of partial coverage over pixels of the text: the share of the antialiased edge
    float EdgeShare(const Bitmap& bitmap)
    {
        int ink = 0, edge = 0;
        Vec2I size = bitmap.GetSize();
        const UInt8* data = bitmap.GetData();
        for (int i = 0; i < size.x*size.y; i++)
        {
            int value = data[i*4];
            if (value < 240)
                ink++;
            if (value > 40 && value < 215)
                edge++;
        }

        return ink > 0 ? (float)edge/(float)ink : 0.0f;
    }

    Ref<FontStyle> MakeStyle(const Vector<Ref<VectorFont::Effect>>& effects)
    {
        auto style = mmake<FontStyle>();
        style->SetEffects(effects);
        return style;
    }

    // The styles of the game: colour + strokes + shadows, gradient + stroke + shadow
    Vector<Pair<String, Ref<FontStyle>>> TestStyles()
    {
        Vector<Pair<String, Ref<FontStyle>>> styles;
        styles.Add({ "plain", nullptr });
        styles.Add({ "color", MakeStyle({ mmake<FontColorEffect>(Color4(146, 62, 80, 255)) }) });
        styles.Add({ "stroke", MakeStyle({ mmake<FontColorEffect>(Color4(40, 40, 40, 255)),
                                          mmake<FontStrokeEffect>(3.0f, Color4(230, 60, 60, 255), 100) }) });
        styles.Add({ "shadow", MakeStyle({ mmake<FontColorEffect>(Color4(60, 60, 200, 255)),
                                          mmake<FontShadowEffect>(3.0f, Vec2I(3, 4), Color4(0, 0, 0, 160)) }) });
        styles.Add({ "gradient", MakeStyle({ mmake<FontGradientEffect>(Color4(234, 170, 54, 255), Color4(60, 20, 180, 255)) }) });
        styles.Add({ "sharpen", MakeStyle({ mmake<FontColorEffect>(Color4(30, 30, 30, 255)), mmake<FontSharpenEffect>(3.0f, 128) }) });
        styles.Add({ "game", MakeStyle({ mmake<FontGradientEffect>(Color4(234, 170, 54, 255), Color4(255, 246, 194, 255)),
                                        mmake<FontStrokeEffect>(3.0f, Color4(255, 232, 160, 255), 100),
                                        mmake<FontStrokeEffect>(7.0f, Color4(89, 53, 15, 255), 100),
                                        mmake<FontShadowEffect>(2.0f, Vec2I(0, 5), Color4(54, 30, 8, 140)) }) });
        return styles;
    }
}

TEST(SdfGlyphGenerator, FieldIsInsideTheOutlineAndOutsideAround)
{
    FT_Library library;
    ASSERT_EQ(FT_Init_FreeType(&library), 0);
    FT_Face face;
    ASSERT_EQ(FT_New_Face(library, TestFontPath().Data(), 0, &face), 0);

    SdfFontSettings settings;
    SdfGlyphField field;
    ASSERT_TRUE(SdfGlyphGenerator::Generate(face, 'I', settings, field));
    ASSERT_GT(field.size.x, 2*settings.GetPadding());
    ASSERT_GT(field.size.y, 2*settings.GetPadding());
    EXPECT_GT(field.advance, 0.1f);
    EXPECT_EQ(field.pixels.Count(), field.size.x*field.size.y*4);

    // The middle of the stem is inside, the corners of the field are far outside
    auto alpha = [&](int x, int y) { return field.pixels[(y*field.size.x + x)*4 + 3]; };
    EXPECT_GT(alpha(field.size.x/2, field.size.y/2), 128);
    EXPECT_LT(alpha(0, 0), 20);
    EXPECT_LT(alpha(field.size.x - 1, field.size.y - 1), 20);

    // The box of the field in em holds the outline with the padding
    float padding = (float)settings.GetPadding()/(float)settings.glyphSize;
    EXPECT_NEAR(field.bounds.Width(), (float)field.size.x/(float)settings.glyphSize, 1e-4f);
    EXPECT_LT(field.bounds.bottom, -padding*0.5f);
    EXPECT_GT(field.bounds.top, 0.5f);

    SdfGlyphField space;
    ASSERT_TRUE(SdfGlyphGenerator::Generate(face, ' ', settings, space));
    EXPECT_EQ(space.size, Vec2I());
    EXPECT_GT(space.advance, 0.1f);

    FT_Done_Face(face);
    FT_Done_FreeType(library);
}

TEST(SdfTextStyle, EffectsTurnIntoLayers)
{
    SdfTextStyle plain = SdfTextStyle::Build(nullptr);
    EXPECT_TRUE(plain.IsSupported());
    ASSERT_EQ(plain.GetLayers().Count(), 1);
    EXPECT_TRUE(plain.GetLayers()[0].sharpCorners);

    auto game = TestStyles().Last().second;
    SdfTextStyle style = SdfTextStyle::Build(game);
    ASSERT_TRUE(style.IsSupported());
    ASSERT_EQ(style.GetLayers().Count(), 4);
    EXPECT_EQ(style.GetGradients().Count(), 1);

    // The glyph with the gradient, two strokes one around the other, the shadow of all of them
    auto& layers = style.GetLayers();
    EXPECT_EQ(layers[0].gradient, 0);
    EXPECT_EQ(layers[1].gradient, -1);
    EXPECT_GT(layers[2].taps[0].dilation, layers[1].taps[0].dilation);
    EXPECT_TRUE(layers[3].shadow);
    EXPECT_FLOAT_EQ(layers[3].taps[0].dilation, layers[2].taps[0].dilation);
    EXPECT_EQ(layers[3].taps[0].offset, Vec2F(0, 5));

    Vec2F extent = style.GetExtent();
    EXPECT_GT(extent.y, extent.x);
    EXPECT_GT(extent.x, layers[2].taps[0].dilation);

    // A later shadow casts the earlier ones too
    SdfTextStyle twoShadows = SdfTextStyle::Build(MakeStyle({ mmake<FontShadowEffect>(0.5f, Vec2I(3, 7), Color4(0, 0, 0, 128)),
                                                             mmake<FontShadowEffect>(5.0f, Vec2I(2, 5), Color4(0, 0, 0, 89)) }));
    ASSERT_EQ(twoShadows.GetLayers().Count(), 3);
    ASSERT_EQ(twoShadows.GetLayers()[2].taps.Count(), 2);
    EXPECT_EQ(twoShadows.GetLayers()[2].taps[1].offset, Vec2F(5, 12));
    EXPECT_NEAR(twoShadows.GetLayers()[2].taps[1].alpha, 128.0f/255.0f, 1e-3f);

    // An effect without the distance field version leaves the style to the bitmap atlas
    auto custom = MakeStyle({ mmake<InvertEffect>() });
    EXPECT_FALSE(SdfTextStyle::Build(custom).IsSupported());

    auto font = LoadSdfFont();
    EXPECT_TRUE(font->IsDistanceFieldStyle(game));
    EXPECT_FALSE(font->IsDistanceFieldStyle(custom));

    Vector<float> packed;
    style.Pack(packed);
    EXPECT_EQ(packed.Count(), SdfTextStyle::maxLayers*SdfTextStyle::vectorsPerLayer*4);
}

TEST(SdfTextStyle, StrokeCoversWhatBitmapOutlineCovers)
{
    // Where the alpha of Bitmap::Outline fades out of a straight edge, on average over the edge's places in a pixel
    EXPECT_NEAR(SdfTextStyle::GetStrokeWidth(2.0f, 100), 1.9f, 0.3f);
    EXPECT_NEAR(SdfTextStyle::GetStrokeWidth(7.0f, 100), 5.0f, 0.3f);
    EXPECT_EQ(SdfTextStyle::GetStrokeWidth(0.0f, 100), 0.0f);

    float previous = 0.0f;
    for (int radius = 2; radius <= 12; radius++)
    {
        float width = SdfTextStyle::GetStrokeWidth((float)radius, 100);
        EXPECT_GT(width, previous) << radius;
        previous = width;
    }
}

TEST(SdfFont, WarmUpGeneratesOnceForAnySizeAndStyle)
{
    auto font = LoadSdfFont();
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    EXPECT_EQ(font->WarmUp("Hello"), 4);
    EXPECT_EQ(font->WarmUp("Hello, world"), 5);
    EXPECT_EQ(font->WarmUp("Hello"), 0);

    int generated = font->GetStatistics().generatedGlyphs;
    for (auto& style : TestStyles())
    {
        for (int height : { 10, 24, 60 })
        {
            font->CheckCharacters("Hello", height, style.second);
            auto& character = font->GetCharacter('H', height, style.second);
            EXPECT_GT(character.mSize.x, 0.0f) << style.first.Data() << " " << height;
            EXPECT_GT(character.mAdvance, 0.0f);
        }
    }

    EXPECT_EQ(font->GetStatistics().generatedGlyphs, generated);

    // Sizes scale linearly: one field for every height
    auto& small = font->GetCharacter('H', 10, nullptr);
    auto& big = font->GetCharacter('H', 60, nullptr);
    EXPECT_NEAR(big.mAdvance/small.mAdvance, 6.0f, 0.01f);

    // A style with effects makes the quads bigger by their reach
    auto stroked = TestStyles()[2].second;
    EXPECT_GT(font->GetCharacter('H', 24, stroked).mSize.x, font->GetCharacter('H', 24, nullptr).mSize.x + 4.0f);
}

TEST(SdfFont, BakedFieldsLoadTheSameGlyphs)
{
    String bakedPath = "SdfFontTestFields.ttf.sdf";
    SdfFontSettings settings;
    WString characters = "ABCabc 0123 Привет";

    ASSERT_TRUE(SdfFont::Bake(TestFontPath(), settings, characters, bakedPath));
    EXPECT_TRUE(o2FileSystem.IsFileExist(bakedPath));
    EXPECT_FALSE(SdfFont::ReadBakedStamp(bakedPath).IsEmpty());

    auto generated = LoadSdfFont(settings);
    generated->WarmUp(characters);

    // Loading reads the table only: the fields are unpacked when the characters are shown
    auto baked = LoadSdfFont(settings);
    ASSERT_TRUE(baked->LoadBaked(bakedPath));
    EXPECT_EQ(baked->GetGlyphsCount(), 0);
    EXPECT_EQ(baked->GetBakedGlyphsCount(), 17);
    EXPECT_EQ(baked->WarmUp(characters), generated->GetGlyphsCount());
    EXPECT_EQ(baked->GetStatistics().generatedGlyphs, 0);
    EXPECT_EQ(baked->GetStatistics().bakedGlyphs, generated->GetGlyphsCount());

    // A character that wasn't baked is generated
    EXPECT_EQ(baked->WarmUp("Z"), 1);
    EXPECT_EQ(baked->GetStatistics().generatedGlyphs, 1);

    for (int i = 0; i < characters.Length(); i++)
    {
        auto a = generated->GetGlyph(characters[i]);
        auto b = baked->GetGlyph(characters[i]);
        ASSERT_TRUE(a && b);
        EXPECT_EQ(a->atlasRect, b->atlasRect);
        EXPECT_NEAR(a->advance, b->advance, 1e-5f);
        EXPECT_NEAR(a->bounds.left, b->bounds.left, 1e-5f);
        EXPECT_NEAR(a->inkBounds.top, b->inkBounds.top, 1e-5f);
        EXPECT_EQ(a->empty, b->empty);
    }

    // Other settings don't take the fields
    SdfFontSettings other = settings;
    other.glyphSize = 32;
    EXPECT_FALSE(LoadSdfFont(other)->LoadBaked(bakedPath));

    // Both draw the same text
    auto bakedFrame = DrawAndCapture([&]() { MakeText(baked, 30, nullptr, "ABC abc\n0123 Привет")->Draw(); });
    auto generatedFrame = DrawAndCapture([&]() { MakeText(generated, 30, nullptr, "ABC abc\n0123 Привет")->Draw(); });
    ASSERT_TRUE(bakedFrame && generatedFrame);
    BitmapCompareResult result = BitmapCompare::Compare(*bakedFrame, *generatedFrame, 8, background);
    EXPECT_GE(result.similarity, 0.999f);

    o2FileSystem.FileDelete(bakedPath);
}

TEST(SdfFontRender, TextIsDrawnByTheFields)
{
    auto font = LoadSdfFont();
    auto text = MakeText(font, 24);
    text->SetColor(Color4(20, 30, 40, 255));

    auto frame = DrawAndCapture([&]() { text->Draw(); });
    ASSERT_TRUE(frame);
    EXPECT_GT(CountInk(*frame), 500);

    // One quad per visible glyph, in the layout of the field vertices, with the material of the fields
    auto& meshes = text->GetSymbolsSet();
    int visible = 0;
    for (auto& line : meshes.mLines)
        for (auto& symbol : line.mSymbols)
            visible += symbol.mCharId != ' ' ? 1 : 0;

    EXPECT_GT(visible, 20);
    EXPECT_TRUE(font->GetFieldTexture());
}

TEST(SdfFontRender, StylesLookAsOnTheVectorFont)
{
    // Strokes of 7 pixels on a text of 18 need the field to keep a quarter of the em
    SdfFontSettings settings;
    settings.distanceRange = 32.0f;
    auto sdf = LoadSdfFont(settings);
    auto vector = LoadVectorFont();

    for (auto& style : TestStyles())
    {
        for (int height : { 18, 40 })
        {
            auto sdfText = MakeText(sdf, height, style.second);
            auto vectorText = MakeText(vector, height, style.second);
            if (!style.second)
            {
                sdfText->SetColor(Color4(20, 30, 40, 255));
                vectorText->SetColor(Color4(20, 30, 40, 255));
            }

            auto sdfFrame = DrawAndCapture([&]() { sdfText->Draw(); });
            auto vectorFrame = DrawAndCapture([&]() { vectorText->Draw(); });
            ASSERT_TRUE(sdfFrame && vectorFrame);

            String name = "sdf_" + style.first + "_" + (String)height;
            SaveShot(sdfFrame, name + "_field");
            SaveShot(vectorFrame, name + "_bitmap");

            BitmapCompareResult result = BitmapCompare::Compare(*sdfFrame, *vectorFrame, 64, background);
            ASSERT_TRUE(result.comparable);
            printf("  %s %i: similarity %.4f, mean difference %.3f; ink %i vs %i\n", style.first.Data(), height,
                   result.similarity, result.meanDifference, CountInk(*sdfFrame), CountInk(*vectorFrame));

            EXPECT_GE(result.similarity, 0.96f) << style.first.Data() << " " << height;

            // About the same amount of ink: the effects reach as far as the bitmap ones. The bitmap effects darken the
            // edges (they blend straight colours as premultiplied), the fields don't
            float inkRatio = (float)CountInk(*sdfFrame)/(float)Math::Max(CountInk(*vectorFrame), 1);
            EXPECT_NEAR(inkRatio, 1.0f, 0.25f) << style.first.Data() << " " << height;
        }
    }
}

TEST(SdfFontRender, ZoomedTextStaysSharpWithoutNewGlyphs)
{
    auto sdf = LoadSdfFont();
    auto vector = LoadVectorFont();
    auto style = MakeStyle({ mmake<FontColorEffect>(Color4(0, 0, 0, 255)), mmake<FontStrokeEffect>(2.0f, Color4(0, 0, 0, 255), 100) });

    auto sdfText = MakeText(sdf, 20, style, "Sharp 42");
    auto vectorText = MakeText(vector, 20, style, "Sharp 42");
    sdfText->SetPosition(Vec2F(-60, 10));
    vectorText->SetPosition(Vec2F(-60, 10));

    DrawAndCapture([&]() { sdfText->Draw(); });
    int generated = sdf->GetStatistics().generatedGlyphs;

    auto sdfZoomed = DrawAndCapture([&]() { sdfText->Draw(); }, 4.0f);
    auto vectorZoomed = DrawAndCapture([&]() { vectorText->Draw(); }, 4.0f);
    ASSERT_TRUE(sdfZoomed && vectorZoomed);
    SaveShot(sdfZoomed, "sdf_zoom4_field");
    SaveShot(vectorZoomed, "sdf_zoom4_bitmap");

    EXPECT_EQ(sdf->GetStatistics().generatedGlyphs, generated);

    // A styled bitmap glyph is magnified with its pixels, the field keeps the edges a screen pixel wide
    float sdfEdge = EdgeShare(*sdfZoomed), vectorEdge = EdgeShare(*vectorZoomed);
    printf("  edge share at x4: fields %.3f, bitmap %.3f\n", sdfEdge, vectorEdge);
    EXPECT_LT(sdfEdge, vectorEdge*0.7f);
}

TEST(SdfFontRender, AtlasGrowthKeepsTheTexts)
{
    SdfFontSettings settings;
    auto font = LoadSdfFont(settings);
    auto text = MakeText(font, 30, TestStyles()[3].second, "Grow 123");

    auto before = DrawAndCapture([&]() { text->Draw(); });
    Vec2I atlasBefore = font->GetFieldTextureSize();

    WString many;
    for (wchar_t c = 0x410; c <= 0x44F; c++)
        many += c;
    for (wchar_t c = 'a'; c <= 'z'; c++)
        many += c;
    font->WarmUp(many);

    EXPECT_GT(font->GetFieldTextureSize().x, atlasBefore.x);
    EXPECT_GT(font->GetStatistics().atlasGrowths, 0);

    auto after = DrawAndCapture([&]() { text->Draw(); });
    auto freshText = MakeText(font, 30, TestStyles()[3].second, "Grow 123");
    auto fresh = DrawAndCapture([&]() { freshText->Draw(); });
    EXPECT_GE(BitmapCompare::Compare(*fresh, *after, 8, background).similarity, 0.999f);
    SaveShot(before, "sdf_growth_before");
    SaveShot(after, "sdf_growth_after");
    BitmapCompareResult result = BitmapCompare::Compare(*before, *after, 8, background);
    printf("  growth: similarity %.4f, mean %.3f, max %i\n", result.similarity, result.meanDifference, result.maxDifference);
    EXPECT_GE(result.similarity, 0.999f);
}

TEST(SdfFontRender, EffectsOverTheWholeTextKeepTheLettersOverTheStrokes)
{
    SdfFontSettings settings;
    settings.distanceRange = 32.0f;
    auto font = LoadSdfFont(settings);

    const Color4 fill(250, 220, 40, 255), stroke(30, 60, 200, 255);
    auto makeStyle = [&](FontEffectsMode mode) {
        auto style = MakeStyle({ mmake<FontColorEffect>(fill), mmake<FontStrokeEffect>(6.0f, stroke, 100),
                                 mmake<FontShadowEffect>(2.0f, Vec2I(3, 4), Color4(0, 0, 0, 160)) });
        style->SetEffectsMode(mode);
        return style;
    };

    auto countFill = [&](const Bitmap& bitmap) {
        int count = 0;
        Vec2I size = bitmap.GetSize();
        const UInt8* data = bitmap.GetData();
        for (int i = 0; i < size.x*size.y; i++)
            count += Math::Abs(data[i*4] - fill.r) < 30 && Math::Abs(data[i*4 + 1] - fill.g) < 30 && data[i*4 + 2] < 100 ? 1 : 0;
        return count;
    };

    auto perLetter = MakeText(font, 30, makeStyle(FontEffectsMode::PerLetter), "WAVE MAWWA");
    auto wholeText = MakeText(font, 30, makeStyle(FontEffectsMode::WholeText), "WAVE MAWWA");

    auto perLetterFrame = DrawAndCapture([&]() { perLetter->Draw(); });
    int drawCallsBefore = o2Render.GetDrawCallsCount();
    auto wholeTextFrame = DrawAndCapture([&]() { wholeText->Draw(); });
    ASSERT_TRUE(perLetterFrame && wholeTextFrame);
    SaveShot(perLetterFrame, "sdf_mode_per_letter");
    SaveShot(wholeTextFrame, "sdf_mode_whole_text");

    // The strokes of the next letters don't cover the fills
    int perLetterFill = countFill(*perLetterFrame), wholeTextFill = countFill(*wholeTextFrame);
    printf("  fill pixels: per letter %d, whole text %d; draw calls %d -> %d\n", perLetterFill, wholeTextFill,
           drawCallsBefore, o2Render.GetDrawCallsCount());
    EXPECT_GT(wholeTextFill, perLetterFill*11/10);

    // A pass per layer of the style
    EXPECT_EQ(o2Render.GetDrawCallsCount(), 3);

    // A single letter looks the same either way
    auto oneA = MakeText(font, 30, makeStyle(FontEffectsMode::PerLetter), "O");
    auto oneB = MakeText(font, 30, makeStyle(FontEffectsMode::WholeText), "O");
    auto frameA = DrawAndCapture([&]() { oneA->Draw(); });
    auto frameB = DrawAndCapture([&]() { oneB->Draw(); });
    EXPECT_GE(BitmapCompare::Compare(*frameA, *frameB, 8, background).similarity, 0.995f);

    // Switching the mode of the style rebuilds the texts
    wholeText->GetFontStyle()->SetEffectsMode(FontEffectsMode::PerLetter);
    auto switchedFrame = DrawAndCapture([&]() { wholeText->Draw(); });
    EXPECT_GE(BitmapCompare::Compare(*switchedFrame, *perLetterFrame, 8, background).similarity, 0.999f);
}
