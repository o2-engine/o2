#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "EditorVectorUIFixture.h"
#include "o2/Assets/Types/AtlasAsset.h"
#include "o2/Assets/Types/BitmapFontAsset.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Assets/Types/Mesh3DAsset.h"
#include "o2/Assets/Types/SkinnedModelAsset.h"
#include "o2/Assets/Types/SoundAsset.h"
#include "o2/Assets/Types/VectorFontAsset.h"
#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Assets/Types/VideoAsset.h"
#include "o2/Render/Camera.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorRasterCache.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Render/Video.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Sound/SoundPlayer.h"
#include "o2/Utils/Math/Mesh3DPrimitives.h"
#include "o2Editor/Properties/Objects/Assets/FontAssetViewer.h"
#include "o2Editor/Properties/Objects/Assets/MediaAssetViewers.h"
#include "o2Editor/Properties/Objects/Assets/ModelAssetViewers.h"
#include "o2Editor/Properties/Objects/Assets/VectorFontAssetViewer.h"
#include "o2Editor/Properties/Objects/Assets/VectorImageAssetViewer.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/UI/Preview/FontSampleView.h"
#include "o2Editor/UI/Preview/MediaPlayerView.h"
#include "o2Editor/UI/Preview/ModelView.h"
#include "o2Editor/UI/Preview/VectorImageView.h"
#include "o2Editor/Windows/AssetPreviewWindow/AssetPreviewWindow.h"
#include "../../../Sources/Support/Assets/VectorImageTestAssets.h"
#include "../../../Sources/Support/Sound/SoundTestHelpers.h"
#include "../../../Sources/Support/Video/VideoTestData.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UI/Preview/PreviewToolbar.h"
#include "o2Editor/UI/Preview/TextureInspector.h"
#include "o2Editor/UI/Preview/TextureView.h"
#include "o2Editor/Windows/TexturesWindow/TexturesWindow.h"

using namespace o2;
using namespace o2::VectorImageTest;
using namespace Editor;
using namespace EditorVectorUITest;

namespace
{
    template<typename _widget_type>
    Vector<Ref<_widget_type>> FindAllByType(const Ref<Widget>& widget)
    {
        Vector<Ref<_widget_type>> res;
        if (auto typed = DynamicCast<_widget_type>(widget))
            res.Add(typed);

        for (auto& child : widget->GetChildWidgets())
            res.Add(FindAllByType<_widget_type>(child));

        return res;
    }

    // Draws the widget in a few frames, returns the last one
    Ref<Bitmap> DrawWidget(const Ref<Widget>& widget, int frames = 3)
    {
        Ref<Bitmap> captured;
        for (int i = 0; i < frames; i++)
        {
            if (i == frames - 1)
                o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

            widget->Update(0.016f);
            widget->UpdateSelfTransform();
            widget->UpdateChildrenTransforms();
            o2Render.Begin();
            o2Render.Clear(Color4(20, 20, 20, 255));
            o2Render.SetCamera(Camera());
            widget->Draw();
            o2Render.SetCamera(Camera());
            o2Render.End();
        }

        return captured;
    }

    // Returns the color of the captured frame at the point of the default camera
    Color4 FramePixel(const Ref<Bitmap>& frame, const Vec2F& point)
    {
        Vec2I size = frame->GetSize();
        float scale = (float)size.x/(float)o2Render.GetResolution().x;
        Vec2F half = Vec2F(o2Render.GetResolution())*0.5f;
        int x = Math::Clamp((int)((point.x + half.x)*scale), 0, size.x - 1);
        int y = Math::Clamp((int)((point.y + half.y)*scale), 0, size.y - 1);
        const UInt8* pixel = frame->GetData() + ((size_t)y*size.x + x)*4;
        return Color4((int)pixel[0], (int)pixel[1], (int)pixel[2], (int)pixel[3]);
    }

    Ref<o2::MenuPanel::Item> FindMenu(const Vector<o2::MenuPanel::Item>& items, const WString& text)
    {
        for (auto& item : items)
        {
            if (item.text == text)
                return mmake<o2::MenuPanel::Item>(item);
        }

        return nullptr;
    }

    Ref<ContextMenu::Item> FindSubItem(const Vector<Ref<ContextMenu::Item>>& items, const WString& text)
    {
        for (auto& item : items)
        {
            if (item->text == text)
                return item;
        }

        return nullptr;
    }
}

class EditorPreviews: public EditorVectorUIFixture
{};

// The View menu shows the icons of the windows, there is no Help, the engine tools of Debug are in a submenu
TEST_F(EditorPreviews, MenusShowWindowIconsAndKeepEngineToolsInASubmenu)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

    auto items = shell.menu->GetItems();
    EXPECT_FALSE(FindMenu(items, "Help"));

    auto view = FindMenu(items, "View");
    ASSERT_TRUE(view);
    int shown = 0;
    for (auto& item : view->subItems)
    {
        if (!item->text.StartsWith("Show "))
            continue;

        shown++;
        EXPECT_TRUE(item->icon) << (String)item->text;
    }

    EXPECT_GE(shown, 9);

    auto debug = FindMenu(items, "Debug");
    ASSERT_TRUE(debug);
    EXPECT_TRUE(FindSubItem(debug->subItems, "Textures"));
    EXPECT_TRUE(FindSubItem(debug->subItems, "Rebuild assets"));
    EXPECT_TRUE(FindSubItem(debug->subItems, "Vector raster cache"));
    EXPECT_FALSE(FindSubItem(debug->subItems, "Curve editor test"));

    auto engine = FindSubItem(debug->subItems, "Engine development");
    ASSERT_TRUE(engine);
    for (const char* tool : { "Curve editor test", "Save layout as default", "Randomize IDs", "View editor UI tree",
                              "Scalable vector meshes", "Text glyph meshes" })
    {
        EXPECT_TRUE(FindSubItem(engine->subItems, tool)) << tool;
    }
}

// A channel of a pixel is shown as gray, RGB as opaque
TEST(TextureView, ChannelBitmapShowsOneChannelAsGray)
{
    Bitmap source(PixelFormat::R8G8B8A8, Vec2I(1, 1));
    UInt8* data = source.GetData();
    data[0] = 10; data[1] = 20; data[2] = 30; data[3] = 40;

    auto pixel = [&](TextureView::Channel channel) {
        auto bitmap = TextureView::MakeChannelBitmap(source, channel);
        const UInt8* p = bitmap->GetData();
        return Color4((int)p[0], (int)p[1], (int)p[2], (int)p[3]);
    };

    EXPECT_EQ(pixel(TextureView::Channel::RGBA), Color4(10, 20, 30, 40));
    EXPECT_EQ(pixel(TextureView::Channel::RGB), Color4(10, 20, 30, 255));
    EXPECT_EQ(pixel(TextureView::Channel::Red), Color4(10, 10, 10, 255));
    EXPECT_EQ(pixel(TextureView::Channel::Green), Color4(20, 20, 20, 255));
    EXPECT_EQ(pixel(TextureView::Channel::Blue), Color4(30, 30, 30, 255));
    EXPECT_EQ(pixel(TextureView::Channel::Alpha), Color4(40, 40, 40, 255));
}

// The pixels are read and counted from the left top, as the texture is drawn; a channel is drawn as gray
TEST(TextureView, PixelsAreCountedFromTheLeftTopAsTheTextureIsDrawn)
{
    PushEditorScopeOnStack scope;

    // A bitmap keeps its rows from the bottom: red and green are the bottom row of the picture
    Bitmap bitmap(PixelFormat::R8G8B8A8, Vec2I(2, 2));
    const Color4 red(255, 0, 0, 255), green(0, 255, 0, 255), blue(0, 0, 255, 255), white(255, 255, 255, 255);
    const Color4 colors[4] = { red, green, blue, white };
    for (int i = 0; i < 4; i++)
    {
        UInt8* p = bitmap.GetData() + i*4;
        p[0] = (UInt8)colors[i].r; p[1] = (UInt8)colors[i].g; p[2] = (UInt8)colors[i].b; p[3] = (UInt8)colors[i].a;
    }

    TextureRef texture(bitmap);

    auto view = mmake<TextureView>();
    *view->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(200.0f, 200.0f));
    view->SetEnabledForcible(true);
    view->UpdateSelfTransform();
    view->SetTexture(texture);

    ASSERT_TRUE(view->IsReadable());
    EXPECT_EQ(view->GetPixel(Vec2I(0, 0)), blue);
    EXPECT_EQ(view->GetPixel(Vec2I(1, 0)), white);
    EXPECT_EQ(view->GetPixel(Vec2I(0, 1)), red);

    auto frame = DrawWidget(view);
    ASSERT_TRUE(frame);

    // The texture is fitted into the view, centered: the left top texel is up and to the left of the center
    Vec2F center = view->layout->GetWorldRect().Center(), leftTop = center + Vec2F(-20.0f, 20.0f);
    EXPECT_EQ(FramePixel(frame, leftTop), blue);
    EXPECT_EQ(FramePixel(frame, center + Vec2F(20.0f, -20.0f)), green);

    Vec2I pixel;
    ASSERT_TRUE(view->ScreenToPixel(leftTop, pixel));
    EXPECT_EQ(pixel, Vec2I(0, 0));
    EXPECT_FALSE(view->ScreenToPixel(center + Vec2F(95.0f, 0.0f), pixel)) << "out of the texture";

    view->SetChannel(TextureView::Channel::Blue);
    frame = DrawWidget(view);
    EXPECT_EQ(FramePixel(frame, leftTop), white) << "blue as white";
    EXPECT_EQ(FramePixel(frame, center + Vec2F(-20.0f, -20.0f)), Color4(0, 0, 0, 255));

    view->SetPixelZoom(4.0f);
    EXPECT_NEAR(view->GetPixelZoom(), 4.0f, 0.001f);
}

// A rectangle of the texture is the source rectangle of a sprite: its rows go down from the top of the picture
TEST(TextureView, SourceRectangleIsTheOneOfASprite)
{
    PushEditorScopeOnStack scope;

    // A bitmap keeps its rows from the bottom: the row 3 is the top of the picture
    Bitmap bitmap(PixelFormat::R8G8B8A8, Vec2I(1, 4));
    for (int row = 0; row < 4; row++)
    {
        UInt8* p = bitmap.GetData() + row*4;
        p[0] = (UInt8)(row*60); p[1] = 0; p[2] = 0; p[3] = 255;
    }

    TextureRef texture(bitmap);
    auto view = mmake<TextureView>();
    *view->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(100.0f, 100.0f));
    view->UpdateSelfTransform();

    view->SetTexture(texture, RectI(0, 0, 1, 2));
    EXPECT_EQ(view->GetPixel(Vec2I(0, 0)).r, 180);
    EXPECT_EQ(view->GetPixel(Vec2I(0, 1)).r, 120);

    // The sprite of the rectangle draws the same rows
    Sprite sprite(texture, RectI(0, 0, 1, 2));
    sprite.SetRect(RectF(-10.0f, 20.0f, 10.0f, -20.0f));
    Ref<Bitmap> frame;
    for (int i = 0; i < 2; i++)
    {
        if (i == 1)
            o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { frame = bitmap; });

        o2Render.Begin();
        o2Render.Clear(Color4(20, 20, 20, 255));
        o2Render.SetCamera(Camera());
        sprite.Draw();
        o2Render.End();
    }

    ASSERT_TRUE(frame);

    // The centers of the two rows stretched over the sprite
    EXPECT_NEAR(FramePixel(frame, Vec2F(0.0f, 10.0f)).r, 180, 10);
    EXPECT_NEAR(FramePixel(frame, Vec2F(0.0f, -10.0f)).r, 120, 10);
}

// The textures window lists every alive texture, the filter finds one by name and the inspector shows it
TEST_F(EditorPreviews, TexturesWindowListsTheAliveTexturesByName)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

    TextureRef texture(Vec2I(8, 4));
    texture->SetName("texture of the textures window test");
    EXPECT_TRUE(o2Render.GetTextures().Contains(texture)) << "the textures made from data are listed too";

    TexturesWindow::Show();
    auto& window = TexturesWindow::Instance();
    EXPECT_TRUE(window.GetWindow()->IsEnabled());
    EXPECT_GT(window.GetListedCount(), 1);

    window.SetFilter("textures window test");
    ASSERT_EQ(window.GetListedCount(), 1);
    EXPECT_EQ(window.GetListed(0), texture);
    EXPECT_TRUE(window.GetSummaryText().StartsWith("1 of ")) << (String)window.GetSummaryText();

    window.Select(0);
    EXPECT_EQ(window.GetInspector()->GetView()->GetTexture(), texture);
    EXPECT_TRUE(window.GetInspector()->GetStatusText().Contains("8x4")) << (String)window.GetInspector()->GetStatusText();
    EXPECT_TRUE(window.GetInspector()->GetStatusText().Contains("128 B"));

    window.SetFilter("");
    EXPECT_GT(window.GetListedCount(), 1);
}

// The list sorts by a key, a click on the key sorted by turns the direction; the separator sets the width of the list
TEST_F(EditorPreviews, TexturesWindowSortsTheListAndResizesIt)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));

    TextureRef small(Vec2I(4, 4)), big(Vec2I(32, 16)), middle(Vec2I(8, 8));
    small->SetName("sort test b");
    big->SetName("sort test c");
    middle->SetName("sort test a");

    TexturesWindow::Show();
    auto& window = TexturesWindow::Instance();
    window.SetFilter("sort test");
    ASSERT_EQ(window.GetListedCount(), 3);

    window.SetSort(TexturesWindow::SortBy::Name);
    EXPECT_EQ(window.GetListed(0), middle);
    EXPECT_EQ(window.GetListed(1), small);
    EXPECT_EQ(window.GetListed(2), big);

    window.SetSort(TexturesWindow::SortBy::Memory, true);
    EXPECT_EQ(window.GetListed(0), big);
    EXPECT_EQ(window.GetListed(2), small);

    auto& toggles = window.GetSortToggles();
    ASSERT_EQ(toggles.Count(), 5);
    auto memoryToggle = toggles[(int)TexturesWindow::SortBy::Memory];
    EXPECT_TRUE(memoryToggle->GetValue());
    EXPECT_NE(memoryToggle->caption, WString("Memory")) << "the key sorted by shows the direction";

    memoryToggle->onToggleByUser(false);
    EXPECT_TRUE(memoryToggle->GetValue());
    EXPECT_FALSE(window.IsSortDescending());
    EXPECT_EQ(window.GetListed(0), small);

    toggles[(int)TexturesWindow::SortBy::Name]->onToggleByUser(true);
    EXPECT_EQ(window.GetSortBy(), TexturesWindow::SortBy::Name);
    EXPECT_FALSE(memoryToggle->GetValue());
    EXPECT_EQ(memoryToggle->caption, WString("Memory"));
    EXPECT_EQ(window.GetListed(0), middle);

    window.SetSort(TexturesWindow::SortBy::Order);

    *window.GetWindow()->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(900.0f, 500.0f));
    DrawWidget(window.GetWindow());
    window.SetListWidth(320.0f);
    EXPECT_FLOAT_EQ(window.GetListWidth(), 320.0f);
    EXPECT_FLOAT_EQ(window.GetInspector()->layout->offsetLeft, 324.0f);

    window.SetListWidth(10.0f);
    EXPECT_GE(window.GetListWidth(), 200.0f) << "the list keeps a usable width";
    window.SetListWidth(400.0f);

    DrawWidget(window.GetWindow());
    auto edits = FindAllByType<EditBox>(window.GetWindow());
    ASSERT_FALSE(edits.IsEmpty());
    EXPECT_LE(edits[0]->layout->GetHeight(), PreviewToolbar::height) << "the filter fits its toolbar";
}

// A built file goes by its path in the assets of the platform
TEST(TextureView, BuiltFilesGoByTheirPathInTheAssets)
{
    TextureRef texture(Vec2I(2, 2));
    texture->SetName("../../BuiltAssets/Mac/Data/gem.png");
    EXPECT_EQ(TextureView::GetTextureName(texture), String("Data/gem.png"));

    texture->SetName("Atlas of ../../BuiltAssets/Mac/Data/atlas.json");
    EXPECT_EQ(TextureView::GetTextureName(texture), String("Atlas of Data/atlas.json"));
}

namespace
{
    // Counts the pixels of the rectangle that differ between two frames
    int CountChanged(const Ref<Bitmap>& a, const Ref<Bitmap>& b, const RectF& rect, int tolerance = 24)
    {
        int changed = 0;
        for (float y = Math::Min(rect.top, rect.bottom) + 1.0f; y < Math::Max(rect.top, rect.bottom) - 1.0f; y += 2.0f)
        {
            for (float x = Math::Min(rect.left, rect.right) + 1.0f; x < Math::Max(rect.left, rect.right) - 1.0f; x += 2.0f)
            {
                Color4 ca = FramePixel(a, Vec2F(x, y)), cb = FramePixel(b, Vec2F(x, y));
                if (Math::Abs(ca.r - cb.r) + Math::Abs(ca.g - cb.g) + Math::Abs(ca.b - cb.b) > tolerance)
                    changed++;
            }
        }

        return changed;
    }

    // A cube with one joint and a clip moving it up by two of its sizes
    SkinnedModelData BuildMovingCube()
    {
        SkinnedModelData model;
        auto box = Mesh3DPrimitives::BuildBox(Vec3F(1.0f, 1.0f, 1.0f));
        model.positions = box.positions;
        model.normals = box.normals;
        model.uvs = box.uvs;
        model.indices = box.indices;

        for (int i = 0; i < box.positions.Count(); i++)
        {
            SkinnedModelData::VertexInfluence influence;
            influence.weights[0] = 1.0f;
            model.influences.Add(influence);
        }

        SkinnedModelData::Node root;
        root.name = "root";
        model.nodes.Add(root);
        model.joints.Add(0);
        model.inverseBindMatrices.Add(Mat4());

        SkinnedModelData::AnimationChannel channel;
        channel.node = 0;
        channel.path = SkinnedModelData::AnimationChannel::Path::Translation;
        channel.times = { 0.0f, 1.0f };
        channel.vectors = { Vec3F(), Vec3F(0.0f, 2.0f, 0.0f) };

        SkinnedModelData::AnimationClip clip;
        clip.name = "Move";
        clip.duration = 1.0f;
        clip.channels.Add(channel);
        model.animations.Add(clip);

        return model;
    }

    // Builds the viewer of the object in a layout, as the Properties window shows it
    template<typename _viewer>
    Ref<_viewer> BuildViewer(IObject* object, Ref<VerticalLayout>& parent, const Vec2F& size = Vec2F(400.0f, 420.0f))
    {
        auto viewer = mmake<_viewer>();
        parent = o2UI.CreateWidget<VerticalLayout>();
        *parent->layout = WidgetLayout::Based(BaseCorner::Center, size, Vec2F());
        viewer->CheckCreateSpoiler(parent);
        viewer->SetHeaderEnabled(false);
        viewer->Refresh({ { object, nullptr } });
        parent->SetEnabledForcible(true);
        parent->UpdateSelfTransform();
        parent->UpdateChildrenTransforms();
        return viewer;
    }

    struct PreviewWindowGuard
    {
        ~PreviewWindowGuard()
        {
            if (AssetPreviewWindow::IsSingletonInitialzed())
                AssetPreviewWindow::Instance().SetAsset(AssetRef<Asset>());
        }
    };

    // Fits the preview window into the frame
    Ref<o2::Window> PlacePreviewWindow()
    {
        auto& window = AssetPreviewWindow::Instance().GetWindow();
        *window->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(640.0f, 520.0f));
        return window;
    }
}

// The Properties window picks the viewers of the media, the models and the fonts
TEST(EditorPreviewViewers, PropertiesPickTheViewersOfTheAssets)
{
    PushEditorScopeOnStack scope;

    auto expectViewer = [&](const Type& type, const Function<bool(const Ref<IObjectPropertiesViewer>&)>& check, const char* name) {
        auto viewer = o2EditorProperties.CreateObjectViewer(&type, "");
        EXPECT_TRUE(check(viewer)) << name;
        o2EditorProperties.FreeObjectViewer(viewer);
    };

    expectViewer(TypeOf(SoundAsset), [](auto& v) { return (bool)DynamicCast<SoundAssetViewer>(v); }, "sound");
    expectViewer(TypeOf(VideoAsset), [](auto& v) { return (bool)DynamicCast<VideoAssetViewer>(v); }, "video");
    expectViewer(TypeOf(Mesh3DAsset), [](auto& v) { return (bool)DynamicCast<Mesh3DAssetViewer>(v); }, "mesh");
    expectViewer(TypeOf(SkinnedModelAsset), [](auto& v) { return (bool)DynamicCast<SkinnedModelAssetViewer>(v); }, "skinned model");
    expectViewer(TypeOf(VectorFontAsset), [](auto& v) { return (bool)DynamicCast<VectorFontAssetViewer>(v); }, "vector font");
    expectViewer(TypeOf(BitmapFontAsset), [](auto& v) { return (bool)DynamicCast<FontAssetViewer>(v); }, "bitmap font");
}

// The sound viewer plays the sound and stops it
TEST(EditorPreviewViewers, SoundViewerPlaysTheSound)
{
    PushEditorScopeOnStack scope;

    auto sound = MakeTestSoundAsset(1.0f);
    Ref<VerticalLayout> parent;
    auto viewer = BuildViewer<SoundAssetViewer>(sound.Get(), parent);
    auto player = viewer->GetPlayer();
    ASSERT_TRUE(player);
    EXPECT_NEAR(player->GetDuration(), 1.0f, 0.05f);

    player->Play();
    EXPECT_TRUE(player->IsPlaying());
    player->Update(0.25f);
    EXPECT_GT(player->GetTime(), 0.1f);
    EXPECT_TRUE(player->GetTimeText().Contains("/ 1.00 s")) << (String)player->GetTimeText();

    player->Stop();
    EXPECT_FALSE(player->IsPlaying());
    EXPECT_FLOAT_EQ(player->GetTime(), 0.0f);
}

// The video viewer shows the first frame and plays the clip
TEST(EditorPreviewViewers, VideoViewerShowsTheFrameAndPlays)
{
    PushEditorScopeOnStack scope;

    auto video = o2::Tests::MakeTestVideoAsset();
    Ref<VerticalLayout> parent;
    auto viewer = BuildViewer<VideoAssetViewer>(video.Get(), parent);
    auto player = viewer->GetPlayer();
    ASSERT_TRUE(player);
    EXPECT_EQ(player->GetVideo()->GetVideoSize(), Vec2I(64, 64));

    auto frame = DrawWidget(parent);
    ASSERT_TRUE(frame);

    // Green around the red square in the middle of the frame
    RectF rect = player->layout->GetWorldRect();
    Vec2F center((rect.left + rect.right)*0.5f, (Math::Min(rect.top, rect.bottom) + 22.0f + Math::Max(rect.top, rect.bottom))*0.5f);
    float side = Math::Min(Math::Abs(rect.right - rect.left), Math::Abs(rect.top - rect.bottom) - 22.0f);
    Color4 green = FramePixel(frame, center + Vec2F(side*0.35f, 0.0f)), red = FramePixel(frame, center);
    EXPECT_GT(green.g, 140);
    EXPECT_LT(green.r, 60);
    EXPECT_GT(red.r, 170);

    // The moving clip knows its duration
    auto moving = o2::Tests::MakeMovingVideoAsset();
    player->SetVideo(AssetRef<VideoAsset>(moving));
    player->Play();
    EXPECT_TRUE(player->IsPlaying());
    player->Update(0.2f);
    EXPECT_GT(player->GetTime(), 0.1f);
}

// The preview of a vector image: the image, its triangles at the zoom, and its rasters in the pages of the cache
TEST_F(EditorPreviews, PreviewWindowShowsAVectorImageItsMeshAndItsRasters)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    PreviewWindowGuard guard;

    TempVectorAssets assets;
    assets.WriteSvg("previewed", Svg(40, 20, "<rect x=\"0\" y=\"0\" width=\"40\" height=\"20\" fill=\"#ff2000\"/>"));
    assets.WriteMeta("previewed", BorderI(), SpriteMode::Default, false, true);
    assets.AttachSources();

    AssetRef<VectorImageAsset> image(assets.GetPath("previewed"));
    ASSERT_TRUE(image);
    ASSERT_TRUE(image->IsRasterized());

    // A sprite of the image drawn in the game makes its raster
    {
        VectorSprite sprite(image);
        sprite.SetRect(RectF(-40, -20, 40, 20));
        for (int i = 0; i < 2; i++)
        {
            o2Render.Begin();
            o2Render.SetCamera(Camera());
            sprite.Draw();
            o2Render.End();
            o2VectorRasters.Flush();
        }
    }

    AssetPreviewWindow::Show(AssetRef<Asset>(image.GetRef()));
    auto& preview = AssetPreviewWindow::Instance();
    auto window = PlacePreviewWindow();
    EXPECT_EQ(preview.GetModes(), Vector<String>({ "Image", "Mesh", "Rasters" }));
    EXPECT_EQ(preview.GetContent(), DynamicCast<Widget>(preview.GetVectorImageView()));

    preview.SetMode("Mesh");
    DrawWidget(window);
    EXPECT_GT(preview.GetVectorImageView()->GetMeshTrianglesCount(), 0u);

    preview.SetMode("Rasters");
    auto inspector = preview.GetTextureInspector();
    EXPECT_EQ(preview.GetContent(), DynamicCast<Widget>(inspector));
    ASSERT_TRUE(inspector->GetView()->GetTexture());
    EXPECT_TRUE(TextureView::GetTextureName(inspector->GetView()->GetTexture()).StartsWith("Vector rasters page"));

    auto& cells = inspector->GetView()->GetHighlights();
    ASSERT_FALSE(cells.IsEmpty());

    // The highlighted cell holds the opaque raster: the highlight counts its rows from the top, as the pixels are
    const RectI& cell = cells[0];
    Vec2I middle((cell.left + cell.right)/2, (Math::Min(cell.top, cell.bottom) + Math::Max(cell.top, cell.bottom))/2);
    EXPECT_GT(inspector->GetView()->GetPixel(middle).a, 200) << middle.x << " " << middle.y;
    EXPECT_GT(inspector->GetView()->GetPixel(middle).r, 200);
}

// The preview of an image shows it and its atlas page with the image framed
TEST_F(EditorPreviews, PreviewWindowShowsAnImageAndItsAtlasPage)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    PreviewWindowGuard guard;

    AssetRef<ImageAsset> image("ui/UI4_Background.png");
    ASSERT_TRUE(image);
    Sprite sprite(image);

    AssetPreviewWindow::Show(AssetRef<Asset>(image.GetRef()));
    auto& preview = AssetPreviewWindow::Instance();
    EXPECT_EQ(preview.GetModes(), Vector<String>({ "Image", "Atlas page" }));

    auto view = preview.GetTextureInspector()->GetView();
    EXPECT_EQ(view->GetTexture(), sprite.GetTexture());
    EXPECT_EQ(view->GetSourceRect(), sprite.GetTextureSrcRect());
    EXPECT_TRUE(preview.GetInfoText().Contains("atlas")) << (String)preview.GetInfoText();

    preview.SetMode("Atlas page");
    EXPECT_EQ(view->GetSourceRect(), RectI(Vec2I(), sprite.GetTexture()->GetSize()));
    ASSERT_EQ(view->GetHighlights().Count(), 1);
    EXPECT_EQ(view->GetHighlights()[0], sprite.GetTextureSrcRect());

    AssetRef<AtlasAsset> atlas(image->GetAtlasUID());
    ASSERT_TRUE(atlas);
    AssetPreviewWindow::Show(AssetRef<Asset>(atlas.GetRef()));
    EXPECT_EQ(preview.GetModes().Count(), atlas->GetPages().Count());
    EXPECT_GT(view->GetHighlights().Count(), 10) << "the images of the page are framed";
}

// The preview of a font: lines of all the sizes, then the atlas of its glyphs
TEST_F(EditorPreviews, PreviewWindowShowsTheTextAndTheAtlasOfAFont)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    PreviewWindowGuard guard;

    AssetRef<VectorFontAsset> font("stdFont.ttf");
    ASSERT_TRUE(font);

    AssetPreviewWindow::Show(AssetRef<Asset>(font.GetRef()));
    auto& preview = AssetPreviewWindow::Instance();
    EXPECT_EQ(preview.GetModes(), Vector<String>({ "Text", "Atlas" }));

    auto sample = preview.GetFontSampleView();
    EXPECT_EQ(preview.GetContent(), DynamicCast<Widget>(sample));
    ASSERT_EQ(sample->GetLines().Count(), sample->GetSizes().Count());
    EXPECT_EQ(sample->GetLines()[0]->GetFontHeight(), sample->GetSizes()[0]);
    EXPECT_EQ(sample->GetLines().Last()->GetFontHeight(), 48);

    DrawWidget(PlacePreviewWindow());

    preview.SetMode("Atlas");
    EXPECT_EQ(preview.GetTextureInspector()->GetView()->GetTexture(), font->GetFont()->GetTexture());
    EXPECT_TRUE(preview.GetTextureInspector()->GetStatusText().Contains("Font atlas"))
        << (String)preview.GetTextureInspector()->GetStatusText();

    // The viewer in the Properties opens both
    PushEditorScopeOnStack scope;
    Ref<VerticalLayout> parent;
    auto viewer = BuildViewer<VectorFontAssetViewer>(font.Get(), parent);
    Ref<Button> atlasButton;
    for (auto& button : FindAllByType<Button>(parent))
    {
        if (button->GetCaption() == WString("Open atlas"))
            atlasButton = button;
    }

    ASSERT_TRUE(atlasButton);
    preview.SetMode("Text");
    atlasButton->onClick();
    EXPECT_EQ(preview.GetMode(), String("Atlas"));
}

// A model is drawn in 3D; a skinned one plays its clips in the preview
TEST_F(EditorPreviews, ModelViewDrawsTheModelAndPlaysItsClips)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    PreviewWindowGuard guard;

    PushEditorScopeOnStack scope;

    auto view = mmake<ModelView>();
    *view->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(300.0f, 300.0f));
    view->SetEnabledForcible(true);
    view->UpdateSelfTransform();

    view->SetMesh(Mesh3DData());
    auto empty = DrawWidget(view);
    view->SetMesh(Mesh3DPrimitives::BuildBox(Vec3F(1.0f, 1.0f, 1.0f)));
    auto box = DrawWidget(view);
    ASSERT_TRUE(empty && box);
    EXPECT_GT(CountChanged(empty, box, view->layout->GetWorldRect()), 1000) << "the box is drawn";
    EXPECT_EQ(view->GetTrianglesCount(), 12u);

    auto model = mmake<SkinnedModelAsset>();
    model->SetModelData(BuildMovingCube());

    AssetPreviewWindow::Show(AssetRef<Asset>(Ref<Asset>(model)));
    auto& preview = AssetPreviewWindow::Instance();
    EXPECT_EQ(preview.GetModes(), Vector<String>({ "Shaded", "Wireframe", "Shaded wireframe" }));
    EXPECT_TRUE(preview.GetInfoText().Contains("1 animations")) << (String)preview.GetInfoText();

    auto modelView = preview.GetModelView();
    EXPECT_EQ(preview.GetContent(), DynamicCast<Widget>(modelView));
    auto window = PlacePreviewWindow();
    auto still = DrawWidget(window);

    modelView->SetAnimation(0);
    modelView->SetPlaying(true);
    modelView->Update(0.5f);
    EXPECT_NEAR(modelView->GetTime(), 0.5f, 0.05f);
    auto moved = DrawWidget(window, 1);
    EXPECT_GT(CountChanged(still, moved, modelView->layout->GetWorldRect()), 200) << "the clip moves the cube";

    preview.SetMode("Wireframe");
    EXPECT_EQ(modelView->GetDrawMode(), ModelView::DrawMode::Wireframe);
}

// The image, atlas and vector image viewers open their asset in the preview window
TEST_F(EditorPreviews, ImageViewersOpenThePreview)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    PreviewWindowGuard guard;

    AssetRef<VectorImageAsset> image("ui/UI4_Background.svg");
    ASSERT_TRUE(image);

    PushEditorScopeOnStack scope;
    Ref<VerticalLayout> parent;
    auto viewer = BuildViewer<VectorImageAssetViewer>(image.Get(), parent);
    Ref<Button> open;
    for (auto& button : FindAllByType<Button>(parent))
    {
        if (button->GetCaption() == WString("Open preview"))
            open = button;
    }

    ASSERT_TRUE(open);
    open->onClick();
    ASSERT_TRUE(AssetPreviewWindow::IsSingletonInitialzed());
    EXPECT_EQ(AssetPreviewWindow::Instance().GetAsset()->GetUID(), image->GetUID());
    EXPECT_TRUE(AssetPreviewWindow::Instance().GetWindow()->IsEnabled());
}

// The channel and the background are toggles of one choice; the preview window has them in its one toolbar row
TEST_F(EditorPreviews, TextureControlsAreTogglesInTheRowOfThePreviewWindow)
{
    BuildStyle(false);
    EditorShell shell(MakeLayout("scene window", "assets window", "properties window"));
    PreviewWindowGuard guard;

    AssetRef<ImageAsset> image("ui/UI4_Background.png");
    ASSERT_TRUE(image);
    AssetPreviewWindow::Show(AssetRef<Asset>(image.GetRef()));
    auto& preview = AssetPreviewWindow::Instance();
    auto window = PlacePreviewWindow();
    auto inspector = preview.GetTextureInspector();
    auto view = inspector->GetView();

    auto& channels = inspector->GetChannelToggles();
    ASSERT_EQ(channels.Count(), 6);
    channels[(int)TextureView::Channel::Alpha]->onToggleByUser(true);
    EXPECT_EQ(view->GetChannel(), TextureView::Channel::Alpha);
    EXPECT_EQ(PreviewToolbar::GetChoice(channels), (int)TextureView::Channel::Alpha);
    EXPECT_FALSE(channels[(int)TextureView::Channel::RGBA]->GetValue());

    channels[(int)TextureView::Channel::Alpha]->onToggleByUser(false);
    EXPECT_TRUE(channels[(int)TextureView::Channel::Alpha]->GetValue()) << "a click on the toggle on keeps it";
    EXPECT_EQ(view->GetChannel(), TextureView::Channel::Alpha);

    auto& backgrounds = inspector->GetBackgroundToggles();
    ASSERT_EQ(backgrounds.Count(), 3);
    backgrounds[(int)TextureView::Background::Black]->onToggleByUser(true);
    EXPECT_EQ(view->GetBackground(), TextureView::Background::Black);

    inspector->SetBackground(TextureView::Background::White);
    EXPECT_EQ(PreviewToolbar::GetChoice(backgrounds), (int)TextureView::Background::White);
    inspector->SetChannel(TextureView::Channel::RGBA);

    DrawWidget(window);
    EXPECT_FALSE(inspector->GetToolbar()->IsEnabledInHierarchy());
    ASSERT_TRUE(inspector->GetControls()->IsEnabledInHierarchy());
    EXPECT_FLOAT_EQ(view->layout->GetWorldRect().top, inspector->layout->GetWorldRect().top) << "the view takes the row of the toolbar";

    auto modes = FindAllByType<DropDown>(window);
    ASSERT_FALSE(modes.IsEmpty());
    float row = modes[0]->layout->GetWorldRect().Center().y;
    for (auto& toggle : channels)
        EXPECT_NEAR(toggle->layout->GetWorldRect().Center().y, row, 0.5f);

    int fits = 0;
    for (auto& button : FindAllByType<Button>(window))
    {
        if (button->caption == WString("Fit") && button->IsEnabledInHierarchy())
            fits++;
    }

    EXPECT_EQ(fits, 1) << "one Fit in the row";

    TempVectorAssets assets;
    assets.WriteSvg("toolbar", Svg(40, 20, "<rect x=\"0\" y=\"0\" width=\"40\" height=\"20\" fill=\"#ff2000\"/>"));
    assets.WriteMeta("toolbar", BorderI(), SpriteMode::Default, false, true);
    assets.AttachSources();
    AssetRef<VectorImageAsset> vectorImage(assets.GetPath("toolbar"));
    ASSERT_TRUE(vectorImage);

    AssetPreviewWindow::Show(AssetRef<Asset>(vectorImage.GetRef()));
    DrawWidget(window);
    EXPECT_FALSE(inspector->GetControls()->IsEnabledInHierarchy());

    fits = 0;
    for (auto& button : FindAllByType<Button>(window))
    {
        if (button->caption == WString("Fit") && button->IsEnabledInHierarchy())
            fits++;
    }

    EXPECT_EQ(fits, 1) << "the vector image is fitted by the window";
}
