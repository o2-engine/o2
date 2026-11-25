#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Assets.h"
#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Render/Text/Text.h"
#include "o2/Render/Text/VectorFont.h"
#include "o2/Render/VectorGraphics/VectorMesh.h"
#include "o2/Utils/Bitmap/BitmapCompare.h"

using namespace o2;

// Texts drawn by the meshes of the glyph outlines against the ones drawn from the font texture

namespace
{
    const Color4 background(255, 255, 255, 255);

    // Turns on the glyph meshes while it lives
    struct GlyphMeshesGuard
    {
        GlyphMeshesGuard() { Text::SetGlyphMeshesEnabled(true); }
        ~GlyphMeshesGuard() { Text::SetGlyphMeshesEnabled(false); }
    };

    Ref<VectorFont> LoadTestFont()
    {
        return mmake<VectorFont>(o2Assets.GetBuiltAssetsPath() + "debugFont.ttf");
    }

    Ref<Bitmap> DrawAndCapture(const Function<void()>& draw, float zoom = 1.0f)
    {
        Ref<Bitmap> captured;

        // The first frame captured after switching to the capture target may need a repeat
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

    Ref<Text> MakeText(const Ref<VectorFont>& font, int height)
    {
        auto text = mmake<Text>(font);
        text->SetHeight(height);
        text->SetColor(Color4(20, 30, 40, 255));
        text->SetText("Hamburgefonstiv 0123456789\n@ The quick brown fox, jumps: over; lazy dog!");
        text->SetPosition(Vec2F(-150.0f/(float)Math::Max(height/12, 1), 40));
        return text;
    }

    void SaveShot(const Ref<Bitmap>& bitmap, const String& name)
    {
        const char* folder = getenv("O2_TEXT_SHOTS");
        if (folder && bitmap)
            bitmap->Save(String(folder) + "/" + name + ".png", Bitmap::ImageType::Png);
    }
}

TEST(TextGlyphMeshesRender, GlyphMeshIsBuiltOnceForAnySize)
{
    auto font = LoadTestFont();
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    const VectorMesh* glyph = font->GetGlyphMesh('g');
    ASSERT_TRUE(glyph);
    EXPECT_TRUE(glyph->IsScalable());
    EXPECT_GT(glyph->GetTrianglesCount(), 20u);

    UInt triangles = glyph->GetTrianglesCount();

    GlyphMeshesGuard glyphMeshes;
    for (int height : { 10, 18, 40 })
    {
        auto text = MakeText(font, height);
        for (float zoom : { 1.0f, 2.5f })
            ASSERT_TRUE(DrawAndCapture([&]() { text->Draw(); }, zoom));
    }

    EXPECT_EQ(font->GetGlyphMesh('g'), glyph);
    EXPECT_EQ(glyph->GetTrianglesCount(), triangles);

    EXPECT_TRUE(font->GetGlyphMesh(' '));
    EXPECT_EQ(font->GetGlyphMesh(' ')->GetTrianglesCount(), 0u);
}

TEST(TextGlyphMeshesRender, TextIsDrawnAsFromTheFontTexture)
{
    auto font = LoadTestFont();
    ASSERT_FALSE(font->GetFileName().IsEmpty());

    for (int height : { 12, 16, 24, 48 })
    {
        for (float zoom : { 1.0f, 2.0f })
        {
            auto textured = MakeText(font, height);
            auto texturedFrame = DrawAndCapture([&]() { textured->Draw(); }, zoom);
            ASSERT_TRUE(texturedFrame);

            Ref<Bitmap> meshFrame;
            {
                GlyphMeshesGuard glyphMeshes;
                auto text = MakeText(font, height);
                meshFrame = DrawAndCapture([&]() { text->Draw(); }, zoom);
            }

            ASSERT_TRUE(meshFrame);

            String name = String("text_") + (String)height + "_x" + (String)(int)zoom;
            SaveShot(texturedFrame, name + "_texture");
            SaveShot(meshFrame, name + "_mesh");

            BitmapCompareResult result = BitmapCompare::Compare(*texturedFrame, *meshFrame, 64, background);
            ASSERT_TRUE(result.comparable);

            printf("  height %i zoom %.0f: similarity %.4f, mean difference %.3f, max %i\n", height, zoom,
                   result.similarity, result.meanDifference, result.maxDifference);

            EXPECT_GE(result.similarity, 0.97f) << "height " << height << ", zoom " << zoom;
            EXPECT_LE(result.meanDifference, 4.0f) << "height " << height << ", zoom " << zoom;

            // The text is there in both: dark pixels of the glyphs
            int dark = 0;
            Vec2I size = meshFrame->GetSize();
            const UInt8* data = meshFrame->GetData();
            for (int i = 0; i < size.x*size.y; i++)
                dark += data[i*4] < 100 ? 1 : 0;

            EXPECT_GT(dark, 200) << "height " << height << ", zoom " << zoom;
        }
    }
}
