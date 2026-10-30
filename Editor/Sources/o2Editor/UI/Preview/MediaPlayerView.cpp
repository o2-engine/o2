#include "o2Editor/stdafx.h"
#include "MediaPlayerView.h"

#include "o2/Render/Render.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Render/Video.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Sound/SoundPlayer.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/UI/Preview/MediaPreview.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineMediaViews.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

namespace Editor
{
    MediaPlayerView::MediaPlayerView(RefCounter* refCounter):
        Widget(refCounter)
    {
        BuildControls();
    }

    MediaPlayerView::MediaPlayerView(RefCounter* refCounter, const MediaPlayerView& other):
        Widget(refCounter)
    {
        BuildControls();
    }

    MediaPlayerView& MediaPlayerView::operator=(const MediaPlayerView& other)
    {
        return *this;
    }

    void MediaPlayerView::BuildControls()
    {
        mControls = mmake<PipelineMediaControls>();
        mControls->name = "transport";
        *mControls->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, barHeight, 0);
        mControls->SetLoopVisible(true);
        mControls->SetTimeFormatter([](float time, float duration) {
            return MediaPreview::FormatTime(time, duration) + " / " + MediaPreview::FormatTime(duration, duration) +
                (duration < 60.0f ? " s" : "");
        });

        mControls->onPlayPause = [this]() { TogglePlay(); };
        mControls->onSeek = [this](float time) { Seek(time); };
        mControls->onLoopChanged = [this](bool loop) { SetLoop(loop); };
        AddChild(mControls);

        UpdateControls();
    }

    void MediaPlayerView::Play()
    {
        if (!mPlayback)
            return;

        if (mPlayback->GetDuration() > 0.0f && mPlayback->GetTime() >= mPlayback->GetDuration())
            mPlayback->SetTime(0.0f);

        mPlayback->Play();
        UpdateControls();
    }

    void MediaPlayerView::Pause()
    {
        if (!mPlayback)
            return;

        mPlayback->SetPlaying(false);
        UpdateControls();
    }

    void MediaPlayerView::TogglePlay()
    {
        if (IsPlaying())
            Pause();
        else
            Play();
    }

    void MediaPlayerView::Stop()
    {
        if (!mPlayback)
            return;

        mPlayback->Stop();
        mPlayback->SetTime(0.0f);
        UpdateControls();
    }

    void MediaPlayerView::Seek(float time)
    {
        if (!mPlayback)
            return;

        mPlayback->SetTime(Math::Clamp(time, 0.0f, Math::Max(0.0f, mPlayback->GetDuration())));
        UpdateControls();
    }

    void MediaPlayerView::SetLoop(bool loop)
    {
        if (mPlayback)
            mPlayback->SetLoop(loop ? Loop::Repeat : Loop::None);

        mControls->SetLoop(loop);
    }

    bool MediaPlayerView::IsLoop() const
    {
        return mPlayback && mPlayback->GetLoop() == Loop::Repeat;
    }

    bool MediaPlayerView::IsPlaying() const
    {
        return mPlayback && mPlayback->IsPlaying();
    }

    float MediaPlayerView::GetTime() const
    {
        return mPlayback ? mPlayback->GetTime() : 0.0f;
    }

    float MediaPlayerView::GetDuration() const
    {
        return mPlayback ? mPlayback->GetDuration() : 0.0f;
    }

    WString MediaPlayerView::GetTimeText() const
    {
        return mControls->GetTimeText();
    }

    const Ref<PipelineMediaControls>& MediaPlayerView::GetControls() const
    {
        return mControls;
    }

    void MediaPlayerView::Update(float dt)
    {
        Widget::Update(dt);

        if (mPlayback && mPlayback->IsPlaying())
            mPlayback->Update(dt);

        UpdateControls();
    }

    void MediaPlayerView::UpdateControls()
    {
        // The glyph is an asset: it is set only when the state changes, a player stopped as the editor closes loads none
        bool playing = IsPlaying();
        if (playing != mShownPlaying)
        {
            mControls->SetPlaying(playing);
            mShownPlaying = playing;
        }

        mControls->SetTime(GetTime(), GetDuration());
        mControls->SetActive(mPlayback && GetDuration() > 0.0f);
    }

    RectF MediaPlayerView::GetContentRect() const
    {
        RectF rect = layout->GetWorldRect();
        rect.bottom += barHeight + barGap;
        return rect;
    }

    void MediaPlayerView::Draw()
    {
        if (mResEnabledInHierarchy && !mIsClipped)
        {
            DrawContent();
            CursorAreaEventsListener::OnDrawn();
        }

        Widget::Draw();
    }

    bool MediaPlayerView::IsUnderPoint(const Vec2F& point)
    {
        RectF content = GetContentRect();
        return content.Height() > 1.0f && content.IsInside(point) && Widget::IsUnderPoint(point);
    }

    void MediaPlayerView::OnCursorPressed(const Input::Cursor& cursor)
    {
        OnContentPressed(cursor.position);
    }

    void MediaPlayerView::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (cursor.delta != Vec2F())
            OnContentDragged(cursor.position);
    }

    VideoView::VideoView(RefCounter* refCounter):
        MediaPlayerView(refCounter)
    {
        mVideo = mmake<Video>();
        mVideo->SetPlayOnAwake(false);
        mPlayback = mVideo;
        BuildOverlay();
    }

    VideoView::VideoView(RefCounter* refCounter, const VideoView& other):
        MediaPlayerView(refCounter, other)
    {
        mVideo = mmake<Video>();
        mVideo->SetPlayOnAwake(false);
        mPlayback = mVideo;
        BuildOverlay();
        SetVideo(other.mVideo->GetVideoAsset());
    }

    VideoView& VideoView::operator=(const VideoView& other)
    {
        SetVideo(other.mVideo->GetVideoAsset());
        return *this;
    }

    void VideoView::BuildOverlay()
    {
        auto& theme = PipelineTheme::Get();

        auto circle = mmake<PipelineRoundedRect>();
        circle->radius = 24.0f;
        circle->roundTop = true;
        circle->roundBottom = true;
        circle->color = theme.accent;
        circle->opacity = 0.9f;
        mPlayCircle = circle;

        auto glyph = mmake<VectorSprite>("ui/pipeline/al_play.svg");
        glyph->color = theme.accentInk;
        mPlayGlyph = glyph;
    }

    void VideoView::SetVideo(const AssetRef<VideoAsset>& video)
    {
        mVideo->Stop();
        mVideo->SetVideoAsset(video);
        mVideo->SetTime(0.0f);
        mVideo->SetLoop(mControls->IsLoop() ? Loop::Repeat : Loop::None);
        mOverlayCoef = 1.0f;
        UpdateControls();
    }

    const Ref<Video>& VideoView::GetVideo() const
    {
        return mVideo;
    }

    const Ref<IRectDrawable>& VideoView::GetPlayOverlay() const
    {
        return mPlayCircle;
    }

    RectF VideoView::GetFrameRect() const
    {
        RectF area = GetContentRect();
        Vec2F size = (Vec2F)mVideo->GetVideoSize();
        if (size.x <= 0.0f || size.y <= 0.0f || area.Width() <= 0.0f || area.Height() <= 0.0f)
            return RectF();

        float scale = Math::Min(area.Width()/size.x, area.Height()/size.y);
        Vec2F drawn = size*scale, center = area.Center();
        return RectF(center - drawn*0.5f, center + drawn*0.5f);
    }

    void VideoView::Update(float dt)
    {
        MediaPlayerView::Update(dt);

        float target = IsPlaying() ? 0.0f : 1.0f;
        mOverlayCoef = Math::Lerp(mOverlayCoef, target, Math::Min(1.0f, dt*12.0f));
        if (Math::Abs(mOverlayCoef - target) < 0.01f)
            mOverlayCoef = target;
    }

    void VideoView::DrawContent()
    {
        if (!mVideo->GetVideoAsset())
            return;

        RectF frame = GetFrameRect();
        if (frame.Width() <= 0.0f)
            return;

        mVideo->SetBasis(Basis(frame.LeftBottom(), Vec2F(frame.Width(), 0.0f), Vec2F(0.0f, frame.Height())));
        mVideo->Draw();

        if (mOverlayCoef > 0.01f)
        {
            Vec2F center = frame.Center();
            float radius = Math::Min(24.0f, Math::Min(frame.Width(), frame.Height())*0.25f);
            if (auto circle = DynamicCast<PipelineRoundedRect>(mPlayCircle))
                circle->radius = radius;

            mPlayCircle->SetRect(RectF(center - Vec2F(radius, radius), center + Vec2F(radius, radius)));
            mPlayCircle->SetTransparency(mOverlayCoef);
            mPlayCircle->Draw();

            float glyph = radius*0.75f;
            Vec2F glyphCenter = center + Vec2F(radius*0.06f, 0.0f);
            mPlayGlyph->SetRect(RectF(glyphCenter - Vec2F(glyph, glyph)*0.5f, glyphCenter + Vec2F(glyph, glyph)*0.5f));
            mPlayGlyph->SetTransparency(mOverlayCoef);
            mPlayGlyph->Draw();
        }
    }

    void VideoView::OnContentPressed(const Vec2F& point)
    {
        if (GetFrameRect().IsInside(point))
            TogglePlay();
    }

    SoundPlayerView::SoundPlayerView(RefCounter* refCounter):
        MediaPlayerView(refCounter)
    {
        mPlayer = mmake<SoundPlayer>();
        mPlayer->SetScrubPreview(false);
        mPlayback = mPlayer;
        ApplyLook();
    }

    SoundPlayerView::SoundPlayerView(RefCounter* refCounter, const SoundPlayerView& other):
        MediaPlayerView(refCounter, other), mLarge(other.mLarge)
    {
        mPlayer = mmake<SoundPlayer>();
        mPlayer->SetScrubPreview(false);
        mPlayback = mPlayer;
        ApplyLook();
        SetSound(other.mPlayer->GetSound());
    }

    SoundPlayerView::~SoundPlayerView()
    {
        if (mPlayer)
            mPlayer->Stop();
    }

    SoundPlayerView& SoundPlayerView::operator=(const SoundPlayerView& other)
    {
        mLarge = other.mLarge;
        ApplyLook();
        SetSound(other.mPlayer->GetSound());
        return *this;
    }

    void SoundPlayerView::SetSound(const AssetRef<SoundAsset>& sound)
    {
        mPlayer->Stop();
        mPlayer->SetSound(sound);
        mPlayer->SetTime(0.0f);
        mPlayer->SetLoop(mControls->IsLoop() ? Loop::Repeat : Loop::None);

        String extension = sound ? o2FileSystem.GetFileExtension(sound->GetPath()) : String();
        mControls->SetFormat(extension.ToUpperCase());

        mPeaks = MediaPreview::GetPeaks(sound);
        mControls->SetPeaks(mPeaks);
        UpdateControls();
    }

    const Ref<SoundPlayer>& SoundPlayerView::GetPlayer() const
    {
        return mPlayer;
    }

    void SoundPlayerView::SetLarge(bool large)
    {
        mLarge = large;
        ApplyLook();
    }

    bool SoundPlayerView::IsLarge() const
    {
        return mLarge;
    }

    const Vector<float>& SoundPlayerView::GetPeaks() const
    {
        return mPeaks;
    }

    void SoundPlayerView::ApplyLook()
    {
        if (mLarge)
        {
            mControls->SetMode(PipelineMediaControls::Mode::Transport);
            *mControls->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, barHeight, 0);
        }
        else
        {
            mControls->SetMode(PipelineMediaControls::Mode::Audio);
            *mControls->layout = WidgetLayout::BothStretch();
        }
    }

    void SoundPlayerView::Update(float dt)
    {
        MediaPlayerView::Update(dt);

        if (mPeaks.IsEmpty() && mPlayer->GetSound())
        {
            mPeaks = MediaPreview::GetPeaks(mPlayer->GetSound());
            if (!mPeaks.IsEmpty())
                mControls->SetPeaks(mPeaks);
        }
    }

    void SoundPlayerView::DrawContent()
    {
        if (!mLarge)
            return;

        RectF rect = GetContentRect();
        if (rect.Height() < 2.0f)
            return;

        auto& theme = PipelineTheme::Get();
        float duration = GetDuration();
        float progress = duration > 0.0f ? GetTime()/duration : 0.0f;
        MediaPreview::DrawWaveform(rect, mPeaks, progress, theme.portAudio, theme.border, 3.0f, 2.0f);

        if (duration > 0.0f)
        {
            float x = Math::Round(rect.left + rect.Width()*Math::Clamp01(progress));
            o2Render.DrawFilledPolygon({ Vec2F(x - 1, rect.bottom), Vec2F(x - 1, rect.top), Vec2F(x + 1, rect.top),
                                         Vec2F(x + 1, rect.bottom) }, theme.accent);
        }
    }

    void SoundPlayerView::OnContentPressed(const Vec2F& point)
    {
        OnContentDragged(point);
    }

    void SoundPlayerView::OnContentDragged(const Vec2F& point)
    {
        if (!mLarge)
            return;

        RectF rect = GetContentRect();
        if (rect.Width() > 0.0f)
            Seek(Math::Clamp01((point.x - rect.left)/rect.Width())*GetDuration());
    }
}
// --- META ---

DECLARE_CLASS(Editor::MediaPlayerView, Editor__MediaPlayerView);

DECLARE_CLASS(Editor::VideoView, Editor__VideoView);

DECLARE_CLASS(Editor::SoundPlayerView, Editor__SoundPlayerView);
// --- END META ---
