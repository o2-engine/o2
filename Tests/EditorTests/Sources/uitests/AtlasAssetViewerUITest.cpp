#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/AtlasAsset.h"
#include "o2/EngineSettings.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2Editor/Properties/Objects/Assets/AtlasAssetViewer.h"
#include "o2Editor/UI/TexturePreview.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    template<typename _widget_type>
    Ref<_widget_type> FindFirstByType(const Ref<Widget>& widget)
    {
        if (auto typed = DynamicCast<_widget_type>(widget))
            return typed;

        for (auto& child : widget->GetChildWidgets())
        {
            if (auto found = FindFirstByType<_widget_type>(child))
                return found;
        }

        return nullptr;
    }

    // Builds the atlas inspector for the asset, draws it and counts colored (non-grey) pixels
    // inside the preview rect: the checked background alone yields zero
    int DrawViewerAndCountPreviewContent(const AssetRef<AtlasAsset>& atlas)
    {
        auto viewer = mmake<AtlasAssetViewer>();
        auto parent = o2UI.CreateWidget<VerticalLayout>();
        *parent->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(500.0f, 420.0f), Vec2F());
        viewer->CheckCreateSpoiler(parent);
        viewer->SetHeaderEnabled(false); // build the properties immediately, like the expanded inspector does
        viewer->Refresh({ { dynamic_cast<IObject*>(const_cast<AtlasAsset*>(atlas.Get())), nullptr } });

        parent->SetEnabledForcible(true);

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

        if (!captured)
            return -1;

        auto preview = FindFirstByType<TexturePreview>(parent);
        if (!preview)
            return -2;

        Vec2I capSize = captured->GetSize();
        float scale = (float)capSize.x / (float)o2Render.GetResolution().x;
        RectF worldRect = preview->layout->GetWorldRect();
        Vec2F halfRes = Vec2F(o2Render.GetResolution()) * 0.5f;
        RectI pixelRect(
            (int)((worldRect.left + halfRes.x) * scale) + 4,
            (int)((worldRect.bottom + halfRes.y) * scale) + 4,
            (int)((worldRect.right + halfRes.x) * scale) - 4,
            (int)((worldRect.top + halfRes.y) * scale) - 4);

        const UInt8* data = captured->GetData();
        int content = 0;
        for (int y = Math::Max(pixelRect.bottom, 0); y < Math::Min(pixelRect.top, capSize.y); y += 2)
        {
            for (int x = Math::Max(pixelRect.left, 0); x < Math::Min(pixelRect.right, capSize.x); x += 2)
            {
                const UInt8* p = data + (y*capSize.x + x)*4;
                bool grey = Math::Abs((int)p[0] - (int)p[1]) < 12 && Math::Abs((int)p[1] - (int)p[2]) < 12;
                if (!grey)
                    content++;
            }
        }

        return content;
    }
}

// The atlas inspector shows the first page texture on the checked background, like images do
TEST(AtlasAssetViewerUI, ShowsPagePreviewForBuiltAtlas)
{
    SceneCleanGuard guard;

    // The editor's own atlas always exists and has pages with the whole UI packed in
    AssetRef<AtlasAsset> atlas(GetBasicAtlasPath());
    ASSERT_TRUE(atlas.IsValid());
    ASSERT_FALSE(atlas->GetPages().IsEmpty());

    auto viewer = mmake<AtlasAssetViewer>();
    auto parent = o2UI.CreateWidget<VerticalLayout>();
    viewer->CheckCreateSpoiler(parent);
    viewer->SetHeaderEnabled(false); // build the properties immediately, like the expanded inspector does
    viewer->Refresh({ { dynamic_cast<IObject*>(atlas.Get()), nullptr } });

    auto preview = FindFirstByType<TexturePreview>(viewer->GetSpoiler());
    ASSERT_NE(preview, nullptr) << "atlas inspector must contain the texture preview";
    EXPECT_TRUE(preview->IsEnabled());
}

// Regression: the preview must actually rasterize the page texture, not only the checked
// background (mesh vertices built through Vertex::Set used to keep garbage z and vanish)
TEST(AtlasAssetViewerUI, PagePreviewRasterizesEditorAtlas)
{
    SceneCleanGuard guard;

    AssetRef<AtlasAsset> atlas(GetBasicAtlasPath());
    ASSERT_TRUE(atlas.IsValid());
    ASSERT_FALSE(atlas->GetPages().IsEmpty());

    EXPECT_GT(DrawViewerAndCountPreviewContent(atlas), 200);
}

// Same through a game atlas with compressed pages, when the game assets tree is reachable
TEST(AtlasAssetViewerUI, PagePreviewRasterizesGameAtlas)
{
    SceneCleanGuard guard;

    AssetRef<AtlasAsset> atlas("Basic.atlas");
    if (!atlas.IsValid() || atlas->GetPages().IsEmpty())
        GTEST_SKIP() << "game atlas is not reachable from the tests assets tree";

    EXPECT_GT(DrawViewerAndCountPreviewContent(atlas), 200);
}
