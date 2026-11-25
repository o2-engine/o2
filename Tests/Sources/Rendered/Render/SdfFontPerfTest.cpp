#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <chrono>

#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/FontStyleAsset.h"
#include "o2/Render/Camera.h"
#include "o2/Render/Text/FontStyle.h"
#include "o2/Render/Render.h"
#include "o2/Render/Text/SdfFont.h"
#include "o2/Render/Text/Text.h"
#include "o2/Render/Text/VectorFont.h"
#include "o2/Render/Text/VectorFontEffects.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"

using namespace o2;

// Benchmarks of the distance field fonts against the bitmap atlas of the vector font: generation, loading, styled
// glyphs, mesh rebuilds, zoom, a frame of many texts, atlas memory. Not pass/fail: prints PERF lines, runs on request
// (O2_SDF_PERF=1), build it in Release to read the numbers

namespace
{
    double Milliseconds(const std::chrono::steady_clock::time_point& from)
    {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - from).count();
    }

    String FontPath(const String& name)
    {
        return o2Assets.GetBuiltAssetsPath() + name;
    }

    const char* gameFont = "WordFall/UIKit/Fonts/LilitaOne-Regular.ttf";
    const char* textFont = "Generated/UI/Screen_Resources/Fonts/Roboto-Regular.ttf";

    // The heaviest style of the game: gradient, two strokes, shadow (text_47d8720ced811ed9)
    Ref<FontStyle> GameStyle()
    {
        auto style = mmake<FontStyle>();
        style->SetEffects({ mmake<FontGradientEffect>(Color4(234, 170, 54, 255), Color4(255, 246, 194, 255)),
                            mmake<FontStrokeEffect>(3.0f, Color4(255, 232, 160, 255), 100),
                            mmake<FontStrokeEffect>(7.0f, Color4(89, 53, 15, 255), 100),
                            mmake<FontShadowEffect>(2.0f, Vec2I(0, 5), Color4(54, 30, 8, 140)) });
        return style;
    }

    WString Latin()
    {
        WString res;
        for (wchar_t c = 33; c < 127; c++)
            res += c;
        return res;
    }

    double gMaxFrame = 0; // Longest frame of the last measure

    // Frame time with the GPU: every frame is read back, so it waits for the GPU to draw it
    double MeasureFrame(const Function<void()>& draw, int frames, bool waitGpu, const Function<void(int)>& before = {})
    {
        double total = 0;
        gMaxFrame = 0;
        for (int i = 0; i < frames + 2; i++)
        {
            if (before)
                before(i);

            auto start = std::chrono::steady_clock::now();
            if (waitGpu)
                o2Render.CaptureNextFrame([](const Ref<Bitmap>&) {});

            o2Render.Begin();
            o2Render.SetCamera(Camera());
            o2Render.Clear(Color4(96, 96, 96, 255));
            draw();
            o2Render.End();

            if (i >= 2)
            {
                double ms = Milliseconds(start);
                total += ms;
                gMaxFrame = Math::Max(gMaxFrame, ms);
            }
        }

        return total/frames;
    }

    size_t TextureBytes(const TextureRef& texture)
    {
        return texture ? (size_t)texture->GetSize().x*texture->GetSize().y*4 : 0;
    }
}

TEST(SdfFontPerf, Benchmarks)
{
    if (!std::getenv("O2_SDF_PERF"))
        GTEST_SKIP() << "benchmarks run on request: O2_SDF_PERF=1";

    WString characters = SdfFont::GetDefaultCharacters();

    // 1. Generation of the fields, per glyph
    for (auto fontName : { gameFont, textFont })
    {
        for (bool multiChannel : { true, false })
        {
            SdfFontSettings settings;
            settings.multiChannel = multiChannel;
            settings.distanceRange = String(fontName) == gameFont ? 28.0f : 16.0f;
            auto font = mmake<SdfFont>(FontPath(fontName), settings);

            auto start = std::chrono::steady_clock::now();
            int generated = font->WarmUp(characters);
            double ms = Milliseconds(start);
            printf("PERF generate %-6s %-58s %4d glyphs %8.1f ms  %6.3f ms/glyph  atlas %dx%d\n",
                   multiChannel ? "MTSDF" : "SDF", fontName, generated, ms, ms/Math::Max(generated, 1),
                   font->GetFieldTextureSize().x, font->GetFieldTextureSize().y);
        }
    }

    // 2. Baked fields against runtime generation
    {
        SdfFontSettings settings;
        settings.distanceRange = 28.0f;
        String baked = "SdfFontPerfFields.ttf.sdf";

        auto start = std::chrono::steady_clock::now();
        SdfFont::Bake(FontPath(gameFont), settings, characters, baked);
        double bakeMs = Milliseconds(start);

        auto font = mmake<SdfFont>(FontPath(gameFont), settings);
        start = std::chrono::steady_clock::now();
        font->LoadBaked(baked);
        double loadMs = Milliseconds(start);

        start = std::chrono::steady_clock::now();
        int unpacked = font->WarmUp(characters);
        double unpackMs = Milliseconds(start);

        printf("PERF bake LilitaOne (%d chars) %8.1f ms, file %.0f KB; load the table %6.3f ms; unpack %d glyphs %6.2f ms "
               "(%.3f ms/glyph)\n", characters.Length(), bakeMs, (double)o2FileSystem.GetFileInfo(baked).size/1024.0, loadMs,
               unpacked, unpackMs, unpackMs/Math::Max(unpacked, 1));

        o2FileSystem.FileDelete(baked);
    }

    // 3. First show of styled glyphs: the victory amount (height 70) and the counters (height 34) of the game style
    {
        auto style = GameStyle();
        WString digits = "0123456789,";

        auto vector = mmake<VectorFont>(FontPath(gameFont));
        auto start = std::chrono::steady_clock::now();
        vector->CheckCharacters(digits, 70, style);
        vector->CheckCharacters(digits, 34, style);
        double vectorMs = Milliseconds(start);

        SdfFontSettings settings;
        settings.distanceRange = 28.0f;
        auto sdf = mmake<SdfFont>(FontPath(gameFont), settings);
        sdf->WarmUp(digits);
        start = std::chrono::steady_clock::now();
        sdf->CheckCharacters(digits, 70, style);
        sdf->CheckCharacters(digits, 34, style);
        double sdfMs = Milliseconds(start);

        auto sdfCold = mmake<SdfFont>(FontPath(gameFont), settings);
        start = std::chrono::steady_clock::now();
        sdfCold->CheckCharacters(digits, 70, style);
        sdfCold->CheckCharacters(digits, 34, style);
        double sdfColdMs = Milliseconds(start);

        start = std::chrono::steady_clock::now();
        sdfCold->CheckCharacters(digits, 50, style);
        sdfCold->CheckCharacters(digits, 90, style);
        double sdfNewSizesMs = Milliseconds(start);

        printf("PERF styled digits, 2 sizes: bitmap effects %8.2f ms; fields warmed %6.3f ms; fields cold (generate + "
               "material) %6.2f ms; 2 more sizes %6.3f ms\n", vectorMs, sdfMs, sdfColdMs, sdfNewSizesMs);
    }

    // 4. Mesh rebuilds: a label with a changing number
    {
        auto vector = mmake<VectorFont>(FontPath(textFont));
        auto sdf = mmake<SdfFont>(FontPath(textFont));
        sdf->WarmUp(characters);

        for (auto font : { Ref<Font>(vector), Ref<Font>(sdf) })
        {
            auto text = mmake<Text>(font);
            text->SetHeight(24);
            text->SetText("Score: 0 / 268, moves 12");
            text->Draw();

            auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < 2000; i++)
                text->SetText("Score: " + (WString)(String)(i%300) + " / 268, moves 12");
            double ms = Milliseconds(start);
            printf("PERF set text x2000 (%s) %8.2f ms  %6.2f us/rebuild\n", font == sdf ? "fields" : "bitmap", ms,
                   ms*1000.0/2000.0);
        }
    }

    // 5. Zoom: a paragraph scaled 1..4 over 120 frames
    {
        auto vector = mmake<VectorFont>(FontPath(textFont));
        auto sdf = mmake<SdfFont>(FontPath(textFont));
        sdf->WarmUp(characters);

        WString paragraph = "The quick brown fox jumps over the lazy dog 0123456789\nСъешь же ещё этих мягких французских булок";
        for (auto font : { Ref<Font>(vector), Ref<Font>(sdf) })
        {
            auto text = mmake<Text>(font);
            text->SetHeight(16);
            text->SetText(paragraph);
            text->SetPosition(Vec2F(-200, 0));

            double cpu = MeasureFrame([&]() { text->Draw(); }, 120, false, [&](int frame) {
                text->SetScale(Vec2F::One()*(1.0f + 3.0f*(float)frame/120.0f));
            });
            printf("PERF zoom 1..4 over 120 frames (%s): %6.3f ms/frame CPU, max %6.3f ms\n", font == sdf ? "fields" : "bitmap",
                   cpu, gMaxFrame);
        }
    }

    // 6. A frame of many texts: 300 labels, plain and with the game style
    {
        auto vector = mmake<VectorFont>(FontPath(gameFont));
        SdfFontSettings settings;
        settings.distanceRange = 28.0f;
        auto sdf = mmake<SdfFont>(FontPath(gameFont), settings);
        sdf->WarmUp(characters);

        auto wholeTextStyle = GameStyle();
        wholeTextStyle->SetEffectsMode(FontEffectsMode::WholeText);

        for (auto style : { Ref<FontStyle>(), GameStyle(), wholeTextStyle })
        {
            for (auto font : { Ref<Font>(vector), Ref<Font>(sdf) })
            {
                if (style == wholeTextStyle && font == vector)
                    continue;

                Vector<Ref<Text>> texts;
                for (int i = 0; i < 300; i++)
                {
                    auto text = mmake<Text>(font);
                    text->SetFontStyle(style);
                    text->SetHeight(28);
                    text->SetText("LEVEL " + (WString)(String)(i + 1) + " COINS");
                    text->SetPosition(Vec2F((float)(i%6)*150.0f - 450.0f, (float)(i/6)*12.0f - 300.0f));
                    texts.Add(text);
                }

                auto drawAll = [&]() { for (auto& text : texts) text->Draw(); };
                MeasureFrame(drawAll, 1, true);

                double cpu = MeasureFrame(drawAll, 60, false);
                int drawCalls = o2Render.GetDrawCallsCount();
                double gpu = MeasureFrame(drawAll, 30, true);
                double empty = MeasureFrame([]() {}, 30, true);

                const char* kind = !style ? "plain" : (style == wholeTextStyle ? "styled, whole text" : "styled");
                printf("PERF 300 labels x 15 glyphs, %-18s (%s): frame CPU %6.3f ms, with GPU wait %6.3f ms (empty %6.3f), "
                       "draw calls/frame %d\n", kind, font == sdf ? "fields" : "bitmap", cpu, gpu, empty, drawCalls);
            }
        }
    }

    // 7. Atlas memory for the game's set: the styled numbers and words at the heights of the game
    {
        auto vector = mmake<VectorFont>(FontPath(gameFont));
        SdfFontSettings settings;
        settings.distanceRange = 28.0f;
        auto sdf = mmake<SdfFont>(FontPath(gameFont), settings);

        auto style = GameStyle();
        WString used = Latin();
        for (auto font : { Ref<Font>(vector), Ref<Font>(sdf) })
        {
            for (int height : { 31, 34, 44, 48, 50, 56, 58, 60, 62, 70, 73, 76 })
            {
                font->CheckCharacters(used, height, style);
                font->CheckCharacters(used, height, nullptr);
            }
        }

        printf("PERF atlas for 94 glyphs x 12 heights x (plain + styled): bitmap %dx%d (%.1f MB), fields %dx%d (%.1f MB)\n",
               vector->GetTexture()->GetSize().x, vector->GetTexture()->GetSize().y,
               TextureBytes(vector->GetTexture())/1048576.0, sdf->GetFieldTextureSize().x, sdf->GetFieldTextureSize().y,
               TextureBytes(sdf->GetFieldTexture())/1048576.0);
    }

    fflush(stdout);
}

// Frames of the demo video: the same styled texts by the bitmap atlas (left) and by the distance fields (right), scaled
// and rotated. Runs on request: O2_SDF_DEMO_DIR=<folder for the png frames>
TEST(SdfFontDemo, Frames)
{
    const char* folder = std::getenv("O2_SDF_DEMO_DIR");
    if (!folder)
        GTEST_SKIP() << "demo frames are saved on request: O2_SDF_DEMO_DIR=<folder>";

    auto vector = mmake<VectorFont>(FontPath(gameFont));
    SdfFontSettings settings;
    settings.distanceRange = 28.0f;
    auto sdf = mmake<SdfFont>(FontPath(gameFont), settings);
    sdf->WarmUp(SdfFont::GetDefaultCharacters());

    auto captionFont = mmake<SdfFont>(FontPath(textFont));
    auto makeCaption = [&](const WString& string, const Vec2F& position) {
        auto caption = mmake<Text>(captionFont);
        caption->SetHeight(16);
        caption->SetColor(Color4(240, 240, 240, 255));
        caption->SetText(string);
        caption->SetPivot(Vec2F(0.5f, 0.5f));
        caption->SetSize(Vec2F(400, 30));
        caption->SetHorAlign(HorAlign::Middle);
        caption->SetVerAlign(VerAlign::Middle);
        caption->SetPosition(position);
        return caption;
    };

    auto makeText = [&](const Ref<Font>& font, const WString& string, const Vec2F& position) {
        auto text = mmake<Text>(font);
        text->SetFontStyle(GameStyle());
        text->SetHeight(36);
        text->SetText(string);
        text->SetPivot(Vec2F(0.5f, 0.5f));
        text->SetSize(Vec2F(420, 80));
        text->SetHorAlign(HorAlign::Middle);
        text->SetVerAlign(VerAlign::Middle);
        text->SetPosition(position);
        return text;
    };

    struct Scene { const char* name; WString string; int frames; };
    Scene scenes[] = { { "zoom", "LEVEL 7", 150 }, { "rotate", "VICTORY!", 150 }, { "modes", "WAVE 77", 150 } };

    int index = 0;
    for (auto& scene : scenes)
    {
        bool modes = String(scene.name) == "modes";
        auto left = makeText(modes ? Ref<Font>(sdf) : Ref<Font>(vector), scene.string, Vec2F(-240, 0));
        auto right = makeText(sdf, scene.string, Vec2F(240, 0));
        if (modes)
        {
            auto wholeText = GameStyle();
            wholeText->SetEffectsMode(FontEffectsMode::WholeText);
            right->SetFontStyle(wholeText);
        }

        auto sceneCaptions = { makeCaption(modes ? "distance field: effects per letter" : "bitmap atlas (VectorFont): effects on CPU", Vec2F(-240, 280)),
                               makeCaption(modes ? "distance field: effects over the whole text" : "distance field (SdfFont): effects in the shader", Vec2F(240, 280)) };

        for (int frame = 0; frame < scene.frames; frame++)
        {
            float t = (float)frame/(float)scene.frames;
            float wave = 0.5f - 0.5f*Math::Cos(t*2.0f*Math::PI());
            for (auto text : { left, right })
            {
                if (String(scene.name) != "rotate")
                    text->SetScale(Vec3F::One()*(0.5f + 1.6f*wave));
                else
                {
                    text->SetScale(Vec3F::One()*1.2f);
                    text->SetAngle(t*2.0f*Math::PI());
                }
            }

            Ref<Bitmap> captured;
            o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });
            o2Render.Begin();
            o2Render.SetCamera(Camera());
            o2Render.Clear(Color4(52, 36, 28, 255));
            left->Draw();
            right->Draw();
            for (auto& caption : sceneCaptions)
                caption->Draw();
            o2Render.End();

            if (captured)
                captured->Save(String(folder) + String::Format("/frame_%04d.png", index++), Bitmap::ImageType::Png);
        }
    }
}
