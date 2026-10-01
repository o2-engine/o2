#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/Pipeline/Providers/AiRouter.h"

using namespace o2;
using namespace Editor;

namespace
{
    Vector<AiRoute> Routes(std::initializer_list<AiRoute> routes)
    {
        return Vector<AiRoute>(routes);
    }

    String Described(const Vector<AiRoute>& routes)
    {
        String text;
        for (auto& route : routes)
            text += AiRouter::ProviderName(route.provider) + " " + route.model + "; ";
        return text;
    }
}

// The id alone names the native provider
TEST(AiRouter, FamilyComesFromTheModelId)
{
    EXPECT_EQ(AiRouter::FamilyOf("openai/gpt-5.5"), AiProvider::OpenRouter);
    EXPECT_EQ(AiRouter::FamilyOf("gpt-5.5"), AiProvider::OpenAi);
    EXPECT_EQ(AiRouter::FamilyOf("GPT-5.5"), AiProvider::OpenAi);
    EXPECT_EQ(AiRouter::FamilyOf("o3-mini"), AiProvider::OpenAi);
    EXPECT_EQ(AiRouter::FamilyOf("chatgpt-4o-latest"), AiProvider::OpenAi);
    EXPECT_EQ(AiRouter::FamilyOf("dall-e-3"), AiProvider::OpenAi);
    EXPECT_EQ(AiRouter::FamilyOf("gpt-image-1"), AiProvider::OpenAi);
    EXPECT_EQ(AiRouter::FamilyOf("gemini-2.5-flash"), AiProvider::Gemini);
    EXPECT_EQ(AiRouter::FamilyOf("imagen-4.0-generate-001"), AiProvider::Gemini);
    EXPECT_EQ(AiRouter::FamilyOf("models/gemini-2.5-flash"), AiProvider::Gemini);
    EXPECT_EQ(AiRouter::FamilyOf("omni-model"), AiProvider::Gemini);
    EXPECT_EQ(AiRouter::FamilyOf(""), AiProvider::Gemini);

    EXPECT_TRUE(AiRouter::IsOpenRouterModel("openai/gpt-5.5"));
    EXPECT_FALSE(AiRouter::IsOpenRouterModel("gpt-5.5"));
    EXPECT_FALSE(AiRouter::IsOpenRouterModel("models/gemini-2.5-flash"));
    EXPECT_FALSE(AiRouter::IsOpenRouterModel("gemini-pro-latest"));
    EXPECT_TRUE(AiRouter::IsOpenRouterModel("models/openai/gpt-5.5"));
}

TEST(AiRouter, RoutesAreTheNativeOneAndTheAlternative)
{
    auto expect = [](const char* id, const Vector<AiRoute>& routes)
    {
        auto actual = AiRouter::RoutesFor(id);
        EXPECT_TRUE(actual == routes) << id << ": " << Described(actual) << "instead of " << Described(routes);
    };

    expect("gpt-5.5", Routes({ { AiProvider::OpenAi, "gpt-5.5" }, { AiProvider::OpenRouter, "openai/gpt-5.5" } }));
    expect("o3-mini", Routes({ { AiProvider::OpenAi, "o3-mini" }, { AiProvider::OpenRouter, "openai/o3-mini" } }));
    expect("gemini-2.5-flash", Routes({ { AiProvider::Gemini, "gemini-2.5-flash" }, { AiProvider::OpenRouter, "google/gemini-2.5-flash" } }));
    expect("models/gemini-2.5-flash", Routes({ { AiProvider::Gemini, "gemini-2.5-flash" }, { AiProvider::OpenRouter, "google/gemini-2.5-flash" } }));
    expect("google/gemini-3.1-flash-image", Routes({ { AiProvider::OpenRouter, "google/gemini-3.1-flash-image" }, { AiProvider::Gemini, "gemini-3.1-flash-image" } }));
    expect("google/gemma-3-27b-it", Routes({ { AiProvider::OpenRouter, "google/gemma-3-27b-it" }, { AiProvider::Gemini, "gemma-3-27b-it" } }));
    expect("openai/gpt-5.5", Routes({ { AiProvider::OpenRouter, "openai/gpt-5.5" }, { AiProvider::OpenAi, "gpt-5.5" } }));
    expect("anthropic/claude-sonnet-5.5", Routes({ { AiProvider::OpenRouter, "anthropic/claude-sonnet-5.5" } }));
    expect("gpt-image-1", Routes({ { AiProvider::OpenAi, "gpt-image-1" }, { AiProvider::OpenRouter, "openai/gpt-image-1" } }));
    expect("openai/gpt-image-1", Routes({ { AiProvider::OpenRouter, "openai/gpt-image-1" }, { AiProvider::OpenAi, "gpt-image-1" } }));
    expect("openai/gpt-image-2.5-flare", Routes({ { AiProvider::OpenRouter, "openai/gpt-image-2.5-flare" }, { AiProvider::OpenAi, "gpt-image-2.5-flare" } }));
}

// A call for a transparent background takes only the routes whose own id renders it
TEST(AiRouter, ATransparentBackgroundKeepsTheRoutesThatRenderIt)
{
    auto expect = [](const char* id, const Vector<AiRoute>& routes)
    {
        auto actual = AiRouter::RoutesFor(id, true);
        EXPECT_TRUE(actual == routes) << id << ": " << Described(actual) << "instead of " << Described(routes);
    };

    expect("gpt-image-1", Routes({ { AiProvider::OpenAi, "gpt-image-1" }, { AiProvider::OpenRouter, "openai/gpt-image-1" } }));
    expect("openai/gpt-image-1", Routes({ { AiProvider::OpenRouter, "openai/gpt-image-1" }, { AiProvider::OpenAi, "gpt-image-1" } }));
    expect("gpt-image-2", Routes({ { AiProvider::OpenAi, "gpt-image-2" } }));
    expect("openai/gpt-5-image-mini", Routes({ { AiProvider::OpenRouter, "openai/gpt-5-image-mini" } }));
    expect("sourceful/riverflow-v2.5-pro", Routes({ { AiProvider::OpenRouter, "sourceful/riverflow-v2.5-pro" } }));

    // Without the request both routes of the model stay
    EXPECT_EQ(AiRouter::RoutesFor("gpt-image-2").Count(), 2);

    // The only other route cannot render it: the call stays with the native provider, which reports its missing key
    String note;
    AiRouteKeys routerOnly = { "", "", "r" };
    EXPECT_TRUE(AiRouter::ChooseRoute("gpt-image-2", routerOnly, &note, true) == (AiRoute{ AiProvider::OpenAi, "gpt-image-2" }));
    EXPECT_TRUE(note.IsEmpty()) << note;
    EXPECT_TRUE(AiRouter::ChooseRoute("gpt-image-2", routerOnly, &note) == (AiRoute{ AiProvider::OpenRouter, "openai/gpt-image-2" }));

    note = "";
    EXPECT_TRUE(AiRouter::ChooseRoute("gpt-image-1", routerOnly, &note, true) == (AiRoute{ AiProvider::OpenRouter, "openai/gpt-image-1" }));
    EXPECT_EQ(note, String("gpt-image-1 runs on OpenRouter as openai/gpt-image-1 (no OpenAI key)"));

    AiRouteKeys openAiOnly = { "", "o", "" };
    EXPECT_TRUE(AiRouter::ChooseRoute("openai/gpt-image-1", openAiOnly, &note, true) == (AiRoute{ AiProvider::OpenAi, "gpt-image-1" }));
    EXPECT_TRUE(AiRouter::ChooseRoute("openai/gpt-5-image-mini", openAiOnly, &note, true) == (AiRoute{ AiProvider::OpenRouter, "openai/gpt-5-image-mini" }));
}

TEST(AiRouter, SomeModelsHaveNoAlternative)
{
    auto expectOnly = [](const char* id, AiProvider provider)
    {
        auto routes = AiRouter::RoutesFor(id);
        ASSERT_EQ(routes.Count(), 1) << id << ": " << Described(routes);
        EXPECT_EQ(routes[0].provider, provider) << id;
        EXPECT_EQ(routes[0].model, String(id)) << id;
    };

    expectOnly("dall-e-3", AiProvider::OpenAi);
    expectOnly("imagen-4.0-generate-001", AiProvider::Gemini);
    expectOnly("gemini-pro-latest", AiProvider::Gemini);
    expectOnly("gemini-flash-latest", AiProvider::Gemini);
    expectOnly("openai/gpt-5-image", AiProvider::OpenRouter);
    expectOnly("openai/gpt-5.4-image-2", AiProvider::OpenRouter);
    expectOnly("meta-llama/llama-4-maverick:free", AiProvider::OpenRouter);
    expectOnly("openai/gpt-5.5:batch", AiProvider::OpenRouter);
    expectOnly("google/veo-3.1", AiProvider::OpenRouter);
}

// The first route with a key runs; with no key at all the native provider is left to report it
TEST(AiRouter, TheFirstRouteWithAKeyIsChosen)
{
    String note;
    AiRouteKeys all = { "g", "o", "r" };
    EXPECT_TRUE(AiRouter::ChooseRoute("gpt-5.5", all, &note) == (AiRoute{ AiProvider::OpenAi, "gpt-5.5" }));
    EXPECT_TRUE(note.IsEmpty()) << note;

    AiRouteKeys routerOnly = { "", "", "r" };
    EXPECT_TRUE(AiRouter::ChooseRoute("gpt-5.5", routerOnly, &note) == (AiRoute{ AiProvider::OpenRouter, "openai/gpt-5.5" }));
    EXPECT_EQ(note, String("gpt-5.5 runs on OpenRouter as openai/gpt-5.5 (no OpenAI key)"));

    note = "";
    EXPECT_TRUE(AiRouter::ChooseRoute("models/gemini-2.5-flash", routerOnly, &note) == (AiRoute{ AiProvider::OpenRouter, "google/gemini-2.5-flash" }));
    EXPECT_EQ(note, String("gemini-2.5-flash runs on OpenRouter as google/gemini-2.5-flash (no Gemini key)"));

    note = "";
    AiRouteKeys geminiOnly = { "g", "", "" };
    EXPECT_TRUE(AiRouter::ChooseRoute("google/gemini-3.1-flash-image", geminiOnly, &note) == (AiRoute{ AiProvider::Gemini, "gemini-3.1-flash-image" }));
    EXPECT_EQ(note, String("google/gemini-3.1-flash-image runs on Gemini as gemini-3.1-flash-image (no OpenRouter key)"));

    note = "";
    EXPECT_TRUE(AiRouter::ChooseRoute("dall-e-3", routerOnly, &note) == (AiRoute{ AiProvider::OpenAi, "dall-e-3" }));
    EXPECT_TRUE(AiRouter::ChooseRoute("gpt-5.5", AiRouteKeys(), &note) == (AiRoute{ AiProvider::OpenAi, "gpt-5.5" }));
    EXPECT_TRUE(AiRouter::ChooseRoute("anthropic/claude-sonnet-5.5", geminiOnly, &note) == (AiRoute{ AiProvider::OpenRouter, "anthropic/claude-sonnet-5.5" }));
    EXPECT_TRUE(note.IsEmpty()) << note;
}

TEST(AiRouter, KeysComeFromTheSettings)
{
    PipelineSettings settings;
    settings.geminiApiKey = " g ";
    settings.openAiApiKey = "o";
    settings.openRouterApiKey = "r";
    AiRouteKeys keys = AiRouter::KeysOf(settings);
    EXPECT_EQ(keys.Of(AiProvider::Gemini), String("g"));
    EXPECT_EQ(keys.Of(AiProvider::OpenAi), String("o"));
    EXPECT_EQ(keys.Of(AiProvider::OpenRouter), String("r"));
}
