#include "o2Editor/stdafx.h"
#include "AssetPreviewIcon.h"

#include "o2/Assets/Types/AtlasAsset.h"
#include "o2/Assets/Types/FontAsset.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Assets/Types/Mesh3DAsset.h"
#include "o2/Assets/Types/SkinnedModelAsset.h"
#include "o2/Assets/Types/SoundAsset.h"
#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Assets/Types/VideoAsset.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Sound/SoundPlayer.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UI/Preview/ModelView.h"
#include "o2Editor/UI/Style/EditorTheme.h"
#include "o2Editor/Utils/CommonTextures.h"
#include "o2Editor/UI/Preview/MediaPreview.h"
#include "o2Editor/Windows/AssetPreviewWindow/AssetPreviewWindow.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

namespace Editor
{
    AssetPreviewIcon::AssetPreviewIcon(RefCounter* refCounter):
        Widget(refCounter)
    {}

    AssetPreviewIcon::AssetPreviewIcon(RefCounter* refCounter, const AssetPreviewIcon& other):
        Widget(refCounter, other)
    {
        SetAsset(other.mAsset);
    }

    AssetPreviewIcon::~AssetPreviewIcon()
    {
        if (mSoundPlayer)
            mSoundPlayer->Stop();
    }

    AssetPreviewIcon& AssetPreviewIcon::operator=(const AssetPreviewIcon& other)
    {
        Widget::operator=(other);
        SetAsset(other.mAsset);
        return *this;
    }

    void AssetPreviewIcon::SetAsset(const AssetRef<Asset>& asset)
    {
        if (asset == mAsset && (mKind != Kind::None || !asset))
            return;

        PushEditorScopeOnStack scope;

        Clear();
        mAsset = asset;

        auto ref = asset ? asset.GetRef() : Ref<Asset>();
        if (!ref)
            return;

        auto useChecker = [&]() {
            CommonTextures::Initialize();
            mChecker = mmake<Sprite>(CommonTextures::checkedBackground);
            mChecker->SetMode(SpriteMode::Tiled);
        };

        if (auto image = DynamicCast<ImageAsset>(ref))
        {
            mKind = Kind::Image;
            auto sprite = mmake<Sprite>();
            sprite->SetImageAsset(AssetRef<ImageAsset>(image));
            mPreview = sprite;
            mPreviewSize = Vec2F(image->width, image->height);
            useChecker();
        }
        else if (auto vectorImage = DynamicCast<VectorImageAsset>(ref))
        {
            mKind = Kind::VectorImage;
            auto sprite = mmake<VectorSprite>();
            sprite->SetImageAsset(AssetRef<VectorImageAsset>(vectorImage));
            sprite->SetMode(SpriteMode::Default);
            mPreview = sprite;
            mPreviewSize = Vec2F(vectorImage->width, vectorImage->height);
            useChecker();
        }
        else if (auto atlas = DynamicCast<AtlasAsset>(ref))
        {
            auto& pages = atlas->GetPages();
            if (!pages.IsEmpty() && pages[0].GetTexture())
            {
                mKind = Kind::Atlas;
                auto texture = pages[0].GetTexture();
                auto sprite = mmake<Sprite>();
                sprite->SetTexture(texture);
                sprite->SetTextureSrcRect(RectI(Vec2I(), texture->GetSize()));
                mPreview = sprite;
                mPreviewSize = (Vec2F)texture->GetSize();
                useChecker();
            }
        }
        else if (auto font = DynamicCast<FontAsset>(ref))
        {
            mKind = Kind::Font;
            auto text = mmake<Text>();
            text->SetFontAsset(AssetRef<FontAsset>(font));
            text->SetHeight(10);
            text->SetText("Aa");
            text->SetHorAlign(HorAlign::Middle);
            text->SetVerAlign(VerAlign::Middle);
            text->SetColor(EditorTheme::Get().text);
            mPreview = text;
        }
        else if (DynamicCast<Mesh3DAsset>(ref) || DynamicCast<SkinnedModelAsset>(ref))
        {
            mKind = Kind::Model;
            mModelView = mmake<ModelView>();
            mModelView->name = "model";
            mModelView->SetBackColor(Color4(0, 0, 0, 0));
            mModelView->SetInteractable(false);
            *mModelView->layout = WidgetLayout::BothStretch();

            if (auto mesh = DynamicCast<Mesh3DAsset>(ref))
                mModelView->SetMesh(mesh->GetMeshData());
            else
                mModelView->SetSkinnedModel(AssetRef<SkinnedModelAsset>(DynamicCast<SkinnedModelAsset>(ref)));

            AddChild(mModelView);
            mModelView->SetEnabledForcible(true);
        }
        else if (auto sound = DynamicCast<SoundAsset>(ref))
        {
            mKind = Kind::Sound;
            mSoundPlayer = mmake<SoundPlayer>();
            mSoundPlayer->SetScrubPreview(false);
            mSoundPlayer->SetSound(AssetRef<SoundAsset>(sound));

            auto& theme = PipelineTheme::Get();
            auto circle = mmake<PipelineRoundedRect>();
            circle->roundBottom = true;
            circle->color = theme.accent;
            mPlayCircle = circle;

            mPlayGlyph = mmake<VectorSprite>("ui/pipeline/al_play.svg");
            mPlayGlyph->color = theme.accentInk;
        }
        else if (auto video = DynamicCast<VideoAsset>(ref))
        {
            if (auto frame = MediaPreview::GetVideoThumbnail(AssetRef<VideoAsset>(video)))
            {
                mKind = Kind::Video;
                auto sprite = mmake<Sprite>();
                sprite->SetTexture(frame);
                sprite->SetTextureSrcRect(RectI(Vec2I(), frame->GetSize()));
                mPreview = sprite;
                mPreviewSize = (Vec2F)frame->GetSize();
            }
        }

        if (mKind == Kind::None)
        {
            mKind = Kind::Icon;
            String iconPath = DynamicCast<VideoAsset>(ref) ? String("ui/UI4_video_icon.svg")
                : ref->GetType().InvokeStatic<String>("GetEditorIcon");

            if (iconPath.EndsWith(".svg"))
                mPreview = mmake<VectorSprite>(iconPath);
            else
                mPreview = mmake<Sprite>(iconPath);
        }
    }

    void AssetPreviewIcon::Clear()
    {
        if (mSoundPlayer)
            mSoundPlayer->Stop();

        if (mModelView)
            RemoveChild(mModelView);

        mKind = Kind::None;
        mChecker = nullptr;
        mPreview = nullptr;
        mPreviewSize = Vec2F();
        mModelView = nullptr;
        mSoundPlayer = nullptr;
        mPlayCircle = nullptr;
        mPlayGlyph = nullptr;
    }

    const AssetRef<Asset>& AssetPreviewIcon::GetAsset() const
    {
        return mAsset;
    }

    AssetPreviewIcon::Kind AssetPreviewIcon::GetKind() const
    {
        return mKind;
    }

    const Ref<IRectDrawable>& AssetPreviewIcon::GetPreviewDrawable() const
    {
        return mPreview;
    }

    const Ref<ModelView>& AssetPreviewIcon::GetModelView() const
    {
        return mModelView;
    }

    bool AssetPreviewIcon::IsSoundPlaying() const
    {
        return mSoundPlayer && mSoundPlayer->IsPlaying();
    }

    bool AssetPreviewIcon::OpensPreview() const
    {
        if (mKind == Kind::Image || mKind == Kind::VectorImage || mKind == Kind::Atlas || mKind == Kind::Font ||
            mKind == Kind::Model || mKind == Kind::Video)
        {
            return true;
        }

        return mKind == Kind::Icon && mAsset && DynamicCast<VideoAsset>(mAsset.GetRef());
    }

    void AssetPreviewIcon::Activate()
    {
        if (mKind == Kind::Sound)
        {
            if (mSoundPlayer->IsPlaying())
                mSoundPlayer->Stop();
            else
            {
                mSoundPlayer->SetTime(0.0f);
                mSoundPlayer->Play();
            }
        }
        else if (OpensPreview())
            AssetPreviewWindow::Show(mAsset);
    }

    RectF AssetPreviewIcon::GetPreviewRect() const
    {
        RectF rect = layout->GetWorldRect();
        if (mPreviewSize.x < 1.0f || mPreviewSize.y < 1.0f)
            return rect;

        Vec2F size = rect.Size();
        float scale = Math::Min(size.x/mPreviewSize.x, size.y/mPreviewSize.y);
        Vec2F fitted = mPreviewSize*scale;
        Vec2F center = rect.Center();
        return RectF(center - fitted*0.5f, center + fitted*0.5f);
    }

    void AssetPreviewIcon::Draw()
    {
        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        RectF rect = GetPreviewRect();
        if (mChecker)
        {
            mChecker->SetRect(rect);
            mChecker->SetTransparency(mResTransparency);
            mChecker->Draw();
        }

        if (mKind == Kind::Sound)
        {
            RectF square = layout->GetWorldRect();
            float side = Math::Min(square.Width(), square.Height());
            Vec2F center = square.Center();
            RectF circleRect(center - Vec2F(side, side)*0.5f, center + Vec2F(side, side)*0.5f);
            if (auto circle = DynamicCast<PipelineRoundedRect>(mPlayCircle))
                circle->radius = side*0.5f;

            mPlayCircle->SetRect(circleRect);
            mPlayCircle->SetTransparency(mResTransparency);
            mPlayCircle->Draw();

            const char* glyph = IsSoundPlaying() ? "ui/pipeline/btn_pause.svg" : "ui/pipeline/al_play.svg";
            if (mPlayGlyph->GetImageName() != glyph)
                mPlayGlyph->LoadFromImage(glyph);

            float glyphSide = side*0.5f;
            Vec2F glyphCenter = center + Vec2F(side*0.03f, 0.0f);
            mPlayGlyph->SetRect(RectF(glyphCenter - Vec2F(glyphSide, glyphSide)*0.5f, glyphCenter + Vec2F(glyphSide, glyphSide)*0.5f));
            mPlayGlyph->SetTransparency(mResTransparency);
            mPlayGlyph->Draw();
        }
        else if (mPreview)
        {
            mPreview->SetRect(rect);
            mPreview->SetTransparency(mResTransparency);
            mPreview->Draw();
        }

        Widget::Draw();
        CursorAreaEventsListener::OnDrawn();
    }

    void AssetPreviewIcon::Update(float dt)
    {
        Widget::Update(dt);

        if (mSoundPlayer)
            mSoundPlayer->Update(dt);
    }

    bool AssetPreviewIcon::IsUnderPoint(const Vec2F& point)
    {
        return mKind != Kind::None && Widget::IsUnderPoint(point);
    }

    String AssetPreviewIcon::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void AssetPreviewIcon::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (IsUnderPoint(cursor.position))
            Activate();
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::AssetPreviewIcon>);
// --- META ---

ENUM_META(Editor::AssetPreviewIcon::Kind, Editor__AssetPreviewIcon__Kind)
{
    ENUM_ENTRY(Atlas);
    ENUM_ENTRY(Font);
    ENUM_ENTRY(Icon);
    ENUM_ENTRY(Image);
    ENUM_ENTRY(Model);
    ENUM_ENTRY(None);
    ENUM_ENTRY(Sound);
    ENUM_ENTRY(VectorImage);
    ENUM_ENTRY(Video);
}
END_ENUM_META;

DECLARE_CLASS(Editor::AssetPreviewIcon, Editor__AssetPreviewIcon);
// --- END META ---
