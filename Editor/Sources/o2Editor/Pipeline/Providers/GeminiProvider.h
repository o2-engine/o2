#pragma once

#include "o2Editor/Pipeline/Providers/AiHttp.h"

using namespace o2;

namespace Editor
{
    // Image attached to a model request as inline data
    struct AiImageRef
    {
        String mimeType; // MIME type of the encoded image, image/png when empty
        String data;     // Encoded image bytes
    };

    // Binary result of a provider call: image, audio or video bytes
    struct AiBytesResult
    {
        bool   ok = false;  // True when the call produced data
        String error;       // Error description when not ok
        String data;        // Raw bytes of the result
        String mimeType;    // MIME type of data
        int    seconds = 0; // Clip length in seconds, set by the video providers, 0 when unknown
    };

    // Text result of a provider call
    struct AiTextResult
    {
        bool   ok = false; // True when the call produced text
        String error;      // Error description when not ok
        String text;       // Generated text, trimmed
    };

    // Model entry of the Gemini models list
    struct GeminiModelInfo
    {
        String         name;        // Model id without the "models/" prefix
        String         displayName; // Human readable model name
        Vector<String> methods;     // Supported generation methods, e.g. generateContent
    };

    // Result of the models list request
    struct GeminiModelsResult
    {
        bool                    ok = false; // True when the list was fetched
        String                  error;      // Error description when not ok
        Vector<GeminiModelInfo> models;     // Models available to the API key
    };

    // ----------------------------------------------------------------------
    // Google Gemini REST API: text, images (Gemini image models and Imagen),
    // speech (TTS) and music (Lyria via the Interactions API)
    // ----------------------------------------------------------------------
    namespace GeminiProvider
    {
        const String defaultImageModel = "gemini-3.1-flash-image";     // Image model used when the node names none
        const String defaultTextModel = "gemini-pro-latest";           // Text model used when the node names none
        const String defaultTtsModel = "gemini-2.5-flash-preview-tts"; // Speech model used when the node names none
        const String defaultMusicModel = "lyria-3-clip-preview";       // Music model used when the node names none

        extern const Vector<String> voices; // Names of the prebuilt Gemini TTS voices

        // Generates text from the prompt; images are attached to the request as inline parts
        Coroutine<AiTextResult> GenerateText(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& model,
                                             const String& prompt, const Vector<AiImageRef>& images);

        // Generates an image from the prompt and references; Imagen models go through the predict endpoint, seed < 0 is random
        Coroutine<AiBytesResult> GenerateImage(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& model,
                                               const String& prompt, const Vector<AiImageRef>& references, int seed);

        // Speaks the text with a prebuilt voice; style instructions are prepended to the text, PCM answers are wrapped into wav
        Coroutine<AiBytesResult> GenerateSpeech(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& model,
                                                const String& text, const String& voice, const String& styleInstructions);

        // Generates a music clip from the prompt through the Lyria interactions endpoint
        Coroutine<AiBytesResult> GenerateMusic(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& model,
                                               const String& prompt);

        // Fetches every model available to the API key, following the pagination
        Coroutine<GeminiModelsResult> ListModels(const String& apiKey);
    }
}
