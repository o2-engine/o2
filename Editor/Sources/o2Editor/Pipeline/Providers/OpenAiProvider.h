#pragma once

#include "o2Editor/Pipeline/Providers/GeminiProvider.h"

using namespace o2;

namespace Editor
{
    // Body of a multipart/form-data request
    struct AiMultipartBody
    {
        String body;        // Encoded parts, binary
        String boundary;    // Boundary between the parts
        String contentType; // Value of the Content-Type header, names the boundary
    };

    // ------------------------------------------------------------------------
    // OpenAI REST API: text through chat completions, images through the image
    // endpoints (generations, and edits when references are given)
    // ------------------------------------------------------------------------
    namespace OpenAiProvider
    {
        const String baseUrl = "https://api.openai.com/v1"; // Root of the REST API
        const String imageSize = "1024x1024";               // Size asked from the image endpoints

        // Generates text from the prompt; images are attached to the user message as data URL parts
        Coroutine<AiTextResult> GenerateText(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& model,
                                             const String& prompt, const Vector<AiImageRef>& images);

        // Generates an image: generations without references, edits with them. The endpoints take no seed, it is ignored;
        // transparentBackground asks the model to render the background transparent
        Coroutine<AiBytesResult> GenerateImage(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& model,
                                               const String& prompt, const Vector<AiImageRef>& references, int seed,
                                               bool transparentBackground = false, const AiImageOptions& options = AiImageOptions());

        // Returns the frame the model renders nearest the asked one: 1536x1024, 1024x1536 or 1024x1024; DALL-E always 1024x1024
        String SizeFor(const String& model, const String& aspectRatio);

        // Builds the chat completions body
        void BuildChatBody(DataDocument& body, const String& model, const String& prompt, const Vector<AiImageRef>& images);

        // Builds the images/generations body
        void BuildGenerationsBody(DataDocument& body, const String& model, const String& prompt, bool transparentBackground = false,
                                  const String& size = "");

        // Builds the images/edits form: the text fields and one image[] file per reference; an empty boundary gets a random one
        AiMultipartBody BuildEditsBody(const String& model, const String& prompt, const Vector<AiImageRef>& references,
                                       const String& boundary = "", bool transparentBackground = false, const String& size = "");

        // Returns the message of the error object of a response body, empty when the body carries none
        String ParseError(const DataValue& json);

        // Returns true when the error of the body says the account is out of credits
        bool IsQuotaError(const DataValue& json);

        // Returns the note that explains a failed status, empty when there is nothing to add
        String StatusHint(int status, const DataValue& json);

        // Reads the text of a chat completion: a string content or the text parts of an array content
        AiTextResult ParseTextResponse(const DataValue& json, const String& model);

        // Reads the first image of an image answer; an answer that holds an address instead of the bytes gives it in downloadUrl
        AiBytesResult ParseImageResponse(const DataValue& json, const String& model, String& downloadUrl);
    }
}
