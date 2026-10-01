#pragma once

#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/Pipeline/Providers/GeminiProvider.h"

using namespace o2;

namespace Editor
{
    // API a text or image model is called through
    enum class AiProvider { Gemini, OpenAi, OpenRouter };

    // One way to reach a model: the provider and the id the provider knows the model by
    struct AiRoute
    {
        AiProvider provider = AiProvider::Gemini; // API the call goes to
        String     model;                         // Model id as that API names it

        bool operator==(const AiRoute& other) const { return provider == other.provider && model == other.model; }
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
    // the native provider: vendor/model is OpenRouter, gpt-, o3, chatgpt- and
    // dall-e ids are OpenAI, any other is Gemini; a leading models/ is the long
    // Gemini id form. Some models have a second route through another provider
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
// --- END META ---
