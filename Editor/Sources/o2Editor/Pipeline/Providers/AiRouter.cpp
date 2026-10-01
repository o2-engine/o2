#include "o2Editor/stdafx.h"
#include "AiRouter.h"

#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/Providers/OpenAiProvider.h"
#include "o2Editor/Pipeline/Providers/OpenRouterProvider.h"

namespace Editor
{
    const String& AiRouteKeys::Of(AiProvider provider) const
    {
        return provider == AiProvider::OpenAi ? openAi : provider == AiProvider::OpenRouter ? openRouter : gemini;
    }
}

namespace Editor::AiRouter
{
    // Model id trimmed and without the models/ prefix of the long Gemini form
    static String BareModelId(const String& modelIn)
    {
        String model = modelIn.Trimed(" \n\r\t");
        return model.StartsWith("models/") ? model.SubStr(7) : model;
    }

    AiProvider FamilyOf(const String& model)
    {
        String id = BareModelId(model);
        if (id.Contains("/"))
            return AiProvider::OpenRouter;

        return PipelineUtils::IsOpenAiModelId(id) ? AiProvider::OpenAi : AiProvider::Gemini;
    }

    bool IsOpenRouterModel(const String& model)
    {
        return FamilyOf(model) == AiProvider::OpenRouter;
    }

    // The route through the other API, false when that API has no such model
    static bool AlternativeOf(AiProvider family, const String& id, AiRoute& alternative)
    {
        String lower = id.ToLowerCase();
        if (lower.Contains(":"))
            return false;

        if (family == AiProvider::Gemini)
        {
            if (!lower.StartsWith("gemini-") || lower.EndsWith("-latest"))
                return false;

            alternative = { AiProvider::OpenRouter, "google/" + id };
            return true;
        }

        if (family == AiProvider::OpenAi)
        {
            if (lower.StartsWith("dall-e"))
                return false;

            alternative = { AiProvider::OpenRouter, "openai/" + id };
            return true;
        }

        int slash = id.Find("/");
        String vendor = lower.SubStr(0, slash);
        String name = id.SubStr(slash + 1);
        String lowerName = name.ToLowerCase();
        if (vendor == "google" && (lowerName.StartsWith("gemini-") || lowerName.StartsWith("gemma-")))
        {
            alternative = { AiProvider::Gemini, name };
            return true;
        }

        // A composite such as openai/gpt-5-image exists at OpenRouter only
        bool composite = lowerName.Contains("-image") && !lowerName.StartsWith("gpt-image-");
        if (vendor == "openai" && !composite && !name.IsEmpty())
        {
            alternative = { AiProvider::OpenAi, name };
            return true;
        }

        return false;
    }

    Vector<AiRoute> RoutesFor(const String& model, bool transparentBackground /*= false*/)
    {
        String id = BareModelId(model);
        AiProvider family = FamilyOf(id);
        Vector<AiRoute> routes = { { family, id } };
        AiRoute alternative;
        if (AlternativeOf(family, id, alternative))
            routes.Add(alternative);

        if (transparentBackground)
            routes.RemoveAll([](const AiRoute& route) { return !PipelineTransparency::SupportsNativeTransparency(route.model); });

        return routes;
    }

    String ProviderName(AiProvider provider)
    {
        return provider == AiProvider::OpenAi ? "OpenAI" : provider == AiProvider::OpenRouter ? "OpenRouter" : "Gemini";
    }

    AiRouteKeys KeysOf(const PipelineSettings& settings)
    {
        return { settings.GetGeminiKey(), settings.GetOpenAiKey(), settings.GetOpenRouterKey() };
    }

    AiRoute ChooseRoute(const String& model, const AiRouteKeys& keys, String* note /*= nullptr*/,
                        bool transparentBackground /*= false*/)
    {
        AiRoute native = RoutesFor(model)[0];
        for (auto& route : RoutesFor(model, transparentBackground))
        {
            if (keys.Of(route.provider).IsEmpty())
                continue;

            if (note && route.provider != native.provider)
            {
                *note = native.model + " runs on " + ProviderName(route.provider) + " as " + route.model +
                    " (no " + ProviderName(native.provider) + " key)";
            }
            return route;
        }

        return native;
    }

    Coroutine<AiTextResult> GenerateText(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                                         const Vector<AiImageRef>& images)
    {
        AiRouteKeys keys = KeysOf(ctx->settings);
        String note;
        AiRoute route = ChooseRoute(model, keys, &note);
        if (!note.IsEmpty())
            ctx->Log(note);

        AiTextResult result;
        if (route.provider == AiProvider::OpenRouter)
            result = co_await OpenRouterProvider::GenerateText(ctx, keys.openRouter, route.model, prompt, images);
        else if (route.provider == AiProvider::OpenAi)
            result = co_await OpenAiProvider::GenerateText(ctx, keys.openAi, route.model, prompt, images);
        else
            result = co_await GeminiProvider::GenerateText(ctx, keys.gemini, route.model, prompt, images);
        co_return result;
    }

    static ImageStub imageStub;

    void SetImageStub(const ImageStub& stub)
    {
        imageStub = stub;
    }

    Coroutine<AiBytesResult> GenerateImage(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                                           const Vector<AiImageRef>& references, int seed, bool transparentBackground /*= false*/,
                                           const AiImageOptions& options /*= AiImageOptions()*/)
    {
        if (imageStub)
            co_return imageStub(model, prompt, references, seed, transparentBackground, options);

        AiRouteKeys keys = KeysOf(ctx->settings);
        String note;
        AiRoute route = ChooseRoute(model, keys, &note, transparentBackground);
        if (!note.IsEmpty())
            ctx->Log(note);

        AiBytesResult result;
        if (route.provider == AiProvider::OpenRouter)
            result = co_await OpenRouterProvider::GenerateImage(ctx, keys.openRouter, route.model, prompt, references, seed, transparentBackground, options);
        else if (route.provider == AiProvider::OpenAi)
            result = co_await OpenAiProvider::GenerateImage(ctx, keys.openAi, route.model, prompt, references, seed, transparentBackground, options);
        else
            result = co_await GeminiProvider::GenerateImage(ctx, keys.gemini, route.model, prompt, references, seed, options);
        co_return result;
    }
}
// --- META ---

ENUM_META(Editor::AiProvider, Editor__AiProvider)
{
    ENUM_ENTRY(Gemini);
    ENUM_ENTRY(OpenAi);
    ENUM_ENTRY(OpenRouter);
}
END_ENUM_META;
// --- END META ---
