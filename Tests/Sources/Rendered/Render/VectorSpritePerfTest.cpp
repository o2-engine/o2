#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>

#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "Assets/VectorImageTestAssets.h"

using namespace o2;
using namespace o2::VectorImageTest;

namespace
{
    const int spritesCount = 1500;
    const int framesCount = 20;

    struct Source
    {
        String name;
        String svg;
    };

    // SVG files of the folder from O2_VECTOR_SVG_DIR, evenly picked by size; built-in shapes otherwise
    Vector<Source> GetSources()
    {
        Vector<Source> res;

        if (const char* folder = std::getenv("O2_VECTOR_SVG_DIR"))
        {
            Vector<std::pair<uintmax_t, std::filesystem::path>> files;
            std::error_code errorCode;
            for (auto& entry : std::filesystem::directory_iterator(folder, errorCode))
            {
                if (entry.path().extension() == ".svg")
                    files.push_back({ entry.file_size(), entry.path() });
            }

            std::sort(files.begin(), files.end());

            const int picked = 20;
            for (int i = 0; i < picked && !files.empty(); i++)
            {
                auto& path = files[(size_t)i*(files.size() - 1)/(picked - 1)].second;
                res.Add({ String(path.filename().string().c_str()), o2FileSystem.ReadFile(String(path.string().c_str())) });
            }
        }

        if (res.IsEmpty())
        {
            res.Add({ "rect", Svg(16, 16, "<rect x=\"1\" y=\"1\" width=\"14\" height=\"14\" fill=\"#3366ff\"/>") });
            res.Add({ "fractional rect", Svg(16, 16, "<rect x=\"1.5\" y=\"1.5\" width=\"13\" height=\"13\" fill=\"#3366ff\"/>") });
            res.Add({ "circle", Svg(20, 20, "<circle cx=\"10\" cy=\"10\" r=\"7.3\" fill=\"#ffffff\"/>") });
            res.Add({ "frame", Svg(24, 24,
                "<path d=\"M8 1 L16 1 C20 1 23 4 23 8 L23 16 C23 20 20 23 16 23 L8 23 C4 23 1 20 1 16 L1 8 C1 4 4 1 8 1 Z\" "
                "fill=\"#e0e0e0\" stroke=\"#204080\" stroke-width=\"2\"/>") });
        }

        return res;
    }

    double Milliseconds(const std::chrono::steady_clock::time_point& from)
    {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - from).count();
    }

    struct FrameTime
    {
        double submit = 0; // Draw calls of the drawables
        double frame = 0;  // Whole frame with the render flush
    };

    FrameTime MeasureFrames(const Function<void()>& draw, const Function<void(int)>& beforeFrame = {})
    {
        FrameTime res;
        for (int i = 0; i < framesCount + 2; i++)
        {
            auto frameStart = std::chrono::steady_clock::now();

            if (beforeFrame)
                beforeFrame(i);

            o2Render.Begin();
            o2Render.SetCamera(Camera());
            o2Render.Clear(Color4(96, 96, 96, 255));

            auto submitStart = std::chrono::steady_clock::now();
            draw();
            double submit = Milliseconds(submitStart);

            o2Render.End();

            // The first frames build meshes and warm the buffers up
            if (i >= 2)
            {
                res.submit += submit/framesCount;
                res.frame += Milliseconds(frameStart)/framesCount;
            }
        }

        return res;
    }

    Vec2F GridPosition(int index)
    {
        return Vec2F((float)(index%50)*18.0f - 450.0f, (float)(index/50)*20.0f - 300.0f);
    }
}

// Not a pass/fail benchmark: prints mesh sizes and frame times, checks only that nothing is rebuilt per frame
TEST(VectorSpritePerf, DrawingCostAgainstSprites)
{
    Vector<Source> sources = GetSources();
    Vector<AssetRef<VectorImageAsset>> assets;

    printf("%-44s %9s %9s %9s %12s\n", "image", "size", "triangles", "vertices", "tessellate ms");

    UInt totalTriangles = 0, totalVertices = 0;
    for (auto& source : sources)
    {
        auto asset = mmake<VectorImageAsset>();
        if (!asset->SetSource(source.svg))
            continue;

        auto start = std::chrono::steady_clock::now();
        auto mesh = asset->GetMesh(Vec2F(1, 1));
        double tessellation = Milliseconds(start);
        if (!mesh)
            continue;

        printf("%-44s %4dx%-4d %9u %9d %12.2f\n", source.name.Data(), (int)asset->GetWidth(), (int)asset->GetHeight(),
               mesh->mesh.GetTrianglesCount(), (int)mesh->mesh.positions.Count(), tessellation);

        totalTriangles += mesh->mesh.GetTrianglesCount();
        totalVertices += (UInt)mesh->mesh.positions.Count();
        assets.Add(AssetRef<VectorImageAsset>(asset));
    }

    ASSERT_FALSE(assets.IsEmpty());
    printf("images: %d, mean triangles %u, mean vertices %u\n", assets.Count(), totalTriangles/assets.Count(),
           totalVertices/assets.Count());

    Vector<Ref<VectorSprite>> vectorSprites;
    UInt frameTriangles = 0;
    for (int i = 0; i < spritesCount; i++)
    {
        auto sprite = mmake<VectorSprite>(assets[i%assets.Count()]);
        sprite->SetMode(SpriteMode::Default);
        sprite->SetPosition(GridPosition(i));
        vectorSprites.Add(sprite);

        frameTriangles += assets[i%assets.Count()]->GetMesh(Vec2F(1, 1))->mesh.GetTrianglesCount();
    }

    Bitmap bitmap(PixelFormat::R8G8B8A8, Vec2I(32, 32));
    bitmap.Fill(Color4(200, 120, 40, 255));
    TextureRef texture(bitmap);

    Vector<Ref<Sprite>> sprites;
    for (int i = 0; i < spritesCount; i++)
    {
        auto sprite = mmake<Sprite>(texture, RectI(0, 0, 32, 32));
        sprite->SetMode(SpriteMode::Sliced);
        sprite->SetSliceBorder(BorderI(8, 8, 8, 8));
        sprite->SetSize(Vec2F(32, 32));
        sprite->SetPosition(GridPosition(i));
        sprites.Add(sprite);
    }

    auto drawVector = [&]() { for (auto& sprite : vectorSprites) sprite->Draw(); };
    auto drawSprites = [&]() { for (auto& sprite : sprites) sprite->Draw(); };

    FrameTime spritesTime = MeasureFrames(drawSprites);
    FrameTime vectorTime = MeasureFrames(drawVector);

    for (auto& sprite : vectorSprites)
    {
        ASSERT_EQ(sprite->GetMeshRebuildsCount(), 1u) << "static sprites must not rebuild vertices per frame";
        ASSERT_EQ(sprite->GetColorUpdatesCount(), 1u);
    }

    // Worst case: every sprite changes every frame, as in a fade of the whole screen
    FrameTime dirtyVectorTime = MeasureFrames(drawVector, [&](int frame) {
        for (auto& sprite : vectorSprites)
            sprite->SetTransparency(0.5f + 0.01f*(float)(frame%10));
    });

    for (auto& sprite : vectorSprites)
        ASSERT_EQ(sprite->GetMeshRebuildsCount(), 1u) << "recoloring must not rebuild positions";

    FrameTime dirtySpritesTime = MeasureFrames(drawSprites, [&](int frame) {
        for (auto& sprite : sprites)
            sprite->SetTransparency(0.5f + 0.01f*(float)(frame%10));
    });

    FrameTime movedVectorTime = MeasureFrames(drawVector, [&](int frame) {
        for (int i = 0; i < vectorSprites.Count(); i++)
            vectorSprites[i]->SetPosition(GridPosition(i) + Vec2F((float)(frame%10), 0.0f));
    });

    printf("%d drawables, %d frames, vector triangles per frame: %u\n", spritesCount, framesCount, frameTriangles);
    printf("%-34s %12s %12s\n", "", "submit ms", "frame ms");
    printf("%-34s %12.3f %12.3f\n", "Sprite (sliced, 18 triangles)", spritesTime.submit, spritesTime.frame);
    printf("%-34s %12.3f %12.3f\n", "VectorSprite", vectorTime.submit, vectorTime.frame);
    printf("%-34s %12.3f %12.3f\n", "Sprite, all recolored per frame", dirtySpritesTime.submit, dirtySpritesTime.frame);
    printf("%-34s %12.3f %12.3f\n", "VectorSprite, all recolored", dirtyVectorTime.submit, dirtyVectorTime.frame);
    printf("%-34s %12.3f %12.3f\n", "VectorSprite, all moved per frame", movedVectorTime.submit, movedVectorTime.frame);
    fflush(stdout);
}
