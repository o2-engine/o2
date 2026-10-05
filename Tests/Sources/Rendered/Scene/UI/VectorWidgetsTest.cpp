#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Animation/AnimationClip.h"
#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorGraphics/VectorRasterizer.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/CustomList.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Image.h"
#include "o2/Scene/UI/Widgets/LongList.h"
#include "o2/Scene/UI/Widgets/MenuPanel.h"
#include "o2/Scene/UI/Widgets/Tree.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2AssetBuilder/AssetsBuilder.h"
#include "Assets/VectorImageTestAssets.h"
#include "Scene/SceneTestHelpers.h"
#include "Scene/UI/UITestHelpers.h"

using namespace o2;
using namespace o2::VectorImageTest;

namespace
{
    const Color4 background(96, 96, 96, 255);
    const Color4 red(255, 0, 0, 255);
    const Color4 green(0, 255, 0, 255);
    const Color4 blue(0, 0, 255, 255);

    const char* redBody = "<rect x=\"0\" y=\"0\" width=\"16\" height=\"16\" fill=\"#ff0000\"/>";
    const char* greenBody = "<rect x=\"0\" y=\"0\" width=\"16\" height=\"16\" fill=\"#00ff00\"/>";

    class ListProbe: public CustomList
    {
    public:
        using CustomList::CustomList;
        using CustomList::mSelectedItems;
        using CustomList::mSelectionSpritesPool;
    };

    bool ShowsVector(const Ref<IRectDrawable>& drawable, const String& imagePath)
    {
        auto sprite = DynamicCast<VectorSprite>(drawable);
        return sprite && sprite->GetImageName() == imagePath;
    }

    template<typename _widget_type>
    Ref<_widget_type> SaveAndLoad(const Ref<_widget_type>& widget)
    {
        DataDocument data;
        data = Ref<Widget>(widget);

        Ref<Widget> loaded;
        data.Get(loaded);
        return DynamicCast<_widget_type>(loaded);
    }

    template<typename _widget_type>
    Ref<_widget_type> LoadOldFormat(const String& fields)
    {
        String json = String("{\"Type\": \"") + TypeOf(_widget_type).GetName() + "\", \"Value\": {\"mName\": \"old\", " +
            fields + ", \"Id\": 7, \"Transform\": {}, \"InternalWidgets\": [], \"Layers\": [], \"States\": []}}";

        DataDocument data;
        if (!data.LoadFromData(json))
            return nullptr;

        Ref<Widget> loaded;
        data.Get(loaded);
        return DynamicCast<_widget_type>(loaded);
    }

    // Field of a concrete sprite as it was written before the fields became Ref<IRectDrawable>
    String OldSprite(const String& field, int alpha)
    {
        return "\"" + field + "\": {\"Type\": \"o2::Sprite\", \"Value\": {\"mColor\": {\"r\": 10, \"g\": 20, \"b\": 30, \"a\": " +
            (String)alpha + "}, \"mSize\": {\"x\": 50.0, \"y\": 40.0, \"z\": 0.0}}}";
    }

    bool IsOldSprite(const Ref<IRectDrawable>& drawable, int alpha)
    {
        auto sprite = DynamicCast<Sprite>(drawable);
        return sprite && sprite->GetColor() == Color4(10, 20, 30, alpha) && sprite->GetSize2D() == Vec2F(50, 40);
    }

    String RasterPathOf(const String& vectorPath)
    {
        return vectorPath.SubStr(0, vectorPath.Length() - 3) + "png";
    }

    // Vector images built by the real assets builder; with a raster sibling of "red" filled with blue when asked
    TempVectorAssets* BuildImages(bool withRasterSibling)
    {
        auto assets = new TempVectorAssets();
        assets->WriteSvg("red", Svg(16, 16, redBody));
        assets->WriteSvg("green", Svg(16, 16, greenBody));

        if (withRasterSibling)
        {
            Bitmap sibling(PixelFormat::R8G8B8A8, Vec2I(16, 16));
            for (int i = 0; i < 16*16; i++)
            {
                UInt8* pixel = sibling.GetData() + i*4;
                pixel[0] = 0; pixel[1] = 0; pixel[2] = 255; pixel[3] = 255;
            }

            sibling.Save(assets->GetSourcePath() + RasterPathOf(assets->GetPath("red")), Bitmap::ImageType::Png);
        }

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

    class VectorWidgets: public ::testing::Test
    {
    public:
        static void SetUpTestSuite() { assets = BuildImages(false); }

        static void TearDownTestSuite()
        {
            delete assets;
            assets = nullptr;
        }

        static String Path(const String& name) { return assets->GetPath(name); }
        static Ref<VectorSprite> Vector(const String& name) { return mmake<VectorSprite>(Path(name)); }

        static TempVectorAssets* assets;

        SceneCleanGuard guard;
    };

    TempVectorAssets* VectorWidgets::assets = nullptr;

    class BuiltImages: public ::testing::Test
    {
    public:
        static void SetUpTestSuite() { assets = BuildImages(true); }

        static void TearDownTestSuite()
        {
            delete assets;
            assets = nullptr;
        }

        static String Path(const String& name) { return assets->GetPath(name); }

        static String RasterPath(const String& name) { return RasterPathOf(Path(name)); }

        static Ref<VectorSprite> Vector(const String& name) { return mmake<VectorSprite>(Path(name)); }

        static Ref<Bitmap> DrawAndCapture(const Function<void()>& draw)
        {
            Ref<Bitmap> captured;

            // The first frame captured after switching to the capture target may need a repeat
            for (int i = 0; i < 2; i++)
            {
                o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

                o2Render.Begin();
                o2Render.SetCamera(Camera());
                o2Render.Clear(background);
                draw();
                o2Render.End();
            }

            return captured;
        }

        // Color of the frame pixel in the default camera space, where Y is up
        static Color4 PixelAt(const Ref<Bitmap>& frame, int x, int y)
        {
            Vec2I frameSize = frame->GetSize();
            return VectorRasterizer::GetPixel(*frame, Math::RoundToInt(frameSize.x/2.0f) + x,
                                              Math::RoundToInt(frameSize.y/2.0f) - 1 - y);
        }

        static void ExpectColor(const Color4& pixel, const Color4& expected, const char* what)
        {
            EXPECT_NEAR(pixel.r, expected.r, 3) << what;
            EXPECT_NEAR(pixel.g, expected.g, 3) << what;
            EXPECT_NEAR(pixel.b, expected.b, 3) << what;
        }

        // Draws the drawable in a rectangle around the center and expects the color in its middle
        static void ExpectDraws(const Ref<IRectDrawable>& drawable, const Color4& expected, const char* what)
        {
            ASSERT_TRUE(drawable) << what;

            drawable->SetEnabled(true);
            drawable->SetTransparency(1.0f);
            drawable->SetRect(RectF(-20, -20, 20, 20));

            auto frame = DrawAndCapture([&]() { drawable->Draw(); });
            ASSERT_TRUE(frame) << what;
            ExpectColor(PixelAt(frame, 0, 0), expected, what);
            ExpectColor(PixelAt(frame, 40, 40), background, what);
        }

        static TempVectorAssets* assets;

        SceneCleanGuard guard;
    };

    TempVectorAssets* BuiltImages::assets = nullptr;

    class LayerImageRender: public BuiltImages {};
    class VectorWidgetsRender: public BuiltImages {};
}

TEST_F(VectorWidgets, CustomListSelectsWithPooledVectorClones)
{
    auto list = mmake<ListProbe>();
    list->SetItemSample(mmake<Widget>());

    EXPECT_TRUE(list->GetSelectionDrawable()) << "sprites stay by default";
    EXPECT_TRUE(list->GetHoverDrawable());

    auto selection = Vector("red");
    list->SetSelectionDrawable(selection);
    list->SetHoverDrawable(Vector("green"));

    EXPECT_EQ(list->GetSelectionRectDrawable(), selection);
    EXPECT_FALSE(list->GetSelectionDrawable()) << "the sprite getter does not see a vector drawable";
    EXPECT_TRUE(ShowsVector(list->GetHoverRectDrawable(), Path("green")));
    EXPECT_FALSE(list->GetHoverDrawable());

    for (int i = 0; i < 7; i++)
        list->AddItem();

    for (int i = 0; i < 7; i++)
        list->SelectItemAt(i);

    ASSERT_EQ(list->mSelectedItems.Count(), 7) << "more than one pool step";
    for (auto& selected : list->mSelectedItems)
    {
        EXPECT_TRUE(ShowsVector(selected.selection, Path("red")));
        EXPECT_NE(selected.selection, selection);
    }

    EXPECT_NE(list->mSelectedItems[0].selection, list->mSelectedItems[1].selection);

    list->SetSelectionDrawable(Vector("green"));
    ASSERT_EQ(list->mSelectedItems.Count(), 7);
    for (auto& selected : list->mSelectedItems)
        EXPECT_TRUE(ShowsVector(selected.selection, Path("green"))) << "selected items take the new sample";

    for (auto& pooled : list->mSelectionSpritesPool)
        EXPECT_TRUE(ShowsVector(pooled, Path("green"))) << "clones of the old sample are dropped";

    list->ClearSelection();
    list->SelectItemAt(2);
    ASSERT_EQ(list->mSelectedItems.Count(), 1);
    EXPECT_TRUE(ShowsVector(list->mSelectedItems[0].selection, Path("green")));
}

TEST_F(VectorWidgets, CustomListCloneAndSerializationKeepVectorDrawables)
{
    auto list = mmake<CustomList>();
    list->SetItemSample(mmake<Widget>());
    list->SetSelectionDrawable(Vector("red"));
    list->SetHoverDrawable(Vector("green"));

    auto cloned = list->CloneAsRef<CustomList>();
    EXPECT_TRUE(ShowsVector(cloned->GetSelectionRectDrawable(), Path("red")));
    EXPECT_TRUE(ShowsVector(cloned->GetHoverRectDrawable(), Path("green")));
    EXPECT_NE(cloned->GetSelectionRectDrawable(), list->GetSelectionRectDrawable());

    auto loaded = SaveAndLoad(list);
    ASSERT_TRUE(loaded);
    EXPECT_TRUE(ShowsVector(loaded->GetSelectionRectDrawable(), Path("red")));
    EXPECT_TRUE(ShowsVector(loaded->GetHoverRectDrawable(), Path("green")));
}

TEST_F(VectorWidgets, StatesAnimateDrawableSetAfterThem)
{
    auto list = mmake<CustomList>();
    list->SetItemSample(mmake<Widget>());
    list->AddState("hover", AnimationClip::EaseInOut("mHoverDrawable/transparency", 0.0f, 1.0f, 0.1f));

    list->SetHoverDrawable(Vector("green"));

    list->SetStateForcible("hover", true);
    EXPECT_NEAR(list->GetHoverRectDrawable()->GetTransparency(), 1.0f, 0.01f);

    list->SetStateForcible("hover", false);
    EXPECT_NEAR(list->GetHoverRectDrawable()->GetTransparency(), 0.0f, 0.01f);

    auto cloned = list->CloneAsRef<CustomList>();
    cloned->SetStateForcible("hover", true);
    EXPECT_NEAR(cloned->GetHoverRectDrawable()->GetTransparency(), 1.0f, 0.01f);
    EXPECT_NEAR(list->GetHoverRectDrawable()->GetTransparency(), 0.0f, 0.01f);
}

TEST_F(VectorWidgets, StatesAnimateLayerDrawableSetAfterThem)
{
    auto widget = mmake<Widget>();
    widget->AddLayer("icon", Vector("red"));
    widget->AddState("turned", AnimationClip::EaseInOut("layer/icon/mDrawable/angle", 0.0f, 90.0f, 0.1f));
    widget->SetStateForcible("turned", true);

    auto replaced = Vector("green");
    widget->GetLayer("icon")->SetDrawable(replaced);

    widget->SetStateForcible("turned", false);
    EXPECT_NEAR(replaced->GetAngle(), 0.0f, 0.01f);

    widget->SetStateForcible("turned", true);
    EXPECT_NEAR(replaced->GetAngle(), 90.0f, 0.01f);

    auto parent = mmake<Widget>();
    parent->AddChild(widget);
    widget->name = "child";
    parent->AddState("turned", AnimationClip::EaseInOut("child/child/layer/icon/mDrawable/angle", 0.0f, 45.0f, 0.1f));
    parent->SetStateForcible("turned", true);

    auto replacedAgain = Vector("red");
    widget->GetLayer("icon")->SetDrawable(replacedAgain);
    parent->SetStateForcible("turned", false);
    EXPECT_NEAR(replacedAgain->GetAngle(), 0.0f, 0.01f) << "the state of the parent follows the new drawable";
}

TEST_F(VectorWidgets, ReplacedLayerDrawableKeepsStateTimeAndTransparency)
{
    auto widget = mmake<Widget>();
    widget->AddLayer("icon", Vector("red"));
    widget->AddState("turned", AnimationClip::EaseInOut("layer/icon/mDrawable/angle", 0.0f, 90.0f, 1.0f));
    widget->SetState("turned", true);
    widget->GetStateObject("turned")->GetAnimationPlayer()->SetRelTime(0.5f);

    float angle = widget->GetLayerDrawable<VectorSprite>("icon")->GetAngle();
    EXPECT_GT(angle, 1.0f);
    EXPECT_LT(angle, 89.0f);

    widget->GetLayer("icon")->transparency = 0.25f;

    auto back = widget->AddLayer("back", mmake<Sprite>());
    back->GetDrawable()->SetTransparency(0.1f);

    auto replaced = Vector("green");
    widget->GetLayer("icon")->SetDrawable(replaced);

    EXPECT_NEAR(replaced->GetTransparency(), 0.25f, 0.01f);
    EXPECT_NEAR(back->GetDrawable()->GetTransparency(), 0.1f, 0.01f) << "alpha set by code to another drawable stays";

    auto player = widget->GetStateObject("turned")->GetAnimationPlayer();
    EXPECT_NEAR(player->GetRelativeTime(), 0.5f, 0.01f) << "the running state is not thrown to its end";

    player->SetRelTime(0.75f);
    EXPECT_GT(replaced->GetAngle(), angle) << "the state goes on with the new drawable";
}

TEST_F(VectorWidgets, WidgetsSeeLayerDrawableReplacedAsideOfThem)
{
    auto button = mmake<Button>();
    button->AddLayer("icon", Vector("red"));
    EXPECT_TRUE(ShowsVector(button->GetIconDrawable(), Path("red")));

    auto replaced = Vector("green");
    button->GetLayer("icon")->SetDrawable(replaced);
    EXPECT_EQ(button->GetIconDrawable(), replaced);

    auto image = mmake<Image>();
    image->SetImageName(Path("red"));
    EXPECT_FALSE(image->GetImage());

    auto sprite = mmake<Sprite>();
    image->GetLayer("image")->SetDrawable(sprite);
    EXPECT_EQ(image->GetImage(), sprite);
    EXPECT_EQ(image->GetImageDrawable(), sprite);
    EXPECT_EQ(image->GetLayers().Count(), 1);

    auto captioned = mmake<Widget>();
    auto text = mmake<Text>();
    auto caption = captioned->AddLayer("caption", text);
    EXPECT_FALSE(caption->SetImage(Path("red"))) << "a drawable that is not an image stays";
    EXPECT_EQ(caption->GetDrawable(), text);
}

TEST_F(VectorWidgets, LongListTakesVectorDrawables)
{
    auto list = mmake<LongList>();
    EXPECT_TRUE(list->GetSelectionDrawable());
    EXPECT_TRUE(list->GetHoverDrawable());

    list->SetSelectionDrawable(Vector("red"));
    list->SetHoverDrawable(Vector("green"));
    EXPECT_FALSE(list->GetSelectionDrawable());
    EXPECT_FALSE(list->GetHoverDrawable());

    auto cloned = list->CloneAsRef<LongList>();
    EXPECT_TRUE(ShowsVector(cloned->GetSelectionRectDrawable(), Path("red")));
    EXPECT_TRUE(ShowsVector(cloned->GetHoverRectDrawable(), Path("green")));

    auto loaded = SaveAndLoad(list);
    ASSERT_TRUE(loaded);
    EXPECT_TRUE(ShowsVector(loaded->GetSelectionRectDrawable(), Path("red")));
    EXPECT_TRUE(ShowsVector(loaded->GetHoverRectDrawable(), Path("green")));
}

TEST_F(VectorWidgets, TreeTakesVectorDrawables)
{
    auto tree = mmake<Tree>();
    EXPECT_TRUE(tree->GetHoverDrawable());
    EXPECT_TRUE(tree->GetHighlightDrawable());
    EXPECT_FALSE(tree->GetZebraBackLineDrawable());

    tree->SetHoverDrawable(Vector("red"));
    tree->SetHighlightDrawable(Vector("green"));
    tree->SetZebraBackLine(Vector("red"));
    tree->SetHighlightAnimation(AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.1f));

    EXPECT_FALSE(tree->GetHoverDrawable());
    EXPECT_FALSE(tree->GetHighlightDrawable());
    EXPECT_FALSE(tree->GetZebraBackLine());

    auto check = [&](const Ref<Tree>& checked)
    {
        ASSERT_TRUE(checked);
        EXPECT_TRUE(ShowsVector(checked->GetHoverRectDrawable(), Path("red")));
        EXPECT_TRUE(ShowsVector(checked->GetHighlightRectDrawable(), Path("green")));
        EXPECT_TRUE(ShowsVector(checked->GetZebraBackLineDrawable(), Path("red")));
    };

    check(tree);
    check(tree->CloneAsRef<Tree>());
    check(SaveAndLoad(tree));

    tree->SetZebraBackLine(mmake<Sprite>());
    EXPECT_TRUE(tree->GetZebraBackLine());
}

TEST_F(VectorWidgets, ContextMenuAndMenuPanelSelectWithVectorDrawable)
{
    auto menu = mmake<ContextMenu>();
    EXPECT_TRUE(menu->GetSelectionDrawable());
    menu->SetSelectionDrawable(Vector("red"));
    EXPECT_FALSE(menu->GetSelectionDrawable());
    EXPECT_TRUE(ShowsVector(menu->GetSelectionRectDrawable(), Path("red")));
    EXPECT_TRUE(ShowsVector(menu->CloneAsRef<ContextMenu>()->GetSelectionRectDrawable(), Path("red")));

    auto loadedMenu = SaveAndLoad(menu);
    ASSERT_TRUE(loadedMenu);
    EXPECT_TRUE(ShowsVector(loadedMenu->GetSelectionRectDrawable(), Path("red")));

    auto panel = mmake<MenuPanel>();
    EXPECT_TRUE(panel->GetSelectionDrawable());
    panel->SetSelectionDrawable(Vector("green"));
    EXPECT_FALSE(panel->GetSelectionDrawable());
    EXPECT_TRUE(ShowsVector(panel->CloneAsRef<MenuPanel>()->GetSelectionRectDrawable(), Path("green")));

    auto loadedPanel = SaveAndLoad(panel);
    ASSERT_TRUE(loadedPanel);
    EXPECT_TRUE(ShowsVector(loadedPanel->GetSelectionRectDrawable(), Path("green")));
}

TEST_F(VectorWidgets, ContextMenuItemShowsVectorIconAndCheck)
{
    auto widget = mmake<ContextMenuItem>();
    widget->layout->minHeight = 20.0f;
    widget->AddLayer("icon", nullptr);
    widget->AddLayer("check", Vector("green"));

    auto item = mmake<ContextMenu::Item>("Vector", true, Function<void(bool)>(), "",
                                         AssetRef<VectorImageAsset>(Path("red")));
    ASSERT_TRUE(item->icon);
    EXPECT_EQ(item->icon->GetPath(), Path("red"));

    widget->Setup(item);

    auto iconLayer = widget->FindLayer("icon");
    ASSERT_EQ(iconLayer->GetChildren().Count(), 1);
    auto icon = iconLayer->GetChildren()[0]->GetDrawable();
    EXPECT_TRUE(ShowsVector(icon, Path("red")));

    auto check = widget->GetLayerDrawableBasedOn<IRectDrawable>("check");
    ASSERT_TRUE(check);
    EXPECT_TRUE(check->IsEnabled());

    widget->SetChecked(false);
    EXPECT_FALSE(check->IsEnabled()) << "a vector check mark follows the checked state";

    widget->Setup(mmake<ContextMenu::Item>("Other", Function<void()>(), "", AssetRef<VectorImageAsset>(Path("green"))));
    ASSERT_EQ(iconLayer->GetChildren().Count(), 1);
    EXPECT_TRUE(ShowsVector(iconLayer->GetChildren()[0]->GetDrawable(), Path("green")));

    DataDocument data;
    data = *item;

    auto loaded = mmake<ContextMenu::Item>();
    data.Get(*loaded);
    ASSERT_TRUE(loaded->icon);
    EXPECT_TRUE(DynamicCast<VectorImageAsset>(loaded->icon.GetRef()));
    EXPECT_EQ(loaded->icon->GetPath(), Path("red"));
}

TEST_F(VectorWidgets, ButtonIconIsAnyDrawable)
{
    auto button = mmake<Button>();
    auto icon = Vector("red");
    button->AddLayer("icon", icon);

    EXPECT_EQ(button->GetIconDrawable(), icon);
    EXPECT_FALSE(button->GetIcon()) << "the sprite getter does not see a vector icon";

    auto cloned = button->CloneAsRef<Button>();
    EXPECT_TRUE(ShowsVector(cloned->GetIconDrawable(), Path("red")));

    auto loaded = SaveAndLoad(button);
    ASSERT_TRUE(loaded);
    EXPECT_TRUE(ShowsVector(loaded->GetLayerDrawableBasedOn<IRectDrawable>("icon"), Path("red")));

    auto other = Vector("green");
    button->SetIcon(other);
    EXPECT_EQ(button->GetIconDrawable(), other);
    EXPECT_EQ(button->GetLayer("icon")->GetDrawable(), other) << "the icon is put into the layer";

    button->SetIconImage(Path("red"));
    EXPECT_EQ(button->GetIconDrawable(), other) << "the same kind of image keeps the drawable";
    EXPECT_TRUE(ShowsVector(button->GetIconDrawable(), Path("red")));

    auto sprite = mmake<Sprite>();
    button->icon = sprite;
    EXPECT_EQ(button->GetIcon(), sprite);
}

TEST_F(VectorWidgets, WindowIconIsAnyDrawable)
{
    auto window = mmake<o2::Window>();
    window->AddLayer("icon", nullptr);

    auto icon = Vector("red");
    window->SetIcon(icon);
    EXPECT_EQ(window->GetIconDrawable(), icon);
    EXPECT_FALSE(window->GetIcon());

    EXPECT_TRUE(ShowsVector(window->CloneAsRef<o2::Window>()->GetIconDrawable(), Path("red")));

    auto sprite = mmake<Sprite>();
    window->SetIcon(sprite);
    EXPECT_EQ(window->GetIcon(), sprite);
}

TEST_F(VectorWidgets, EditBoxCaretIsAnyDrawable)
{
    auto editBox = mmake<EditBox>();
    EXPECT_TRUE(editBox->GetCaretDrawable());

    auto caret = Vector("red");
    editBox->SetCaretDrawable(caret);
    EXPECT_EQ(editBox->GetCaretRectDrawable(), caret);
    EXPECT_FALSE(editBox->GetCaretDrawable());

    EXPECT_TRUE(ShowsVector(editBox->CloneAsRef<EditBox>()->GetCaretRectDrawable(), Path("red")));

    auto loaded = SaveAndLoad(editBox);
    ASSERT_TRUE(loaded);
    EXPECT_TRUE(ShowsVector(loaded->GetCaretRectDrawable(), Path("red")));
}

TEST_F(VectorWidgets, ImageWidgetTakesVectorImageByName)
{
    auto image = mmake<Image>();
    EXPECT_TRUE(image->GetImage()) << "an empty image widget holds a sprite";

    image->SetImageName(Path("red"));
    EXPECT_TRUE(ShowsVector(image->GetImageDrawable(), Path("red")));
    EXPECT_EQ(image->GetLayer("image")->GetDrawable(), image->GetImageDrawable());
    EXPECT_EQ(image->GetImageName(), Path("red"));
    EXPECT_FALSE(image->GetImage());
    EXPECT_FALSE(image->GetImageAsset());
    EXPECT_EQ(image->GetLayers().Count(), 1);

    auto drawable = image->GetImageDrawable();
    image->imageName = Path("green");
    EXPECT_EQ(image->GetImageDrawable(), drawable) << "the same kind of image keeps the drawable";
    EXPECT_EQ(image->GetImageName(), Path("green"));

    auto cloned = image->CloneAsRef<Image>();
    EXPECT_TRUE(ShowsVector(cloned->GetImageDrawable(), Path("green")));
    EXPECT_EQ(cloned->GetLayers().Count(), 1) << "a vector layer is not doubled by a sprite one";

    auto loaded = SaveAndLoad(image);
    ASSERT_TRUE(loaded);
    EXPECT_EQ(loaded->GetImageName(), Path("green"));
    EXPECT_TRUE(ShowsVector(loaded->GetImageDrawable(), Path("green")));
    EXPECT_EQ(loaded->GetLayers().Count(), 1);

    image->SetImageSource(AssetRef<VectorImageAsset>(Path("red")));
    EXPECT_EQ(image->GetImageName(), Path("red"));

    auto sprite = mmake<Sprite>();
    image->SetImage(sprite);
    EXPECT_EQ(image->GetImage(), sprite);
    EXPECT_TRUE(image->GetImageName().IsEmpty());
}

TEST_F(VectorWidgets, LayerDrawableIsFoundByBaseType)
{
    auto widget = mmake<Widget>();
    auto vector = Vector("red");
    auto back = widget->AddLayer("back", nullptr);
    back->AddChildLayer("icon", vector);
    widget->AddLayer("text", mmake<Text>());

    EXPECT_FALSE(widget->GetLayerDrawable<Sprite>("back/icon")) << "exact type as before";
    EXPECT_FALSE(widget->GetLayerDrawable<IRectDrawable>("back/icon"));
    EXPECT_EQ(widget->GetLayerDrawable<VectorSprite>("back/icon"), vector);

    EXPECT_EQ(widget->GetLayerDrawableBasedOn<IRectDrawable>("back/icon"), vector);
    EXPECT_EQ(widget->GetLayerDrawableBasedOn<VectorSprite>("back/icon"), vector);
    EXPECT_FALSE(widget->GetLayerDrawableBasedOn<Sprite>("back/icon"));
    EXPECT_FALSE(widget->GetLayerDrawableBasedOn<IRectDrawable>("back"));
    EXPECT_FALSE(widget->GetLayerDrawableBasedOn<IRectDrawable>("missing"));

    EXPECT_FALSE(widget->GetLayerDrawableByType<IRectDrawable>());
    EXPECT_TRUE(DynamicCast<Text>(widget->GetLayerDrawableByBaseType<IRectDrawable>())) << "layers go before children";
    EXPECT_EQ(widget->GetLayerDrawableByBaseType<VectorSprite>(), vector);
    EXPECT_FALSE(widget->GetLayerDrawableByBaseType<Sprite>());
}

TEST_F(VectorWidgets, LayerImageOfSameKindKeepsDrawable)
{
    auto widget = mmake<Widget>(ActorCreateMode::NotInScene);
    auto vector = Vector("red");
    vector->SetColor(Color4(10, 20, 30, 255));
    widget->AddLayer("icon", vector);

    *widget->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(40, 30));
    widget->UpdateTransform();

    EXPECT_EQ(widget->SetLayerImage("icon", Path("green")), vector);
    EXPECT_EQ(vector->GetImageName(), Path("green"));
    EXPECT_EQ(vector->GetColor(), Color4(10, 20, 30, 255));
    EXPECT_EQ(vector->GetRect(), RectF(-20, -15, 20, 15)) << "the layer keeps the drawable in its rectangle";

    EXPECT_FALSE(widget->SetLayerImage("missing", Path("green")));

    auto empty = widget->AddLayer("empty", nullptr);
    EXPECT_TRUE(ShowsVector(empty->SetImage(Path("red")), Path("red")));
}

TEST_F(VectorWidgets, OldFormatSpriteFieldsLoad)
{
    auto list = LoadOldFormat<CustomList>(OldSprite("mSelectionDrawable", 255) + ", " + OldSprite("mHoverDrawable", 0));
    ASSERT_TRUE(list);
    EXPECT_TRUE(IsOldSprite(list->GetSelectionDrawable(), 255));
    EXPECT_TRUE(IsOldSprite(list->GetHoverDrawable(), 0));
    EXPECT_EQ(list->GetSelectionRectDrawable(), Ref<IRectDrawable>(list->GetSelectionDrawable()));

    auto longList = LoadOldFormat<LongList>(OldSprite("mSelectionDrawable", 255) + ", " + OldSprite("mHoverDrawable", 0));
    ASSERT_TRUE(longList);
    EXPECT_TRUE(IsOldSprite(longList->GetSelectionDrawable(), 255));
    EXPECT_TRUE(IsOldSprite(longList->GetHoverDrawable(), 0));

    auto tree = LoadOldFormat<Tree>(OldSprite("mHoverDrawable", 0) + ", " + OldSprite("mHighlightSprite", 255) + ", " +
                                    OldSprite("mZebraBackLine", 13));
    ASSERT_TRUE(tree);
    EXPECT_TRUE(IsOldSprite(tree->GetHoverDrawable(), 0));
    EXPECT_TRUE(IsOldSprite(tree->GetHighlightDrawable(), 255));
    EXPECT_TRUE(IsOldSprite(tree->GetZebraBackLine(), 13));

    auto menu = LoadOldFormat<ContextMenu>(OldSprite("mSelectionDrawable", 0));
    ASSERT_TRUE(menu);
    EXPECT_TRUE(IsOldSprite(menu->GetSelectionDrawable(), 0));

    auto panel = LoadOldFormat<MenuPanel>(OldSprite("mSelectionDrawable", 0));
    ASSERT_TRUE(panel);
    EXPECT_TRUE(IsOldSprite(panel->GetSelectionDrawable(), 0));

    auto editBox = LoadOldFormat<EditBox>(OldSprite("mCaretDrawable", 255));
    ASSERT_TRUE(editBox);
    EXPECT_TRUE(IsOldSprite(editBox->GetCaretDrawable(), 255));
}

TEST_F(VectorWidgets, SpriteFieldsAreWrittenInTheOldFormat)
{
    auto list = mmake<CustomList>();
    list->SetItemSample(mmake<Widget>());
    list->GetSelectionDrawable()->SetColor(Color4(10, 20, 30, 255));

    DataDocument data;
    data = Ref<Widget>(list);

    auto field = data.FindMember("Value")->FindMember("mSelectionDrawable");
    ASSERT_TRUE(field);
    EXPECT_EQ((String)*field->FindMember("Type"), String("o2::Sprite"));
    EXPECT_EQ((int)(*field->FindMember("Value"))["mColor"]["g"], 20);

    DataDocument itemData;
    itemData.LoadFromData("{\"text\": \"Old\", \"icon\": {\"id\": \"00000000000000000000000000000000\", \"path\": \"" +
                          Path("red") + "\"}}");

    auto item = mmake<ContextMenu::Item>();
    itemData.Get(*item);
    EXPECT_EQ(item->text, WString("Old"));
    ASSERT_TRUE(item->icon) << "icon written as an asset reference loads into the generic reference";
    EXPECT_EQ(item->icon->GetPath(), Path("red"));
}

TEST_F(LayerImageRender, SetImageSwapsKindOfDrawable)
{
    auto widget = mmake<Widget>();
    auto layer = widget->AddLayer("image", Vector("green"));

    Ref<IRectDrawable> vector = layer->GetDrawable();
    vector->SetEnabled(false);
    EXPECT_EQ(layer->GetImage(), o2Assets.GetAssetRef(Path("green")));

    auto raster = layer->SetImage(RasterPath("red"));
    ASSERT_TRUE(DynamicCast<Sprite>(raster));
    EXPECT_EQ(layer->GetDrawable(), raster);
    EXPECT_NE(raster, vector);
    EXPECT_FALSE(raster->IsEnabled());
    EXPECT_EQ(layer->GetImage()->GetPath(), RasterPath("red"));

    EXPECT_EQ(layer->SetImage(RasterPath("red")), raster) << "the same kind of image keeps the drawable";
    ExpectDraws(raster, blue, "raster");

    auto back = layer->SetImage(AssetRef<Asset>(o2Assets.GetAssetRef(Path("red"))));
    EXPECT_TRUE(ShowsVector(back, Path("red")));
    ExpectDraws(back, red, "vector");

    EXPECT_TRUE(DynamicCast<Sprite>(widget->AddLayer("empty", nullptr)->SetImage(String())));
    EXPECT_FALSE(widget->AddLayer("text", mmake<Text>())->GetImage());
}

TEST_F(LayerImageRender, WidgetsTakeBothKindsOfImages)
{
    auto image = mmake<Image>();
    *image->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(40, 40));

    image->SetImageName(Path("red"));
    image->UpdateTransform();
    EXPECT_TRUE(ShowsVector(image->GetImageDrawable(), Path("red")));

    auto frame = DrawAndCapture([&]() { image->GetLayer("image")->Draw(); });
    ExpectColor(PixelAt(frame, 0, 0), red, "vector image widget");

    image->SetImageName(RasterPath("red"));
    image->UpdateTransform();
    EXPECT_EQ(Ref<IRectDrawable>(image->GetImage()), image->GetImageDrawable()) << "raster image gives a sprite back";
    EXPECT_TRUE(image->GetImageAsset());
    EXPECT_EQ(image->GetLayers().Count(), 1);
    EXPECT_EQ(image->GetImageDrawable()->GetRect(), RectF(-20, -20, 20, 20));

    frame = DrawAndCapture([&]() { image->GetLayer("image")->Draw(); });
    ExpectColor(PixelAt(frame, 0, 0), blue, "raster image widget");

    auto itemWidget = mmake<ContextMenuItem>();
    itemWidget->layout->minHeight = 20.0f;
    itemWidget->AddLayer("icon", nullptr);
    itemWidget->Setup(mmake<ContextMenu::Item>("Item", Function<void()>(), "", AssetRef<VectorImageAsset>(Path("red"))));

    auto iconLayer = itemWidget->FindLayer("icon");
    ASSERT_EQ(iconLayer->GetChildren().Count(), 1);
    EXPECT_TRUE(ShowsVector(iconLayer->GetChildren()[0]->GetDrawable(), Path("red")));

    itemWidget->Setup(mmake<ContextMenu::Item>("Item", Function<void()>(), "", AssetRef<ImageAsset>(RasterPath("red"))));
    ASSERT_EQ(iconLayer->GetChildren().Count(), 1);
    EXPECT_TRUE(DynamicCast<Sprite>(iconLayer->GetChildren()[0]->GetDrawable()));

    auto button = mmake<Button>();
    button->AddLayer("icon", mmake<VectorSprite>(Path("green")));
    button->SetIconImage(RasterPath("red"));
    EXPECT_TRUE(button->GetIcon());
    EXPECT_EQ(button->GetLayer("icon")->GetDrawable(), button->GetIconDrawable());
}

TEST_F(VectorWidgetsRender, DrawablesOfWidgetsDrawVectorImages)
{
    auto list = mmake<ListProbe>();
    list->SetItemSample(mmake<Widget>());
    list->SetSelectionDrawable(Vector("red"));
    list->SetHoverDrawable(Vector("green"));
    list->AddItem();
    list->SelectItemAt(0);
    ASSERT_EQ(list->mSelectedItems.Count(), 1);
    ExpectDraws(list->mSelectedItems[0].selection, red, "pooled selection of the list");
    ExpectDraws(list->CloneAsRef<CustomList>()->GetHoverRectDrawable(), green, "hover of the cloned list");

    auto longList = mmake<LongList>();
    longList->SetSelectionDrawable(Vector("red"));
    ExpectDraws(SaveAndLoad(longList)->GetSelectionRectDrawable(), red, "selection of the loaded long list");

    auto tree = mmake<Tree>();
    tree->SetHoverDrawable(Vector("red"));
    tree->SetHighlightDrawable(Vector("green"));
    tree->SetZebraBackLine(Vector("red"));
    auto clonedTree = tree->CloneAsRef<Tree>();
    ExpectDraws(clonedTree->GetHoverRectDrawable(), red, "tree hover");
    ExpectDraws(clonedTree->GetHighlightRectDrawable(), green, "tree highlight");
    ExpectDraws(clonedTree->GetZebraBackLineDrawable(), red, "tree zebra line");

    auto menu = mmake<ContextMenu>();
    menu->SetSelectionDrawable(Vector("green"));
    ExpectDraws(menu->CloneAsRef<ContextMenu>()->GetSelectionRectDrawable(), green, "context menu selection");

    auto panel = mmake<MenuPanel>();
    panel->SetSelectionDrawable(Vector("red"));
    ExpectDraws(panel->CloneAsRef<MenuPanel>()->GetSelectionRectDrawable(), red, "menu panel selection");

    auto editBox = mmake<EditBox>();
    editBox->SetCaretDrawable(Vector("green"));
    ExpectDraws(editBox->CloneAsRef<EditBox>()->GetCaretRectDrawable(), green, "edit box caret");
}

TEST_F(VectorWidgetsRender, LayersOfWidgetsDrawVectorImages)
{
    auto drawLayer = [&](const Ref<Widget>& widget, const String& path, const Color4& expected, const char* what)
    {
        *widget->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(40, 40));
        widget->UpdateTransform();

        auto layer = widget->GetLayer(path);
        ASSERT_TRUE(layer) << what;

        auto frame = DrawAndCapture([&]() { layer->Draw(); });
        ASSERT_TRUE(frame) << what;
        ExpectColor(PixelAt(frame, 0, 0), expected, what);
        ExpectColor(PixelAt(frame, 40, 40), background, what);
    };

    auto button = mmake<Button>();
    button->AddLayer("icon", Vector("green"));
    button->SetIconImage(Path("red"));
    drawLayer(button->CloneAsRef<Button>(), "icon", red, "button icon");

    auto window = mmake<o2::Window>();
    window->AddLayer("icon", nullptr);
    window->SetIcon(Vector("green"));
    drawLayer(window, "icon", green, "window icon");

    auto image = mmake<Image>();
    image->SetImageName(Path("red"));
    drawLayer(SaveAndLoad(image), "image", red, "image widget");

    auto itemWidget = mmake<ContextMenuItem>();
    itemWidget->AddLayer("icon", nullptr);
    itemWidget->AddLayer("check", Vector("green"));
    itemWidget->Setup(mmake<ContextMenu::Item>("Item", true, Function<void(bool)>(), "",
                                               AssetRef<VectorImageAsset>(Path("red"))));
    drawLayer(itemWidget, "check", green, "context menu check");

    itemWidget->GetLayer("icon/sprite")->layout = Layout::BothStretch();
    drawLayer(itemWidget, "icon/sprite", red, "context menu item icon");
}

TEST_F(VectorWidgetsRender, ListDrawsVectorSelectionOverSelectedItem)
{
    auto list = mmake<CustomList>();
    list->SetItemSample(mmake<Widget>());
    list->GetItemSample()->layout->minHeight = 20.0f;
    list->SetSelectionDrawable(Vector("red"));
    list->SetHoverDrawable(mmake<VectorSprite>());
    *list->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(100, 60));

    list->AddItem();
    list->AddItem();
    list->AddItem();
    list->SelectItemAt(1);

    TickAndUpdateLayout(2);

    auto frame = DrawAndCapture([&]() { list->Draw(); });
    ASSERT_TRUE(frame);
    ExpectColor(PixelAt(frame, 0, 0), red, "selected middle item");
    ExpectColor(PixelAt(frame, 0, 20), background, "not selected first item");
    ExpectColor(PixelAt(frame, 0, -20), background, "not selected last item");
}
