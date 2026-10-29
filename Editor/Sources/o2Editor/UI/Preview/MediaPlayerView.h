#pragma once

#include "o2/Animation/IAnimation.h"
#include "o2/Assets/Types/SoundAsset.h"
#include "o2/Assets/Types/VideoAsset.h"
#include "o2/Render/Video.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Sound/SoundPlayer.h"

using namespace o2;

namespace o2
{
    class Button;
    class HorizontalLayout;
    class Label;
}

namespace Editor
{
    // --------------------------------------------------------------------------------------
    // Playback of a sound or a video in the editor: play or pause, stop, the time of the clip;
    // the widget updates the playback while it is shown
    // --------------------------------------------------------------------------------------
    class MediaPlayerView: public Widget
    {
    public:
        // Default constructor
        MediaPlayerView(RefCounter* refCounter);

        // Copy-constructor
        MediaPlayerView(RefCounter* refCounter, const MediaPlayerView& other);

        // Copy-operator
        MediaPlayerView& operator=(const MediaPlayerView& other);

        // Plays from the current time
        void Play();

        // Pauses at the current time
        void Pause();

        // Stops and rewinds
        void Stop();

        // Returns is the clip playing
        bool IsPlaying() const;

        // Returns time of the clip
        float GetTime() const;

        // Returns duration of the clip
        float GetDuration() const;

        // Returns the time label text
        WString GetTimeText() const;

        // Updates the playback and the time label
        void Update(float dt) override;

        SERIALIZABLE(MediaPlayerView);
        CLONEABLE_REF(MediaPlayerView);

    protected:
        static constexpr float barHeight = 22.0f; // Height of the controls

        Ref<IAnimation>       mPlayback;   // Played clip
        Ref<HorizontalLayout> mBar;        // Controls
        Ref<Button>           mPlayButton; // Play or pause
        Ref<Label>            mTimeLabel;  // Time of the clip

    protected:
        // Builds the controls at the bottom
        void BuildBar();

        // Updates the caption of the play button and the time label
        void UpdateBar();
    };

    // ------------------------------------------------------------------------
    // Video player: the frame fitted into the widget over the playback controls
    // ------------------------------------------------------------------------
    class VideoView: public MediaPlayerView
    {
    public:
        // Default constructor
        VideoView(RefCounter* refCounter);

        // Copy-constructor
        VideoView(RefCounter* refCounter, const VideoView& other);

        // Copy-operator
        VideoView& operator=(const VideoView& other);

        // Shows the first frame of the video
        void SetVideo(const AssetRef<VideoAsset>& video);

        // Returns the video drawable
        const Ref<Video>& GetVideo() const;

        // Draws the frame and the controls
        void Draw() override;

        SERIALIZABLE(VideoView);
        CLONEABLE_REF(VideoView);

    protected:
        Ref<Video> mVideo; // Video drawable
    };

    // ------------------------------------
    // Sound player: the playback controls
    // ------------------------------------
    class SoundPlayerView: public MediaPlayerView
    {
    public:
        // Default constructor
        SoundPlayerView(RefCounter* refCounter);

        // Copy-constructor
        SoundPlayerView(RefCounter* refCounter, const SoundPlayerView& other);

        // Destructor, stops the sound
        ~SoundPlayerView();

        // Copy-operator
        SoundPlayerView& operator=(const SoundPlayerView& other);

        // Sets the sound to play
        void SetSound(const AssetRef<SoundAsset>& sound);

        // Returns the sound player
        const Ref<SoundPlayer>& GetPlayer() const;

        SERIALIZABLE(SoundPlayerView);
        CLONEABLE_REF(SoundPlayerView);

    protected:
        Ref<SoundPlayer> mPlayer; // Sound player
    };
}
// --- META ---

CLASS_BASES_META(Editor::MediaPlayerView)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::MediaPlayerView)
{
    FIELD().PROTECTED().NAME(mPlayback);
    FIELD().PROTECTED().NAME(mBar);
    FIELD().PROTECTED().NAME(mPlayButton);
    FIELD().PROTECTED().NAME(mTimeLabel);
}
END_META;
CLASS_METHODS_META(Editor::MediaPlayerView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const MediaPlayerView&);
    FUNCTION().PUBLIC().SIGNATURE(void, Play);
    FUNCTION().PUBLIC().SIGNATURE(void, Pause);
    FUNCTION().PUBLIC().SIGNATURE(void, Stop);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPlaying);
    FUNCTION().PUBLIC().SIGNATURE(float, GetTime);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetTimeText);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildBar);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateBar);
}
END_META;

CLASS_BASES_META(Editor::VideoView)
{
    BASE_CLASS(Editor::MediaPlayerView);
}
END_META;
CLASS_FIELDS_META(Editor::VideoView)
{
    FIELD().PROTECTED().NAME(mVideo);
}
END_META;
CLASS_METHODS_META(Editor::VideoView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const VideoView&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetVideo, const AssetRef<VideoAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Video>&, GetVideo);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
}
END_META;

CLASS_BASES_META(Editor::SoundPlayerView)
{
    BASE_CLASS(Editor::MediaPlayerView);
}
END_META;
CLASS_FIELDS_META(Editor::SoundPlayerView)
{
    FIELD().PROTECTED().NAME(mPlayer);
}
END_META;
CLASS_METHODS_META(Editor::SoundPlayerView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const SoundPlayerView&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSound, const AssetRef<SoundAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<SoundPlayer>&, GetPlayer);
}
END_META;
// --- END META ---
