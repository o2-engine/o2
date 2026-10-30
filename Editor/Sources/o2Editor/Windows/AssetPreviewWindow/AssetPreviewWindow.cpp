#include "o2Editor/stdafx.h"
#include "AssetPreviewWindow.h"

#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/AtlasAsset.h"
#include "o2/Assets/Types/FontAsset.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Assets/Types/Mesh3DAsset.h"
#include "o2/Assets/Types/SkinnedModelAsset.h"
#include "o2/Assets/Types/SoundAsset.h"
#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Assets/Types/VideoAsset.h"
#include "o2Editor/UI/Preview/MediaPreview.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/VectorRasterCache.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UI/Preview/FontSampleView.h"
#include "o2Editor/UI/Preview/MediaPlayerView.h"
#include "o2Editor/UI/Preview/ModelView.h"
#include "o2Editor/UI/Preview/PreviewToolbar.h"
#include "o2Editor/UI/Preview/TextureInspector.h"
#include "o2Editor/UI/Preview/VectorImageView.h"
#include "o2Editor/UIRoot.h"

DECLARE_SINGLETON(Editor::AssetPreviewWindow);

namespace Editor
{
    namespace
    {
        const float infoHeight = 18.0f;
    }

    AssetPreviewWindow::AssetPreviewWindow(RefCounter* refCounter):
        Singleton<AssetPreviewWindow>(refCounter)
    {
        InitializeWindow();
    }

    AssetPreviewWindow::~AssetPreviewWindow()
    {
        // The window goes after the assets when the editor closes: the players stop themselves, the transport bars stay
        if (mInstance == this)
            mInstance = nullptr;
    }

    void AssetPreviewWindow::Show(const AssetRef<Asset>& asset, const String& mode /*= ""*/)
    {
        PushEditorScopeOnStack scope;

        if (!IsSingletonInitialzed())
            InitializeSingleton();

        auto& self = Instance();
        self.SetAsset(asset);
        if (!mode.IsEmpty())
            self.SetMode(mode);

        self.mWindow->Show();
    }

    Ref<Button> AssetPreviewWindow::CreateOpenButton(const Function<AssetRef<Asset>()>& getAsset,
                                                     const WString& caption /*= "Open preview"*/, const String& mode /*= ""*/)
    {
        auto button = o2UI.CreateButton(caption, [getAsset, mode]() {
            auto asset = getAsset();
            if (asset)
                Show(asset, mode);
        });

        button->name = (String)caption;
        button->layout->minHeight = 22.0f;
        return button;
    }

    void AssetPreviewWindow::InitializeWindow()
    {
        PushEditorScopeOnStack scope;

        mWindow = DynamicCast<o2::Window>(EditorUIRoot.AddWidget(o2UI.CreateWindow("Preview")));
        mWindow->name = "asset preview window";
        mWindow->SetClippingLayout(Layout::BothStretch(-1, -2, 0, 19));
        mWindow->SetViewLayout(Layout::BothStretch(-2, -2, 0, 20));
        mWindow->SetIcon(mmake<VectorSprite>("ui/UI4_image_component.svg"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 1)));
        mWindow->onClosed = [this]() { StopPlayback(); };

        mToolbar = PreviewToolbar::Create();
        mWindow->AddChild(mToolbar);

        mModesList = PreviewToolbar::AddDropDown(mToolbar, {}, 130.0f, [this](int position) {
            if (position >= 0 && position < mModes.Count() && mModes[position] != mMode)
                SetMode(mModes[position]);
        });
        mModesList->name = "modes";

        mAnimations = PreviewToolbar::AddDropDown(mToolbar, {}, 170.0f, [this](int position) {
            mModelView->SetAnimation(position - 1);
            mModelView->SetPlaying(position > 0);
            mAnimationPlay->caption = position > 0 ? "Pause" : "Play";
        });
        mAnimations->name = "animations";

        mAnimationPlay = PreviewToolbar::AddButton(mToolbar, "Play", 60.0f, [this]() {
            mModelView->SetPlaying(!mModelView->IsPlaying());
            mAnimationPlay->caption = mModelView->IsPlaying() ? "Pause" : "Play";
        });
        mAnimationPlay->name = "animation play";

        mFit = PreviewToolbar::AddButton(mToolbar, "Fit", 36.0f, [this]() {
            if (mContent == mVectorImageView)
                mVectorImageView->FitToView();
            else if (mContent == mModelView)
                mModelView->FitToView();
        });

        auto spacer = PreviewToolbar::AddLabel(mToolbar, "");

        mInfo = o2UI.CreateLabel("");
        mInfo->name = "info";
        mInfo->horAlign = HorAlign::Left;
        mInfo->horOverflow = Label::HorOverflow::Cut;
        *mInfo->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 4, 4, infoHeight, 0);
        mWindow->AddChild(mInfo);

        mContentArea = mmake<Widget>();
        mContentArea->name = "content";
        *mContentArea->layout = WidgetLayout::BothStretch(0, infoHeight, 0, PreviewToolbar::height + 2);
        mWindow->AddChild(mContentArea);

        auto addContent = [&](const Ref<Widget>& widget, const String& name) {
            widget->name = name;
            *widget->layout = WidgetLayout::BothStretch();
            mContentArea->AddChild(widget);
            widget->SetEnabledForcible(false);
        };

        mTextureInspector = mmake<TextureInspector>();
        addContent(mTextureInspector, "texture");
        mTextureInspector->PlaceControlsInto(mToolbar, mToolbar->GetChildWidgets().IndexOf(Ref<Widget>(spacer)));

        mVectorImageView = mmake<VectorImageView>();
        addContent(mVectorImageView, "vector image");

        mFontSampleView = mmake<FontSampleView>();
        addContent(mFontSampleView, "font");

        mModelView = mmake<ModelView>();
        addContent(mModelView, "model");

        mVideoView = mmake<VideoView>();
        addContent(mVideoView, "video");

        mSoundPlayerView = mmake<SoundPlayerView>();
        mSoundPlayerView->SetLarge(true);
        addContent(mSoundPlayerView, "sound");
        *mSoundPlayerView->layout = WidgetLayout(Vec2F(0, 0.5f), Vec2F(1, 0.5f), Vec2F(24, -130), Vec2F(-24, 130));

        mNoPreview = o2UI.CreateLabel("No preview for this asset");
        addContent(mNoPreview, "no preview");

        mWindow->Hide(true);
        mWindow->layout->size2D = Vec2F(1000, 760);
    }

    void AssetPreviewWindow::SetAsset(const AssetRef<Asset>& asset)
    {
        PushEditorScopeOnStack scope;
        StopPlayback();

        mAsset = asset;
        mModes.Clear();

        auto ref = asset ? asset.GetRef() : Ref<Asset>();
        if (DynamicCast<ImageAsset>(ref))
            mModes = { "Image", "Atlas page" };
        else if (auto atlas = DynamicCast<AtlasAsset>(ref))
        {
            for (int i = 0; i < atlas->GetPages().Count(); i++)
                mModes.Add("Page " + (String)(i + 1));
        }
        else if (DynamicCast<VectorImageAsset>(ref))
            mModes = { "Image", "Mesh", "Rasters" };
        else if (DynamicCast<FontAsset>(ref))
            mModes = { "Text", "Atlas" };
        else if (DynamicCast<Mesh3DAsset>(ref) || DynamicCast<SkinnedModelAsset>(ref))
            mModes = { "Shaded", "Wireframe", "Shaded wireframe" };
        else if (DynamicCast<VideoAsset>(ref))
            mModes = { "Video" };
        else if (DynamicCast<SoundAsset>(ref))
            mModes = { "Sound" };

        if (mModes.IsEmpty())
            mModes = { "None" };

        mModesList->RemoveAllItems();
        for (auto& mode : mModes)
            mModesList->AddItem(mode);

        auto skinned = DynamicCast<SkinnedModelAsset>(ref);
        mAnimations->RemoveAllItems();
        mAnimations->AddItem("Bind pose");
        if (skinned)
        {
            for (auto& clip : skinned->GetModelData().animations)
                mAnimations->AddItem(clip.name.IsEmpty() ? WString("(no name)") : WString(clip.name));
        }

        mAnimations->SetEnabledForcible(skinned != nullptr);
        mAnimationPlay->SetEnabledForcible(skinned != nullptr);

        if (DynamicCast<Mesh3DAsset>(ref))
            mModelView->SetMesh(DynamicCast<Mesh3DAsset>(ref)->GetMeshData());
        else if (skinned)
            mModelView->SetSkinnedModel(AssetRef<SkinnedModelAsset>(skinned));

        mAnimations->SelectItemAt(0);
        mAnimationPlay->caption = "Play";

        mWindow->caption = "Preview: " + (asset ? asset->GetPath() : String());

        mMode = "";
        SetMode(mModes[0]);
    }

    const AssetRef<Asset>& AssetPreviewWindow::GetAsset() const
    {
        return mAsset;
    }

    const Vector<String>& AssetPreviewWindow::GetModes() const
    {
        return mModes;
    }

    void AssetPreviewWindow::SetMode(const String& mode)
    {
        int position = mModes.IndexOf(mode);
        if (position < 0)
            return;

        mMode = mode;
        if (mModesList->GetSelectedItemPosition() != position)
            mModesList->SelectItemAt(position);

        ApplyMode();
    }

    const String& AssetPreviewWindow::GetMode() const
    {
        return mMode;
    }

    const Ref<Widget>& AssetPreviewWindow::GetContent() const
    {
        return mContent;
    }

    WString AssetPreviewWindow::GetInfoText() const
    {
        return mInfo->GetText();
    }

    const Ref<o2::Window>& AssetPreviewWindow::GetWindow() const
    {
        return mWindow;
    }

    const Ref<TextureInspector>& AssetPreviewWindow::GetTextureInspector() const
    {
        return mTextureInspector;
    }

    const Ref<VectorImageView>& AssetPreviewWindow::GetVectorImageView() const
    {
        return mVectorImageView;
    }

    const Ref<FontSampleView>& AssetPreviewWindow::GetFontSampleView() const
    {
        return mFontSampleView;
    }

    const Ref<ModelView>& AssetPreviewWindow::GetModelView() const
    {
        return mModelView;
    }

    const Ref<VideoView>& AssetPreviewWindow::GetVideoView() const
    {
        return mVideoView;
    }

    const Ref<SoundPlayerView>& AssetPreviewWindow::GetSoundPlayerView() const
    {
        return mSoundPlayerView;
    }

    void AssetPreviewWindow::ShowContent(const Ref<Widget>& content)
    {
        for (auto& child : mContentArea->GetChildWidgets())
            child->SetEnabledForcible(child == content);

        mTextureInspector->GetControls()->SetEnabledForcible(content == mTextureInspector);
        mFit->SetEnabledForcible(content == mVectorImageView || content == mModelView);

        // A content laid out while it was hidden keeps the old size of its view
        content->SetLayoutDirty();
        mContent = content;
    }

    void AssetPreviewWindow::StopPlayback()
    {
        if (mSoundPlayerView)
            mSoundPlayerView->Stop();

        if (mVideoView)
            mVideoView->Pause();

        if (mModelView)
            mModelView->SetPlaying(false);
    }

    void AssetPreviewWindow::ApplyMode()
    {
        auto ref = mAsset ? mAsset.GetRef() : Ref<Asset>();
        mInfo->SetText("");
        mTextureInspector->SetNote("");
        mTextureInspector->SetBackground(TextureView::Background::Checker);

        if (DynamicCast<ImageAsset>(ref))
            ApplyImageMode();
        else if (DynamicCast<AtlasAsset>(ref))
            ApplyAtlasMode();
        else if (DynamicCast<VectorImageAsset>(ref))
            ApplyVectorImageMode();
        else if (DynamicCast<FontAsset>(ref))
            ApplyFontMode();
        else if (DynamicCast<Mesh3DAsset>(ref) || DynamicCast<SkinnedModelAsset>(ref))
            ApplyModelMode();
        else if (auto video = DynamicCast<VideoAsset>(ref))
        {
            mVideoView->SetVideo(AssetRef<VideoAsset>(video));
            mInfo->SetText(MediaPreview::GetVideoInfo(*video));
            ShowContent(mVideoView);
        }
        else if (auto sound = DynamicCast<SoundAsset>(ref))
        {
            mSoundPlayerView->SetSound(AssetRef<SoundAsset>(sound));
            mInfo->SetText(MediaPreview::GetSoundInfo(*sound));
            ShowContent(mSoundPlayerView);
        }
        else
            ShowContent(mNoPreview);

        // A texture has its line of the size and the pixel under the cursor, the info goes first in it
        bool texture = mContent == mTextureInspector;
        if (texture && mTextureInspector->GetNote().IsEmpty())
            mTextureInspector->SetNote(mInfo->GetText());

        mInfo->SetEnabledForcible(!texture);
        *mContentArea->layout = WidgetLayout::BothStretch(0, texture ? 0.0f : infoHeight, 0, PreviewToolbar::height + 2);
    }

    void AssetPreviewWindow::ApplyImageMode()
    {
        auto image = DynamicCast<ImageAsset>(mAsset.GetRef());
        auto sprite = mmake<Sprite>(AssetRef<ImageAsset>(image));
        TextureRef texture = sprite->GetTexture();
        RectI rect = sprite->GetTextureSrcRect();

        if (mMode == "Atlas page")
        {
            mTextureInspector->SetTexture(texture);
            mTextureInspector->SetHighlights({ rect });
        }
        else
            mTextureInspector->SetTexture(texture, rect);

        Vec2F size = image->GetSize();
        String atlas = o2Assets.GetAssetPath(image->GetAtlasUID());
        mInfo->SetText(String::Format("Image %ix%i", (int)size.x, (int)size.y) +
                       (atlas.IsEmpty() ? String() : ", atlas " + atlas + " page " + (String)(int)(image->GetAtlasPage() + 1)));
        ShowContent(mTextureInspector);
    }

    void AssetPreviewWindow::ApplyAtlasMode()
    {
        auto atlas = DynamicCast<AtlasAsset>(mAsset.GetRef());
        int page = mModes.IndexOf(mMode);
        auto& pages = atlas->GetPages();
        if (page < 0 || page >= pages.Count())
        {
            ShowContent(mNoPreview);
            return;
        }

        Vector<RectI> rects;
        for (auto& kv : pages[page].ImagesRects())
            rects.Add(kv.second);

        mTextureInspector->SetTexture(pages[page].GetTexture());
        mTextureInspector->SetHighlights(rects);
        mInfo->SetText((String)rects.Count() + " images on the page, " + (String)atlas->GetImages().Count() + " in the atlas");
        ShowContent(mTextureInspector);
    }

    void AssetPreviewWindow::ApplyVectorImageMode()
    {
        auto image = DynamicCast<VectorImageAsset>(mAsset.GetRef());
        Vec2F size = image->GetSize();

        if (mMode != "Rasters")
        {
            if (mVectorImageView->GetImage() != AssetRef<VectorImageAsset>(image))
                mVectorImageView->SetImage(AssetRef<VectorImageAsset>(image));

            mVectorImageView->SetMode(mMode == "Mesh" ? VectorImageView::Mode::Mesh : VectorImageView::Mode::Image);
            String drawing = image->IsRasterized() ? "rasterized" : (image->IsMeshBaked() ? "baked mesh" : "tessellated");
            mInfo->SetText(String::Format("Image %gx%g, %i shapes, ", size.x, size.y, (int)image->GetImage().shapes.Count()) + drawing +
                           (mMode == "Mesh" ? ": the triangles of the zoom" : ""));
            ShowContent(mVectorImageView);
            return;
        }

        // The rasters live in the pages of the cache while the sprites of the image are drawn
        Vector<Ref<VectorRasterCache::Raster>> rasters;
        if (VectorRasterCache::IsSingletonInitialzed())
        {
            for (auto& raster : o2VectorRasters.GetRasters())
            {
                if (raster->IsReady() && raster->GetImageVersion() == image->GetVersion())
                    rasters.Add(raster);
            }
        }

        if (rasters.IsEmpty())
        {
            mTextureInspector->SetTexture(TextureRef());
            mTextureInspector->SetNote(image->IsRasterized() ? "No rasters: the image is not drawn now" :
                                       "No rasters: the image is not rasterized, see Rasterized in its meta");
            mInfo->SetText("0 rasters");
            ShowContent(mTextureInspector);
            return;
        }

        int page = rasters[0]->GetPageIndex();
        TextureRef texture = o2VectorRasters.GetPageTexture(page);
        int height = texture->GetSize().y;

        // A cell counts its rows from the bottom of the page, a source rectangle from its top
        Vector<RectI> cells;
        for (auto& raster : rasters)
        {
            if (raster->GetPageIndex() != page)
                continue;

            const RectI& cell = raster->GetCell();
            int bottom = Math::Min(cell.top, cell.bottom), top = Math::Max(cell.top, cell.bottom);
            cells.Add(RectI(cell.left, height - top, cell.right, height - bottom));
        }

        mTextureInspector->SetTexture(texture);
        mTextureInspector->SetHighlights(cells);
        mTextureInspector->SetNote(TextureView::GetTextureName(texture));
        mInfo->SetText((String)rasters.Count() + " rasters of the image, " + (String)cells.Count() + " on this page");
        ShowContent(mTextureInspector);
    }

    void AssetPreviewWindow::ApplyFontMode()
    {
        auto font = DynamicCast<FontAsset>(mAsset.GetRef())->GetFont();
        if (!font)
        {
            ShowContent(mNoPreview);
            return;
        }

        if (mMode == "Atlas")
        {
            mTextureInspector->SetTexture(font->GetTexture(), font->GetTextureSrcRect());
            mTextureInspector->SetBackground(TextureView::Background::Black);
            mTextureInspector->SetNote(TextureView::GetTextureName(font->GetTexture()));
            mInfo->SetText("The glyphs drawn so far, the atlas grows with them");
            ShowContent(mTextureInspector);
            return;
        }

        mFontSampleView->SetFont(font);
        mInfo->SetText("Sizes 10 to 48");
        ShowContent(mFontSampleView);
    }

    void AssetPreviewWindow::ApplyModelMode()
    {
        if (mMode == "Wireframe")
            mModelView->SetDrawMode(ModelView::DrawMode::Wireframe);
        else if (mMode == "Shaded wireframe")
            mModelView->SetDrawMode(ModelView::DrawMode::ShadedWireframe);
        else
            mModelView->SetDrawMode(ModelView::DrawMode::Shaded);

        String info = (String)(int)mModelView->GetVerticesCount() + " vertices, " + (String)(int)mModelView->GetTrianglesCount() +
            " triangles";

        if (auto skinned = DynamicCast<SkinnedModelAsset>(mAsset.GetRef()))
        {
            auto& model = skinned->GetModelData();
            info += ", " + (String)model.joints.Count() + " joints, " + (String)model.animations.Count() + " animations";
        }

        mInfo->SetText(info);
        ShowContent(mModelView);
    }
}
