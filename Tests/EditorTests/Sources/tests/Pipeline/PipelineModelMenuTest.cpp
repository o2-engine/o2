#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineModelMenu.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

using namespace o2;
using namespace Editor;

// Rules of the model menu shared with AssetsLine (shared/modelMenu.ts): groups, order, folding, search,
// Recent and the custom id row

namespace
{
    struct WorkDirGuard
    {
        String path;

        WorkDirGuard()
        {
            String relative = "./pipeline-model-menu-work-" + (String)(int)Math::Random(0, 1000000);
            o2FileSystem.FolderCreate(relative, true);
            path = o2FileSystem.CanonicalizePath(relative) + "/";
            PipelineUtils::SetWorkPathOverride(path);
        }

        ~WorkDirGuard()
        {
            PipelineUtils::SetWorkPathOverride("");
            o2FileSystem.FolderRemove(path, true);
        }
    };

    Vector<String> Keys(const Vector<PipelineMenuGroup>& groups)
    {
        Vector<String> keys;
        for (auto& group : groups)
            keys.Add(group.key);
        return keys;
    }

    Vector<String> Ids(const PipelineMenuGroup& group)
    {
        Vector<String> ids;
        for (auto& model : group.models)
            ids.Add(model.id);
        return ids;
    }

    const PipelineMenuGroup* Find(const Vector<PipelineMenuGroup>& groups, const String& key)
    {
        int index = groups.IndexOf([&](const PipelineMenuGroup& g) { return g.key == key; });
        return index < 0 ? nullptr : &groups[index];
    }

    const PipelineVisibleGroup* FindVisible(const Vector<PipelineVisibleGroup>& groups, const String& key)
    {
        int index = groups.IndexOf([&](const PipelineVisibleGroup& g) { return g.group.key == key; });
        return index < 0 ? nullptr : &groups[index];
    }

    String Joined(const Vector<String>& items)
    {
        String text;
        for (auto& item : items)
            text += item + "; ";
        return text;
    }

    const Vector<String> textList = {
        "gemini-pro-latest", "gemini-2.5-flash", "gpt-5.5", "gpt-5.4-mini",
        "openai/gpt-5.5", "anthropic/claude-sonnet-5.5", "x-ai/grok-4.7", "google/gemini-3.5-flash"
    };
}

TEST(PipelineModelMenu, EveryIdFamilyHasItsGroup)
{
    auto expect = [](const char* id, const char* key, const String& label, PipelineModelProvider provider)
    {
        auto group = PipelineModelMenu::GroupOf(id);
        EXPECT_EQ(group.key, String(key)) << id;
        EXPECT_EQ(group.label, label) << id;
        EXPECT_EQ(group.provider, provider) << id;
    };

    const String dot = " \xC2\xB7 ";
    expect("gemini-3.1-flash-image", "google", "Google Gemini", PipelineModelProvider::Google);
    expect("models/gemini-2.5-flash", "google", "Google Gemini", PipelineModelProvider::Google);
    expect("imagen-4.0-generate-001", "google", "Google Gemini", PipelineModelProvider::Google);
    expect("veo-3.1-generate-preview", "google", "Google Gemini", PipelineModelProvider::Google);
    expect("lyria-3-clip-preview", "google", "Google Gemini", PipelineModelProvider::Google);
    expect("gemini-2.5-flash-preview-tts", "google", "Google Gemini", PipelineModelProvider::Google);
    expect("gpt-5.5", "openai", "OpenAI", PipelineModelProvider::OpenAi);
    expect("GPT-Image-1", "openai", "OpenAI", PipelineModelProvider::OpenAi);
    expect("o3-mini", "openai", "OpenAI", PipelineModelProvider::OpenAi);
    expect("chatgpt-4o-latest", "openai", "OpenAI", PipelineModelProvider::OpenAi);
    expect("dall-e-3", "openai", "OpenAI", PipelineModelProvider::OpenAi);
    expect("kling-v2-1-master", "kling", "Kling", PipelineModelProvider::Kling);
    expect("eleven_multilingual_v2", "elevenlabs", "ElevenLabs", PipelineModelProvider::ElevenLabs);
    expect("openai/gpt-5.5", "openrouter:openai", "OpenRouter" + dot + "OpenAI", PipelineModelProvider::OpenRouter);
    expect("~anthropic/claude-sonnet-latest", "openrouter:anthropic", "OpenRouter" + dot + "Anthropic", PipelineModelProvider::OpenRouter);
    expect("models/x-ai/grok-4.7", "openrouter:x-ai", "OpenRouter" + dot + "xAI", PipelineModelProvider::OpenRouter);
    expect("meta/llama-5", "openrouter:meta", "OpenRouter" + dot + "Meta", PipelineModelProvider::OpenRouter);
    expect("black-forest-labs/flux.2-pro", "openrouter:black-forest-labs", "OpenRouter" + dot + "Black Forest Labs", PipelineModelProvider::OpenRouter);
    expect("bytedance-seed/seedream-4.5", "openrouter:bytedance-seed", "OpenRouter" + dot + "ByteDance Seed", PipelineModelProvider::OpenRouter);
    expect("new-lab/some-model", "openrouter:new-lab", "OpenRouter" + dot + "New Lab", PipelineModelProvider::OpenRouter);
}

// A row under a group header leaves out what the header says
TEST(PipelineModelMenu, RowNamesDropTheProviderPart)
{
    EXPECT_EQ(PipelineModelMenu::RowName("anthropic/claude-sonnet-5.5"), String("Claude Sonnet 5.5"));
    EXPECT_EQ(PipelineModelMenu::RowName("google/gemini-3.1-flash-image"), String("Nano Banana 2"));
    EXPECT_EQ(PipelineModelMenu::RowName("z-ai/glm-5"), String("GLM 5"));
    EXPECT_EQ(PipelineModelMenu::RowName("gpt-5.4-mini"), String("GPT-5.4 Mini"));
    EXPECT_EQ(PipelineModelMenu::RowName("gemini-3.1-flash-image"), String("Gemini 3.1 Flash Image \xC2\xB7 Nano Banana 2"));
    EXPECT_EQ(PipelineModelMenu::RowName("models/gemini-2.5-flash"), String("Gemini 2.5 Flash"));
}

TEST(PipelineModelMenu, GroupsFollowTheProviderAndVendorOrder)
{
    Vector<String> ids = {
        "qwen/qwen3.7-plus", "z-ai/glm-5", "amazon/nova-3", "cohere/command-a", "mistralai/mistral-large-2512",
        "meta-llama/llama-4-maverick", "deepseek/deepseek-v4-pro", "x-ai/grok-4.7", "google/gemini-3.5-flash",
        "anthropic/claude-sonnet-5.5", "openai/gpt-5.5", "eleven_v3", "kling-v2-1", "gpt-5.5", "gemini-2.5-flash", "gemini-pro-latest"
    };

    auto groups = PipelineModelMenu::BuildGroups(ids, PipelineModelKind::Text, "gemini-2.5-flash", {});
    Vector<String> expected = {
        "google", "openai", "kling", "elevenlabs", "openrouter:openai", "openrouter:anthropic", "openrouter:google",
        "openrouter:x-ai", "openrouter:deepseek", "openrouter:meta-llama", "openrouter:mistralai", "openrouter:qwen",
        "openrouter:amazon", "openrouter:cohere", "openrouter:z-ai"
    };
    EXPECT_EQ(Keys(groups), expected) << Joined(Keys(groups));

    // Inside a group the rows keep the order of the source list
    EXPECT_EQ(Ids(groups[0]), (Vector<String>{ "gemini-2.5-flash", "gemini-pro-latest" }));
    EXPECT_FALSE(Find(groups, "current")) << "the node's value is offered";
    EXPECT_EQ(groups[0].models[0].name, String("Gemini 2.5 Flash"));
}

TEST(PipelineModelMenu, AValueOutsideTheListHasItsOwnGroup)
{
    auto groups = PipelineModelMenu::BuildGroups(textList, PipelineModelKind::Text, "vendor/private-model", { "vendor/private-model", "gpt-5.5" });
    ASSERT_FALSE(groups.IsEmpty());
    EXPECT_EQ(groups[0].key, String("current"));
    EXPECT_EQ(groups[0].label, String("Current"));
    ASSERT_EQ(groups[0].models.Count(), 1);
    EXPECT_EQ(groups[0].models[0].id, String("vendor/private-model"));
    EXPECT_EQ(groups[0].models[0].name, String("Vendor: Private Model \xC2\xB7 OpenRouter")) << "a row on its own keeps the whole name";

    // Recent never lists it a second time
    auto recent = Find(groups, "recent");
    ASSERT_TRUE(recent);
    EXPECT_EQ(Ids(*recent), (Vector<String>{ "gpt-5.5" }));
}

TEST(PipelineModelMenu, RecentHoldsTheLastFiveOfferedPicks)
{
    Vector<String> recentIds = { "x-ai/grok-4.7", "gone-model", "gpt-5.5", "x-ai/grok-4.7", "gemini-2.5-flash",
                                 "openai/gpt-5.5", "gpt-5.4-mini", "gemini-pro-latest" };
    auto groups = PipelineModelMenu::BuildGroups(textList, PipelineModelKind::Text, "gpt-5.5", recentIds);

    EXPECT_EQ(Keys(groups)[0], String("recent")) << "Recent comes first while the value is offered";
    auto recent = Find(groups, "recent");
    ASSERT_TRUE(recent);
    EXPECT_EQ(Ids(*recent), (Vector<String>{ "x-ai/grok-4.7", "gpt-5.5", "gemini-2.5-flash", "openai/gpt-5.5", "gpt-5.4-mini" }));
    EXPECT_EQ(recent->models[0].name, String("xAI: Grok 4.7 \xC2\xB7 OpenRouter"));

    // A recent model stays in its own group too
    EXPECT_TRUE(Ids(*Find(groups, "openrouter:x-ai")).Contains("x-ai/grok-4.7"));
    EXPECT_TRUE(Ids(*Find(groups, "openai")).Contains("gpt-5.5"));

    EXPECT_EQ(PipelineModelMenu::PushRecent({ "a", "b", "c", "d", "e" }, "c"), (Vector<String>{ "c", "a", "b", "d", "e" }));
    EXPECT_EQ(PipelineModelMenu::PushRecent({ "a", "b", "c", "d", "e" }, "f"), (Vector<String>{ "f", "a", "b", "c", "d" }));
    EXPECT_EQ(PipelineModelMenu::PushRecent({}, "a"), (Vector<String>{ "a" }));
}

// Each list kind keeps its own Recent, in a file under the pipeline work folder
TEST(PipelineModelMenu, RecentIsStoredPerKind)
{
    WorkDirGuard work;
    EXPECT_TRUE(PipelineModelMenu::LoadRecent(PipelineModelKind::Text).IsEmpty());

    for (auto id : { "gpt-5.5", "gemini-2.5-flash", "a", "b", "c", "d", "gpt-5.5" })
        PipelineModelMenu::RememberPick(PipelineModelKind::Text, id);
    PipelineModelMenu::RememberPick(PipelineModelKind::Image, "gpt-image-1");

    EXPECT_EQ(PipelineModelMenu::LoadRecent(PipelineModelKind::Text), (Vector<String>{ "gpt-5.5", "d", "c", "b", "a" }));
    EXPECT_EQ(PipelineModelMenu::LoadRecent(PipelineModelKind::Image), (Vector<String>{ "gpt-image-1" }));
    EXPECT_TRUE(PipelineModelMenu::LoadRecent(PipelineModelKind::Video).IsEmpty());
    EXPECT_TRUE(PipelineModelMenu::GetRecentPath().StartsWith(work.path));
    EXPECT_TRUE(o2FileSystem.IsFileExist(PipelineModelMenu::GetRecentPath()));
}

TEST(PipelineModelMenu, VendorGroupsStartFoldedExceptTheCurrentOne)
{
    Vector<String> ids = textList;
    ids.Add("anthropic/claude-opus-5.5");
    auto groups = PipelineModelMenu::BuildGroups(ids, PipelineModelKind::Text, "anthropic/claude-opus-5.5", { "gpt-5.5" });

    auto visible = PipelineModelMenu::VisibleGroups(groups, PipelineMenuFilter(), "anthropic/claude-opus-5.5", {});
    for (auto& group : visible)
    {
        bool vendor = group.group.key.StartsWith("openrouter:");
        bool holdsCurrent = group.group.key == "openrouter:anthropic";
        EXPECT_EQ(group.folded, vendor && !holdsCurrent) << group.group.key;
    }

    // A header click wins over the default until the menu closes
    Map<String, bool> toggled;
    toggled["openrouter:openai"] = false;
    toggled["google"] = true;
    visible = PipelineModelMenu::VisibleGroups(groups, PipelineMenuFilter(), "anthropic/claude-opus-5.5", toggled);
    EXPECT_FALSE(FindVisible(visible, "openrouter:openai")->folded);
    EXPECT_TRUE(FindVisible(visible, "google")->folded);

    // A search opens every group with a match and drops the others; the toggles wait
    PipelineMenuFilter search;
    search.query = "grok";
    visible = PipelineModelMenu::VisibleGroups(groups, search, "anthropic/claude-opus-5.5", toggled);
    ASSERT_EQ(visible.Count(), 1);
    EXPECT_EQ(visible[0].group.key, String("openrouter:x-ai"));
    EXPECT_FALSE(visible[0].folded);
    EXPECT_EQ(visible[0].shown.Count(), 1);

    // The transparency chip opens every group, like a search
    auto imageGroups = PipelineModelMenu::BuildGroups({ "gemini-3.1-flash-image", "gpt-image-1", "openai/gpt-5-image-mini", "openai/gpt-5.4-image-2" },
                                                      PipelineModelKind::Image, "gemini-3.1-flash-image", {});
    PipelineMenuFilter alpha;
    alpha.alphaOnly = true;
    visible = PipelineModelMenu::VisibleGroups(imageGroups, alpha, "gemini-3.1-flash-image", {});
    Vector<String> shown;
    for (auto& group : visible)
    {
        EXPECT_FALSE(group.folded) << group.group.key;
        for (auto& model : group.shown)
            shown.Add(model.id);
    }
    EXPECT_EQ(shown, (Vector<String>{ "gpt-image-1", "openai/gpt-5-image-mini" }));
}

TEST(PipelineModelMenu, SearchMatchesEveryTokenWithOrWithoutPunctuation)
{
    auto groups = PipelineModelMenu::BuildGroups(textList, PipelineModelKind::Text, "gemini-pro-latest", {});
    auto haystack = [&](const String& id)
    {
        for (auto& group : groups)
        {
            for (auto& model : group.models)
            {
                if (model.id == id)
                    return model.fields;
            }
        }
        return Vector<String>();
    };

    EXPECT_TRUE(PipelineModelMenu::Matches("gpt55", haystack("gpt-5.5")));
    EXPECT_TRUE(PipelineModelMenu::Matches("gpt55", haystack("openai/gpt-5.5")));
    EXPECT_FALSE(PipelineModelMenu::Matches("gpt55", haystack("gpt-5.4-mini")));
    EXPECT_TRUE(PipelineModelMenu::Matches("sonnet 5", haystack("anthropic/claude-sonnet-5.5")));
    EXPECT_TRUE(PipelineModelMenu::Matches("  SONNET   anthropic ", haystack("anthropic/claude-sonnet-5.5")));
    EXPECT_FALSE(PipelineModelMenu::Matches("sonnet 6", haystack("anthropic/claude-sonnet-5.5")));
    EXPECT_TRUE(PipelineModelMenu::Matches("openrouter grok", haystack("x-ai/grok-4.7"))) << "the group label is searched too";
    EXPECT_TRUE(PipelineModelMenu::Matches("gemini-pro", haystack("gemini-pro-latest")));
    EXPECT_TRUE(PipelineModelMenu::Matches("", haystack("gpt-5.5")));
    EXPECT_FALSE(PipelineModelMenu::Matches("---", haystack("gpt-5.4-mini"))) << "punctuation alone matches only as typed";
    EXPECT_TRUE(PipelineModelMenu::Matches("sonnet5", haystack("anthropic/claude-sonnet-5.5")));

    // Chips and the search combine
    PipelineMenuFilter filter;
    filter.query = "gpt";
    filter.anyProvider = false;
    filter.provider = PipelineModelProvider::OpenRouter;
    auto visible = PipelineModelMenu::VisibleGroups(groups, filter, "gemini-pro-latest", {});
    ASSERT_EQ(visible.Count(), 1);
    EXPECT_EQ(visible[0].group.key, String("openrouter:openai"));
}

TEST(PipelineModelMenu, ChipsListThePresentProviders)
{
    auto groups = PipelineModelMenu::BuildGroups(textList, PipelineModelKind::Text, "gemini-pro-latest", {});
    auto providers = PipelineModelMenu::ProvidersOf(groups);
    EXPECT_EQ(providers, (Vector<PipelineModelProvider>{ PipelineModelProvider::Google, PipelineModelProvider::OpenAi,
                                                         PipelineModelProvider::OpenRouter }));
    EXPECT_EQ(PipelineModelMenu::ChipLabel(PipelineModelProvider::Google), String("Google"));
    EXPECT_EQ(PipelineModelMenu::ChipLabel(PipelineModelProvider::OpenRouter), String("OpenRouter"));

    auto single = PipelineModelMenu::BuildGroups({ "eleven_multilingual_v2", "eleven_turbo_v2_5" }, PipelineModelKind::ElevenLabs,
                                                 "eleven_multilingual_v2", {});
    EXPECT_EQ(PipelineModelMenu::ProvidersOf(single).Count(), 1);
}

// Each field is matched on its own: squashing never glues the end of one field to the start of the next
TEST(PipelineModelMenu, TokensNeverMatchAcrossTwoFields)
{
    auto groups = PipelineModelMenu::BuildGroups({ "gpt-image-1", "openai/gpt-image-1" }, PipelineModelKind::Image, "gpt-image-1", {});
    auto direct = Find(groups, "openai")->models[0];
    auto routed = Find(groups, "openrouter:openai")->models[0];
    EXPECT_EQ(direct.name, String("GPT Image 1"));
    EXPECT_EQ(routed.name, String("GPT Image 1"));

    // "GPT Image 1" followed by "gpt-image-1" reads "gptimage1gptimage1" joined
    EXPECT_FALSE(PipelineModelMenu::Matches("image1gpt", direct.fields));
    EXPECT_FALSE(PipelineModelMenu::Matches("image1gpt", routed.fields));
    EXPECT_FALSE(PipelineModelMenu::Matches("image1gpt", { "GPT Image 1", "openai/gpt-image-1" }));
    EXPECT_TRUE(PipelineModelMenu::Matches("gptimage1", direct.fields));
    EXPECT_TRUE(PipelineModelMenu::Matches("image 1 gpt", direct.fields)) << "separate tokens may sit anywhere";
}

// A changed filter highlights the row of the exact id typed, so Enter takes that model and not a longer one
TEST(PipelineModelMenu, TheExactIdTypedIsHighlighted)
{
    auto groups = PipelineModelMenu::BuildGroups({ "openai/gpt-image-1.5", "openai/gpt-image-1", "gemini-3.1-flash-image" },
                                                 PipelineModelKind::Image, "gemini-3.1-flash-image", {});
    PipelineMenuFilter filter;
    filter.query = " openai/gpt-image-1 ";
    auto visible = PipelineModelMenu::VisibleGroups(groups, filter, "gemini-3.1-flash-image", {});
    auto ids = PipelineModelMenu::PickableIds(visible, "");
    EXPECT_EQ(ids, (Vector<String>{ "openai/gpt-image-1.5", "openai/gpt-image-1" }));
    EXPECT_EQ(PipelineModelMenu::HighlightFor(filter.query, ids), 1);
    EXPECT_TRUE(PipelineModelMenu::CustomIdFor(filter.query, groups).IsEmpty());

    EXPECT_EQ(PipelineModelMenu::HighlightFor("gpt-image", ids), 0) << "no exact id: the first row";
    EXPECT_EQ(PipelineModelMenu::HighlightFor("", ids), 0);
    EXPECT_EQ(PipelineModelMenu::HighlightFor("my-model", {}), 0);
}

// Search text that no row has, without whitespace, can be used as the id itself
TEST(PipelineModelMenu, TheCustomIdRowTakesAnUnlistedId)
{
    auto groups = PipelineModelMenu::BuildGroups(textList, PipelineModelKind::Text, "vendor/private-model", {});
    EXPECT_EQ(PipelineModelMenu::CustomIdFor("my-model", groups), String("my-model"));
    EXPECT_EQ(PipelineModelMenu::CustomIdFor("  moonshotai/kimi-k3  ", groups), String("moonshotai/kimi-k3"));
    EXPECT_EQ(PipelineModelMenu::CustomIdFor("gpt-5.5", groups), String());
    EXPECT_EQ(PipelineModelMenu::CustomIdFor("vendor/private-model", groups), String()) << "the Current row lists it";
    EXPECT_EQ(PipelineModelMenu::CustomIdFor("sonnet 5", groups), String()) << "text with a space is a search";
    EXPECT_EQ(PipelineModelMenu::CustomIdFor("   ", groups), String());
    EXPECT_EQ(PipelineModelMenu::CustomIdFor("", groups), String());

    PipelineMenuFilter filter;
    filter.query = "my-model";
    auto visible = PipelineModelMenu::VisibleGroups(groups, filter, "vendor/private-model", {});
    EXPECT_TRUE(visible.IsEmpty());
    EXPECT_EQ(PipelineModelMenu::PickableIds(visible, PipelineModelMenu::CustomIdFor(filter.query, groups)),
              (Vector<String>{ "my-model" }));
}

// Image lists mark the models that render a transparent background themselves
TEST(PipelineModelMenu, TheAlphaFlagMarksNativeTransparencyInImageLists)
{
    EXPECT_TRUE(PipelineModelMenu::IsAlpha(PipelineModelKind::Image, "gpt-image-1"));
    EXPECT_TRUE(PipelineModelMenu::IsAlpha(PipelineModelKind::Image, "openai/gpt-5-image-mini"));
    EXPECT_FALSE(PipelineModelMenu::IsAlpha(PipelineModelKind::Image, "openai/gpt-5.4-image-2"));
    EXPECT_FALSE(PipelineModelMenu::IsAlpha(PipelineModelKind::Image, "gemini-3.1-flash-image"));
    EXPECT_FALSE(PipelineModelMenu::IsAlpha(PipelineModelKind::Text, "gpt-image-1")) << "only image lists carry the badge";

    auto groups = PipelineModelMenu::BuildGroups({ "gemini-3.1-flash-image", "gpt-image-1" }, PipelineModelKind::Image,
                                                 "gemini-3.1-flash-image", {});
    EXPECT_TRUE(PipelineModelMenu::HasAlpha(groups));
    EXPECT_FALSE(Find(groups, "google")->models[0].alpha);
    EXPECT_TRUE(Find(groups, "openai")->models[0].alpha);

    EXPECT_FALSE(PipelineModelMenu::HasAlpha(PipelineModelMenu::BuildGroups(textList, PipelineModelKind::Text, "gpt-5.5", {})));
}

// The menu opens on the node's value, which Recent lists first when it holds it
TEST(PipelineModelMenu, TheMenuOpensOnTheCurrentRow)
{
    auto groups = PipelineModelMenu::BuildGroups(textList, PipelineModelKind::Text, "gpt-5.5", { "gemini-2.5-flash", "gpt-5.5" });
    auto visible = PipelineModelMenu::VisibleGroups(groups, PipelineMenuFilter(), "gpt-5.5", {});
    auto pickable = PipelineModelMenu::PickableIds(visible, "");
    ASSERT_GE(pickable.Count(), 4);
    EXPECT_EQ(pickable[0], String("gemini-2.5-flash"));
    EXPECT_EQ(PipelineModelMenu::InitialHighlight(pickable, "gpt-5.5"), 1);
    EXPECT_FALSE(pickable.Contains("openai/gpt-5.5")) << "folded groups have no rows to move over";

    EXPECT_EQ(PipelineModelMenu::InitialHighlight(pickable, "unknown"), 0);
}
