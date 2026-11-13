#pragma once

#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/Pipeline/Providers/GeminiProvider.h"

using namespace o2;

namespace Editor
{
    // API a text or image model is called through
    enum class AiProvider { Gemini, OpenAi, OpenRouter, Local };

    // One way to reach a model: the provider and the id the provider knows the model by
    struct AiRoute
    {
        AiProvider provider = AiProvider::Gemini; // API the call goes to
        String     model;                         // Model id as that API names it

        bool operator==(const AiRoute& other) const { return provider == other.provider && model == other.model; }
    };

    // Where a call runs: on a provider directly, through AssetsLine, or nowhere
    enum class AiRouteKind { Direct, AssetsLine, None };

    // How one call runs
    struct AiPlan
    {
        AiRouteKind kind = AiRouteKind::None; // Where the call goes
        AiRoute     route;                    // Provider and model id of a direct call
        String      error;                    // Why the call cannot run, kind None only
    };

    // Keys of the providers the router chooses between
    struct AiRouteKeys
    {
        String gemini;     // Gemini API key, empty when not configured
        String openAi;     // OpenAI API key, empty when not configured
        String openRouter; // OpenRouter API key, empty when not configured

        // Returns the key of the provider
        const String& Of(AiProvider provider) const;
    };

    // ------------------------------------------------------------------------
    // Picks the provider of a text or image call by the model id. The id names
    // the native provider: local: ids are the local agents (no key, no second
    // route), vendor/model is OpenRouter, gpt-, o3, chatgpt- and dall-e ids
    // are OpenAI, any other is Gemini; a leading models/ is the long Gemini
    // id form. Some models have a second route through another provider
    // under a rewritten id, taken when the native provider has no key
    // ------------------------------------------------------------------------
    namespace AiRouter
    {
        // Returns the provider the model id belongs to
        AiProvider FamilyOf(const String& model);

        // True when the model id names an OpenRouter model
        bool IsOpenRouterModel(const String& model);

        // Returns the routes of the model in the order they are tried: the native one, then the alternative when there is one.
        // A call for a transparent background keeps only the routes whose own id renders it
        Vector<AiRoute> RoutesFor(const String& model, bool transparentBackground = false);

        // Returns the name of the provider as the messages show it
        String ProviderName(AiProvider provider);

        // Returns the effective keys of the settings
        AiRouteKeys KeysOf(const PipelineSettings& settings);

        // Returns the first route that has a key, the native route when none has; note gets the log line of a non-native choice
        AiRoute ChooseRoute(const String& model, const AiRouteKeys& keys, String* note = nullptr, bool transparentBackground = false);

        // What a node shows for a call with neither a key here nor a connection to AssetsLine
        String NoKeyMessage(const String& model);

        // How a text or image call runs: a local agent here; a route with a key of this editor directly; with no key and a
        // connection, through AssetsLine, billed on the connected account and project. A key that fails never falls back to
        // AssetsLine, which would spend its tokens unasked
        AiPlan Plan(const String& model, const AiRouteKeys& keys, bool connected, bool transparentBackground = false);

        // How a call runs that a node makes to its provider itself (video, speech, sound effects, music): on the key of that
        // provider, else through AssetsLine when connected
        AiPlan PlanDirect(const String& model, bool hasKey, bool connected);

        // The plan with the keys of the context settings and the AssetsLine connection
        AiPlan Plan(const Ref<PipelineExecContext>& ctx, const String& model, bool transparentBackground = false);

        // The plan of a node's own provider call with the AssetsLine connection
        AiPlan PlanDirect(const String& model, bool hasKey);

        // The log line of a direct call on a key of this editor
        String DirectRouteLine(const String& providerName);

        // Generates text on the route chosen for the model, with the keys from the context settings
        Coroutine<AiTextResult> GenerateText(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                                             const Vector<AiImageRef>& images);

        // Stand-in for the image providers: while set, every image request goes to it; for tests
        using ImageStub = Function<AiBytesResult(const String& model, const String& prompt, const Vector<AiImageRef>& references,
                                                 int seed, bool transparentBackground, const AiImageOptions& options)>;

        // Sets or, with an empty function, clears the stand-in for the image providers
        void SetImageStub(const ImageStub& stub);

        // Generates an image on the route chosen for the model, with the keys from the context settings, seed < 0 is random;
        // transparentBackground asks the model for a transparent background, Gemini ignores it; options ask for the render size
        // and the frame
        Coroutine<AiBytesResult> GenerateImage(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                                               const Vector<AiImageRef>& references, int seed, bool transparentBackground = false,
                                               const AiImageOptions& options = AiImageOptions());
    }
}
// --- META ---

PRE_ENUM_META(Editor::AiProvider);

PRE_ENUM_META(Editor::AiRouteKind);
// --- END META ---
