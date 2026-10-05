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
#include "o2Editor/Properties/Basic/AssetProperty.h"
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
