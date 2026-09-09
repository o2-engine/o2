#pragma once

#include "o2Editor/Pipeline/Providers/GeminiProvider.h"

using namespace o2;

namespace Editor
{
    // Request of a video generation call
    struct VideoGenerateInput
    {
        String             prompt;               // Text prompt describing the clip
        String             model;                // Model id, provider default when empty
        Vector<AiImageRef> references;           // Reference images guiding the clip, provider limits apply
        String             aspectRatio = "16:9"; // Requested aspect ratio, snapped to what the model supports
        int                durationSeconds = 8;  // Requested clip length, snapped to what the model supports
    };

    // Google Veo through the long-running predict endpoint with polling
    namespace VeoProvider
    {
        const String defaultModel = "veo-3.1-fast-generate-preview"; // Model used when the input names none

        // Rounds the requested length to one the model renders; references force 8 seconds on Veo 3
        int SnapDuration(const String& model, int requested, bool withReferences);

        // Returns 9:16 when requested, 16:9 otherwise
        String SnapAspectRatio(const String& requested);

        // Starts the generation, polls the operation until it is done and downloads the mp4
        Coroutine<AiBytesResult> GenerateVideo(const Ref<PipelineExecContext>& ctx, const String& apiKey, const VideoGenerateInput& input);
    }

    // Kling (Kuaishou) task based API with JWT or API key auth
    namespace KlingProvider
    {
        extern const Vector<String> models; // Supported Kling model ids, the first one is the default

        // Builds an HS256 JWT from the access and secret keys, valid for 30 minutes
        String MakeJwt(const String& accessKey, const String& secretKey);

        // Returns the bearer token: the access key itself for "api-key-" keys, a fresh JWT otherwise
        String Bearer(const String& accessKey, const String& secretKey);

        // Creates the video task, polls it until it succeeds and downloads the mp4
        Coroutine<AiBytesResult> GenerateVideo(const Ref<PipelineExecContext>& ctx, const String& accessKey, const String& secretKey,
                                               const VideoGenerateInput& input);
    }
}
