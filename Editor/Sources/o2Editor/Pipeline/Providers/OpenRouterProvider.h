#pragma once

#include "o2Editor/Pipeline/Providers/GeminiProvider.h"

using namespace o2;

namespace Editor
{
    // -----------------------------------------------------------------------
    // OpenRouter: text and images through the chat completions API (OpenAI
    // compatible) on any model of the catalogue, named by a vendor/model id,
    // and images through the images API, which also serves the image-only
    // models and renders a transparent background when asked
    // -----------------------------------------------------------------------
    namespace OpenRouterProvider
    {
        const String baseUrl = "https://openrouter.ai/api/v1"; // Root of the REST API
        const String referer = "https://assetsline.app";       // Sent as HTTP-Referer, names the app in the OpenRouter stats
        const String title = "AssetsLine o2 editor";           // Sent as X-Title

        // Generates text from the prompt; images are attached to the user message as data URL parts
        Coroutine<AiTextResult> GenerateText(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& model,
                                             const String& prompt, const Vector<AiImageRef>& images);

        // Generates an image from the prompt and references, seed < 0 is random. A transparent background goes through
        // the images API; any other call through chat completions and, when the model is an image-only one, the images API
        Coroutine<AiBytesResult> GenerateImage(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& model,
                                               const String& prompt, const Vector<AiImageRef>& references, int seed,
                                               bool transparentBackground = false, const AiImageOptions& options = AiImageOptions());

        // Builds the chat completions body; withImage asks for the image modality and passes the seed, and the frame and the
        // render size (Gemini 3 image models only) as image_config
        void BuildRequestBody(DataDocument& body, const String& model, const String& prompt, const Vector<AiImageRef>& images,
                              bool withImage, int seed, const AiImageOptions& options = AiImageOptions());

        // Builds the body of the images API: no seed and no size, references as data URLs
        void BuildImagesBody(DataDocument& body, const String& model, const String& prompt, const Vector<AiImageRef>& references,
                             bool transparentBackground);

        // Returns the message of the error object of a response body, empty when the body carries none
        String ParseError(const DataValue& json);

        // Reads the text of a chat completion: a string content or the text parts of an array content
        AiTextResult ParseTextResponse(const DataValue& json, const String& model);

        // Reads the first image of a chat completion, from message.images or from the image parts of the content
        AiBytesResult ParseImageResponse(const DataValue& json, const String& model);

        // Reads the first image of an images API answer
        AiBytesResult ParseImagesResponse(const DataValue& json, const String& model);
    }
}
