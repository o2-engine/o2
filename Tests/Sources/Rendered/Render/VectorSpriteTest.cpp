#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <cmath>

#include "o2/Assets/Assets.h"
#include "o2/Application/Application.h"
#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Render/VectorGraphics/VectorRasterizer.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Bitmap/BitmapCompare.h"
#include "o2AssetBuilder/AssetsBuilder.h"
#include "Assets/VectorImageTestAssets.h"

using namespace o2;
using namespace o2::VectorImageTest;

namespace
{
    const Color4 background(96, 96, 96, 255);

    const char* shapesBody =
        "<defs><linearGradient id=\"g\" x1=\"0\" y1=\"0\" x2=\"0\" y2=\"40\" gradientUnits=\"userSpaceOnUse\">"
        "<stop offset=\"0\" stop-color=\"#ffcc00\"/><stop offset=\"1\" stop-color=\"#cc0044\"/></linearGradient></defs>"
        "<rect x=\"3.3\" y=\"4.6\" width=\"20.5\" height=\"13.25\" fill=\"#3366ff\"/>"
        "<circle cx=\"44\" cy=\"20\" r=\"13.4\" fill=\"url(#g)\"/>"
        "<path d=\"M6 30 C 12 22, 22 38, 30 27\" fill=\"none\" stroke=\"#ffffff\" stroke-width=\"2.5\" "
        "stroke-linecap=\"round\"/>"
        "<path d=\"M34 4 L60 8 L40 14 Z\" fill=\"#00cc66\" fill-opacity=\"0.6\"/>";

    const char* frameBody =
        "<path d=\"M8 1 L16 1 C20 1 23 4 23 8 L23 16 C23 20 20 23 16 23 L8 23 C4 23 1 20 1 16 L1 8 C1 4 4 1 8 1 Z\" "
        "fill=\"#e0e0e0\" stroke=\"#204080\" stroke-width=\"2\"/>";

    const char* redBody = "<rect x=\"0\" y=\"0\" width=\"16\" height=\"16\" fill=\"#ff0000\"/>";

    const char* blueBody = "<rect x=\"0\" y=\"0\" width=\"16\" height=\"16\" fill=\"#0000ff\"/>";

    const char* barBody = "<rect x=\"2.55\" y=\"2\" width=\"14.9\" height=\"20\" fill=\"#ffffff\"/>";

    const char* circleBody = "<circle cx=\"10\" cy=\"10\" r=\"7.3\" fill=\"#ffffff\"/>";

    const char* wideBody = "<rect x=\"0\" y=\"0\" width=\"20\" height=\"10\" fill=\"#00ff00\"/>";

    // More triangles than indexes of one render batch on any backend
    String ManyRectsSvg(int side)
    {
        String body;
        for (int y = 0; y < side; y++)
        {
            for (int x = 0; x < side; x++)
            {
                body += "<rect x=\"" + (String)(x*6 + 1.3f) + "\" y=\"" + (String)(y*6 + 1.3f) +
                    "\" width=\"3.4\" height=\"3.4\" fill=\"#ffffff\"/>";
            }
        }

        return Svg((float)side*6, (float)side*6, body);
    }

    Ref<Bitmap> DrawAndCapture(const Function<void()>& draw, float cameraScale = 1.0f)
    {
        Ref<Bitmap> captured;

        // The first frame captured after switching to the capture target may need a repeat
        for (int i = 0; i < 2; i++)
        {
            o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

            o2Render.Begin();

            // Default camera takes the size of the current resolution, which is set by Begin
            Camera camera;
            camera.SetScale(Vec2F(cameraScale, cameraScale));
            o2Render.SetCamera(camera);
            o2Render.Clear(background);
            draw();
            o2Render.SetCamera(Camera());
            o2Render.End();
        }

        return captured;
    }

    // Returns part of the captured frame by rectangle in the default camera space, where Y is up
    Ref<Bitmap> Crop(const Ref<Bitmap>& frame, const RectI& rect)
    {
        Vec2I frameSize = frame->GetSize();
        Vec2I half(Math::RoundToInt(frameSize.x/2.0f), Math::RoundToInt(frameSize.y/2.0f));
        Vec2I size(rect.right - rect.left, rect.top - rect.bottom);

        auto res = mmake<Bitmap>(PixelFormat::R8G8B8A8, size);
        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                Color4 pixel = VectorRasterizer::GetPixel(*frame, half.x + rect.left + x, half.y - rect.top + y);
                UInt8* target = res->GetData() + ((size_t)(size.y - 1 - y)*size.x + x)*4;
                target[0] = (UInt8)pixel.r; target[1] = (UInt8)pixel.g; target[2] = (UInt8)pixel.b; target[3] = 255;
            }
        }

        return res;
    }

    Color4 PixelAt(const Ref<Bitmap>& frame, int x, int y)
    {
        Vec2I frameSize = frame->GetSize();
        return VectorRasterizer::GetPixel(*frame, Math::RoundToInt(frameSize.x/2.0f) + x,
                                          Math::RoundToInt(frameSize.y/2.0f) - 1 - y);
    }

    // Software reference: the mesh with positions mapped into target pixels, over the background
    Ref<Bitmap> RasterizeMapped(const VectorMesh& source, const Vec2F& targetSize,
                                const Function<Vec2F(const Vec2F&)>& map)
    {
        VectorMesh mesh = source;
        for (auto& position : mesh.positions)
            position = map(position);

        mesh.size = targetSize;
        return VectorRasterizer::Rasterize(mesh, 1.0f, background);
    }

    void ExpectSame(const Ref<Bitmap>& rendered, const Ref<Bitmap>& expected, const char* what,
                    int tolerance = 3, float minSimilarity = 0.999f)
    {
        BitmapCompareResult result = BitmapCompare::Compare(*rendered, *expected, tolerance, background);
        ASSERT_TRUE(result.comparable) << what;
        EXPECT_GE(result.similarity, minSimilarity) << what << ": max difference " << result.maxDifference
            << ", mean " << result.meanDifference;
        EXPECT_LE(result.maxDifference, 12) << what;
    }

    int CountDifferent(const Ref<Bitmap>& frame, const Color4& color, int tolerance = 2)
    {
        int count = 0;
        Vec2I size = frame->GetSize();
        const UInt8* data = frame->GetData();
        for (int i = 0; i < size.x*size.y; i++)
        {
            if (Math::Abs((int)data[i*4] - color.r) > tolerance || Math::Abs((int)data[i*4 + 1] - color.g) > tolerance ||
                Math::Abs((int)data[i*4 + 2] - color.b) > tolerance)
            {
                count++;
            }
        }

        return count;
    }

    // Vector images built by the real assets builder into a temporary tree
    TempVectorAssets* BuildVectorAssets(const Function<void(TempVectorAssets&)>& write)
    {
        auto assets = new TempVectorAssets();
        write(*assets);

        String builtPath = assets->GetRootPath() + "Built/";
        String treePath = assets->GetRootPath() + "Built.json";

        {
            AssetsBuilder builder;
            builder.BuildAssets(GetEnginePlatform(), assets->GetSourcePath(), builtPath, treePath,
                                String(GetEditorAssetsPath()) + "../../CompressToolsConfig.json");
        }

        auto tree = mmake<AssetsTree>();
        tree->DeserializeFromString(o2FileSystem.ReadFile(treePath));
        tree->assetsPath = assets->GetSourcePath();
        tree->builtAssetsPath = builtPath;
        assets->Attach(tree);

        return assets;
    }

    class VectorImageBuild: public ::testing::Test
    {
    public:
        static void SetUpTestSuite()
        {
            assets = BuildVectorAssets([](TempVectorAssets& assets) {
                assets.WriteSvg("shapes", Svg(64, 40, shapesBody));
                assets.WriteSvg("frame", Svg(24, 24, frameBody));
                assets.WriteMeta("frame", BorderI(8, 9, 10, 7), SpriteMode::Sliced);
            });
        }

        static void TearDownTestSuite()
        {
            delete assets;
            assets = nullptr;
        }

        static AssetRef<VectorImageAsset> GetAsset(const String& name)
        {
            return AssetRef<VectorImageAsset>(assets->GetPath(name));
        }

        static TempVectorAssets* assets;
    };

    TempVectorAssets* VectorImageBuild::assets = nullptr;

    class VectorSpriteRender: public ::testing::Test
    {
    public:
        static void SetUpTestSuite()
        {
            assets = BuildVectorAssets([](TempVectorAssets& assets) {
                assets.WriteSvg("shapes", Svg(64, 40, shapesBody));
                assets.WriteSvg("red", Svg(16, 16, redBody));
                assets.WriteSvg("blue", Svg(16, 16, blueBody));
                assets.WriteSvg("circle", Svg(20, 20, circleBody));
                assets.WriteSvg("wide", Svg(20, 10, wideBody));
                assets.WriteSvg("many", ManyRectsSvg(36));
                assets.WriteSvg("broken", "<svg width=\"16\" height=\"16\"><rect x=\"0\" y=\"0\" width=");

                assets.WriteSvg("frame", Svg(24, 24, frameBody));
                assets.WriteMeta("frame", BorderI(8, 9, 10, 7), SpriteMode::Sliced);

                assets.WriteSvg("bar", Svg(20, 24, barBody));
                assets.WriteMeta("bar", BorderI(0, 6, 0, 6), SpriteMode::Sliced);
            });
        }

        static void TearDownTestSuite()
        {
            delete assets;
            assets = nullptr;
        }

        static AssetRef<VectorImageAsset> GetAsset(const String& name)
        {
            return AssetRef<VectorImageAsset>(assets->GetPath(name));
        }

        static TempVectorAssets* assets;
    };

    TempVectorAssets* VectorSpriteRender::assets = nullptr;
}

TEST_F(VectorImageBuild, AssetsBuilderGivesSvgTheVectorImageMeta)
{
    auto info = assets->GetTree()->Find(assets->GetPath("shapes"));
    ASSERT_TRUE(info);
    ASSERT_TRUE(info->meta);
    EXPECT_EQ(info->meta->GetAssetType(), &TypeOf(VectorImageAsset));
    EXPECT_TRUE(DynamicCast<VectorImageAsset::Meta>(info->meta)) << "the builder generates meta by extension";

    String metaPath = assets->GetSourcePath() + assets->GetPath("shapes") + ".meta";
    EXPECT_TRUE(o2FileSystem.IsFileExist(metaPath));

    String builtFile = assets->GetRootPath() + "Built/" + assets->GetPath("shapes");
    EXPECT_EQ(o2FileSystem.ReadFile(builtFile), Svg(64, 40, shapesBody)) << "built file is a plain copy";

    auto asset = GetAsset("shapes");
    ASSERT_TRUE(asset);
    EXPECT_TRUE(asset->IsValid());
    EXPECT_EQ(asset->GetSize(), Vec2F(64, 40));
    EXPECT_EQ(asset->GetDefaultMode(), SpriteMode::Default);
}

TEST_F(VectorImageBuild, AssetKeepsSliceBordersFromMeta)
{
    auto asset = GetAsset("frame");
    ASSERT_TRUE(asset);
    EXPECT_EQ(asset->GetSize(), Vec2F(24, 24));
    EXPECT_EQ(asset->GetSliceBorder(), BorderI(8, 9, 10, 7));
    EXPECT_EQ(asset->GetDefaultMode(), SpriteMode::Sliced);

    VectorSprite sprite(asset);
    EXPECT_EQ(sprite.GetMode(), SpriteMode::Sliced);
    EXPECT_EQ(sprite.GetSliceBorder(), BorderI(8, 9, 10, 7));
    EXPECT_EQ(sprite.GetSize(), Vec2F(24, 24));
}

TEST_F(VectorSpriteRender, RenderMatchesSoftwareRasterizer)
{
    auto asset = GetAsset("shapes");
    VectorSprite sprite(asset);

    RectI rect(-100, 50, -36, 90);
    sprite.SetRect(rect);

    auto frame = DrawAndCapture([&]() { sprite.Draw(); });
    ASSERT_TRUE(frame);

    EXPECT_EQ(sprite.GetMeshPixelScale(), Vec2F(1, 1));
    EXPECT_GT(sprite.GetTrianglesCount(), 0u);

    auto expected = VectorRasterizer::Rasterize(asset->GetMesh(Vec2F(1, 1))->mesh, 1.0f, background);
    ASSERT_GT(CountDifferent(expected, background), 500) << "the reference must not be empty";

    ExpectSame(Crop(frame, rect), expected, "scale 1");
}

TEST_F(VectorSpriteRender, DoubleSizeUsesMeshOfDoubleScale)
{
    auto asset = GetAsset("shapes");
    VectorSprite sprite(asset);

    RectI rect(-100, -60, 28, 20);
    sprite.SetRect(rect);

    auto frame = DrawAndCapture([&]() { sprite.Draw(); });
    ASSERT_TRUE(frame);

    EXPECT_EQ(sprite.GetMeshPixelScale(), Vec2F(2, 2));

    auto expected = VectorRasterizer::Rasterize(asset->GetMesh(Vec2F(2, 2))->mesh, 2.0f, background);
    ExpectSame(Crop(frame, rect), expected, "scale 2");
}

TEST_F(VectorSpriteRender, FringeIsOnePixelAtAnySize)
{
    auto asset = GetAsset("circle");

    for (int scale : { 1, 4, 8 })
    {
        VectorSprite sprite(asset);
        RectI rect(-80, -80, -80 + 20*scale, -80 + 20*scale);
        sprite.SetRect(rect);

        auto frame = DrawAndCapture([&]() { sprite.Draw(); });
        ASSERT_TRUE(frame);
        EXPECT_EQ(sprite.GetMeshPixelScale(), Vec2F((float)scale, (float)scale));

        // Row through the circle center, from the left edge of the image to the center
        int intermediate = 0;
        for (int x = 0; x < 10*scale; x++)
        {
            int value = PixelAt(frame, rect.left + x, rect.bottom + 10*scale).r;
            if (value > background.r + 8 && value < 247)
                intermediate++;
        }

        EXPECT_GE(intermediate, 1) << "scale " << scale << ": the edge must be anti-aliased";
        EXPECT_LE(intermediate, 2) << "scale " << scale << ": the fringe must stay one pixel wide";

        EXPECT_EQ(PixelAt(frame, rect.left + 10*scale, rect.bottom + 10*scale), Color4(255, 255, 255, 255));
    }
}

TEST_F(VectorSpriteRender, CameraZoomSelectsMeshOfViewScale)
{
    auto asset = GetAsset("circle");
    VectorSprite sprite(asset);
    sprite.SetRect(RectF(-10, -10, 10, 10));

    Vec2F viewScale;
    auto frame = DrawAndCapture([&]() { viewScale = o2Render.GetViewPixelScale(); sprite.Draw(); }, 0.25f);
    ASSERT_TRUE(frame);

    EXPECT_EQ(viewScale, Vec2F(4, 4));
    EXPECT_EQ(sprite.GetMeshPixelScale(), Vec2F(4, 4));

    auto expected = VectorRasterizer::Rasterize(asset->GetMesh(Vec2F(4, 4))->mesh, 4.0f, background);
    ExpectSame(Crop(frame, RectI(-40, -40, 40, 40)), expected, "camera zoom 4");
}

TEST_F(VectorSpriteRender, SlicedKeepsCornersAndStretchesCenter)
{
    auto asset = GetAsset("frame");
    BorderI slices = asset->GetSliceBorder();

    VectorSprite small(asset);
    RectI smallRect(-200, 100, -176, 124);
    small.SetRect(smallRect);

    VectorSprite sliced(asset);
    RectI slicedRect(-100, 0, -4, 60);
    sliced.SetRect(slicedRect);
    ASSERT_EQ(sliced.GetMode(), SpriteMode::Sliced);

    auto frame = DrawAndCapture([&]() { small.Draw(); sliced.Draw(); });
    ASSERT_TRUE(frame);
    EXPECT_EQ(sliced.GetMeshPixelScale(), Vec2F(1, 1)) << "sliced image keeps the scale of the view";

    // Corners are the same pixels as in the image of original size; top border is at the top of the image
    ExpectSame(Crop(frame, RectI(slicedRect.left, slicedRect.top - slices.top, slicedRect.left + slices.left, slicedRect.top)),
               Crop(frame, RectI(smallRect.left, smallRect.top - slices.top, smallRect.left + slices.left, smallRect.top)),
               "left top corner", 1, 1.0f);

    ExpectSame(Crop(frame, RectI(slicedRect.right - slices.right, slicedRect.bottom, slicedRect.right, slicedRect.bottom + slices.bottom)),
               Crop(frame, RectI(smallRect.right - slices.right, smallRect.bottom, smallRect.right, smallRect.bottom + slices.bottom)),
               "right bottom corner", 1, 1.0f);

    EXPECT_EQ(PixelAt(frame, slicedRect.left + 48, slicedRect.bottom + 30), Color4(0xe0, 0xe0, 0xe0, 255));
    EXPECT_EQ(PixelAt(frame, slicedRect.left + 1, slicedRect.bottom + 30), Color4(0x20, 0x40, 0x80, 255))
        << "the stroke of the stretched side stays 2 pixels wide";

    Vec2F targetSize(96, 60);
    BorderF borders = slices;
    auto mesh = asset->GetSlicedMesh(Vec2F(1, 1), borders);
    auto expected = RasterizeMapped(mesh->mesh, targetSize, [&](const Vec2F& point) {
        return VectorMesh::MapSlicedPoint(point, Vec2F(24, 24), borders, targetSize);
    });

    ExpectSame(Crop(frame, slicedRect), expected, "sliced");
}

TEST_F(VectorSpriteRender, DefaultModeStretchesWholeImage)
{
    auto asset = GetAsset("frame");

    VectorSprite sprite(asset);
    sprite.SetMode(SpriteMode::Default);

    RectI rect(-100, 0, -4, 60);
    sprite.SetRect(rect);

    auto frame = DrawAndCapture([&]() { sprite.Draw(); });
    ASSERT_TRUE(frame);
    EXPECT_EQ(sprite.GetMeshPixelScale(), VectorImageAsset::QuantizePixelScale(Vec2F(4.0f, 2.5f)));

    EXPECT_EQ(PixelAt(frame, rect.left + 4, rect.bottom + 30), Color4(0x20, 0x40, 0x80, 255))
        << "the stroke is stretched 4 times horizontally";

    auto mesh = asset->GetMesh(Vec2F(4.0f, 2.5f));
    auto expected = RasterizeMapped(mesh->mesh, Vec2F(96, 60), [&](const Vec2F& point) {
        return Vec2F(point.x*4.0f, point.y*2.5f);
    });

    ExpectSame(Crop(frame, rect), expected, "stretched");
}

TEST_F(VectorSpriteRender, FixedAspectFitsImageIntoRect)
{
    VectorSprite sprite(GetAsset("wide"));
    sprite.SetMode(SpriteMode::FixedAspect);

    RectI rect(0, 0, 40, 40);
    sprite.SetRect(rect);

    auto frame = DrawAndCapture([&]() { sprite.Draw(); });
    ASSERT_TRUE(frame);

    EXPECT_EQ(PixelAt(frame, 20, 20), Color4(0, 255, 0, 255));
    EXPECT_EQ(PixelAt(frame, 1, 11), Color4(0, 255, 0, 255));
    EXPECT_EQ(PixelAt(frame, 38, 28), Color4(0, 255, 0, 255));
    EXPECT_EQ(PixelAt(frame, 20, 5), background);
    EXPECT_EQ(PixelAt(frame, 20, 34), background);
}

TEST_F(VectorSpriteRender, ColorIsMultipliedAndTransparencyBlends)
{
    VectorSprite sprite(GetAsset("red"));
    sprite.SetRect(RectF(0, 0, 16, 16));

    auto frame = DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(PixelAt(frame, 8, 8), Color4(255, 0, 0, 255));

    sprite.SetColor(Color4(128, 255, 255, 255));
    frame = DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(PixelAt(frame, 8, 8), Color4(128, 0, 0, 255));

    VectorSprite colored(GetAsset("shapes"));
    colored.SetRect(RectF(-100, 50, -36, 90));
    DrawAndCapture([&]() { colored.Draw(); });
    colored.SetColor(Color4(200, 150, 100, 180));
    auto recolored = DrawAndCapture([&]() { colored.Draw(); });

    VectorSprite created(GetAsset("shapes"));
    created.SetRect(RectF(-100, 50, -36, 90));
    created.SetColor(Color4(200, 150, 100, 180));
    auto createdFrame = DrawAndCapture([&]() { created.Draw(); });
    EXPECT_FLOAT_EQ(BitmapCompare::GetSimilarity(*recolored, *createdFrame, 0), 1.0f)
        << "recoloring built vertices gives the same pixels as building them with the color";

    sprite.SetColor(Color4::White());
    sprite.SetTransparency(0.5f);
    frame = DrawAndCapture([&]() { sprite.Draw(); });

    Color4 blended = PixelAt(frame, 8, 8);
    EXPECT_NEAR(blended.r, (255 + background.r)/2, 2);
    EXPECT_NEAR(blended.g, background.g/2, 2);
    EXPECT_NEAR(blended.b, background.b/2, 2);

    sprite.SetTransparency(1.0f);
    sprite.SetOverrideColor(Color4(255, 255, 255, 0));
    frame = DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(PixelAt(frame, 8, 8), background);
}

TEST_F(VectorSpriteRender, DisabledSpriteDrawsNothing)
{
    VectorSprite sprite(GetAsset("red"));
    sprite.SetRect(RectF(0, 0, 16, 16));
    sprite.SetEnabled(false);

    auto frame = DrawAndCapture([&]() { sprite.Draw(); });
    ASSERT_TRUE(frame);
    EXPECT_EQ(CountDifferent(frame, background), 0);
    EXPECT_EQ(sprite.GetMeshRebuildsCount(), 0u);

    sprite.SetEnabled(true);
    frame = DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(CountDifferent(frame, background), 16*16);
}

TEST_F(VectorSpriteRender, VerticesAreRebuiltOnlyOnChange)
{
    VectorSprite sprite(GetAsset("shapes"));
    sprite.SetRect(RectF(0, 0, 64, 40));
    EXPECT_EQ(sprite.GetMeshRebuildsCount(), 0u) << "nothing is built before the first drawing";

    DrawAndCapture([&]() { sprite.Draw(); sprite.Draw(); });
    DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(sprite.GetMeshRebuildsCount(), 1u);
    EXPECT_EQ(sprite.GetColorUpdatesCount(), 1u);

    sprite.SetRect(RectF(10, 0, 74, 40));
    sprite.SetTransparency(0.7f);
    sprite.SetColor(Color4(255, 200, 200, 255));
    DrawAndCapture([&]() { sprite.Draw(); sprite.Draw(); });
    EXPECT_EQ(sprite.GetMeshRebuildsCount(), 2u) << "several changes before a drawing cost one rebuild";
    EXPECT_EQ(sprite.GetColorUpdatesCount(), 2u);

    sprite.SetTransparency(0.4f);
    sprite.SetColor(Color4(200, 200, 255, 255));
    DrawAndCapture([&]() { sprite.Draw(); sprite.Draw(); });
    EXPECT_EQ(sprite.GetMeshRebuildsCount(), 2u) << "a color change keeps the positions";
    EXPECT_EQ(sprite.GetColorUpdatesCount(), 3u);

    sprite.SetMode(SpriteMode::Default);
    sprite.SetSliceBorder(BorderI());
    DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(sprite.GetMeshRebuildsCount(), 2u) << "setting the same values changes nothing";

    // Camera scale inside one step of the mesh cache keeps the vertices of a sprite standing between pixels
    sprite.SetRect(RectF(10.5f, 0.5f, 74.5f, 40.5f));
    DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(sprite.GetMeshRebuildsCount(), 3u);

    DrawAndCapture([&]() { sprite.Draw(); }, 1.01f);
    EXPECT_EQ(sprite.GetMeshRebuildsCount(), 3u);
    EXPECT_EQ(sprite.GetColorUpdatesCount(), 4u);
}

TEST_F(VectorSpriteRender, CloneAndDeserializedDrawSamePixels)
{
    Ref<IRectDrawable> source = mmake<VectorSprite>(GetAsset("frame"));
    RectI rect(-60, -30, 20, 30);
    source->SetRect(rect);
    source->SetColor(Color4(255, 220, 180, 255));

    auto sourceFrame = DrawAndCapture([&]() { source->Draw(); });
    ASSERT_TRUE(sourceFrame);
    ASSERT_GT(CountDifferent(sourceFrame, background), 1000);

    Ref<IRectDrawable> clone = source->CloneAsRef<IRectDrawable>();
    ASSERT_TRUE(DynamicCast<VectorSprite>(clone));
    auto cloneFrame = DrawAndCapture([&]() { clone->Draw(); });
    EXPECT_FLOAT_EQ(BitmapCompare::GetSimilarity(*sourceFrame, *cloneFrame, 0), 1.0f);

    DataDocument data;
    data = source;

    Ref<IRectDrawable> loaded;
    data.Get(loaded);
    ASSERT_TRUE(DynamicCast<VectorSprite>(loaded));

    auto loadedFrame = DrawAndCapture([&]() { loaded->Draw(); });
    EXPECT_FLOAT_EQ(BitmapCompare::GetSimilarity(*sourceFrame, *loadedFrame, 0), 1.0f);
}

TEST_F(VectorSpriteRender, DrawsAsWidgetLayerWithLayerTransparency)
{
    auto asset = GetAsset("frame");

    auto widget = mmake<Widget>(ActorCreateMode::NotInScene);
    auto layer = widget->AddLayer("back", mmake<VectorSprite>(asset));

    DataDocument data;
    data = widget;

    Ref<Widget> loaded;
    data.Get(loaded);
    ASSERT_TRUE(loaded);

    auto loadedCopy = loaded->CloneAsRef<Widget>();
    auto sprite = loadedCopy->GetLayerDrawable<VectorSprite>("back");
    ASSERT_TRUE(sprite);
    EXPECT_EQ(sprite->GetMode(), SpriteMode::Sliced);

    RectI rect(-60, -30, 20, 30);
    *loadedCopy->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(80, 60), Vec2F(-20, 0));
    loadedCopy->UpdateTransform();
    ASSERT_EQ(sprite->GetRect(), RectF(rect)) << "the layer lays the drawable out by the widget rectangle";

    auto frame = DrawAndCapture([&]() { loadedCopy->GetLayer("back")->Draw(); });
    ASSERT_TRUE(frame);

    VectorSprite reference(asset);
    reference.SetRect(rect);
    auto referenceFrame = DrawAndCapture([&]() { reference.Draw(); });
    EXPECT_FLOAT_EQ(BitmapCompare::GetSimilarity(*frame, *referenceFrame, 0), 1.0f);

    loadedCopy->GetLayer("back")->SetTransparency(0.5f);
    reference.SetTransparency(0.5f);

    frame = DrawAndCapture([&]() { loadedCopy->GetLayer("back")->Draw(); });
    referenceFrame = DrawAndCapture([&]() { reference.Draw(); });
    EXPECT_FLOAT_EQ(BitmapCompare::GetSimilarity(*frame, *referenceFrame, 0), 1.0f);

    Color4 center = PixelAt(frame, -20, 0);
    EXPECT_NEAR(center.r, (0xe0 + background.r)/2, 2);
}

TEST_F(VectorSpriteRender, MeshBiggerThanRenderBatchIsDrawnInChunks)
{
    const int side = 36;

    auto asset = GetAsset("many");
    ASSERT_TRUE(asset->IsValid());

    VectorSprite sprite(asset);
    RectI rect(-200, -100, -200 + side*6, -100 + side*6);
    sprite.SetRect(rect);

    auto frame = DrawAndCapture([&]() { sprite.Draw(); });
    ASSERT_TRUE(frame);

    EXPECT_GT(sprite.GetTrianglesCount()*3, (UInt)USHRT_MAX) << "more indexes than a batch of GL backends holds";

    int missed = 0;
    for (int y = 0; y < side; y++)
    {
        for (int x = 0; x < side; x++)
        {
            if (PixelAt(frame, rect.left + x*6 + 3, rect.bottom + y*6 + 2) != Color4(255, 255, 255, 255))
                missed++;

            if (PixelAt(frame, rect.left + x*6, rect.bottom + y*6) != background)
                missed++;
        }
    }

    EXPECT_EQ(missed, 0);

    auto expected = VectorRasterizer::Rasterize(asset->GetMesh(Vec2F(1, 1))->mesh, 1.0f, background);
    ExpectSame(Crop(frame, rect), expected, "chunked");
}

TEST_F(VectorSpriteRender, MalformedImageDrawsNothing)
{
    auto asset = GetAsset("broken");
    ASSERT_TRUE(asset) << "the asset exists, its content is invalid";
    EXPECT_FALSE(asset->IsValid());

    VectorSprite sprite(asset);
    sprite.SetRect(RectF(0, 0, 50, 50));

    for (SpriteMode mode : { SpriteMode::Default, SpriteMode::Sliced, SpriteMode::FixedAspect, SpriteMode::Tiled })
    {
        sprite.SetMode(mode);
        sprite.SetSliceBorder(BorderI(4, 4, 4, 4));

        auto frame = DrawAndCapture([&]() { sprite.Draw(); });
        ASSERT_TRUE(frame);
        EXPECT_EQ(CountDifferent(frame, background), 0);
    }

    EXPECT_EQ(sprite.GetTrianglesCount(), 0u);

    VectorSprite empty;
    empty.SetRect(RectF(0, 0, 50, 50));
    auto frame = DrawAndCapture([&]() { empty.Draw(); });
    EXPECT_EQ(CountDifferent(frame, background), 0);
}

TEST_F(VectorSpriteRender, DegenerateRectDrawsNothing)
{
    VectorSprite sprite(GetAsset("frame"));

    for (SpriteMode mode : { SpriteMode::Default, SpriteMode::Sliced, SpriteMode::FixedAspect })
    {
        sprite.SetMode(mode);
        for (const RectF& rect : { RectF(10, 10, 10, 40), RectF(10, 10, 40, 10), RectF(10, 10, 10, 10) })
        {
            sprite.SetRect(rect);

            auto frame = DrawAndCapture([&]() { sprite.Draw(); });
            ASSERT_TRUE(frame);
            EXPECT_EQ(CountDifferent(frame, background), 0) << "mode " << (int)mode;
            EXPECT_EQ(sprite.GetTrianglesCount(), 0u) << "mode " << (int)mode;
        }
    }
}

TEST_F(VectorSpriteRender, BetweenPixelsDrawsMeshWithFringeOnEveryEdge)
{
    VectorSprite sprite(GetAsset("frame"));
    Vec2F size = sprite.GetOriginalSize();

    sprite.SetRect(RectF(10.0f, 10.0f, 10.0f + size.x, 10.0f + size.y));
    DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_TRUE(sprite.IsMeshPixelSnapped());

    sprite.SetRect(RectF(10.5f, 10.5f, 10.5f + size.x, 10.5f + size.y));
    DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_FALSE(sprite.IsMeshPixelSnapped()) << "hard edges of a snapped mesh would alias between pixels";

    sprite.SetRect(RectF(10.0f, 10.0f, 10.5f + size.x, 10.0f + size.y));
    DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_FALSE(sprite.IsMeshPixelSnapped()) << "the far corner is between pixels";

    sprite.SetRect(RectF(12.0f, 14.0f, 12.0f + size.x, 14.0f + size.y));
    DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_TRUE(sprite.IsMeshPixelSnapped());

    sprite.angle = Math::Deg2rad(30.0f);
    DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_FALSE(sprite.IsMeshPixelSnapped());
}

TEST_F(VectorSpriteRender, TotalDrawnTrianglesCountEverySubmittedMesh)
{
    VectorSprite sprite(GetAsset("frame"));
    sprite.SetRect(RectF(10, 10, 60, 60));

    UInt64 before = VectorSprite::GetTotalDrawnTriangles();
    DrawAndCapture([&]() { sprite.Draw(); sprite.Draw(); });
    EXPECT_EQ(VectorSprite::GetTotalDrawnTriangles() - before, (UInt64)sprite.GetTrianglesCount()*4)
        << "two drawings in each of the two frames a capture takes";
    EXPECT_GT(sprite.GetTrianglesCount(), 0u);

    sprite.enabled = false;
    before = VectorSprite::GetTotalDrawnTriangles();
    DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(VectorSprite::GetTotalDrawnTriangles(), before);
}

TEST_F(VectorSpriteRender, SlicedSmallerThanBordersCutsCorners)
{
    auto asset = GetAsset("frame");

    VectorSprite original(asset);
    RectI originalRect(-100, 0, -76, 24);
    original.SetRect(originalRect);

    VectorSprite sprite(asset);
    RectI rect(0, 0, 12, 12);
    sprite.SetRect(rect);

    auto frame = DrawAndCapture([&]() { original.Draw(); sprite.Draw(); });
    ASSERT_TRUE(frame);

    // Borders 8, 9, 10, 7 lose half of the excess: 5, 7, 7, 5; what is left of a corner is not squeezed
    ExpectSame(Crop(frame, RectI(rect.left, rect.top - 5, rect.left + 5, rect.top)),
               Crop(frame, RectI(originalRect.left, originalRect.top - 5, originalRect.left + 5, originalRect.top)),
               "left top corner", 1, 1.0f);

    ExpectSame(Crop(frame, RectI(rect.right - 7, rect.bottom, rect.right, rect.bottom + 7)),
               Crop(frame, RectI(originalRect.right - 7, originalRect.bottom, originalRect.right, originalRect.bottom + 7)),
               "right bottom corner", 1, 1.0f);

    ExpectSame(Crop(frame, RectI(rect.left, rect.bottom, rect.left + 5, rect.bottom + 7)),
               Crop(frame, RectI(originalRect.left, originalRect.bottom, originalRect.left + 5, originalRect.bottom + 7)),
               "left bottom corner", 1, 1.0f);
}

TEST_F(VectorSpriteRender, SlicedAxisWithoutBordersKeepsFringeOnePixel)
{
    auto asset = GetAsset("bar");

    VectorSprite sprite(asset);
    ASSERT_EQ(sprite.GetMode(), SpriteMode::Sliced);

    RectI rect(-100, -60, 100, 0);
    sprite.SetRect(rect);

    auto frame = DrawAndCapture([&]() { sprite.Draw(); });
    ASSERT_TRUE(frame);

    EXPECT_EQ(sprite.GetMeshPixelScale(), VectorImageAsset::QuantizePixelScale(Vec2F(10, 1)))
        << "the axis without borders is stretched as a whole, the axis with borders keeps the scale of the view";

    int intermediate = 0;
    for (int x = 0; x < 100; x++)
    {
        int value = PixelAt(frame, rect.left + x, rect.bottom + 30).r;
        if (value > background.r + 8 && value < 247)
            intermediate++;
    }

    EXPECT_GE(intermediate, 1);
    EXPECT_LE(intermediate, 2) << "the left edge is stretched 10 times, its fringe is not";

    EXPECT_EQ(PixelAt(frame, rect.left + 22, rect.bottom + 30), background);
    EXPECT_EQ(PixelAt(frame, rect.left + 28, rect.bottom + 30), Color4(255, 255, 255, 255));
}

TEST_F(VectorSpriteRender, AnotherImageOfSameSizeReplacesMesh)
{
    VectorSprite sprite(GetAsset("red"));
    sprite.SetRect(RectF(0, 0, 16, 16));

    auto frame = DrawAndCapture([&]() { sprite.Draw(); });
    ASSERT_EQ(PixelAt(frame, 8, 8), Color4(255, 0, 0, 255));

    sprite.SetImageAsset(GetAsset("blue"));
    frame = DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(PixelAt(frame, 8, 8), Color4(0, 0, 255, 255)) << "SetImageAsset";

    sprite.imageName = assets->GetPath("red");
    frame = DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(PixelAt(frame, 8, 8), Color4(255, 0, 0, 255)) << "imageName";

    VectorSprite blue(GetAsset("blue"));
    blue.SetRect(RectF(0, 0, 16, 16));
    sprite = blue;
    frame = DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(PixelAt(frame, 8, 8), Color4(0, 0, 255, 255)) << "operator=";

    VectorSprite red(GetAsset("red"));
    red.SetRect(RectF(0, 0, 16, 16));

    DataDocument data;
    red.Serialize(data);
    sprite.Deserialize(data);
    frame = DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(PixelAt(frame, 8, 8), Color4(255, 0, 0, 255)) << "deserialization";
}

TEST_F(VectorSpriteRender, ViewPixelScaleCountsScreenGraphicsScale)
{
    VectorSprite sprite(GetAsset("circle"));
    sprite.SetRect(RectF(-10, -10, 10, 10));

    TextureRef target(Vec2I(64, 64), TextureFormat::R8G8B8A8, Texture::Usage::RenderTarget);
    float graphicsScale = o2Application.GetGraphicsScale();
    RecordProperty("graphicsScale", (int)graphicsScale);

    Vec2F screenScale, targetScale, screenMeshScale, targetMeshScale;

    o2Render.Begin();
    o2Render.SetCamera(Camera());
    o2Render.Clear(background);

    screenScale = o2Render.GetViewPixelScale();
    sprite.Draw();
    screenMeshScale = sprite.GetMeshPixelScale();

    o2Render.BindRenderTexture(target);
    o2Render.SetCamera(Camera());
    targetScale = o2Render.GetViewPixelScale();
    sprite.Draw();
    targetMeshScale = sprite.GetMeshPixelScale();
    o2Render.UnbindRenderTexture();

    o2Render.SetCamera(Camera());
    o2Render.End();

    EXPECT_EQ(screenScale, Vec2F(graphicsScale, graphicsScale));
    EXPECT_EQ(screenMeshScale, VectorImageAsset::QuantizePixelScale(Vec2F(graphicsScale, graphicsScale)));
    EXPECT_EQ(targetScale, Vec2F(1, 1));
    EXPECT_EQ(targetMeshScale, Vec2F(1, 1));
}

TEST_F(VectorSpriteRender, ChangedImageIsPickedUpOnNextDrawing)
{
    auto asset = mmake<VectorImageAsset>();
    ASSERT_TRUE(asset->SetSource(Svg(16, 16, redBody)));

    VectorSprite sprite{ AssetRef<VectorImageAsset>(asset) };
    sprite.SetRect(RectF(0, 0, 16, 16));

    auto frame = DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(PixelAt(frame, 8, 8), Color4(255, 0, 0, 255));

    ASSERT_TRUE(asset->SetSource(Svg(16, 16, "<rect x=\"0\" y=\"0\" width=\"16\" height=\"16\" fill=\"#0000ff\"/>")));

    frame = DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(PixelAt(frame, 8, 8), Color4(0, 0, 255, 255));
}

TEST_F(VectorSpriteRender, SpriteOutOfScreenOrScissorSendsNoVertices)
{
    VectorSprite sprite(GetAsset("circle"));

    auto drawnVertices = [&](const Function<void()>& draw)
    {
        VectorSprite::Statistics before = VectorSprite::GetStatistics();
        o2Render.Begin();
        o2Render.SetCamera(Camera());
        o2Render.Clear(background);
        draw();
        o2Render.End();

        VectorSprite::Statistics after = VectorSprite::GetStatistics();
        EXPECT_EQ(after.draws + after.culledDraws - before.draws - before.culledDraws, 1u);
        EXPECT_EQ(o2Render.GetBatchStatistics().vertices, (UInt)(after.drawnVertices - before.drawnVertices));
        return (int)o2Render.GetBatchStatistics().vertices;
    };

    sprite.SetRect(RectF(0, 0, 20, 20));
    EXPECT_GT(drawnVertices([&]() { sprite.Draw(); }), 0);

    Vec2F half = (Vec2F)o2Render.GetResolution()*0.5f;
    sprite.SetRect(RectF(half.x + 10, 0, half.x + 30, 20));
    EXPECT_EQ(drawnVertices([&]() { sprite.Draw(); }), 0) << "out of the screen";

    sprite.SetRect(RectF(half.x - 10, 0, half.x + 10, 20));
    EXPECT_GT(drawnVertices([&]() { sprite.Draw(); }), 0) << "crossing the screen edge";

    sprite.SetRect(RectF(100, 0, 120, 20));
    auto drawClipped = [&](const RectI& scissor)
    {
        o2Render.EnableScissorTest(scissor);
        sprite.Draw();
        o2Render.DisableScissorTest();
    };

    EXPECT_EQ(drawnVertices([&]() { drawClipped(RectI(-50, -50, 50, 50)); }), 0) << "out of the scissor";
    EXPECT_GT(drawnVertices([&]() { drawClipped(RectI(-50, -50, 110, 50)); }), 0) << "crossing the scissor";

    sprite.SetRect(RectF(0, 0, 20, 20));
    sprite.SetTransparency(0.0f);
    EXPECT_EQ(drawnVertices([&]() { sprite.Draw(); }), 0) << "fully transparent";

    sprite.SetTransparency(1.0f);
    EXPECT_GT(drawnVertices([&]() { sprite.Draw(); }), 0);
}

TEST_F(VectorSpriteRender, CulledSpriteIsDrawnWhenItComesBack)
{
    VectorSprite sprite(GetAsset("red"));
    Vec2F half = (Vec2F)o2Render.GetResolution()*0.5f;

    sprite.SetRect(RectF(half.x + 10, 0, half.x + 26, 16));
    auto frame = DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(CountDifferent(frame, background), 0);

    sprite.SetRect(RectF(0, 0, 16, 16));
    frame = DrawAndCapture([&]() { sprite.Draw(); });
    EXPECT_EQ(CountDifferent(frame, background), 16*16);
}

TEST_F(VectorSpriteRender, StillSpriteIsNotCopiedByRender)
{
    bool wasMultithreaded = o2Render.IsMultithreadedRenderEnabled();
    o2Render.SetMultithreadedRenderEnabled(true);

    if (!o2Render.IsMultithreadedRenderEnabled())
        GTEST_SKIP() << "the render does not record frames on this platform";

    VectorSprite sprite(GetAsset("shapes"));
    sprite.SetRect(RectF(0, 0, 64, 40));

    auto drawFrame = [&]()
    {
        o2Render.Begin();
        o2Render.SetCamera(Camera());
        o2Render.Clear(background);
        sprite.Draw();
        o2Render.End();
        return o2Render.GetBatchStatistics();
    };

    drawFrame();
    for (int i = 0; i < 3; i++)
    {
        Render::BatchStatistics statistics = drawFrame();
        EXPECT_EQ(statistics.vertices, sprite.GetVerticesCount());
        EXPECT_EQ(statistics.retainedVertices, sprite.GetVerticesCount()) << "frame " << i;
    }

    EXPECT_EQ(sprite.GetMeshRebuildsCount(), 1u);

    // The same rectangle set again, as a layout update does, keeps the vertices and the batch
    sprite.SetRect(RectF(0, 0, 64, 40));
    EXPECT_EQ(drawFrame().retainedVertices, sprite.GetVerticesCount());
    EXPECT_EQ(sprite.GetMeshRebuildsCount(), 1u);

    sprite.SetRect(RectF(5, 0, 69, 40));
    EXPECT_EQ(drawFrame().retainedVertices, 0u) << "moved sprite rebuilds the batch";
    EXPECT_EQ(drawFrame().retainedVertices, sprite.GetVerticesCount());

    sprite.SetTransparency(0.5f);
    EXPECT_EQ(drawFrame().retainedVertices, 0u) << "recolored sprite rebuilds the batch";
    EXPECT_EQ(drawFrame().retainedVertices, sprite.GetVerticesCount());

    o2Render.SetMultithreadedRenderEnabled(wasMultithreaded);
}

TEST_F(VectorSpriteRender, SpriteDrawnAtSeveralPlacesInOneFrame)
{
    VectorSprite sprite(GetAsset("red"));

    auto draw = [&]()
    {
        for (int i = 0; i < 4; i++)
        {
            sprite.SetRect(RectF((float)(i*30), 0, (float)(i*30 + 16), 16));
            sprite.SetColor(i%2 == 0 ? Color4::White() : Color4(128, 128, 128, 255));
            sprite.Draw();
        }
    };

    // Several frames: the batch must not be taken from the previous frame with the vertices of the last place
    Ref<Bitmap> frame;
    for (int i = 0; i < 3; i++)
        frame = DrawAndCapture(draw);

    ASSERT_TRUE(frame);
    for (int i = 0; i < 4; i++)
    {
        Color4 pixel = PixelAt(frame, i*30 + 8, 8);
        EXPECT_NEAR(pixel.r, i%2 == 0 ? 255 : 128, 2) << "place " << i;
        EXPECT_NEAR(pixel.g, 0, 2) << "place " << i;

        Color4 between = PixelAt(frame, i*30 + 23, 8);
        EXPECT_NEAR(between.r, background.r, 2) << "after place " << i;
    }

    EXPECT_EQ(CountDifferent(frame, background), 4*16*16);
}

TEST_F(VectorSpriteRender, MovedSpriteDrawsTheSameAsBuiltAtThePlace)
{
    VectorSprite moved(GetAsset("shapes"));
    moved.SetRect(RectF(-100, -60, -36, -20));
    DrawAndCapture([&]() { moved.Draw(); });

    VectorSprite::Statistics before = VectorSprite::GetStatistics();

    // More moves than the vertices are shifted without a rebuild from the image
    Ref<Bitmap> frame;
    for (int i = 1; i <= 40; i++)
    {
        moved.SetRect(RectF(-100.0f + (float)(i*3), -60.0f + (float)i, -36.0f + (float)(i*3), -20.0f + (float)i));
        frame = DrawAndCapture([&]() { moved.Draw(); });
    }

    VectorSprite::Statistics after = VectorSprite::GetStatistics();
    EXPECT_GT(after.shiftedVertices, before.shiftedVertices);
    EXPECT_GT(after.rebuiltVertices, before.rebuiltVertices);
    EXPECT_LT(after.rebuiltVertices - before.rebuiltVertices, after.shiftedVertices - before.shiftedVertices);

    VectorSprite built(GetAsset("shapes"));
    built.SetRect(RectF(20, -20, 84, 20));
    auto expected = DrawAndCapture([&]() { built.Draw(); });

    ExpectSame(frame, expected, "moved against built", 1, 1.0f);

    // A move between pixels takes the mesh with the fringe on every edge, built from the image
    moved.SetRect(RectF(20.5f, -20, 84.5f, 20));
    built.SetRect(RectF(20.5f, -20, 84.5f, 20));
    frame = DrawAndCapture([&]() { moved.Draw(); });
    expected = DrawAndCapture([&]() { built.Draw(); });
    ExpectSame(frame, expected, "moved between pixels against built", 1, 1.0f);
    EXPECT_FALSE(moved.IsMeshPixelSnapped());
}

TEST_F(VectorSpriteRender, SmoothZoomBuildsFewMeshesAndSpreadsThemByFrames)
{
    Vector<Ref<VectorSprite>> sprites;
    for (const char* name : { "shapes", "red", "blue", "circle", "wide" })
    {
        auto sprite = mmake<VectorSprite>(GetAsset(name));
        sprite->SetPosition(Vec2F((float)sprites.Count()*3.0f - 6.0f, 0.5f));
        sprites.Add(sprite);
    }

    Vec2F viewScale;
    auto drawFrame = [&](float zoom)
    {
        o2Render.Begin();

        Camera camera;
        camera.SetScale(Vec2F(1.0f/zoom, 1.0f/zoom));
        o2Render.SetCamera(camera);
        viewScale = o2Render.GetViewPixelScale();

        for (auto& sprite : sprites)
            sprite->Draw();

        o2Render.SetCamera(Camera());
        o2Render.End();
    };

    drawFrame(1.0f);

    UInt before = VectorImageAsset::GetTessellationsCount();
    UInt maxPerFrame = 0;
    const int steps = 48;
    for (int i = 1; i <= steps; i++)
    {
        UInt frameBefore = VectorImageAsset::GetTessellationsCount();
        drawFrame(std::pow(2.0f, (float)i/16.0f));
        maxPerFrame = Math::Max(maxPerFrame, VectorImageAsset::GetTessellationsCount() - frameBefore);
    }

    for (int i = 0; i < 20; i++)
        drawFrame(8.0f);

    UInt built = VectorImageAsset::GetTessellationsCount() - before;
    EXPECT_LE(built, (UInt)sprites.Count()*2*3) << "a mesh per octave of the zoom, for the sprite on and between pixels";
    EXPECT_LE(maxPerFrame, 1u) << "new meshes of a zoom step come frame by frame";

    for (auto& sprite : sprites)
        EXPECT_EQ(sprite->GetMeshPixelScale(), viewScale) << "the mesh of the final zoom is built when it stops";
}

TEST_F(VectorSpriteRender, StillSpriteIsDrawnFromGpuCopyWithoutRenderThread)
{
    if (o2Render.IsMultithreadedRenderSupported())
        GTEST_SKIP() << "the render thread keeps the batches on this platform";

    VectorSprite sprite(GetAsset("shapes"));
    sprite.SetRect(RectF(0, 0, 64, 40));

    auto firstFrame = DrawAndCapture([&]() { sprite.Draw(); });
    ASSERT_TRUE(firstFrame);

    Render::BatchStatistics statistics;
    Ref<Bitmap> stillFrame;
    for (int i = 0; i < 8; i++)
    {
        stillFrame = DrawAndCapture([&]() { sprite.Draw(); statistics = o2Render.GetBatchStatistics(); });
        ASSERT_TRUE(stillFrame);
    }

    EXPECT_EQ(statistics.retainedVertices, sprite.GetVerticesCount()) << "a batch that stays the same is not copied";
    ExpectSame(stillFrame, firstFrame, "drawn from the GPU copy");

    auto other = mmake<VectorSprite>(GetAsset("circle"));
    other->SetRect(RectF(-60, -60, -40, -40));
    auto withOther = DrawAndCapture([&]() { sprite.Draw(); other->Draw(); statistics = o2Render.GetBatchStatistics(); });
    EXPECT_EQ(statistics.retainedVertices, 0u) << "another batch is copied again";
    EXPECT_GT(BitmapCompare::Compare(*withOther, *firstFrame, 0, background).maxDifference, 0);

    sprite.SetRect(RectF(5, 0, 69, 40));
    DrawAndCapture([&]() { sprite.Draw(); statistics = o2Render.GetBatchStatistics(); });
    EXPECT_EQ(statistics.retainedVertices, 0u) << "moved sprite rebuilds the batch";
}
