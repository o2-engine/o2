#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/AssetRef.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Render/Render.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "Assets/VectorImageTestAssets.h"

using namespace o2;
using namespace o2::VectorImageTest;

namespace
{
    const char* rectBody = "<rect x=\"2\" y=\"2\" width=\"16\" height=\"8\" fill=\"#ff0000\"/>";
}

TEST(VectorImageAsset, ExtensionIsBound)
{
    auto types = Assets::GetAssetsExtensionsTypes();
    ASSERT_TRUE(types.ContainsKey("svg"));
    EXPECT_EQ(types["svg"], &TypeOf(VectorImageAsset));
    EXPECT_EQ(Assets::GetAssetTypeByExtension("svg"), &TypeOf(VectorImageAsset));
}

TEST(VectorImageAsset, NewAssetHasOwnMetaType)
{
    auto asset = mmake<VectorImageAsset>();
    ASSERT_TRUE(asset->GetMeta());
    EXPECT_EQ(asset->GetMeta()->GetAssetType(), &TypeOf(VectorImageAsset));
    EXPECT_EQ(asset->GetDefaultMode(), SpriteMode::Default);
    EXPECT_EQ(asset->GetSliceBorder(), BorderI());
    EXPECT_FALSE(asset->IsValid());
    EXPECT_FALSE(asset->GetMesh(Vec2F(1, 1)));
}

TEST(VectorImageAsset, MetaSerializationRoundTrip)
{
    auto meta = mmake<VectorImageAsset::Meta>();
    meta->sliceBorder = BorderI(3, 4, 5, 6);
    meta->defaultMode = SpriteMode::Sliced;

    DataDocument data;
    data = Ref<AssetMeta>(meta);

    Ref<AssetMeta> loaded;
    data.Get(loaded);

    auto loadedMeta = DynamicCast<VectorImageAsset::Meta>(loaded);
    ASSERT_TRUE(loadedMeta);
    EXPECT_EQ(loadedMeta->sliceBorder, BorderI(3, 4, 5, 6));
    EXPECT_EQ(loadedMeta->defaultMode, SpriteMode::Sliced);
    EXPECT_TRUE(meta->IsEqual(loadedMeta.Get()));

    loadedMeta->sliceBorder.left = 9;
    EXPECT_FALSE(meta->IsEqual(loadedMeta.Get())) << "the builder detects changed borders by IsEqual";
}

TEST(VectorImageAsset, SetSourceParsesImage)
{
    auto asset = mmake<VectorImageAsset>();
    UInt version = asset->GetVersion();

    EXPECT_TRUE(asset->SetSource(Svg(20, 12, rectBody)));
    EXPECT_TRUE(asset->IsValid());
    EXPECT_TRUE(asset->GetError().IsEmpty());
    EXPECT_EQ(asset->GetSize(), Vec2F(20, 12));
    EXPECT_FLOAT_EQ(asset->GetWidth(), 20.0f);
    EXPECT_FLOAT_EQ(asset->GetHeight(), 12.0f);
    EXPECT_EQ(asset->GetImage().shapes.Count(), 1);
    EXPECT_NE(asset->GetVersion(), version);
}

TEST(VectorImageAsset, MalformedSourceIsInvalidAndHasNoMesh)
{
    auto asset = mmake<VectorImageAsset>();
    EXPECT_TRUE(asset->SetSource(Svg(20, 12, rectBody)));

    EXPECT_FALSE(asset->SetSource("<svg width=\"10\" height=\"10\"><rect"));
    EXPECT_FALSE(asset->IsValid());
    EXPECT_FALSE(asset->GetError().IsEmpty());
    EXPECT_EQ(asset->GetSize(), Vec2F());
    EXPECT_FALSE(asset->GetMesh(Vec2F(1, 1)));
    EXPECT_FALSE(asset->GetSlicedMesh(Vec2F(1, 1), BorderF(1, 1, 1, 1)));

    EXPECT_FALSE(asset->SetSource("not an svg at all"));
    EXPECT_FALSE(asset->SetSource(""));
}

TEST(VectorImageAsset, UnsupportedElementsGoToWarnings)
{
    auto asset = mmake<VectorImageAsset>();
    EXPECT_TRUE(asset->SetSource(Svg(20, 12, String(rectBody) + "<text x=\"1\" y=\"5\">hello</text>")));
    EXPECT_TRUE(asset->IsValid());
    EXPECT_FALSE(asset->GetWarnings().IsEmpty());
}

TEST(VectorImageAsset, PixelScaleIsQuantizedByPowersOfTwo)
{
    EXPECT_EQ(VectorImageAsset::QuantizePixelScale(Vec2F(1, 2)), Vec2F(1, 2));
    EXPECT_EQ(VectorImageAsset::QuantizePixelScale(Vec2F(1.02f, 1.97f)), Vec2F(1, 2));
    EXPECT_EQ(VectorImageAsset::QuantizePixelScale(Vec2F(0.5f, 2.0f)), Vec2F(0.5f, 2.0f));
    EXPECT_EQ(VectorImageAsset::QuantizePixelScale(Vec2F(1.3f, 1.3f)), Vec2F(1, 1));
    EXPECT_EQ(VectorImageAsset::QuantizePixelScale(Vec2F(1.5f, 3.0f)), Vec2F(2, 4));
    EXPECT_EQ(VectorImageAsset::QuantizePixelScale(Vec2F(0.7f, 0.7f)), Vec2F(0.5f, 0.5f));

    Vec2F clamped = VectorImageAsset::QuantizePixelScale(Vec2F(0.0f, 100000.0f));
    EXPECT_GT(clamped.x, 0.0f);
    EXPECT_LE(clamped.y, 64.0f);
}

TEST(VectorImageAsset, PixelScaleAxesDifferFourTimesAtMost)
{
    EXPECT_EQ(VectorImageAsset::QuantizePixelScale(Vec2F(1, 4)), Vec2F(1, 4));
    EXPECT_EQ(VectorImageAsset::QuantizePixelScale(Vec2F(1, 16)), Vec2F(4, 16));
    EXPECT_EQ(VectorImageAsset::QuantizePixelScale(Vec2F(3840.0f, 0.0000045f)), Vec2F(64, 16));
}

TEST(VectorImageAsset, VersionsOfDifferentAssetsNeverMatch)
{
    auto first = mmake<VectorImageAsset>();
    auto second = mmake<VectorImageAsset>();
    ASSERT_TRUE(first->SetSource(Svg(20, 12, rectBody)));
    ASSERT_TRUE(second->SetSource(Svg(20, 12, rectBody)));
    EXPECT_NE(first->GetVersion(), second->GetVersion());

    VectorImageAsset copy(*first);
    EXPECT_NE(copy.GetVersion(), first->GetVersion());
    EXPECT_NE(copy.GetVersion(), second->GetVersion());
}

TEST(VectorImageAsset, SlicedMeshIsSplitFromCachedWholeMesh)
{
    auto asset = mmake<VectorImageAsset>();
    ASSERT_TRUE(asset->SetSource(Svg(20, 12, rectBody)));

    auto sliced = asset->GetSlicedMesh(Vec2F(2, 2), BorderF(4, 4, 4, 4));
    ASSERT_TRUE(sliced);
    EXPECT_EQ(asset->GetCachedMeshesCount(), 2) << "the whole mesh of the scale is cached next to the sliced one";

    auto whole = asset->GetMesh(Vec2F(2, 2));
    EXPECT_EQ(asset->GetCachedMeshesCount(), 2);
    EXPECT_GT(sliced->mesh.GetTrianglesCount(), whole->mesh.GetTrianglesCount());
    EXPECT_EQ(sliced->mesh.pixelScale, Vec2F(2, 2));
    EXPECT_EQ(sliced->mesh.size, Vec2F(20, 12));

    for (int i = 0; i < 100; i++)
        asset->GetSlicedMesh(Vec2F(2, 2), BorderF(4.0f - (float)i*0.01f, 4, 4, 4));

    EXPECT_LE(asset->GetCachedMeshesCount(), 32);
    EXPECT_EQ(asset->GetMesh(Vec2F(2, 2)), whole) << "the whole mesh survives the cache overflow";
}

TEST(VectorImageAsset, MeshesAreCachedByQuantizedScaleAndSlices)
{
    auto asset = mmake<VectorImageAsset>();
    ASSERT_TRUE(asset->SetSource(Svg(20, 12, rectBody)));
    EXPECT_EQ(asset->GetCachedMeshesCount(), 0);

    auto mesh = asset->GetMesh(Vec2F(1, 1));
    ASSERT_TRUE(mesh);
    EXPECT_GT(mesh->mesh.GetTrianglesCount(), 0u);
    EXPECT_EQ(mesh->mesh.size, Vec2F(20, 12));
    EXPECT_EQ(mesh->mesh.pixelScale, Vec2F(1, 1));
    EXPECT_FALSE(mesh->sliced);

    EXPECT_EQ(asset->GetMesh(Vec2F(1, 1)), mesh);
    EXPECT_EQ(asset->GetMesh(Vec2F(1.01f, 0.99f)), mesh) << "close scales share one mesh";
    EXPECT_EQ(asset->GetCachedMeshesCount(), 1);

    auto doubleMesh = asset->GetMesh(Vec2F(2, 2));
    EXPECT_NE(doubleMesh, mesh);
    EXPECT_EQ(doubleMesh->mesh.pixelScale, Vec2F(2, 2));

    auto nonUniform = asset->GetMesh(Vec2F(2, 1));
    EXPECT_NE(nonUniform, mesh);
    EXPECT_NE(nonUniform, doubleMesh);
    EXPECT_EQ(asset->GetCachedMeshesCount(), 3);

    auto sliced = asset->GetSlicedMesh(Vec2F(1, 1), BorderF(4, 4, 4, 4));
    EXPECT_NE(sliced, mesh);
    EXPECT_TRUE(sliced->sliced);
    EXPECT_EQ(asset->GetSlicedMesh(Vec2F(1, 1), BorderF(4, 4, 4, 4)), sliced);
    EXPECT_NE(asset->GetSlicedMesh(Vec2F(1, 1), BorderF(5, 4, 4, 4)), sliced);
    EXPECT_EQ(asset->GetSlicedMesh(Vec2F(1, 1), BorderF()), mesh) << "empty borders need no split";
    EXPECT_EQ(asset->GetCachedMeshesCount(), 5);

    // Edges on the pixel bounds of the scale have no fringe
    Vec2F min, max;
    ASSERT_TRUE(mesh->mesh.GetBounds(min, max));
    EXPECT_NEAR(min.x, 2.0f, 0.01f);
    ASSERT_TRUE(doubleMesh->mesh.GetBounds(min, max));
    EXPECT_NEAR(min.x, 2.0f, 0.01f);
}

TEST(VectorImageAsset, NotPixelSnappedMeshIsCachedApartAndHasFringeOnPixelBounds)
{
    auto asset = mmake<VectorImageAsset>();
    ASSERT_TRUE(asset->SetSource(Svg(20, 12, rectBody)));

    auto snapped = asset->GetMesh(Vec2F(1, 1));
    auto fringed = asset->GetMesh(Vec2F(1, 1), false);
    ASSERT_TRUE(snapped && fringed);
    EXPECT_NE(snapped, fringed);
    EXPECT_TRUE(snapped->pixelSnapped);
    EXPECT_FALSE(fringed->pixelSnapped);
    EXPECT_EQ(asset->GetMesh(Vec2F(1, 1), false), fringed);
    EXPECT_EQ(asset->GetMesh(Vec2F(1, 1), true), snapped);
    EXPECT_EQ(asset->GetCachedMeshesCount(), 2);

    Vec2F min, max;
    ASSERT_TRUE(fringed->mesh.GetBounds(min, max));
    EXPECT_LT(min.x, 1.99f) << "the fringe goes out of the edge standing on a pixel bound";

    auto sliced = asset->GetSlicedMesh(Vec2F(1, 1), BorderF(4, 4, 4, 4), false);
    ASSERT_TRUE(sliced);
    EXPECT_FALSE(sliced->pixelSnapped);
    EXPECT_NE(asset->GetSlicedMesh(Vec2F(1, 1), BorderF(4, 4, 4, 4)), sliced);
}

TEST(VectorImageAsset, NewSourceDropsCachedMeshes)
{
    auto asset = mmake<VectorImageAsset>();
    ASSERT_TRUE(asset->SetSource(Svg(20, 12, rectBody)));

    auto mesh = asset->GetMesh(Vec2F(1, 1));
    UInt version = asset->GetVersion();

    ASSERT_TRUE(asset->SetSource(Svg(30, 12, rectBody)));
    EXPECT_NE(asset->GetVersion(), version);
    EXPECT_EQ(asset->GetCachedMeshesCount(), 0);

    auto newMesh = asset->GetMesh(Vec2F(1, 1));
    EXPECT_NE(newMesh, mesh);
    EXPECT_EQ(newMesh->mesh.size, Vec2F(30, 12));
    EXPECT_EQ(mesh->mesh.size, Vec2F(20, 12)) << "a mesh in use stays alive and unchanged";
}

TEST(VectorImageAsset, CacheIsBounded)
{
    auto asset = mmake<VectorImageAsset>();
    ASSERT_TRUE(asset->SetSource(Svg(20, 12, rectBody)));

    for (int i = 0; i < 100; i++)
        asset->GetMesh(Vec2F(0.25f*std::pow(2.0f, (float)i/8.0f), 1.0f));

    EXPECT_LE(asset->GetCachedMeshesCount(), 32);
}

TEST(VectorImageAsset, LoadsFromAssetsTreeWithoutRender)
{
    TempVectorAssets assets;
    assets.WriteSvg("frame", Svg(20, 12, rectBody));
    assets.WriteMeta("frame", BorderI(3, 4, 5, 6), SpriteMode::Sliced);
    assets.AttachSources();

    EXPECT_FALSE(Render::IsSingletonInitialzed()) << "the headless tier has no render";

    AssetRef<VectorImageAsset> asset(assets.GetPath("frame"));
    ASSERT_TRUE(asset);
    EXPECT_EQ(asset->GetType(), TypeOf(VectorImageAsset));
    EXPECT_TRUE(asset->IsValid());
    EXPECT_EQ(asset->GetSize(), Vec2F(20, 12));
    EXPECT_EQ(asset->GetSliceBorder(), BorderI(3, 4, 5, 6));
    EXPECT_EQ(asset->GetDefaultMode(), SpriteMode::Sliced);
    EXPECT_TRUE(asset->GetMesh(Vec2F(1, 1)));

    AssetRef<Asset> generic = o2Assets.GetAssetRef(assets.GetPath("frame"));
    EXPECT_EQ(generic.Get(), asset.Get()) << "one shared instance per file";
}

TEST(VectorImageAsset, MalformedFileLoadsAsInvalidAsset)
{
    TempVectorAssets assets;
    assets.WriteSvg("broken", "<svg width=\"10\" height=\"10\"><path d=\"M0 0");
    assets.WriteMeta("broken", BorderI(), SpriteMode::Default);
    assets.AttachSources();

    AssetRef<VectorImageAsset> asset(assets.GetPath("broken"));
    ASSERT_TRUE(asset);
    EXPECT_FALSE(asset->IsValid());
    EXPECT_FALSE(asset->GetError().IsEmpty());
    EXPECT_FALSE(asset->GetMesh(Vec2F(1, 1)));
}

TEST(VectorImageAsset, ReloadsWhenRebuilt)
{
    TempVectorAssets assets;
    assets.WriteSvg("reloaded", Svg(20, 12, rectBody));
    assets.WriteMeta("reloaded", BorderI(), SpriteMode::Default);
    assets.AttachSources();

    AssetRef<VectorImageAsset> asset(assets.GetPath("reloaded"));
    ASSERT_TRUE(asset);
    auto mesh = asset->GetMesh(Vec2F(1, 1));
    UInt version = asset->GetVersion();

    assets.WriteSvg("reloaded", Svg(40, 12, rectBody));

    UID other;
    other.Randomize();
    o2Assets.onAssetsRebuilt({ other });
    EXPECT_EQ(asset->GetVersion(), version) << "rebuild of other assets must not reload this one";

    o2Assets.onAssetsRebuilt({ asset->GetUID() });
    EXPECT_NE(asset->GetVersion(), version);
    EXPECT_EQ(asset->GetSize(), Vec2F(40, 12));
    EXPECT_NE(asset->GetMesh(Vec2F(1, 1)), mesh);
}

TEST(VectorImageAsset, SaveWritesSource)
{
    TempVectorAssets assets;
    assets.WriteSvg("saved", Svg(20, 12, rectBody));
    assets.WriteMeta("saved", BorderI(), SpriteMode::Default);
    assets.AttachSources();

    AssetRef<VectorImageAsset> asset(assets.GetPath("saved"));
    ASSERT_TRUE(asset);

    String source = Svg(8, 8, rectBody);
    asset->SetSource(source);
    asset->SetSliceBorder(BorderI(1, 2, 3, 4));
    asset->Save();

    EXPECT_EQ(o2FileSystem.ReadFile(assets.GetSourcePath() + assets.GetPath("saved")), source);

    DataDocument metaData;
    ASSERT_TRUE(metaData.LoadFromFile(assets.GetSourcePath() + assets.GetPath("saved") + ".meta"));
    Ref<AssetMeta> meta;
    metaData.Get(meta);
    auto vectorMeta = DynamicCast<VectorImageAsset::Meta>(meta);
    ASSERT_TRUE(vectorMeta);
    EXPECT_EQ(vectorMeta->sliceBorder, BorderI(1, 2, 3, 4));
}

TEST(VectorSpriteHeadless, DefaultConstructionIsValid)
{
    VectorSprite sprite;
    EXPECT_EQ(sprite.GetMode(), SpriteMode::Default);
    EXPECT_FALSE(sprite.GetImageAsset());
    EXPECT_TRUE(sprite.GetImageName().IsEmpty());
    EXPECT_EQ(sprite.GetOriginalSize(), Vec2F());
    EXPECT_EQ(sprite.GetTrianglesCount(), 0u);
    EXPECT_TRUE(sprite.IsEnabled());
}

TEST(VectorSpriteHeadless, LoadFromImageTakesSizeModeAndSlicesFromAsset)
{
    TempVectorAssets assets;
    assets.WriteSvg("panel", Svg(20, 12, rectBody));
    assets.WriteMeta("panel", BorderI(3, 4, 5, 6), SpriteMode::Sliced);
    assets.AttachSources();

    VectorSprite byPath(assets.GetPath("panel"));
    ASSERT_TRUE(byPath.GetImageAsset());
    EXPECT_EQ(byPath.GetImageName(), assets.GetPath("panel"));
    EXPECT_EQ(byPath.GetSize(), Vec2F(20, 12));
    EXPECT_EQ(byPath.GetOriginalSize(), Vec2F(20, 12));
    EXPECT_EQ(byPath.GetMode(), SpriteMode::Sliced);
    EXPECT_EQ(byPath.GetSliceBorder(), BorderI(3, 4, 5, 6));

    AssetRef<VectorImageAsset> asset(assets.GetPath("panel"));
    VectorSprite byRef(asset);
    EXPECT_EQ(byRef.GetImageAsset(), asset);
    EXPECT_EQ(byRef.GetSize(), Vec2F(20, 12));

    VectorSprite byId;
    byId.LoadFromImage(asset->GetUID());
    EXPECT_EQ(byId.GetImageAsset(), asset);

    VectorSprite keepSize;
    keepSize.SetSize(Vec2F(100, 50));
    keepSize.SetImageAsset(asset);
    EXPECT_EQ(keepSize.GetSize(), Vec2F(100, 50)) << "SetImageAsset keeps the size, like Sprite";
    EXPECT_EQ(keepSize.GetMode(), SpriteMode::Sliced);

    keepSize.NormalizeAspectByWidth();
    EXPECT_EQ(keepSize.GetSize(), Vec2F(100, 60));
    keepSize.NormalizeSize();
    EXPECT_EQ(keepSize.GetSize(), Vec2F(20, 12));

    VectorSprite missing("no/such/image.svg");
    EXPECT_FALSE(missing.GetImageAsset());
}

TEST(VectorSpriteHeadless, CopyAndCloneKeepState)
{
    TempVectorAssets assets;
    assets.WriteSvg("copied", Svg(20, 12, rectBody));
    assets.WriteMeta("copied", BorderI(3, 4, 5, 6), SpriteMode::Sliced);
    assets.AttachSources();

    VectorSprite source(assets.GetPath("copied"));
    source.SetRect(RectF(10, 20, 110, 80));
    source.SetColor(Color4(10, 20, 30, 40));
    source.SetEnabled(false);

    VectorSprite copy(source);
    EXPECT_TRUE(copy == source);
    EXPECT_EQ(copy.GetImageAsset(), source.GetImageAsset());
    EXPECT_EQ(copy.GetMode(), SpriteMode::Sliced);
    EXPECT_EQ(copy.GetSliceBorder(), BorderI(3, 4, 5, 6));
    EXPECT_EQ(copy.GetRect(), RectF(10, 20, 110, 80));
    EXPECT_EQ(copy.GetColor(), Color4(10, 20, 30, 40));
    EXPECT_FALSE(copy.IsEnabled());

    Ref<IRectDrawable> cloned = source.CloneAsRef<IRectDrawable>();
    auto clonedSprite = DynamicCast<VectorSprite>(cloned);
    ASSERT_TRUE(clonedSprite) << "WidgetLayer clones drawables through IRectDrawable";
    EXPECT_TRUE(*clonedSprite == source);

    VectorSprite assigned;
    assigned = source;
    EXPECT_TRUE(assigned == source);

    assigned.SetMode(SpriteMode::Default);
    EXPECT_TRUE(assigned != source);
}

TEST(VectorSpriteHeadless, SerializationRoundTrip)
{
    TempVectorAssets assets;
    assets.WriteSvg("serialized", Svg(20, 12, rectBody));
    assets.WriteMeta("serialized", BorderI(), SpriteMode::Default);
    assets.AttachSources();

    Ref<IRectDrawable> source = mmake<VectorSprite>(assets.GetPath("serialized"));
    auto sourceSprite = DynamicCast<VectorSprite>(source);
    sourceSprite->SetMode(SpriteMode::Sliced);
    sourceSprite->SetSliceBorder(BorderI(1, 2, 3, 4));
    sourceSprite->SetColor(Color4(200, 100, 50, 255));
    sourceSprite->SetRect(RectF(0, 0, 64, 32));

    DataDocument data;
    data = source;
    EXPECT_EQ((String)data["Type"], "o2::VectorSprite") << "drawables are serialized polymorphically by type name";

    Ref<IRectDrawable> loaded;
    data.Get(loaded);

    auto loadedSprite = DynamicCast<VectorSprite>(loaded);
    ASSERT_TRUE(loadedSprite);
    EXPECT_EQ(loadedSprite->GetImageAsset(), sourceSprite->GetImageAsset());
    EXPECT_EQ(loadedSprite->GetMode(), SpriteMode::Sliced);
    EXPECT_EQ(loadedSprite->GetSliceBorder(), BorderI(1, 2, 3, 4));
    EXPECT_EQ(loadedSprite->GetColor(), Color4(200, 100, 50, 255));
    EXPECT_EQ(loadedSprite->GetRect(), RectF(0, 0, 64, 32));
}

TEST(VectorSpriteHeadless, WorksAsWidgetLayerDrawable)
{
    TempVectorAssets assets;
    assets.WriteSvg("layer", Svg(20, 12, rectBody));
    assets.WriteMeta("layer", BorderI(2, 2, 2, 2), SpriteMode::Sliced);
    assets.AttachSources();

    auto widget = mmake<Widget>();
    widget->AddLayer("back", mmake<VectorSprite>(assets.GetPath("layer")));

    auto checkLayer = [&](const Ref<Widget>& checked)
    {
        auto layer = checked->GetLayer("back");
        ASSERT_TRUE(layer);

        auto sprite = DynamicCast<VectorSprite>(layer->GetDrawable());
        ASSERT_TRUE(sprite);
        EXPECT_EQ(sprite->GetImageName(), assets.GetPath("layer"));
        EXPECT_EQ(sprite->GetMode(), SpriteMode::Sliced);
        EXPECT_EQ(sprite->GetSliceBorder(), BorderI(2, 2, 2, 2));
        EXPECT_EQ(checked->GetLayerDrawable<VectorSprite>("back"), sprite);
    };

    checkLayer(widget);

    auto cloned = widget->CloneAsRef<Widget>();
    checkLayer(cloned);
    EXPECT_NE(cloned->GetLayer("back")->GetDrawable(), widget->GetLayer("back")->GetDrawable());

    DataDocument data;
    data = widget;

    Ref<Widget> loaded;
    data.Get(loaded);
    ASSERT_TRUE(loaded);
    checkLayer(loaded);

    auto loadedLayer = loaded->GetLayer("back");
    loadedLayer->SetTransparency(0.5f);
    EXPECT_NEAR(loadedLayer->GetDrawable()->GetTransparency(), 0.5f, 0.01f);
}
