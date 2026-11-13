#pragma once

#include "o2Editor/Pipeline/Providers/GeminiProvider.h"
#include "o2Editor/Pipeline/Providers/VideoProviders.h"
#include "o2Editor/Pipeline/Sync/AssetsLineClient.h"

using namespace o2;

namespace Editor
{
    // ------------------------------------------------------------------------
    // One answer of an AssetsLine AI call: the body and the billing headers on
    // a success, the message a node shows otherwise
    // ------------------------------------------------------------------------
    struct AssetsLineAiAnswer
    {
        bool      ok = false;         // The call ran
        int       status = 0;         // HTTP status, 0 without an answer
        String    error;              // Readable message for the node
        String    code;               // out_of_tokens, route_blocked, bad_request, refused, provider_error
        String    topUpUrl;           // Where the balance is topped up, named by a 402
        String    accountUrl;         // The page where a blocked route is switched on, named by a 403
        String    body;               // Result bytes, or the JSON of a text call
        String    mimeType;           // Content type of the body without parameters
        long long spent = 0;          // X-AssetsLine-Spent: tokens the call cost
        long long balance = -1;       // X-AssetsLine-Balance: tokens left, -1 when not reported
        String    route;              // X-AssetsLine-Route: what the call ran on
        int       seconds = 0;        // X-AssetsLine-Seconds of a clip
    };

    // ------------------------------------------------------------------------
    // AI calls through AssetsLine (POST /api/sync/ai/*) for an editor connected
    // to an AssetsLine project: the call runs and is billed on the connected
    // account and project by AssetsLine's own routing, on its tokens or on the
    // keys saved there. No key of this editor is ever part of a request
    // ------------------------------------------------------------------------
    namespace AssetsLineAi
    {
        extern const String outOfTokens;  // Start of the message of a call refused for the balance
        extern const String connectToRun; // Message of a call that needs the connection

        // Gives the client of the calls; false while not connected
        using ClientSource = Function<bool(AssetsLineClient& client)>;

        // Replaces where the client comes from (tests); an empty function restores the sync's connection
        void SetClientSource(const ClientSource& source);

        // Gives the client of the calls: the sync's while it is connected, never in a test sandbox
        bool GetClient(AssetsLineClient& client);

        // True when a call can run through AssetsLine
        bool IsConnected();

        // Body of POST /ai/text
        DataDocument TextBody(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                              const Vector<AiImageRef>& images);

        // Body of POST /ai/image; seed < 0 leaves it to the model
        DataDocument ImageBody(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                               const Vector<AiImageRef>& references, int seed, bool transparentBackground,
                               const AiImageOptions& options);

        // Body of POST /ai/video; provider veo or kling
        DataDocument VideoBody(const Ref<PipelineExecContext>& ctx, const String& provider, const VideoGenerateInput& input);

        // Body of POST /ai/speech; provider gemini or elevenlabs
        DataDocument SpeechBody(const Ref<PipelineExecContext>& ctx, const String& provider, const String& model,
                                const String& text, const String& voice, const String& styleInstructions);

        // Body of POST /ai/sfx; a negative duration or influence leaves it to the model
        DataDocument SfxBody(const Ref<PipelineExecContext>& ctx, const String& model, const String& text, float durationSeconds,
                             bool loop, float promptInfluence);

        // Body of POST /ai/music
        DataDocument MusicBody(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt);

        // The message a node shows for an error answer of AssetsLine
        String ErrorMessage(int status, const String& code, const String& serverMessage);

        // Reads an answer: the billing headers on every one, the mapped error on a failure
        AssetsLineAiAnswer ReadAnswer(const AssetsLineResponse& response, const String& serverUrl);

        // The log line of a call that ran
        String RouteLine(const AssetsLineAiAnswer& answer);

        // Posts a call to /api/sync/ai/<kind> and reads the answer
        Coroutine<AssetsLineAiAnswer> Call(const Ref<PipelineExecContext>& ctx, const String& kind, const DataDocument& body);

        // Text through AssetsLine
        Coroutine<AiTextResult> GenerateText(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                                             const Vector<AiImageRef>& images);

        // An image through AssetsLine
        Coroutine<AiBytesResult> GenerateImage(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                                               const Vector<AiImageRef>& references, int seed, bool transparentBackground,
                                               const AiImageOptions& options);

        // A clip through AssetsLine; provider veo or kling
        Coroutine<AiBytesResult> GenerateVideo(const Ref<PipelineExecContext>& ctx, const String& provider, const VideoGenerateInput& input);

        // Speech through AssetsLine; provider gemini or elevenlabs
        Coroutine<AiBytesResult> GenerateSpeech(const Ref<PipelineExecContext>& ctx, const String& provider, const String& model,
                                                const String& text, const String& voice, const String& styleInstructions);

        // A sound effect through AssetsLine
        Coroutine<AiBytesResult> GenerateSoundEffect(const Ref<PipelineExecContext>& ctx, const String& model, const String& text,
                                                     float durationSeconds, bool loop, float promptInfluence);

        // Music through AssetsLine
        Coroutine<AiBytesResult> GenerateMusic(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt);
    }
}
