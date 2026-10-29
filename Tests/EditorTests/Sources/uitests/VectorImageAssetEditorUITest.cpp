#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Types/BinaryAsset.h"
#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2Editor/Properties/Basic/AssetProperty.h"
#include "o2Editor/Properties/Objects/Assets/VectorImageAssetViewer.h"
#include "o2Editor/UI/ImageSlicesEditorWidget.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Properties/PropertiesContext.h"
#include "o2Editor/Windows/AssetsWindow/AssetIcon.h"
#include "o2Editor/Windows/AssetsWindow/AssetsIconsScroll.h"

#include "../../../Sources/Support/Assets/VectorImageTestAssets.h"

using namespace o2;
using namespace o2::VectorImageTest;
using namespace Editor;

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

    struct IconsScrollAccess: public AssetsIconsScrollArea
    {
        static void Setup(AssetsIconsScrollArea& area, const Ref<Widget>& widget, Ref<AssetInfo>& info)
        {
            (area.*&IconsScrollAccess::SetupItemWidget)(widget, &info);
        }
    };
}

TEST(VectorImageAssetEditorUI, AssetRefFieldGetsAssetPicker)
{
    EXPECT_EQ(o2EditorProperties.GetFieldPropertyType(&TypeOf(AssetRef<VectorImageAsset>)), &TypeOf(AssetProperty));

    auto& type = dynamic_cast<const ObjectType&>(TypeOf(VectorSprite));
    ASSERT_NE(type.GetField("mImageAsset"), nullptr);

    PushEditorScopeOnStack scope;
    auto context = mmake<PropertiesContext>();
    auto layout = o2UI.CreateWidget<VerticalLayout>();
    auto field = o2EditorProperties.BuildField(layout, type, "mImageAsset", "", context);
    ASSERT_NE(field, nullptr);
    EXPECT_TRUE(DynamicCast<AssetProperty>(field));
}

TEST(VectorImageAssetEditorUI, AssetsWindowIconShowsVectorPreview)
{
    TempVectorAssets assets;
    assets.WriteSvg("preview", Svg(40, 20, "<rect x=\"2\" y=\"2\" width=\"36\" height=\"16\" fill=\"#ff0000\"/>"));
    assets.WriteMeta("preview", BorderI(4, 4, 4, 4), SpriteMode::Sliced);
    assets.WriteSvg("other", Svg(40, 20, "<rect x=\"2\" y=\"2\" width=\"36\" height=\"16\" fill=\"#0000ff\"/>"));
    assets.WriteMeta("other", BorderI(), SpriteMode::Default);
    assets.WriteSvg("broken", "<svg width=\"16\" height=\"16\"><rect x=\"0\" y=\"0\" width=");
    assets.WriteMeta("broken", BorderI(), SpriteMode::Default);
    assets.AttachSources();

    auto vectorInfo = assets.GetTree()->Find(assets.GetPath("preview"));
    ASSERT_TRUE(vectorInfo);
    auto otherInfo = assets.GetTree()->Find(assets.GetPath("other"));
    ASSERT_TRUE(otherInfo);
    auto brokenInfo = assets.GetTree()->Find(assets.GetPath("broken"));
    ASSERT_TRUE(brokenInfo);

    PushEditorScopeOnStack scope;
    auto area = o2UI.CreateWidget<AssetsIconsScrollArea>();
    auto icon = o2UI.CreateWidget<AssetIcon>();
    ASSERT_TRUE(area);
    ASSERT_TRUE(icon);

    IconsScrollAccess::Setup(*area, icon, vectorInfo);

    auto previewLayer = icon->FindLayer("vectorPreview");
    ASSERT_TRUE(previewLayer);
    EXPECT_TRUE(previewLayer->IsEnabled());
    EXPECT_FALSE(icon->layer["icon"]->IsEnabled()) << "the generic icon is replaced by the preview";

    auto sprite = DynamicCast<VectorSprite>(previewLayer->GetDrawable());
    ASSERT_TRUE(sprite);
    EXPECT_EQ(sprite->GetImageName(), assets.GetPath("preview"));
    EXPECT_EQ(sprite->GetMode(), SpriteMode::Default) << "the preview shows the whole image stretched";
    EXPECT_EQ(previewLayer->layout.GetOffsetMax() - previewLayer->layout.GetOffsetMin(), Vec2F(30, 15)) << "the preview keeps the image aspect";

    // Icons are pooled: the same widget shows another image next
    IconsScrollAccess::Setup(*area, icon, otherInfo);
    EXPECT_TRUE(previewLayer->IsEnabled());
    EXPECT_EQ(sprite->GetImageName(), assets.GetPath("other"));

    IconsScrollAccess::Setup(*area, icon, brokenInfo);
    EXPECT_FALSE(previewLayer->IsEnabled()) << "an invalid image has nothing to preview";
    EXPECT_TRUE(icon->layer["icon"]->IsEnabled());
    EXPECT_EQ(icon->layer["icon"]->GetImage()->GetPath(), VectorImageAsset::GetEditorIcon());

    IconsScrollAccess::Setup(*area, icon, vectorInfo);
    EXPECT_TRUE(previewLayer->IsEnabled());
    EXPECT_FALSE(icon->layer["icon"]->IsEnabled());

    auto binaryInfo = mmake<AssetInfo>(mmake<DefaultAssetMeta<BinaryAsset>>());
    binaryInfo->path = "some.bin";
    IconsScrollAccess::Setup(*area, icon, binaryInfo);

    EXPECT_FALSE(previewLayer->IsEnabled());
    EXPECT_TRUE(icon->layer["icon"]->IsEnabled());
}

// An svg selected in the assets shows its picture over the checker with the slices editor, the size and the mesh
TEST(VectorImageAssetEditorUI, PropertiesViewerShowsTheImage)
{
    {
        PushEditorScopeOnStack scope;
        auto chosen = o2EditorProperties.CreateObjectViewer(&TypeOf(VectorImageAsset), "");
        EXPECT_TRUE(DynamicCast<VectorImageAssetViewer>(chosen)) << "the properties window picks the vector image viewer";
        o2EditorProperties.FreeObjectViewer(chosen);
    }

    TempVectorAssets assets;
    assets.WriteSvg("viewed", Svg(40, 20, "<rect x=\"2\" y=\"2\" width=\"36\" height=\"16\" fill=\"#ff2000\"/>"));
    assets.WriteMeta("viewed", BorderI(4, 5, 6, 7), SpriteMode::Sliced);
    assets.AttachSources();

    AssetRef<VectorImageAsset> asset(assets.GetPath("viewed"));
    ASSERT_TRUE(asset);

    PushEditorScopeOnStack scope;
    auto viewer = mmake<VectorImageAssetViewer>();
    auto parent = o2UI.CreateWidget<VerticalLayout>();
    *parent->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(400.0f, 420.0f), Vec2F());
    viewer->CheckCreateSpoiler(parent);
    viewer->SetHeaderEnabled(false);
    viewer->Refresh({ { dynamic_cast<IObject*>(asset.Get()), nullptr } });
    parent->SetEnabledForcible(true);

    bool sized = false, meshed = false;
    for (auto& label : FindAllByType<Label>(parent))
    {
        sized = sized || label->GetText() == WString("Size: 40x20");
        meshed = meshed || label->GetText().Contains("triangles at 1x");
    }

    EXPECT_TRUE(sized);
    EXPECT_TRUE(meshed);

    auto slices = FindAllByType<ImageSlicesEditorWidget>(parent);
    ASSERT_EQ(slices.Count(), 1);
    EXPECT_EQ(slices[0]->GetImageSize(), Vec2F(40, 20));

    auto preview = DynamicCast<VectorSprite>(slices[0]->GetPreviewDrawable());
    ASSERT_TRUE(preview) << "the picture is drawn by a vector sprite";
    EXPECT_EQ(preview->GetImageAsset(), asset);
    EXPECT_EQ(preview->GetMode(), SpriteMode::Default) << "the whole image, the slices are shown by the lines";

    Ref<Bitmap> captured;
    for (int i = 0; i < 3; i++)
    {
        if (i == 2)
            o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { captured = bitmap; });

        parent->UpdateSelfTransform();
        parent->UpdateChildrenTransforms();
        o2Render.Begin();
        o2Render.Clear(Color4(20, 20, 20, 255));
        o2Render.SetCamera(Camera());
        parent->Draw();
        o2Render.SetCamera(Camera());
        o2Render.End();
    }

    ASSERT_TRUE(captured);

    Vec2I size = captured->GetSize();
    float scale = (float)size.x/(float)o2Render.GetResolution().x;
    Vec2F halfResolution = Vec2F(o2Render.GetResolution())*0.5f;
    RectF rect = slices[0]->layout->GetWorldRect();

    int red = 0;
    const UInt8* data = captured->GetData();
    for (int y = Math::Max((int)((rect.bottom + halfResolution.y)*scale), 0);
         y < Math::Min((int)((rect.top + halfResolution.y)*scale), size.y); y++)
    {
        for (int x = Math::Max((int)((rect.left + halfResolution.x)*scale), 0);
             x < Math::Min((int)((rect.right + halfResolution.x)*scale), size.x); x++)
        {
            const UInt8* pixel = data + ((size_t)y*size.x + x)*4;
            red += pixel[0] > 200 && pixel[1] < 80 && pixel[2] < 60 ? 1 : 0;
        }
    }

    EXPECT_GT(red, 500) << "the picture of the svg is drawn";
}
