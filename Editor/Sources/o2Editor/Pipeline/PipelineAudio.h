#pragma once

#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // --------------------------------------------------------
    // Audio helpers: PCM packaging and ffmpeg based processing
    // --------------------------------------------------------
    namespace PipelineAudio
    {
        // Wraps raw 16-bit little-endian PCM into a WAV container
        String PcmToWav(const String& pcm, int sampleRate, int channels);

        // Sample rate from a mime like "audio/L16;codec=pcm;rate=24000" (24000 default)
        int PcmRateFromMime(const String& mime);

        // Processing settings; "keep" and 0 leave the source property unchanged
        struct ProcessOptions
        {
            String format = "keep";         // keep / wav / ogg / mp3
            int    sampleRate = 0;          // Output sample rate, 0 = keep
            int    channels = 0;            // 0 = keep, 1 mono, 2 stereo
            bool   trimSilence = false;     // Cut silence below -50 dB from both ends
            bool   normalize = false;       // Loudness normalization to loudnessTarget
            float  loudnessTarget = -14.0f; // Integrated loudness in LUFS for normalize
            bool   seamlessLoop = false;    // Crossfade the tail into the head so the clip loops without a click
            float  crossfadeMs = 250.0f;    // Crossfade length of seamlessLoop in milliseconds
            float  fadeInMs = 0.0f;         // Fade in length in milliseconds, 0 = none
            float  fadeOutMs = 0.0f;        // Fade out length in milliseconds, 0 = none
        };

        // Outcome of Process
        struct ProcessResult
        {
            bool   ok = false;            // Output clip was produced
            bool   ffmpegMissing = false; // ffmpeg is not installed, nothing was processed
            String error;                 // Failure reason when ok is false
            String data;                  // Output clip bytes
            String mimeType;              // Mime type of the output clip
        };

        // Runs ffmpeg over the clip. ffmpegMissing is set when the tool is not installed
        ProcessResult Process(const String& data, const String& mimeType, const ProcessOptions& options);

        // Clip duration in seconds through ffprobe, 0 when unavailable
        float DurationSeconds(const String& data, const String& mimeType);

        // Returns the path of an ffmpeg tool ("ffmpeg", "ffprobe"): a known install folder or the bare name
        String ToolPath(const char* name);

        // Returns a unique file path with the extension inside the pipelines temp folder
        String TempFilePath(const String& ext);

        // True when ffmpeg can be launched; probed once per session, always false on mobile and wasm
        bool IsFfmpegAvailable();
    }
}
