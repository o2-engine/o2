#include "o2Editor/stdafx.h"
#include "MediaPlayerView.h"

#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Video.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Sound/SoundPlayer.h"
#include "o2Editor/UI/Preview/PreviewToolbar.h"

namespace Editor
{
    MediaPlayerView::MediaPlayerView(RefCounter* refCounter):
        Widget(refCounter)
    {
        BuildBar();
    }

    MediaPlayerView::MediaPlayerView(RefCounter* refCounter, const MediaPlayerView& other):
        Widget(refCounter)
    {
        BuildBar();
    }

    MediaPlayerView& MediaPlayerView::operator=(const MediaPlayerView& other)
    {
        return *this;
    }

    void MediaPlayerView::BuildBar()
    {
        mBar = PreviewToolbar::Create("playback");
        *mBar->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, barHeight, 0);
        AddChild(mBar);

        mPlayButton = PreviewToolbar::AddButton(mBar, "Play", 70.0f, [this]() { IsPlaying() ? Pause() : Play(); });
        mPlayButton->name = "play";
        PreviewToolbar::AddButton(mBar, "Stop", 60.0f, [this]() { Stop(); })->name = "stop";
        mTimeLabel = PreviewToolbar::AddLabel(mBar, "");
        mTimeLabel->name = "time";

        UpdateBar();
    }

    void MediaPlayerView::Play()
    {
        if (!mPlayback)
            return;

        if (mPlayback->GetDuration() > 0.0f && mPlayback->GetTime() >= mPlayback->GetDuration())
            mPlayback->SetTime(0.0f);

        mPlayback->Play();
        UpdateBar();
    }

    void MediaPlayerView::Pause()
    {
        if (!mPlayback)
            return;

        mPlayback->SetPlaying(false);
        UpdateBar();
    }

    void MediaPlayerView::Stop()
    {
        if (!mPlayback)
            return;

        mPlayback->Stop();
        mPlayback->SetTime(0.0f);
        UpdateBar();
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
        return mTimeLabel->GetText();
    }

    void MediaPlayerView::Update(float dt)
    {
        Widget::Update(dt);

        if (mPlayback && mPlayback->IsPlaying())
        {
            mPlayback->Update(dt);
            UpdateBar();
        }
    }

    void MediaPlayerView::UpdateBar()
    {
        mPlayButton->caption = IsPlaying() ? "Pause" : "Play";
        mTimeLabel->SetText(String::Format("%.2f / %.2f s", GetTime(), GetDuration()));
    }

    VideoView::VideoView(RefCounter* refCounter):
        MediaPlayerView(refCounter)
    {
        mVideo = mmake<Video>();
        mVideo->SetPlayOnAwake(false);
        mPlayback = mVideo;
    }

    VideoView::VideoView(RefCounter* refCounter, const VideoView& other):
        MediaPlayerView(refCounter, other)
    {
        mVideo = mmake<Video>();
        mVideo->SetPlayOnAwake(false);
        mPlayback = mVideo;
        SetVideo(other.mVideo->GetVideoAsset());
    }

    VideoView& VideoView::operator=(const VideoView& other)
    {
        SetVideo(other.mVideo->GetVideoAsset());
        return *this;
    }

    void VideoView::SetVideo(const AssetRef<VideoAsset>& video)
    {
        mVideo->Stop();
        mVideo->SetVideoAsset(video);
        mVideo->SetTime(0.0f);
        UpdateBar();
    }

    const Ref<Video>& VideoView::GetVideo() const
    {
        return mVideo;
    }

    void VideoView::Draw()
    {
        Widget::Draw();

        if (!IsEnabledInHierarchy() || !mVideo->GetVideoAsset())
            return;

        RectF world = layout->GetWorldRect();
        Vec2F min(Math::Min(world.left, world.right), Math::Min(world.top, world.bottom) + barHeight);
        Vec2F max(Math::Max(world.left, world.right), Math::Max(world.top, world.bottom));
        Vec2F area = max - min, size = (Vec2F)mVideo->GetVideoSize();
        if (size.x <= 0.0f || size.y <= 0.0f || area.x <= 0.0f || area.y <= 0.0f)
            return;

        float scale = Math::Min(area.x/size.x, area.y/size.y);
        Vec2F drawn = size*scale, center = (min + max)*0.5f;
        mVideo->SetBasis(Basis(center - drawn*0.5f, Vec2F(drawn.x, 0.0f), Vec2F(0.0f, drawn.y)));
        mVideo->Draw();
    }

    SoundPlayerView::SoundPlayerView(RefCounter* refCounter):
        MediaPlayerView(refCounter)
    {
        mPlayer = mmake<SoundPlayer>();
        mPlayback = mPlayer;
    }

    SoundPlayerView::SoundPlayerView(RefCounter* refCounter, const SoundPlayerView& other):
        MediaPlayerView(refCounter, other)
    {
        mPlayer = mmake<SoundPlayer>();
        mPlayback = mPlayer;
        SetSound(other.mPlayer->GetSound());
    }

    SoundPlayerView::~SoundPlayerView()
    {
        if (mPlayer)
            mPlayer->Stop();
    }

    SoundPlayerView& SoundPlayerView::operator=(const SoundPlayerView& other)
    {
        SetSound(other.mPlayer->GetSound());
        return *this;
    }

    void SoundPlayerView::SetSound(const AssetRef<SoundAsset>& sound)
    {
        mPlayer->Stop();
        mPlayer->SetSound(sound);
        mPlayer->SetTime(0.0f);
        UpdateBar();
    }

    const Ref<SoundPlayer>& SoundPlayerView::GetPlayer() const
    {
        return mPlayer;
    }
}
// --- META ---

DECLARE_CLASS(Editor::MediaPlayerView, Editor__MediaPlayerView);

DECLARE_CLASS(Editor::VideoView, Editor__VideoView);

DECLARE_CLASS(Editor::SoundPlayerView, Editor__SoundPlayerView);
// --- END META ---
