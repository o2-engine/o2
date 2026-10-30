#pragma once

#include "o2/Animation/IAnimation.h"
#include "o2/Assets/Types/SoundAsset.h"
#include "o2/Assets/Types/VideoAsset.h"
#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Render/Video.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Sound/SoundPlayer.h"

using namespace o2;

namespace Editor
{
    FORWARD_CLASS_REF(PipelineMediaControls);

    // --------------------------------------------------------------------------------------------------------
    // Playback of a sound or a video in the editor under the transport bar of the pipeline nodes: play / pause,
    // the seek bar, the time and the loop. The content above the bar takes clicks; the widget updates the
    // playback while it is shown
    // --------------------------------------------------------------------------------------------------------
    class MediaPlayerView: public Widget, public CursorAreaEventsListener
    {
    public:
        // Default constructor
        MediaPlayerView(RefCounter* refCounter);

        // Copy-constructor
        MediaPlayerView(RefCounter* refCounter, const MediaPlayerView& other);

        // Copy-operator
        MediaPlayerView& operator=(const MediaPlayerView& other);

        // Plays from the current time, from the start when the clip has ended
        void Play();

        // Pauses at the current time
        void Pause();

        // Plays or pauses
        void TogglePlay();

        // Stops and rewinds
        void Stop();

        // Moves the playback to the time in seconds
        void Seek(float time);

        // Sets is the clip looped
        void SetLoop(bool loop);

        // Returns is the clip looped
        bool IsLoop() const;

        // Returns is the clip playing
        bool IsPlaying() const;

        // Returns time of the clip
        float GetTime() const;

        // Returns duration of the clip
        float GetDuration() const;

        // Returns the time text of the transport bar
        WString GetTimeText() const;

        // Returns the transport bar
        const Ref<PipelineMediaControls>& GetControls() const;

        // Updates the playback and the transport bar
        void Update(float dt) override;

        // Draws the content, then the transport bar over it
        void Draw() override;

        // Returns true if point is in the content above the transport bar
        bool IsUnderPoint(const Vec2F& point) override;

        SERIALIZABLE(MediaPlayerView);
        CLONEABLE_REF(MediaPlayerView);

    protected:
        static constexpr float barHeight = 22.0f; // Height of the transport bar
        static constexpr float barGap = 6.0f;     // Gap between the content and the transport bar

        Ref<IAnimation>            mPlayback;            // Played clip
        Ref<PipelineMediaControls> mControls;            // Transport bar
        bool                       mShownPlaying = false; // Playing state the transport bar shows

    protected:
        // Builds the transport bar at the bottom
        void BuildControls();

        // Shows the state of the playback in the transport bar
        void UpdateControls();

        // Returns the rectangle of the content above the transport bar
        RectF GetContentRect() const;

        // Draws the content above the transport bar
        virtual void DrawContent() {}

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when the content is pressed at the point
        virtual void OnContentPressed(const Vec2F& point) {}

        // Called when the press on the content is dragged to the point
        virtual void OnContentDragged(const Vec2F& point) {}

        REF_COUNTERABLE_IMPL(Widget);
    };

    // -------------------------------------------------------------------------------------------------------------
    // Video player: the frame fitted above the transport bar, a click plays or pauses; a paused video shows a play
    // button in the middle
    // -------------------------------------------------------------------------------------------------------------
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

        // Returns the rectangle the frame is drawn in
        RectF GetFrameRect() const;

        // Returns the circle of the play button over a paused video
        const Ref<IRectDrawable>& GetPlayOverlay() const;

        // Fades the play button with the playback
        void Update(float dt) override;

        SERIALIZABLE(VideoView);
        CLONEABLE_REF(VideoView);

    protected:
        Ref<Video>         mVideo;       // Video drawable
        Ref<IRectDrawable> mPlayCircle;  // Circle of the play button over a paused video
        Ref<IRectDrawable> mPlayGlyph;   // Glyph of the play button
        float              mOverlayCoef = 1.0f; // Visibility of the play button, fades with the playback

    protected:
        // Builds the play button over the frame
        void BuildOverlay();

        // Draws the frame and the play button
        void DrawContent() override;

        // Plays or pauses by a click on the frame
        void OnContentPressed(const Vec2F& point) override;
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Sound player: the waveform of the pipeline audio node in the transport bar; the large one draws the waveform over
    // the whole widget above the bar, a click or a drag on it seeks
    // ---------------------------------------------------------------------------------------------------------------
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

        // Sets is the waveform drawn large above the transport bar
        void SetLarge(bool large);

        // Returns is the waveform drawn large
        bool IsLarge() const;

        // Returns the peaks of the waveform, empty while they are decoded
        const Vector<float>& GetPeaks() const;

        // Updates the waveform once decoded
        void Update(float dt) override;

        SERIALIZABLE(SoundPlayerView);
        CLONEABLE_REF(SoundPlayerView);

    protected:
        Ref<SoundPlayer> mPlayer;        // Sound player
        Vector<float>    mPeaks;         // Waveform peaks, empty while they are decoded
        bool             mLarge = false; // Is the waveform drawn large above the bar

    protected:
        // Draws the large waveform with the playhead
        void DrawContent() override;

        // Seeks to the pressed point of the large waveform
        void OnContentPressed(const Vec2F& point) override;

        // Seeks to the dragged point of the large waveform
        void OnContentDragged(const Vec2F& point) override;

        // Lays the transport bar out for the size
        void ApplyLook();
    };
}
// --- META ---

CLASS_BASES_META(Editor::MediaPlayerView)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::CursorAreaEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::MediaPlayerView)
{
    FIELD().PROTECTED().NAME(mPlayback);
    FIELD().PROTECTED().NAME(mControls);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mShownPlaying);
}
END_META;
CLASS_METHODS_META(Editor::MediaPlayerView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const MediaPlayerView&);
    FUNCTION().PUBLIC().SIGNATURE(void, Play);
    FUNCTION().PUBLIC().SIGNATURE(void, Pause);
    FUNCTION().PUBLIC().SIGNATURE(void, TogglePlay);
    FUNCTION().PUBLIC().SIGNATURE(void, Stop);
    FUNCTION().PUBLIC().SIGNATURE(void, Seek, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLoop, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLoop);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPlaying);
    FUNCTION().PUBLIC().SIGNATURE(float, GetTime);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetTimeText);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<PipelineMediaControls>&, GetControls);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildControls);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateControls);
    FUNCTION().PROTECTED().SIGNATURE(RectF, GetContentRect);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawContent);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContentPressed, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContentDragged, const Vec2F&);
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
    FIELD().PROTECTED().NAME(mPlayCircle);
    FIELD().PROTECTED().NAME(mPlayGlyph);
    FIELD().PROTECTED().DEFAULT_VALUE(1.0f).NAME(mOverlayCoef);
}
END_META;
CLASS_METHODS_META(Editor::VideoView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const VideoView&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetVideo, const AssetRef<VideoAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Video>&, GetVideo);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetFrameRect);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetPlayOverlay);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildOverlay);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawContent);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContentPressed, const Vec2F&);
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
    FIELD().PROTECTED().NAME(mPeaks);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mLarge);
}
END_META;
CLASS_METHODS_META(Editor::SoundPlayerView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const SoundPlayerView&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSound, const AssetRef<SoundAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<SoundPlayer>&, GetPlayer);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLarge, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLarge);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<float>&, GetPeaks);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawContent);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContentPressed, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnContentDragged, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, ApplyLook);
}
END_META;
// --- END META ---
