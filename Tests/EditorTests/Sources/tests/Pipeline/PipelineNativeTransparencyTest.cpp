#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineRegions.h"

using namespace o2;
using namespace Editor;

// A model that renders a transparent background itself replaces the two local methods:
// the effective mode, what the executor and the price make of it, and the stored config it leaves alone

namespace
{
    Ref<PipelineNode> MakeNode(const String& type, const String& model, bool transparent, const String& mode)
    {
        auto node = PipelineNodeRegistry::CreateNode(type, Vec2F());
        node->SetConfigString("model", model);
        node->SetConfigBool("transparentBg", transparent);
        node->SetConfigString("transparentMode", mode);
        return node;
    }

    // An extract node with parts a, b and c
    Ref<PipelineNode> MakeExtract(const String& model, bool transparent, const String& mode)
    {
        auto node = MakeNode("imageExtract", model, transparent, mode);
        Vector<PipelineExtractRegion> regions;
        for (auto id : { "a", "b", "c" })
        {
            PipelineExtractRegion region;
            region.id = id;
            region.name = String("part ") + id;
            region.x = 0.1f; region.y = 0.2f; region.w = 0.3f; region.h = 0.4f;
            regions.Add(region);
        }
        PipelineRegions::Write(*node, regions);
        PipelineRegions::SyncPorts(*node);
        return node;
    }

    void SetOwn(PipelineNode& node, const String& portId, const char* json)
    {
        DataDocument own;
        own.LoadFromData(json);
        for (auto& region : node.config["regions"])
        {
            if (String(region["id"].GetString()) == portId)
                region["transparency"] = static_cast<const DataValue&>(own);
        }
    }
}

TEST(PipelineNativeTransparency, SomeModelsRenderTheAlphaThemselves)
{
    for (auto id : { "gpt-image-1", "gpt-image-1-mini", "gpt-image-1.5", "gpt-image-2", "gpt-image-2.5-flare", "gpt-image-2.5-sunburst",
                     "models/gpt-image-1", "GPT-Image-1", "  gpt-image-1  ",
                     "openai/gpt-image-1", "openai/gpt-image-1-mini", "openai/gpt-image-2.5-flare", "openai/gpt-image-2.5-sunburst",
                     "openai/gpt-image-2-mini", "openai/gpt-image-2-2026-04", "openai/gpt-image-2-2026-04-2x",
                     "openai/gpt-5-image", "openai/gpt-5-image-mini", "OpenAI/GPT-5-Image-Mini", "sourceful/riverflow-v2.5-pro" })
        EXPECT_TRUE(PipelineTransparency::SupportsNativeTransparency(id)) << id;

    for (auto id : { "openai/gpt-image-2", "openai/gpt-image-2-2026-04-21", "openai/gpt-5.4-image-2", "openai/gpt-5-image-mini-2",
                     "openai/gpt-5-image-pro", "sourceful/riverflow-v2.5-fast", "sourceful/riverflow-v2.5-pro-max",
                     "dall-e-3", "gemini-3.1-flash-image", "imagen-4.0-generate-001", "gpt-5.5", "google/gemini-3.1-flash-image",
                     "google/gemini-2.5-flash-image", "" })
        EXPECT_FALSE(PipelineTransparency::SupportsNativeTransparency(id)) << id;
}

// The id the node stores decides the mode, whichever route the call takes later
TEST(PipelineNativeTransparency, OpenRouterModelsFollowTheSameRule)
{
    auto native = MakeNode("nanoBananaGen", "openai/gpt-5-image-mini", true, "chroma");
    EXPECT_EQ(PipelineTransparency::Read(*native).mode, String("native"));
    EXPECT_TRUE(PipelineTransparency::UsesNativeTransparency(*native));
    EXPECT_FALSE(PipelineTransparency::UsesChromaPostStep(*native));
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*native), 1);

    auto local = MakeNode("nanoBananaGen", "openai/gpt-5.4-image-2", true, "chroma");
    EXPECT_EQ(PipelineTransparency::Read(*local).mode, String("chroma"));
    EXPECT_FALSE(PipelineTransparency::UsesNativeTransparency(*local));
    EXPECT_TRUE(PipelineTransparency::UsesChromaPostStep(*local));

    auto opaque = MakeNode("nanoBananaGen", "openai/gpt-image-2", true, "twoPass");
    EXPECT_EQ(PipelineTransparency::Read(*opaque).mode, String("twoPass"));
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*opaque), 2);
}

TEST(PipelineNativeTransparency, TheModelDecidesTheModeAndTheConfigKeepsTheMethod)
{
    for (auto type : { "nanoBananaGen", "imageEdit" })
    {
        for (auto stored : { "chroma", "twoPass" })
        {
            auto node = MakeNode(type, "gpt-image-1", true, stored);
            auto config = PipelineTransparency::Read(*node);
            EXPECT_TRUE(config.transparent);
            EXPECT_EQ(config.mode, String("native")) << type << " " << stored;
            EXPECT_EQ(config.storedMode, String(stored));
            EXPECT_TRUE(PipelineTransparency::UsesNativeTransparency(*node));
            EXPECT_FALSE(PipelineTransparency::UsesChromaPostStep(*node));
            EXPECT_FALSE(PipelineTransparency::AnyChromaPostStep(*node));
            EXPECT_EQ(PipelineTransparency::ImageGenerations(*node), 1);
            EXPECT_EQ(node->GetConfigString("transparentMode", ""), String(stored)) << "reading changes nothing";

            // Another model brings the method back
            node->SetConfigString("model", "gemini-3.1-flash-image");
            auto restored = PipelineTransparency::Read(*node);
            EXPECT_EQ(restored.mode, String(stored));
            EXPECT_FALSE(PipelineTransparency::UsesNativeTransparency(*node));
            EXPECT_EQ(PipelineTransparency::UsesChromaPostStep(*node), String(stored) == "chroma");
            EXPECT_EQ(PipelineTransparency::ImageGenerations(*node), String(stored) == "chroma" ? 1 : 2);
        }
    }
}

TEST(PipelineNativeTransparency, WithoutATransparentBackgroundNothingChanges)
{
    auto node = MakeNode("nanoBananaGen", "gpt-image-1", false, "chroma");
    EXPECT_FALSE(PipelineTransparency::UsesNativeTransparency(*node));
    EXPECT_FALSE(PipelineTransparency::UsesChromaPostStep(*node));
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*node), 1);
}

// The node is transparent by nature, whatever its flag says
TEST(PipelineNativeTransparency, RemoveBackgroundIsAlwaysTransparent)
{
    auto node = PipelineNodeRegistry::CreateNode("aiRemoveBg", Vec2F());
    node->SetConfigString("model", "gpt-image-1.5");
    node->SetConfigString("transparentMode", "twoPass");
    EXPECT_TRUE(PipelineTransparency::UsesNativeTransparency(*node));
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*node), 1);

    node->SetConfigBool("transparentBg", false);
    EXPECT_TRUE(PipelineTransparency::UsesNativeTransparency(*node));

    node->SetConfigString("model", "gemini-3.1-flash-image");
    EXPECT_FALSE(PipelineTransparency::UsesNativeTransparency(*node));
}

// The executor keeps a raw render and cuts it only for a chroma node: the key of a native node has neither the
// raw variant nor the cut settings left out
TEST(PipelineNativeTransparency, TheExecutorSeesNoChromaNode)
{
    auto native = MakeNode("nanoBananaGen", "gpt-image-1", true, "chroma");
    String before = PipelineExecutor::ContentSignature(*native, {}, -1);
    native->SetConfigNumber("chromaTolerance", 77.0f);
    EXPECT_NE(PipelineExecutor::ContentSignature(*native, {}, -1), before) << "no cut settings are left out of the key";

    auto chroma = MakeNode("nanoBananaGen", "gemini-3.1-flash-image", true, "chroma");
    before = PipelineExecutor::ContentSignature(*chroma, {}, -1);
    chroma->SetConfigNumber("chromaTolerance", 77.0f);
    EXPECT_EQ(PipelineExecutor::ContentSignature(*chroma, {}, -1), before) << "a chroma node re-cuts the same render";
}

TEST(PipelineNativeTransparency, APartWithOwnSettingsFollowsTheModelToo)
{
    auto node = MakeExtract("gpt-image-1", true, "twoPass");
    SetOwn(*node, "b", R"json({"transparentBg":true,"transparentMode":"chroma","chromaColor":"#ff00ff",
                               "chromaTolerance":40,"chromaSoftness":5,"chromaSpill":70})json");
    SetOwn(*node, "c", R"json({"transparentBg":false,"transparentMode":"chroma"})json");

    EXPECT_EQ(PipelineTransparency::Read(*node, "a").mode, String("native"));
    EXPECT_TRUE(PipelineTransparency::UsesNativeTransparency(*node, "a"));

    auto b = PipelineTransparency::Read(*node, "b");
    EXPECT_EQ(b.mode, String("native"));
    EXPECT_EQ(b.storedMode, String("chroma"));
    EXPECT_FLOAT_EQ(b.chroma.tolerance, 40.0f);
    EXPECT_TRUE(PipelineTransparency::UsesNativeTransparency(*node, "b"));
    EXPECT_FALSE(PipelineTransparency::UsesChromaPostStep(*node, "b"));

    EXPECT_FALSE(PipelineTransparency::Read(*node, "c").transparent);
    EXPECT_FALSE(PipelineTransparency::UsesNativeTransparency(*node, "c"));
    EXPECT_FALSE(PipelineTransparency::AnyChromaPostStep(*node));
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*node), 3);

    // The key of the part names the method of the config and has no raw render variant
    Vector<String> exclude;
    String suffix = PipelineTransparency::PortCacheSuffix(*node, "b", true, exclude);
    EXPECT_EQ(suffix, String("|bg:[true,\"chroma\",\"#FF00FF\"]"));

    // A copy of the settings carries the method of the config, never the effective mode
    DataDocument copy;
    PipelineTransparency::WriteSettings(*node, "b", copy);
    EXPECT_EQ(String(copy["transparentMode"].GetString()), String("chroma"));

    // The same parts on a model that cannot render the alpha: each by its own method again
    node->SetConfigString("model", "gemini-3.1-flash-image");
    EXPECT_EQ(PipelineTransparency::Read(*node, "a").mode, String("twoPass"));
    EXPECT_TRUE(PipelineTransparency::UsesChromaPostStep(*node, "b"));
    EXPECT_TRUE(PipelineTransparency::AnyChromaPostStep(*node));
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*node), 4);
}
