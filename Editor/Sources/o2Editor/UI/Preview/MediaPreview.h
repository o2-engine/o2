#pragma once

#include "o2/Assets/AssetRef.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Types/Containers/Vector.h"

using namespace o2;

namespace o2
{
    class SoundAsset;
    class VideoAsset;
}

namespace Editor
{
    // -------------------------------------------------------------------------------------------------------
    // Previews of sounds and videos: the waveform peaks of a sound, decoded once in a background thread, the
    // waveform bars as the pipeline audio player draws them, and the first frame of a video
    // -------------------------------------------------------------------------------------------------------
    namespace MediaPreview
    {
        // Returns the waveform peaks of the sound, 0..1; empty while they are decoded
        Vector<float> GetPeaks(const AssetRef<SoundAsset>& sound);

        // Returns the peaks of the sound, waiting for the decoding
        Vector<float> WaitPeaks(const AssetRef<SoundAsset>& sound);

        // Draws the peaks as bars of the width with the gap, the played part by its color; a line without peaks
        void DrawWaveform(const RectF& rect, const Vector<float>& peaks, float progress, const Color4& played,
                          const Color4& idle, float barWidth = 2.0f, float gap = 1.0f);

        // Returns a frame of the start of the video, made once per asset; empty when it doesn't decode
        TextureRef GetVideoThumbnail(const AssetRef<VideoAsset>& video);

        // Returns the time as the players show it: tenths of a second for a clip shorter than a minute
        String FormatTime(float seconds, float duration);

        // Returns the description of the sound: duration, sample rate, channels and size
        String GetSoundInfo(const SoundAsset& sound);

        // Returns the description of the video: frame size, frame rate, duration and size
        String GetVideoInfo(const VideoAsset& video);
    }
}
