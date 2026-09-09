#pragma once

#include "o2/Render/TextureRef.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Function/Function.h"
#include "o2Editor/Pipeline/PipelineValue.h"
#include "o2Editor/Pipeline/PipelineVideo.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"

#include <atomic>
#include <memory>

using namespace o2;

namespace o2
{
    class Button;
    class HorizontalProgress;
    class Label;
    class SoundAsset;
    class SoundPlayer;
    class Sprite;
    class Toggle;
}

namespace Editor
{
    // ------------------------------------------------------------------------------------
    // Transport bar of the media views: play / pause, seek slider, time, loop and volume
    // ------------------------------------------------------------------------------------
    class PipelineMediaControls : public Widget
    {
    public:
        Function<void()>      onPlayPause;     // Called when the play / pause button is pressed
        Function<void(float)> onSeek;          // Called with the time in seconds while the seek slider is dragged
        Function<void(bool)>  onLoopChanged;   // Called when the loop toggle changes
        Function<void(float)> onVolumeChanged; // Called with the volume 0..1 while the volume slider is dragged

        static const float height; // Height of the bar

    public:
        // Default constructor
        explicit PipelineMediaControls(RefCounter* refCounter);

        // Sets the play / pause icon from the playing state
        void SetPlaying(bool playing);

        // Sets the seek slider (unless it is being dragged) and the time text
        void SetTime(float time, float duration);

        // Sets the loop toggle
        void SetLoop(bool loop);

        // Returns true when the loop toggle is on
        bool IsLoop() const;

        // Sets the volume slider, 0..1
        void SetVolume(float volume);

        // Returns the volume slider value, 0..1
        float GetVolume() const;

        // Enables or disables every control
        void SetActive(bool active);

        // Returns true while the seek slider is dragged
        bool IsSeeking() const { return mSeeking; }

        // Returns the play / pause button
        const Ref<Button>& GetPlayButton() const { return mPlayButton; }

        // Returns the seek slider
        const Ref<HorizontalProgress>& GetSeekSlider() const { return mSeek; }

        // Updates the widget; ends the seek drag once the cursor is released
        void Update(float dt) override;

        SERIALIZABLE(PipelineMediaControls);

    protected:
        Ref<Button>             mPlayButton; // Play / pause button
        Ref<HorizontalProgress> mSeek;       // Seek slider over the clip duration
        Ref<WidgetLayer>        mTimeLayer;  // Elapsed / total time text layer
        Ref<Toggle>             mLoopToggle; // Loop toggle
        Ref<HorizontalProgress> mVolume;     // Volume slider, 0..1
        Ref<WidgetLayer>        mVolumeIcon; // Speaker icon before the volume slider

        float mDuration = 0.0f;  // Clip duration shown by the slider
        bool  mSeeking = false;  // True while the seek slider is dragged
        bool  mPlaying = false;  // Playing state shown by the button
    };

    // -------------------------------------------------------------------------------
    // Audio result: title line and a transport bar playing the clip through the engine
    // -------------------------------------------------------------------------------
    class PipelineAudioView : public Widget
    {
    public:
        static const float height; // Height of the view: title line plus the transport bar

    public:
        // Default constructor
        explicit PipelineAudioView(RefCounter* refCounter);

        // Destructor, stops the playback
        ~PipelineAudioView() override;

        // Sets the audio value and the title; a value without audio data clears the view
        void SetAudio(const PipelineValue& value, const String& title);

        // Sets the text shown in place of the title when there is no audio
        void SetHint(const String& hint);

        // Starts the playback, from the beginning when the clip has ended
        void Play();

        // Pauses the playback keeping the position
        void Pause();

        // Starts or pauses the playback
        void TogglePlay();

        // Moves the playback position to the time in seconds
        void Seek(float time);

        // Returns true while playing
        bool IsPlaying() const { return mPlaying; }

        // Returns the playback position in seconds
        float GetTime() const;

        // Returns the clip duration in seconds, 0 when unknown
        float GetDuration() const { return mDuration; }

        // Returns the transport bar
        const Ref<PipelineMediaControls>& GetControls() const { return mControls; }

        // Updates the player and the transport bar
        void Update(float dt) override;

        // Draws the player; zoomed far out only the title and info labels are drawn
        void Draw() override;

        SERIALIZABLE(PipelineAudioView);

    protected:
        Ref<Label>                 mTitleLabel; // Title or hint label
        Ref<Label>                 mInfoLabel;  // Format and size text at the right of the title
        Ref<PipelineMediaControls> mControls;   // Transport bar

        Ref<SoundPlayer> mPlayer;           // Player created on the first play or seek
        Ref<SoundAsset>  mSoundAsset;       // Sound asset built from the value data
        PipelineValue    mValue;            // Audio value
        bool             mHasAudio = false; // True when the value holds audio data
        bool             mPlaying = false;  // True while playing
        float            mDuration = 0.0f;  // Clip duration in seconds

    protected:
        // Creates the player from the value data; returns false when there is no audio
        bool EnsurePlayer();

        // Applies the loop and volume settings of the transport bar to the player
        void ApplyPlayerSettings();

        // Updates the transport bar from the player state
        void RefreshControls();
    };

    // ---------------------------------------------------------------------------------------
    // Video result: frames extracted with ffmpeg played from an atlas with the audio track,
    // under a transport bar; the extraction runs in a background thread and is cached
    // ---------------------------------------------------------------------------------------
    class PipelineVideoView : public Widget
    {
    public:
        // Default constructor
        explicit PipelineVideoView(RefCounter* refCounter);

        // Destructor, stops the playback
        ~PipelineVideoView() override;

        // Sets the video value; the preview is loaded from cacheDir or extracted into it
        void SetVideo(const PipelineValue& value, const String& cacheDir);

        // Sets the text shown when there is no video
        void SetHint(const String& hint);

        // Starts the playback, from the beginning when the clip has ended
        void Play();

        // Pauses the playback keeping the position
        void Pause();

        // Starts or pauses the playback
        void TogglePlay();

        // Moves the playback position to the time in seconds
        void Seek(float time);

        // Returns true while playing
        bool IsPlaying() const { return mPlaying; }

        // Returns the playback position in seconds
        float GetTime() const { return mTime; }

        // Returns the clip duration in seconds, 0 when the preview is not ready
        float GetDuration() const { return mInfo.duration; }

        // Returns true when a video value is set
        bool HasVideo() const { return mHasVideo; }

        // Returns true when the frames are loaded and can be played
        bool IsReady() const { return mReady; }

        // Returns the index of the frame shown now
        int GetFrameIndex() const;

        // Returns the transport bar
        const Ref<PipelineMediaControls>& GetControls() const { return mControls; }

        // Updates the extraction, the clock, the audio and the transport bar
        void Update(float dt) override;

        // Draws the player; zoomed far out only the frame is drawn
        void Draw() override;

        SERIALIZABLE(PipelineVideoView);

    protected:
        // -----------------------------------------------
        // Background extraction of the preview of a clip
        // -----------------------------------------------
        struct ExtractJob
        {
            String                      data;   // Clip bytes
            String                      dir;    // Preview folder
            PipelineVideo::PreviewInfo  result; // Extraction result, valid once done is set
            std::atomic<bool>           done { false }; // Set by the worker thread when result is ready
        };

        Ref<WidgetLayer>           mBackLayer;  // Dark back of the frame area
        Ref<WidgetLayer>           mFrameLayer; // Frame border
        Ref<Label>                 mHintLabel;  // Hint or status text over the frame area
        Ref<PipelineMediaControls> mControls;   // Transport bar

        PipelineValue              mValue;    // Video value
        String                     mCacheDir; // Preview folder of the value
        String                     mHint;     // Text shown when there is no video
        PipelineVideo::PreviewInfo mInfo;     // Loaded preview description
        TextureRef                 mAtlas;    // Frame atlas texture
        Ref<Sprite>                mFrame;    // Sprite drawing the current frame

        Ref<SoundPlayer> mPlayer;     // Audio track player, when the clip has audio
        Ref<SoundAsset>  mSoundAsset; // Sound asset of the audio track

        std::shared_ptr<ExtractJob> mJob; // Running extraction, null when idle

        bool  mHasVideo = false; // True when a video value is set
        bool  mReady = false;    // True when the atlas is loaded
        bool  mPlaying = false;  // True while playing
        float mTime = 0.0f;      // Playback position in seconds

    protected:
        // Starts the extraction thread for the current value
        void StartExtraction();

        // Loads the atlas and the audio track of the preview
        void ApplyPreview(const PipelineVideo::PreviewInfo& info);

        // Drops the atlas, the player and the extraction
        void ClearPreview();

        // Seeks the audio track to the clock, starting or stopping it with the playback
        void SyncAudio(bool seek);

        // Returns the rectangle of the frame area above the transport bar
        RectF GetFrameArea() const;

        // Draws the current frame fitted into the frame area
        void DrawFrame();

        // Updates the transport bar from the playback state
        void RefreshControls();
    };
}
// --- META ---

CLASS_BASES_META(Editor::PipelineMediaControls)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineMediaControls)
{
    FIELD().PUBLIC().NAME(onPlayPause);
    FIELD().PUBLIC().NAME(onSeek);
    FIELD().PUBLIC().NAME(onLoopChanged);
    FIELD().PUBLIC().NAME(onVolumeChanged);
    FIELD().PROTECTED().NAME(mPlayButton);
    FIELD().PROTECTED().NAME(mSeek);
    FIELD().PROTECTED().NAME(mTimeLayer);
    FIELD().PROTECTED().NAME(mLoopToggle);
    FIELD().PROTECTED().NAME(mVolume);
    FIELD().PROTECTED().NAME(mVolumeIcon);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mDuration);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSeeking);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPlaying);
}
END_META;
CLASS_METHODS_META(Editor::PipelineMediaControls)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPlaying, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTime, float, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLoop, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLoop);
    FUNCTION().PUBLIC().SIGNATURE(void, SetVolume, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetVolume);
    FUNCTION().PUBLIC().SIGNATURE(void, SetActive, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSeeking);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Button>&, GetPlayButton);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<HorizontalProgress>&, GetSeekSlider);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
}
END_META;

CLASS_BASES_META(Editor::PipelineAudioView)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineAudioView)
{
    FIELD().PROTECTED().NAME(mTitleLabel);
    FIELD().PROTECTED().NAME(mInfoLabel);
    FIELD().PROTECTED().NAME(mControls);
    FIELD().PROTECTED().NAME(mPlayer);
    FIELD().PROTECTED().NAME(mSoundAsset);
    FIELD().PROTECTED().NAME(mValue);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHasAudio);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPlaying);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mDuration);
}
END_META;
CLASS_METHODS_META(Editor::PipelineAudioView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAudio, const PipelineValue&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHint, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, Play);
    FUNCTION().PUBLIC().SIGNATURE(void, Pause);
    FUNCTION().PUBLIC().SIGNATURE(void, TogglePlay);
    FUNCTION().PUBLIC().SIGNATURE(void, Seek, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPlaying);
    FUNCTION().PUBLIC().SIGNATURE(float, GetTime);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<PipelineMediaControls>&, GetControls);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PROTECTED().SIGNATURE(bool, EnsurePlayer);
    FUNCTION().PROTECTED().SIGNATURE(void, ApplyPlayerSettings);
    FUNCTION().PROTECTED().SIGNATURE(void, RefreshControls);
}
END_META;

CLASS_BASES_META(Editor::PipelineVideoView)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineVideoView)
{
    FIELD().PROTECTED().NAME(mBackLayer);
    FIELD().PROTECTED().NAME(mFrameLayer);
    FIELD().PROTECTED().NAME(mHintLabel);
    FIELD().PROTECTED().NAME(mControls);
    FIELD().PROTECTED().NAME(mValue);
    FIELD().PROTECTED().NAME(mCacheDir);
    FIELD().PROTECTED().NAME(mHint);
    FIELD().PROTECTED().NAME(mInfo);
    FIELD().PROTECTED().NAME(mAtlas);
    FIELD().PROTECTED().NAME(mFrame);
    FIELD().PROTECTED().NAME(mPlayer);
    FIELD().PROTECTED().NAME(mSoundAsset);
    FIELD().PROTECTED().NAME(mJob);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHasVideo);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mReady);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPlaying);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mTime);
}
END_META;
CLASS_METHODS_META(Editor::PipelineVideoView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetVideo, const PipelineValue&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHint, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, Play);
    FUNCTION().PUBLIC().SIGNATURE(void, Pause);
    FUNCTION().PUBLIC().SIGNATURE(void, TogglePlay);
    FUNCTION().PUBLIC().SIGNATURE(void, Seek, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPlaying);
    FUNCTION().PUBLIC().SIGNATURE(float, GetTime);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(bool, HasVideo);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsReady);
    FUNCTION().PUBLIC().SIGNATURE(int, GetFrameIndex);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<PipelineMediaControls>&, GetControls);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PROTECTED().SIGNATURE(void, StartExtraction);
    FUNCTION().PROTECTED().SIGNATURE(void, ApplyPreview, const PipelineVideo::PreviewInfo&);
    FUNCTION().PROTECTED().SIGNATURE(void, ClearPreview);
    FUNCTION().PROTECTED().SIGNATURE(void, SyncAudio, bool);
    FUNCTION().PROTECTED().SIGNATURE(RectF, GetFrameArea);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawFrame);
    FUNCTION().PROTECTED().SIGNATURE(void, RefreshControls);
}
END_META;
// --- END META ---
