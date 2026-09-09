#pragma once

#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // -----------------------------------------------------------------------
    // Video helpers: preview frames and the audio track extracted with ffmpeg
    // -----------------------------------------------------------------------
    namespace PipelineVideo
    {
        // Preview of a clip: a grid atlas of scaled frames and the audio track next to it
        struct PreviewInfo
        {
            bool   ok = false;       // True when the atlas exists and the meta was read
            String error;            // Failure reason when ok is false
            int    frameWidth = 0;   // Width of one frame in the atlas
            int    frameHeight = 0;  // Height of one frame in the atlas
            int    frameCount = 0;   // Number of frames in the atlas
            int    columns = 0;      // Frames per atlas row
            float  fps = 0.0f;       // Frame rate of the extracted frames
            float  duration = 0.0f;  // Clip duration in seconds
            bool   hasAudio = false; // True when the clip has an audio track
            String atlasPath;        // Atlas PNG path
            String audioPath;        // Audio track WAV path, empty when there is none
        };

        // Returns the preview stored in dir, extracting it from the clip bytes with ffmpeg when it is missing
        PreviewInfo BuildPreview(const String& videoData, const String& dir, int frameWidth = 256, int maxFrames = 200);

        // Reads the preview stored in dir; ok is false when there is none
        PreviewInfo LoadPreview(const String& dir);

        // Returns the atlas rectangle of the frame in y-up texture coordinates
        RectI FrameRect(const PreviewInfo& info, int frame);
    }
}
