#include "o2Editor/stdafx.h"
#include "AiRouter.h"

#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/Providers/AssetsLineAi.h"
#include "o2Editor/Pipeline/Providers/LocalAgentProvider.h"
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
        if (LocalAgentProvider::IsLocalModel(id))
            return AiProvider::Local;

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
        if (family == AiProvider::Local || lower.Contains(":"))
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
        switch (provider)
        {
            case AiProvider::OpenAi: return "OpenAI";
            case AiProvider::OpenRouter: return "OpenRouter";
            case AiProvider::Local: return "Local agent";
            default: return "Gemini";
        }
    }

    AiRouteKeys KeysOf(const PipelineSettings& settings)
    {
        return { settings.GetGeminiKey(), settings.GetOpenAiKey(), settings.GetOpenRouterKey() };
    }

    AiRoute ChooseRoute(const String& model, const AiRouteKeys& keys, String* note /*= nullptr*/,
                        bool transparentBackground /*= false*/)
    {
        AiRoute native = RoutesFor(model)[0];
        if (native.provider == AiProvider::Local)
            return native;

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

    String NoKeyMessage(const String& model)
    {
        return "No API key for " + model + ": add one in the pipeline settings, or connect AssetsLine to run it on your AssetsLine account";
    }

    AiPlan Plan(const String& model, const AiRouteKeys& keys, bool connected, bool transparentBackground /*= false*/)
    {
        AiPlan plan;
        AiRoute native = RoutesFor(model)[0];
        if (native.provider == AiProvider::Local)
        {
            plan.kind = AiRouteKind::Direct;
            plan.route = native;
            return plan;
        }

        AiRoute route = ChooseRoute(model, keys, nullptr, transparentBackground);
        if (!keys.Of(route.provider).IsEmpty())
        {
            plan.kind = AiRouteKind::Direct;
            plan.route = route;
            return plan;
        }

        if (connected)
        {
            plan.kind = AiRouteKind::AssetsLine;
            return plan;
        }

        plan.error = NoKeyMessage(BareModelId(model));
        return plan;
    }

    AiPlan PlanDirect(const String& model, bool hasKey, bool connected)
    {
        AiPlan plan;
        if (hasKey)
            plan.kind = AiRouteKind::Direct;
        else if (connected)
            plan.kind = AiRouteKind::AssetsLine;
        else
            plan.error = NoKeyMessage(model);

        return plan;
    }

    AiPlan Plan(const Ref<PipelineExecContext>& ctx, const String& model, bool transparentBackground /*= false*/)
    {
        return Plan(model, KeysOf(ctx->settings), AssetsLineAi::IsConnected(), transparentBackground);
    }

    AiPlan PlanDirect(const String& model, bool hasKey)
    {
        return PlanDirect(model, hasKey, !hasKey && AssetsLineAi::IsConnected());
    }

    String DirectRouteLine(const String& providerName)
    {
        return "route: own key (" + providerName + ")";
    }

    // Logs the direct route: the key it runs on and, for another API than the model's own, why
    static void LogDirect(const Ref<PipelineExecContext>& ctx, const String& model, const AiRoute& route, bool transparentBackground)
    {
        if (route.provider == AiProvider::Local)
            return;

        String note;
        ChooseRoute(model, KeysOf(ctx->settings), &note, transparentBackground);
        ctx->Log(DirectRouteLine(ProviderName(route.provider)));
        if (!note.IsEmpty())
            ctx->Log(note);
    }

    Coroutine<AiTextResult> GenerateText(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                                         const Vector<AiImageRef>& images)
    {
        AiTextResult result;
        AiPlan plan = Plan(ctx, model);
        if (plan.kind == AiRouteKind::None)
        {
            result.error = plan.error;
            co_return result;
        }

        if (plan.kind == AiRouteKind::AssetsLine)
        {
            result = co_await AssetsLineAi::GenerateText(ctx, BareModelId(model), prompt, images);
            co_return result;
        }

        AiRouteKeys keys = KeysOf(ctx->settings);
        AiRoute route = plan.route;
        LogDirect(ctx, model, route, false);
        if (route.provider == AiProvider::Local)
            result = co_await LocalAgentProvider::GenerateText(ctx, route.model, prompt, images);
        else if (route.provider == AiProvider::OpenRouter)
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

        AiBytesResult result;
        AiPlan plan = Plan(ctx, model, transparentBackground);
        if (plan.kind == AiRouteKind::None)
        {
            result.error = plan.error;
            co_return result;
        }

        if (plan.kind == AiRouteKind::AssetsLine)
        {
            result = co_await AssetsLineAi::GenerateImage(ctx, BareModelId(model), prompt, references, seed, transparentBackground, options);
            co_return result;
        }

        AiRouteKeys keys = KeysOf(ctx->settings);
        AiRoute route = plan.route;
        LogDirect(ctx, model, route, transparentBackground);
        if (route.provider == AiProvider::Local)
            result = co_await LocalAgentProvider::GenerateImage(ctx, route.model, prompt, references, transparentBackground, options);
        else if (route.provider == AiProvider::OpenRouter)
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
    ENUM_ENTRY(Local);
    ENUM_ENTRY(OpenAi);
    ENUM_ENTRY(OpenRouter);
}
END_ENUM_META;

ENUM_META(Editor::AiRouteKind, Editor__AiRouteKind)
{
    ENUM_ENTRY(AssetsLine);
    ENUM_ENTRY(Direct);
    ENUM_ENTRY(None);
}
END_ENUM_META;
// --- END META ---
