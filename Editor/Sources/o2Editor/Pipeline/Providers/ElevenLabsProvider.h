#pragma once

#include "o2Editor/Pipeline/Providers/GeminiProvider.h"

using namespace o2;

namespace Editor
{
    // Voice entry of the ElevenLabs voices list
    struct ElevenVoiceInfo
    {
        String voiceId;  // Voice id passed to the text-to-speech endpoint
        String name;     // Display name of the voice
        String category; // Voice category reported by the API, e.g. premade or cloned
    };

    // Result of the voices list request
    struct ElevenVoicesResult
    {
        bool                    ok = false; // True when the list was fetched
        String                  error;      // Error description when not ok
        Vector<ElevenVoiceInfo> voices;     // Voices available to the API key
    };

    // --------------------------------------------------------
    // ElevenLabs REST API: sound effects and expressive speech
    // --------------------------------------------------------
    namespace ElevenLabsProvider
    {
        const String sfxModel = "eleven_text_to_sound_v2";       // Sound effects model
        const String defaultTtsModel = "eleven_multilingual_v2"; // Speech model used when the node names none
        const int    sfxMaxPromptChars = 450;                    // Hard prompt length limit of the sound effects endpoint
        const float  sfxMinSeconds = 0.5f;                       // Shortest sound effect the endpoint renders
        const float  sfxMaxSeconds = 30.0f;                      // Longest sound effect the endpoint renders

        // Renders a sound effect from text; negative duration or promptInfluence keeps the endpoint defaults
        Coroutine<AiBytesResult> GenerateSoundEffect(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& text,
                                                     const String& model, float durationSeconds, bool loop,
                                                     float promptInfluence, const String& outputFormat);

        // Speaks the text with the voice; an empty voiceId falls back to the default voice
        Coroutine<AiBytesResult> GenerateSpeech(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& text,
                                                const String& voiceId, const String& model, const String& outputFormat);

        // Fetches the voices available to the API key
        Coroutine<ElevenVoicesResult> ListVoices(const String& apiKey);

        // Returns the MIME type of a clip in the given ElevenLabs output format
        String MimeForOutputFormat(const String& outputFormat);
    }
}
