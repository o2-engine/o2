#include "o2Editor/stdafx.h"
#include "PipelineMediaViews.h"

#include "o2/Assets/Types/SoundAsset.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/HorizontalProgress.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Sound/SoundPlayer.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

#include <cstring>
#include <thread>

namespace Editor
{
    const float PipelineMediaControls::height = 22.0f;
    const float PipelineAudioView::height = 46.0f;

    static String FormatTime(float seconds)
    {
        int total = (int)Math::Max(0.0f, seconds);
        return String::Format("%d:%02d", total / 60, total % 60);
    }

    PipelineMediaControls::PipelineMediaControls(RefCounter* refCounter):
        Widget(refCounter)
    {
        layout->minHeight = height;

        mPlayButton = PipelineControls::MakeIconButton("ui/pipeline/btn_play.png", PipelineControls::accentColor, Color4(0, 0, 0, 0));
        mPlayButton->name = "play";
        *mPlayButton->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(0, 0), Vec2F(22, 0));
        mPlayButton->onClick = [this]() { if (onPlayPause) onPlayPause(); };
        AddChild(mPlayButton);

        mSeek = o2UI.CreateHorProgress();
        mSeek->name = "seek";
        *mSeek->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(26, 2), Vec2F(-154, -2));
        mSeek->SetValueRange(0.0f, 1.0f);
        mSeek->onChangeByUser = [this](float value)
        {
            mSeeking = true;
            if (onSeek)
                onSeek(value);
        };
        AddChild(mSeek);

        auto time = mmake<Text>("stdFont.ttf");
        time->horAlign = HorAlign::Right;
        time->verAlign = VerAlign::Middle;
        time->color = PipelineControls::dimTextColor;
        mTimeLayer = AddLayer("time", time, Layout(Vec2F(1, 0), Vec2F(1, 1), Vec2F(-150, 0), Vec2F(-82, 0)));

        mLoopToggle = PipelineControls::MakeSegment("", false);
        mLoopToggle->name = "loop";
        *mLoopToggle->layout = WidgetLayout(Vec2F(1, 0), Vec2F(1, 1), Vec2F(-78, 1), Vec2F(-58, -1));
        auto loopIcon = mmake<Sprite>("ui/pipeline/btn_loop.png");
        loopIcon->color = PipelineControls::textColor;
        mLoopToggle->AddLayer("icon", loopIcon, Layout::Based(BaseCorner::Center, Vec2F(14, 14)), 5.0f);
        mLoopToggle->onToggleByUser = [this](bool value) { if (onLoopChanged) onLoopChanged(value); };
        AddChild(mLoopToggle);

        auto volumeIcon = mmake<Sprite>("ui/pipeline/btn_volume.png");
        volumeIcon->color = PipelineControls::dimTextColor;
        mVolumeIcon = AddLayer("volume", volumeIcon, Layout(Vec2F(1, 0.5f), Vec2F(1, 0.5f), Vec2F(-54, -7), Vec2F(-40, 7)));

        mVolume = o2UI.CreateHorProgress();
        mVolume->name = "volume";
        *mVolume->layout = WidgetLayout(Vec2F(1, 0), Vec2F(1, 1), Vec2F(-38, 4), Vec2F(0, -4));
        mVolume->SetValueRange(0.0f, 1.0f);
        mVolume->SetValue(1.0f);
        mVolume->onChangeByUser = [this](float value) { if (onVolumeChanged) onVolumeChanged(value); };
        AddChild(mVolume);

        SetPlaying(false);
    }

    void PipelineMediaControls::SetPlaying(bool playing)
    {
        mPlaying = playing;
        if (auto icon = mPlayButton->GetLayerDrawable<Sprite>("icon"))
        {
            icon->imageName = playing ? "ui/pipeline/btn_pause.png" : "ui/pipeline/btn_play.png";
            icon->color = PipelineControls::accentColor;
        }
    }

    void PipelineMediaControls::SetTime(float time, float duration)
    {
        mDuration = duration;
        if (!mSeeking)
        {
            mSeek->SetValueRange(0.0f, Math::Max(duration, 0.001f));
            mSeek->SetValue(Math::Clamp(time, 0.0f, Math::Max(duration, 0.001f)));
        }

        if (auto text = DynamicCast<Text>(mTimeLayer->GetDrawable()))
            text->text = FormatTime(time) + " / " + FormatTime(duration);
    }

    void PipelineMediaControls::SetLoop(bool loop)
    {
        mLoopToggle->SetValue(loop);
    }

    bool PipelineMediaControls::IsLoop() const
    {
        return mLoopToggle->GetValue();
    }

    void PipelineMediaControls::SetVolume(float volume)
    {
        mVolume->SetValue(Math::Clamp01(volume));
    }

    float PipelineMediaControls::GetVolume() const
    {
        return Math::Clamp01(mVolume->GetValue());
    }

    void PipelineMediaControls::SetActive(bool active)
    {
        mPlayButton->interactable = active;
        mSeek->interactable = active;
        mLoopToggle->interactable = active;
        mVolume->interactable = active;
        mVolumeIcon->transparency = active ? 1.0f : 0.5f;
    }

    void PipelineMediaControls::Update(float dt)
    {
        Widget::Update(dt);
        if (mSeeking && !o2Input.IsCursorDown())
            mSeeking = false;
    }

    PipelineAudioView::PipelineAudioView(RefCounter* refCounter):
        Widget(refCounter)
    {
        layout->minHeight = height;

        AddLayer("back", mmake<Sprite>("ui/UI4_Editbox_regular.png"), Layout::BothStretch(-9, -9, -9, -9));

        mTitleLabel = PipelineControls::MakeLabel("", false);
        mTitleLabel->name = "title";
        *mTitleLabel->layout = WidgetLayout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(6, -22), Vec2F(-80, -2));
        AddChild(mTitleLabel);

        mInfoLabel = PipelineControls::MakeLabel("", true);
        mInfoLabel->name = "info";
        mInfoLabel->horAlign = HorAlign::Right;
        *mInfoLabel->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-100, -22), Vec2F(-6, -2));
        AddChild(mInfoLabel);

        mControls = mmake<PipelineMediaControls>();
        mControls->name = "controls";
        *mControls->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 0), Vec2F(4, 2), Vec2F(-4, 2 + PipelineMediaControls::height));
        auto weakThis = WeakRef(this);
        mControls->onPlayPause = [weakThis]() { if (auto self = weakThis.Lock()) self->TogglePlay(); };
        mControls->onSeek = [weakThis](float time) { if (auto self = weakThis.Lock()) self->Seek(time); };
        mControls->onLoopChanged = [weakThis](bool) { if (auto self = weakThis.Lock()) self->ApplyPlayerSettings(); };
        mControls->onVolumeChanged = [weakThis](float) { if (auto self = weakThis.Lock()) self->ApplyPlayerSettings(); };
        AddChild(mControls);

        RefreshControls();
    }

    PipelineAudioView::~PipelineAudioView()
    {
        if (mPlayer)
            mPlayer->Stop();
    }

    void PipelineAudioView::SetAudio(const PipelineValue& value, const String& title)
    {
        if (mPlayer)
        {
            mPlayer->Stop();
            mPlayer = nullptr;
        }
        mSoundAsset = nullptr;
        mPlaying = false;
        mDuration = 0.0f;
        mValue = value;
        mHasAudio = value.IsAudio() && !value.data.IsEmpty();
        mTitleLabel->text = mHasAudio ? title : String();
        mInfoLabel->text = mHasAudio ? (String)(value.data.Length() / 1024) + " KB" : String();
        if (mHasAudio)
            EnsurePlayer();

        RefreshControls();
    }

    void PipelineAudioView::SetHint(const String& hint)
    {
        if (!mHasAudio)
            mTitleLabel->text = hint;
    }

    bool PipelineAudioView::EnsurePlayer()
    {
        if (mPlayer)
            return true;

        if (!mHasAudio)
            return false;

        mSoundAsset = mmake<SoundAsset>();
        char* data = mnew char[mValue.data.Length()];
        std::memcpy(data, mValue.data.Data(), mValue.data.Length());
        mSoundAsset->SetData(data, (UInt)mValue.data.Length());

        mPlayer = mmake<SoundPlayer>();
        mPlayer->SetSound(AssetRef<SoundAsset>(mSoundAsset));
        mDuration = Math::Max(0.0f, mPlayer->GetDuration());
        ApplyPlayerSettings();
        return true;
    }

    void PipelineAudioView::ApplyPlayerSettings()
    {
        if (!mPlayer)
            return;

        mPlayer->SetLoop(mControls->IsLoop() ? Loop::Repeat : Loop::None);
        mPlayer->SetVolume(mControls->GetVolume());
    }

    void PipelineAudioView::Play()
    {
        if (!EnsurePlayer())
            return;

        ApplyPlayerSettings();
        if (mDuration > 0.0f && mPlayer->GetTime() >= mDuration - 0.01f)
            mPlayer->RewindAndPlay();
        else
            mPlayer->Play();

        mPlaying = true;
        RefreshControls();
    }

    void PipelineAudioView::Pause()
    {
        if (mPlayer)
            mPlayer->Stop();

        mPlaying = false;
        RefreshControls();
    }

    void PipelineAudioView::TogglePlay()
    {
        if (mPlaying)
            Pause();
        else
            Play();
    }

    void PipelineAudioView::Seek(float time)
    {
        if (!EnsurePlayer())
            return;

        mPlayer->SetTime(Math::Clamp(time, 0.0f, Math::Max(mDuration, 0.0f)));
        RefreshControls();
    }

    float PipelineAudioView::GetTime() const
    {
        return mPlayer ? Math::Clamp(mPlayer->GetTime(), 0.0f, Math::Max(mDuration, 0.0f)) : 0.0f;
    }

    void PipelineAudioView::RefreshControls()
    {
        mControls->SetActive(mHasAudio);
        mControls->SetPlaying(mPlaying);
        mControls->SetTime(GetTime(), mDuration);
    }

    void PipelineAudioView::Update(float dt)
    {
        Widget::Update(dt);
        if (mPlayer)
        {
            mPlayer->Update(dt);
            if (mPlaying && !mPlayer->IsPlaying())
                mPlaying = false;
        }

        RefreshControls();
    }

    PipelineVideoView::PipelineVideoView(RefCounter* refCounter):
        Widget(refCounter)
    {
        layout->minHeight = 120;

        mFrameLayer = AddLayer("frame", mmake<Sprite>("ui/UI4_Editbox_regular.png"), Layout::BothStretch(-9, -9, -9, -9), -1.0f);
        mBackLayer = AddLayer("back", mmake<Sprite>(Color4(38, 46, 52, 255)),
                              Layout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(1, PipelineMediaControls::height + 5), Vec2F(-1, -1)));

        mHintLabel = PipelineControls::MakeLabel("", true);
        mHintLabel->name = "hint";
        mHintLabel->horAlign = HorAlign::Middle;
        mHintLabel->verAlign = VerAlign::Middle;
        *mHintLabel->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(6, PipelineMediaControls::height + 6), Vec2F(-6, -4));
        AddChild(mHintLabel);

        mControls = mmake<PipelineMediaControls>();
        mControls->name = "controls";
        *mControls->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 0), Vec2F(4, 2), Vec2F(-4, 2 + PipelineMediaControls::height));
        auto weakThis = WeakRef(this);
        mControls->onPlayPause = [weakThis]() { if (auto self = weakThis.Lock()) self->TogglePlay(); };
        mControls->onSeek = [weakThis](float time) { if (auto self = weakThis.Lock()) self->Seek(time); };
        mControls->onVolumeChanged = [weakThis](float volume)
        {
            if (auto self = weakThis.Lock())
            {
                if (self->mPlayer)
                    self->mPlayer->SetVolume(volume);
            }
        };
        AddChild(mControls);

        mFrame = mmake<Sprite>();
        onDraw = [this]() { DrawFrame(); };

        RefreshControls();
    }

    PipelineVideoView::~PipelineVideoView()
    {
        if (mPlayer)
            mPlayer->Stop();
    }

    void PipelineVideoView::SetVideo(const PipelineValue& value, const String& cacheDir)
    {
        bool sameVideo = mHasVideo && value.IsVideo() && cacheDir == mCacheDir && value.data.Length() == mValue.data.Length();
        if (sameVideo)
            return;

        ClearPreview();
        mValue = value;
        mCacheDir = cacheDir;
        mHasVideo = value.IsVideo() && !value.data.IsEmpty();
        if (mHasVideo)
        {
            auto existing = PipelineVideo::LoadPreview(cacheDir);
            if (existing.ok)
                ApplyPreview(existing);
            else
                StartExtraction();
        }

        RefreshControls();
    }

    void PipelineVideoView::SetHint(const String& hint)
    {
        mHint = hint;
        if (!mHasVideo)
            mHintLabel->text = hint;
    }

    void PipelineVideoView::StartExtraction()
    {
        auto job = std::make_shared<ExtractJob>();
        job->data = mValue.data;
        job->dir = mCacheDir;
        mJob = job;
        mHintLabel->text = "Preparing preview...";
        mHintLabel->enabled = true;

        std::thread([job]()
        {
            job->result = PipelineVideo::BuildPreview(job->data, job->dir);
            job->done = true;
        }).detach();
    }

    void PipelineVideoView::ApplyPreview(const PipelineVideo::PreviewInfo& info)
    {
        mInfo = info;
        mReady = false;
        if (!info.ok)
        {
            mHintLabel->text = "Preview failed: " + info.error;
            mHintLabel->enabled = true;
            return;
        }

        auto atlas = mmake<Bitmap>();
        if (!atlas->Load(info.atlasPath, Bitmap::ImageType::Png))
        {
            mHintLabel->text = "Preview atlas is unreadable";
            mHintLabel->enabled = true;
            return;
        }

        mAtlas = TextureRef(*atlas);
        mFrame->SetTexture(mAtlas);
        mFrame->SetTextureSrcRect(PipelineVideo::FrameRect(info, 0));
        mFrame->mode = SpriteMode::Default;

        if (info.hasAudio)
        {
            String wav = PipelineUtils::ReadFileBytes(info.audioPath);
            if (!wav.IsEmpty())
            {
                mSoundAsset = mmake<SoundAsset>();
                char* data = mnew char[wav.Length()];
                std::memcpy(data, wav.Data(), wav.Length());
                mSoundAsset->SetData(data, (UInt)wav.Length());

                mPlayer = mmake<SoundPlayer>();
                mPlayer->SetSound(AssetRef<SoundAsset>(mSoundAsset));
                mPlayer->SetVolume(mControls->GetVolume());
            }
        }

        mTime = 0.0f;
        mPlaying = false;
        mReady = true;
        mHintLabel->enabled = false;
    }

    void PipelineVideoView::ClearPreview()
    {
        if (mPlayer)
            mPlayer->Stop();

        mPlayer = nullptr;
        mSoundAsset = nullptr;
        mAtlas = TextureRef();
        mJob = nullptr;
        mInfo = PipelineVideo::PreviewInfo();
        mReady = false;
        mPlaying = false;
        mHasVideo = false;
        mTime = 0.0f;
        mHintLabel->text = mHint;
        mHintLabel->enabled = true;
    }

    void PipelineVideoView::SyncAudio(bool seek)
    {
        if (!mPlayer)
            return;

        float audioDuration = Math::Max(0.0f, mPlayer->GetDuration());
        if (seek)
            mPlayer->SetTime(Math::Clamp(mTime, 0.0f, audioDuration));

        if (mPlaying && mTime < audioDuration && !mPlayer->IsPlaying())
            mPlayer->Play();
        else if (!mPlaying && mPlayer->IsPlaying())
            mPlayer->Stop();
    }

    void PipelineVideoView::Play()
    {
        if (!mReady)
            return;

        if (mTime >= mInfo.duration - 0.01f)
            mTime = 0.0f;

        mPlaying = true;
        SyncAudio(true);
        RefreshControls();
    }

    void PipelineVideoView::Pause()
    {
        mPlaying = false;
        SyncAudio(false);
        RefreshControls();
    }

    void PipelineVideoView::TogglePlay()
    {
        if (mPlaying)
            Pause();
        else
            Play();
    }

    void PipelineVideoView::Seek(float time)
    {
        if (!mReady)
            return;

        mTime = Math::Clamp(time, 0.0f, mInfo.duration);
        mFrame->SetTextureSrcRect(PipelineVideo::FrameRect(mInfo, GetFrameIndex()));
        SyncAudio(true);
        RefreshControls();
    }

    int PipelineVideoView::GetFrameIndex() const
    {
        if (!mReady || mInfo.frameCount <= 0)
            return 0;

        return Math::Clamp((int)(mTime * mInfo.fps), 0, mInfo.frameCount - 1);
    }

    RectF PipelineVideoView::GetFrameArea() const
    {
        RectF area = layout->GetWorldRect();
        area.bottom += PipelineMediaControls::height + 6.0f;
        area.left += 2.0f;
        area.right -= 2.0f;
        area.top -= 2.0f;
        return area;
    }

    void PipelineAudioView::Draw()
    {
        if (!PipelineControls::IsFarView())
        {
            Widget::Draw();
            return;
        }

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        DrawLayers();
        OnDrawn();
        mTitleLabel->Draw();
        mInfoLabel->Draw();
        DrawTopLayers();
    }

    void PipelineVideoView::Draw()
    {
        if (!PipelineControls::IsFarView())
        {
            Widget::Draw();
            return;
        }

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        DrawLayers();
        OnDrawn();
        DrawTopLayers();
    }

    void PipelineVideoView::DrawFrame()
    {
        if (!mReady || !mResEnabledInHierarchy || mInfo.frameWidth <= 0 || mInfo.frameHeight <= 0)
            return;

        RectF area = GetFrameArea();
        if (area.Width() <= 0.0f || area.Height() <= 0.0f)
            return;

        float k = Math::Min(area.Width() / mInfo.frameWidth, area.Height() / mInfo.frameHeight);
        Vec2F size(mInfo.frameWidth * k, mInfo.frameHeight * k);
        Vec2F center = area.Center();
        mFrame->rect = RectF(center - size * 0.5f, center + size * 0.5f);
        mFrame->Draw();
    }

    void PipelineVideoView::RefreshControls()
    {
        mControls->SetActive(mReady);
        mControls->SetPlaying(mPlaying);
        mControls->SetTime(mTime, mInfo.duration);
    }

    void PipelineVideoView::Update(float dt)
    {
        Widget::Update(dt);

        if (mJob && mJob->done)
        {
            auto job = mJob;
            mJob = nullptr;
            if (job->dir == mCacheDir)
                ApplyPreview(job->result);
        }

        if (mReady && mPlaying)
        {
            mTime += dt;
            if (mTime >= mInfo.duration)
            {
                if (mControls->IsLoop())
                {
                    mTime = Math::Mod(mTime, Math::Max(mInfo.duration, 0.001f));
                    SyncAudio(true);
                }
                else
                {
                    mTime = mInfo.duration;
                    Pause();
                }
            }

            mFrame->SetTextureSrcRect(PipelineVideo::FrameRect(mInfo, GetFrameIndex()));
        }

        if (mPlayer)
            mPlayer->Update(dt);

        RefreshControls();
    }
}
// --- META ---

DECLARE_CLASS(Editor::PipelineMediaControls, Editor__PipelineMediaControls);

DECLARE_CLASS(Editor::PipelineAudioView, Editor__PipelineAudioView);

DECLARE_CLASS(Editor::PipelineVideoView, Editor__PipelineVideoView);
// --- END META ---
