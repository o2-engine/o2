#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/Nodes/UiNodes.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineNodeGroups.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocOps.h"
#include "o2Editor/Pipeline/UiDoc/UiDocUiml.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"

using namespace o2;
using namespace Editor;

// The node types and port types of AssetsLine's UI kit / Text style / AI UI layout and Image crop, the add-node groups,
// the extract's own part models and densities, inputs read by port id

namespace
{
    struct WorkDirGuard
    {
        String path;

        WorkDirGuard()
        {
            String relative = "./pipeline-ui-nodes-work-" + (String)(int)(Math::Random()%1000000);
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

    Ref<PipelineEdge> Link(PipelineGraph& graph, const Ref<PipelineNode>& from, const String& fromPort, const Ref<PipelineNode>& to,
                           const String& toPort)
    {
        auto edge = mmake<PipelineEdge>();
        edge->id = PipelineNode::GenerateId();
        edge->fromNodeId = from->id;
        edge->fromPortId = fromPort;
        edge->toNodeId = to->id;
        edge->toPortId = toPort;
        graph.edges.Add(edge);
        return edge;
    }

    // The web's pngMeta on a 1 x 1 red PNG: withPngDpi(png, 960.04, 2.00004), then withPngDpi(that, undefined)
    const char* redPng = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8DwHwAFBQIAX8jx0gAAAABJRU5ErkJggg==";
    const char* redTagged = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAACXBIWXMAAJOlAACTpQFRsbRdAAAAEnRFWHRBc3NldHNMaW5lOmRwaQA5NjAZlQNZAAAAEnRFWHRBc3NldHNMaW5lOnNjYWxlADJ9mvXbAAAADUlEQVR42mP8z8DwHwAFBQIAX8jx0gAAAABJRU5ErkJggg==";
    const char* redUntagged = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAACXBIWXMAAJOlAACTpQFRsbRdAAAAEnRFWHRBc3NldHNMaW5lOnNjYWxlADJ9mvXbAAAADUlEQVR42mP8z8DwHwAFBQIAX8jx0gAAAABJRU5ErkJggg==";
}

TEST(PipelineUiPorts, NewPortTypesSurviveTheFile)
{
    EXPECT_EQ(PipelinePortTypeToString(PipelinePortType::UiKit), "uiKit");
    EXPECT_EQ(PipelinePortTypeToString(PipelinePortType::TextStyle), "textStyle");
    EXPECT_EQ(PipelinePortTypeFromString("uiKit"), PipelinePortType::UiKit);
    EXPECT_EQ(PipelinePortTypeFromString("textStyle"), PipelinePortType::TextStyle);
    EXPECT_TRUE(IsTextLikePortType(PipelinePortType::UiKit));
    EXPECT_FALSE(IsTextLikePortType(PipelinePortType::Image));

    // A pipeline synced from AssetsLine: the ports keep their types through a load and a save (they used to come back "text")
    const char* doc = R"json({"schemaVersion":1,"id":"p","name":"p","nodes":[
        {"id":"s","type":"textStyle","position":{"x":0,"y":0},"config":{"name":"title"},"inputs":[],"outputs":[{"id":"style","name":"style","type":"textStyle"}]},
        {"id":"k","type":"uiKit","position":{"x":0,"y":0},"config":{"customInputs":[{"id":"ks","name":"","type":"textStyle"}]},
         "inputs":[{"id":"ks","name":"","type":"textStyle","custom":true}],"outputs":[{"id":"kit","name":"kit","type":"uiKit"}]},
        {"id":"l","type":"uiLayout","position":{"x":0,"y":0},"config":{},"inputs":[{"id":"kit","name":"kit","type":"uiKit"}],
         "outputs":[{"id":"image","name":"image","type":"image"},{"id":"layout","name":"layout","type":"text"}]}],
        "edges":[{"id":"e1","fromNodeId":"s","fromPortId":"style","toNodeId":"k","toPortId":"ks"},
                 {"id":"e2","fromNodeId":"k","fromPortId":"kit","toNodeId":"l","toPortId":"kit"}]})json";
    PipelineGraph graph;
    ASSERT_TRUE(graph.LoadFromJsonString(doc));
    EXPECT_TRUE(graph.Validate().IsEmpty());
    String saved = graph.ToJsonString();
    EXPECT_TRUE(saved.Contains("\"type\":\"uiKit\"") || saved.Contains("\"type\": \"uiKit\"")) << saved;
    EXPECT_TRUE(saved.Contains("\"type\":\"textStyle\"") || saved.Contains("\"type\": \"textStyle\"")) << saved;
    EXPECT_FALSE(saved.Contains("\"name\":\"style\",\"type\":\"text\""));

    PipelineGraph again;
    ASSERT_TRUE(again.LoadFromJsonString(saved));
    EXPECT_EQ(again.FindNode("k")->inputs[0].portType, PipelinePortType::TextStyle);
    EXPECT_EQ(again.FindNode("l")->inputs[0].portType, PipelinePortType::UiKit);

    // A port connects only to a port of its own type
    auto bad = mmake<PipelineEdge>();
    bad->id = "bad";
    bad->fromNodeId = "s";
    bad->fromPortId = "style";
    bad->toNodeId = "l";
    bad->toPortId = "kit";
    graph.edges.Add(bad);
    graph.RemoveEdge("e2");
    EXPECT_FALSE(graph.Validate().IsEmpty());
}

TEST(PipelineUiPorts, TextLikeValuesAreStoredAsText)
{
    WorkDirGuard work;
    auto kit = PipelineValue::TextOf(PipelinePortType::UiKit, "{\"version\":1}");
    EXPECT_EQ(kit.type, PipelinePortType::UiKit);
    EXPECT_TRUE(kit.IsTextLike());
    EXPECT_FALSE(kit.IsText());
    EXPECT_EQ(kit.GetExtension(), "txt");
    EXPECT_EQ(PipelineValue::TextOf(PipelinePortType::Image, "x").type, PipelinePortType::Text);

    PipelineExecutor::SaveContent("ui", "kitsig", kit);
    EXPECT_EQ(PipelineUtils::ReadFileBytes(PipelineExecutor::GetContentPath("ui", "kitsig", "txt")), kit.data);
    String preview = PipelineExecutor::GetPortPreviewPath("ui", "n", "kit", "txt");
    o2FileSystem.FolderCreate(o2FileSystem.GetParentPath(preview), true);
    PipelineUtils::WriteFileBytes(preview, kit.data);
    auto back = PipelineExecutor::LoadPortPreview("ui", "n", "kit", PipelinePortType::UiKit);
    EXPECT_EQ(back.type, PipelinePortType::UiKit);
    EXPECT_EQ(back.data, kit.data);
}

TEST(PipelineUiNodes, SchemasAsAssetsLine)
{
    auto style = PipelineNodeRegistry::GetSchema("textStyle");
    ASSERT_TRUE(style);
    EXPECT_EQ(style->category, PipelineNodeCategory::Source);
    EXPECT_TRUE(style->instant);
    ASSERT_EQ(style->outputs.Count(), 1);
    EXPECT_EQ(style->outputs[0].name, "style");
    EXPECT_EQ(style->outputs[0].portType, PipelinePortType::TextStyle);
    EXPECT_EQ(style->addableInputs, Vector<PipelinePortType>({ PipelinePortType::Image }));
    EXPECT_EQ(style->defaultSize, Vec2F(360, 0));

    auto kit = PipelineNodeRegistry::GetSchema("uiKit");
    ASSERT_TRUE(kit);
    EXPECT_TRUE(kit->instant && kit->partialInputs);
    EXPECT_TRUE(kit->inputs.IsEmpty());
    EXPECT_EQ(kit->outputs[0].name, "kit");
    EXPECT_EQ(kit->outputs[0].portType, PipelinePortType::UiKit);
    EXPECT_EQ(kit->addableInputs, Vector<PipelinePortType>({ PipelinePortType::Image, PipelinePortType::TextStyle }));
    EXPECT_EQ(kit->defaultSize, Vec2F(760, 0));

    auto layout = PipelineNodeRegistry::GetSchema("uiLayout");
    ASSERT_TRUE(layout);
    EXPECT_EQ(layout->category, PipelineNodeCategory::AI);
    EXPECT_TRUE(layout->instant && layout->partialInputs && layout->perPortRun);
    ASSERT_EQ(layout->inputs.Count(), 1);
    EXPECT_EQ(layout->inputs[0].name, "kit");
    EXPECT_EQ(layout->inputs[0].portType, PipelinePortType::UiKit);
    ASSERT_EQ(layout->outputs.Count(), 2);
    EXPECT_EQ(layout->outputs[0].name, "image");
    EXPECT_EQ(layout->outputs[1].name, "layout");
    EXPECT_EQ(layout->outputs[1].portType, PipelinePortType::Text);
    EXPECT_EQ(layout->addableInputs, Vector<PipelinePortType>({ PipelinePortType::Image, PipelinePortType::Text }));

    auto crop = PipelineNodeRegistry::GetSchema("imageCrop");
    ASSERT_TRUE(crop);
    EXPECT_TRUE(crop->instant);
    EXPECT_EQ(crop->category, PipelineNodeCategory::Transform);
    EXPECT_EQ(crop->defaultSize, Vec2F(420, 0));

    // New nodes store the schema defaults the web stores
    auto styleNode = PipelineNodeRegistry::CreateNode("textStyle", Vec2F());
    EXPECT_EQ(styleNode->GetConfigString("name"), "title");
    EXPECT_EQ(styleNode->GetConfigString("role"), "title");
    EXPECT_EQ(styleNode->GetConfigString("font"), "Roboto");
    EXPECT_EQ(styleNode->GetConfigNumber("size"), 64.0f);
    EXPECT_EQ(PipelineNodeRegistry::CreateNode("uiLayout", Vec2F())->GetConfigString("agentModel"), "gpt-6.1-sol");
    auto extract = PipelineNodeRegistry::CreateNode("imageExtract", Vec2F());
    EXPECT_EQ(extract->GetConfigString("model"), "openai/gpt-5-image-mini");
    EXPECT_TRUE(extract->GetConfigBool("transparentBg"));
    EXPECT_EQ(extract->GetConfigString("transparentMode"), "chroma");

    // The view state of the new cards is UI-only
    for (auto key : { "kitSelected", "tsSample", "styleModel", "uiView", "uiRedo", "agentModel", "agentReview", "splitModel", "uiDevice", "uiScale" })
        EXPECT_TRUE(PipelineGraph::GetUiOnlyConfigKeys().Contains(key)) << key;
    EXPECT_FALSE(PipelineGraph::GetUiOnlyConfigKeys().Contains("sourceDpi"));
    EXPECT_FALSE(PipelineGraph::GetUiOnlyConfigKeys().Contains("steps"));
}

TEST(PipelineUiNodes, MenusInSixGroupsWithoutRetiredNodes)
{
    EXPECT_TRUE(PipelineNodeRegistry::GetSchema("removeBackground")->retired);
    EXPECT_TRUE(PipelineNodeRegistry::GetSchema("textCompose")->retired);

    auto groups = PipelineNodeGroups::Group(PipelineNodeRegistry::AllSchemas());
    Vector<String> ids;
    Map<String, Vector<String>> members;
    for (auto& entry : groups)
    {
        ids.Add(entry.group->id);
        for (auto schema : entry.items)
            members[entry.group->id].Add(schema->type);
    }
    EXPECT_EQ(ids, Vector<String>({ "io", "aiImage", "imageTools", "text", "gameUi", "av" }));
    EXPECT_EQ(members["gameUi"], Vector<String>({ "imageToUi", "uiKit", "textStyle", "uiLayout" }));
    EXPECT_EQ(members["aiImage"], Vector<String>({ "nanoBananaGen", "imageEdit", "imageExtract", "aiRemoveBg", "aiUpscale" }));
    EXPECT_EQ(members["imageTools"].IndexOf("imageCrop"), 1);
    for (auto& group : members)
    {
        EXPECT_FALSE(group.second.Contains("removeBackground"));
        EXPECT_FALSE(group.second.Contains("textCompose"));
    }

    // Inputs before results in "Input & output"
    auto& io = members["io"];
    EXPECT_LT(io.IndexOf("sourceAudio"), io.IndexOf("finishImage"));

    // The menu of a dropped link: only what takes a UI kit, still grouped
    Vector<const PipelineNodeSchema*> takesKit;
    for (auto schema : PipelineNodeRegistry::AllSchemas())
    {
        if (schema->AcceptsInputType(PipelinePortType::UiKit))
            takesKit.Add(schema);
    }
    auto kitGroups = PipelineNodeGroups::Group(takesKit);
    ASSERT_EQ(kitGroups.Count(), 1);
    EXPECT_EQ(kitGroups[0].group->id, "gameUi");
    EXPECT_EQ(kitGroups[0].items.Count(), 1);
    EXPECT_EQ(kitGroups[0].items[0]->type, "uiLayout");
}

TEST(PipelineUiNodes, ReferenceInputsStayOutOfRunsAndSignatures)
{
    PipelineGraph graph;
    auto picture = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F());
    auto style = PipelineNodeRegistry::CreateNode("textStyle", Vec2F());
    auto layout = PipelineNodeRegistry::CreateNode("uiLayout", Vec2F());
    style->SetCustomInputs({ PipelinePort("ref", "", PipelinePortType::Image, true) });
    PipelineNodeRegistry::SyncNodeWithSchema(style);
    layout->SetCustomInputs({ PipelinePort("ref", "", PipelinePortType::Image, true) });
    PipelineNodeRegistry::SyncNodeWithSchema(layout);
    graph.nodes = { picture, style, layout };

    auto before = graph.ComputeSignatures();
    Link(graph, picture, picture->outputs[0].id, style, "ref");
    Link(graph, picture, picture->outputs[0].id, layout, "ref");
    auto after = graph.ComputeSignatures();
    EXPECT_EQ(before[style->id], after[style->id]);
    EXPECT_EQ(before[layout->id], after[layout->id]);
    EXPECT_TRUE(graph.GetRunInputEdges(style->id).IsEmpty());
    EXPECT_TRUE(graph.GetRunInputEdges(layout->id).IsEmpty());
    EXPECT_EQ(graph.GetIncomingEdges(layout->id).Count(), 1);

    // The kit is no reference
    EXPECT_FALSE(PipelineNodeRegistry::IsReferenceInput(*layout, layout->inputs[0].id));
    EXPECT_TRUE(PipelineNodeRegistry::IsReferenceInput(*layout, "ref"));
}

TEST(PipelineUiNodes, TextStyleIsItsConfig)
{
    auto style = PipelineNodeRegistry::CreateNode("textStyle", Vec2F());
    style->SetConfigString("font", "Lobster");
    style->SetConfigNumber("size", 40);
    PipelineValue value;
    String error;
    ASSERT_TRUE(ResolveSourceValue(*style, "", value, error));
    EXPECT_EQ(value.type, PipelinePortType::TextStyle);
    UiLayout::Json json;
    ASSERT_TRUE(UiLayout::Json::Parse(value.data, json));
    EXPECT_EQ(json.Get("font")->Str(), "Lobster");
    EXPECT_EQ(json.Get("size")->Num(), 40.0);
    EXPECT_EQ(json.Get("name")->Str(), "title");
}

TEST(PipelineUiNodes, KitCollectsGraphicsAndStylesItsInputsHave)
{
    auto kitNode = PipelineNodeRegistry::CreateNode("uiKit", Vec2F());
    kitNode->SetCustomInputs({ PipelinePort("btn", "btn_green", PipelinePortType::Image, true),
                               PipelinePort("missing", "icon_coin", PipelinePortType::Image, true),
                               PipelinePort("st", "", PipelinePortType::TextStyle, true) });
    PipelineNodeRegistry::SyncNodeWithSchema(kitNode);

    // A green button with a transparent margin and a picture tagged 960 dpi
    auto button = PipelineImageOps::Blank(40, 20, Color4(0, 0, 0, 0));
    for (int y = 2; y < 18; y++)
        for (int x = 2; x < 38; x++)
            std::memcpy(PipelineImageOps::Pixel(*button, x, y), "\x20\xC0\x40\xFF", 4);
    String png = PipelinePngMeta::WithDpi(EncodeBitmapPng(*button), 960.0);

    auto styleNode = PipelineNodeRegistry::CreateNode("textStyle", Vec2F());
    Map<String, PipelineValue> byId;
    byId["btn"] = PipelineValue::ImageBytes(png);
    byId["st"] = PipelineValue::TextOf(PipelinePortType::TextStyle, PipelineUiNodes::TextStyleOutput(*styleNode));

    // The input not produced yet is left out, the kit is still made
    auto kit = PipelineUiNodes::BuildKit(*kitNode, byId, {});
    ASSERT_EQ(kit.elements.Count(), 1);
    EXPECT_EQ(kit.elements[0].name, "btn_green");
    EXPECT_EQ(kit.elements[0].type, "button");
    // Trimmed to the button; a uniform middle collapses
    EXPECT_GT(kit.elements[0].width, 0.0);
    EXPECT_LE(kit.elements[0].height, 16.0);
    EXPECT_EQ(kit.elements[0].dpi.value_or(0), 960.0);
    EXPECT_EQ(kit.elements[0].dpiFrom.value_or(""), "input");
    EXPECT_EQ(*kit.elements[0].port, "btn");
    ASSERT_EQ(kit.textStyles.Count(), 1);
    EXPECT_EQ(kit.textStyles[0].name, "title");
}

TEST(PipelineUiNodes, ImageCropCutsScalesAndKeepsTheDensity)
{
    WorkDirGuard work;
    o2FileSystem.FolderCreate(PipelineUtils::GetUploadsPath(), true);
    auto picture = PipelineImageOps::Blank(100, 50, Color4(255, 0, 0, 255));
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("pic.png"), PipelinePngMeta::WithDpi(EncodeBitmapPng(*picture), 480.0));

    PipelineGraph graph;
    auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F());
    source->SetConfigString("uploadId", "pic.png");
    auto crop = PipelineNodeRegistry::CreateNode("imageCrop", Vec2F());
    DataDocument frame;
    frame.LoadFromData(R"json({"x":0.1,"y":0.2,"w":0.5,"h":0.6})json");
    crop->config["crop"] = static_cast<const DataValue&>(frame);
    graph.nodes = { source, crop };
    Link(graph, source, source->outputs[0].id, crop, crop->inputs[0].id);

    auto run = [&]()
    {
        PipelineValue out;
        bool done = false;
        String fatal;
        auto executor = mmake<PipelineExecutor>();
        executor->onEvent = [&](const PipelineExecEvent& e)
        {
            if (e.type == PipelineExecEvent::Type::NodeOutput && e.nodeId == crop->id) out = e.value;
            if (e.type == PipelineExecEvent::Type::Done) done = true;
            if (e.type == PipelineExecEvent::Type::Fatal) fatal = e.error;
        };
        executor->Execute("crop", graph, crop->id, { crop->id });
        EXPECT_TRUE(NetPumpUntil([&] { return done || !fatal.IsEmpty(); }, 20.0f));
        EXPECT_TRUE(fatal.IsEmpty()) << fatal;
        return out;
    };

    auto cut = run();
    ASSERT_TRUE(cut.IsImage());
    EXPECT_EQ(cut.GetBitmap()->GetSize(), Vec2I(50, 30));
    EXPECT_EQ(PipelinePngMeta::ReadDpi(cut.data).value_or(0), 480.0);

    crop->SetConfigBool("resize", true);
    crop->SetConfigNumber("resizeW", 100);
    crop->SetConfigNumber("resizeH", 60);
    auto scaled = run();
    EXPECT_EQ(scaled.GetBitmap()->GetSize(), Vec2I(100, 60));
    EXPECT_EQ(PipelinePngMeta::ReadDpi(scaled.data).value_or(0), 960.0);
}

TEST(PipelinePngMeta, SameBytesAsAssetsLine)
{
    String png = PipelineUtils::Base64Decode(redPng);
    String tagged = PipelinePngMeta::WithDpi(png, 960.04, 2.00004);
    EXPECT_EQ(PipelineUtils::Base64Encode(tagged), redTagged);
    EXPECT_EQ(PipelinePngMeta::ReadDpi(tagged).value_or(0), 960.0);
    EXPECT_EQ(PipelinePngMeta::ReadScale(tagged).value_or(0), 2.0);

    String untagged = PipelinePngMeta::WithDpi(tagged, std::nullopt);
    EXPECT_EQ(PipelineUtils::Base64Encode(untagged), redUntagged);
    EXPECT_FALSE(PipelinePngMeta::ReadDpi(untagged).has_value());

    // Still a picture; anything else passes through untouched
    EXPECT_TRUE(DecodeImageBytes(tagged));
    EXPECT_EQ(PipelinePngMeta::WithDpi("not a png", 480.0), "not a png");
    int w = 0, h = 0;
    EXPECT_TRUE(PipelinePngMeta::Size(tagged, w, h));
    EXPECT_EQ(w, 1);
}

TEST(PipelineExtractParts, AnOwnModelMovesOnlyItsOwnKey)
{
    auto node = PipelineNodeRegistry::CreateNode("imageExtract", Vec2F());
    node->SetConfigString("model", "gemini-3.1-flash-image");
    node->SetConfigBool("transparentBg", false);
    Vector<PipelineExtractRegion> regions;
    for (auto id : { "ga", "gb", "gc" })
    {
        PipelineExtractRegion r;
        r.id = id;
        r.name = String("part ") + id;
        regions.Add(r);
    }
    PipelineRegions::Write(*node, regions);
    PipelineRegions::SyncPorts(*node);

    auto keys = [&]()
    {
        Map<String, String> k;
        for (auto& port : node->outputs)
            k[port.id] = PipelineExecutor::PortSignature(*node, {}, -1, port.id);
        return k;
    };
    auto k0 = keys();

    PipelineRegions::FindPart(*node, "gb")->AddMember("model") = "openai/gpt-5-image-mini";
    EXPECT_EQ(PipelineRegions::OwnModel(*node, "gb"), "openai/gpt-5-image-mini");
    EXPECT_EQ(PipelineRegions::PartModel(*node, "gb"), "openai/gpt-5-image-mini");
    EXPECT_EQ(PipelineRegions::PartModel(*node, "ga"), "gemini-3.1-flash-image");
    EXPECT_TRUE(PipelineRegions::Read(*node)[1].IsOwn());
    auto k1 = keys();
    EXPECT_NE(k1["gb"], k0["gb"]);
    EXPECT_EQ(k1["ga"], k0["ga"]);
    EXPECT_EQ(k1["gc"], k0["gc"]);

    // The node's model no longer reaches it
    node->SetConfigString("model", "gemini-2.5-flash-image");
    auto k2 = keys();
    EXPECT_EQ(k2["gb"], k1["gb"]);
    EXPECT_NE(k2["ga"], k1["ga"]);

    // A part's model decides how its background is drawn: this one draws the alpha itself
    node->SetConfigBool("transparentBg", true);
    node->SetConfigString("transparentMode", "twoPass");
    EXPECT_TRUE(PipelineTransparency::UsesNativeTransparency(*node, "gb"));
    EXPECT_FALSE(PipelineTransparency::UsesNativeTransparency(*node, "ga"));
    EXPECT_EQ(PipelineTransparency::ImageGenerations(*node), 2 + 1 + 2);

    // "This part" makes the model and the background its own together; "All parts" drops both
    EXPECT_TRUE(PipelineRegions::SetOwnPart(*node, "gc", true));
    EXPECT_EQ(PipelineRegions::OwnModel(*node, "gc"), "gemini-2.5-flash-image");
    EXPECT_TRUE(PipelineRegions::FindTransparency(*node, "gc"));
    EXPECT_TRUE(PipelineRegions::SetOwnPart(*node, "gc", false));
    EXPECT_TRUE(PipelineRegions::OwnModel(*node, "gc").IsEmpty());
    EXPECT_FALSE(PipelineRegions::FindTransparency(*node, "gc"));
}

TEST(PipelineExtractParts, SourceDpiRetagsWithoutANewGeneration)
{
    auto node = PipelineNodeRegistry::CreateNode("imageExtract", Vec2F());
    PipelineNodeRegistry::SyncNodeWithSchema(node);
    String port = node->outputs[0].id;
    String raw = PipelineGraph::ComputePortSignature(*node, {}, -1, port, true);
    String down = PipelineGraph::ComputePortSignature(*node, {}, -1, port, false);
    String nodeSig = PipelineGraph::ComputeNodeSignature(*node, {}, {}, -1);

    node->SetConfigNumber("sourceDpi", 960);
    EXPECT_EQ(PipelineGraph::ComputePortSignature(*node, {}, -1, port, true), raw);
    EXPECT_NE(PipelineGraph::ComputePortSignature(*node, {}, -1, port, false), down);
    EXPECT_NE(PipelineGraph::ComputeNodeSignature(*node, {}, {}, -1), nodeSig);
}

TEST(PipelineExtractParts, PartsCarryTheirDensity)
{
    EXPECT_EQ(PipelineRegions::PartScale(100, 50, 400, 100), 2.0);
    EXPECT_EQ(PipelineRegions::PartScale(3, 3, 10, 10), 3.3333);
    EXPECT_EQ(PipelineRegions::PartScale(0, 50, 400, 100), 0.0);

    auto node = PipelineNodeRegistry::CreateNode("imageExtract", Vec2F());
    PipelineNodeRegistry::SyncNodeWithSchema(node);
    String port = node->outputs[0].id;
    String part = EncodeBitmapPng(*PipelineImageOps::Blank(10, 10, Color4(255, 0, 0, 255)));
    String source = EncodeBitmapPng(*PipelineImageOps::Blank(100, 100, Color4(0, 0, 255, 255)));

    // The scale the run measured, times the base density
    String stored = PipelinePngMeta::WithDpi(part, std::nullopt, 2.0);
    String tagged = PipelineRegions::TagExtractPart(*node, port, source, stored, part, false);
    EXPECT_EQ(PipelinePngMeta::ReadDpi(tagged).value_or(0), 960.0);
    EXPECT_EQ(PipelinePngMeta::ReadScale(tagged).value_or(0), 2.0);

    // The source's own density, then the node's setting
    String source300 = PipelinePngMeta::WithDpi(source, 300.0);
    EXPECT_EQ(PipelinePngMeta::ReadDpi(PipelineRegions::TagExtractPart(*node, port, source300, stored, part, false)).value_or(0), 600.0);
    node->SetConfigNumber("sourceDpi", 160);
    EXPECT_EQ(PipelinePngMeta::ReadDpi(PipelineRegions::TagExtractPart(*node, port, source300, stored, part, false)).value_or(0), 320.0);

    // A raw render cached before parts carried their scale is measured against its crop: the whole 100 px source, 10 px drawn
    node->RemoveConfig("sourceDpi");
    EXPECT_EQ(PipelinePngMeta::ReadScale(PipelineRegions::TagExtractPart(*node, port, source, part, part, true)).value_or(0), 0.1);
    // Not raw and untagged: unchanged
    EXPECT_EQ(PipelineRegions::TagExtractPart(*node, port, source, part, part, false), part);
}

TEST(PipelineExtractParts, AutoSplitAsksForKitPiecesAndReadsThem)
{
    PipelineRegions::SplitFrame pixels;
    pixels.boxes = "pixels";
    pixels.w = 800;
    pixels.h = 600;
    String prompt = PipelineRegions::AutoSplitPrompt(pixels);
    EXPECT_TRUE(prompt.StartsWith("You are cutting this 2D game image into the pieces of a UI KIT"));
    EXPECT_TRUE(prompt.Contains("integers in pixels of this 800 x 600 image"));
    EXPECT_TRUE(prompt.Contains("\"box\": [x0, y0, x1, y1]}]"));
    EXPECT_TRUE(PipelineRegions::AutoSplitPrompt(PipelineRegions::SplitFrame()).Contains("\"box_2d\": [ymin, xmin, ymax, xmax]}]"));

    String answer = R"json(```json
[{"name": "btn_green", "cut": "the rounded green button, without its PLAY label", "box": [400, 300, 600, 360]},
 {"name": "bg_forest", "cut": "the forest scene alone", "box": [0, 0, 800, 590]},
 {"name": "icon_coin", "box": [10, 10, 30, 30]}]
```)json";
    auto regions = PipelineRegions::ParseAutoSplit(answer, &pixels);
    ASSERT_EQ(regions.Count(), 3);
    // The background first and whole
    EXPECT_TRUE(regions[0].background);
    EXPECT_EQ(regions[0].name, "bg_forest - the forest scene alone");
    EXPECT_EQ(regions[0].x, 0.0f);
    EXPECT_EQ(regions[0].w, 1.0f);
    EXPECT_EQ(regions[1].name, "btn_green - the rounded green button, without its PLAY label");
    // A little air, a share of the piece's own size
    EXPECT_NEAR(regions[1].x, 0.5f - 0.012f, 1e-5f);
    EXPECT_EQ(regions[2].name, "icon_coin");
    EXPECT_FALSE(regions[2].background);

    // Gemini's grid
    auto grid = PipelineRegions::ParseAutoSplit(R"([{"name": "panel_purple", "box_2d": [100, 200, 500, 800]}])");
    ASSERT_EQ(grid.Count(), 1);
    EXPECT_NEAR(grid[0].x, 0.2f - 0.015f, 1e-5f);
    EXPECT_NEAR(grid[0].y, 0.1f - 0.015f, 1e-5f);

    EXPECT_TRUE(PipelineRegions::IsBackgroundPart("bg_forest - the scene", false));
    EXPECT_TRUE(PipelineRegions::IsBackgroundPart("Background plate", false));
    EXPECT_FALSE(PipelineRegions::IsBackgroundPart("bg_forest", true));
    EXPECT_FALSE(PipelineRegions::IsBackgroundPart("bgm_icon", false));
}

TEST(PipelineInputsById, UnnamedAndDuplicateInputsDoNotCollide)
{
    // The labels a prompt gives them
    PipelineNode node;
    node.inputs = { PipelinePort("p", "prompt", PipelinePortType::Text), PipelinePort("a", "", PipelinePortType::Text, true),
                    PipelinePort("b", "", PipelinePortType::Text, true), PipelinePort("c", "notes", PipelinePortType::Text, true),
                    PipelinePort("d", "notes", PipelinePortType::Text, true), PipelinePort("e", "style", PipelinePortType::Text, true) };
    struct Probe : PipelineNodeBase
    {
        static String Label(const PipelineNode& n, const PipelinePort& p) { return InputLabel(n, p); }
    };
    EXPECT_EQ(Probe::Label(node, node.inputs[1]), "input 1");
    EXPECT_EQ(Probe::Label(node, node.inputs[2]), "input 2");
    EXPECT_EQ(Probe::Label(node, node.inputs[3]), "notes 1");
    EXPECT_EQ(Probe::Label(node, node.inputs[4]), "notes 2");
    EXPECT_EQ(Probe::Label(node, node.inputs[5]), "style");

    // Text concat joins two unnamed inputs (one used to be lost)
    WorkDirGuard work;
    PipelineGraph graph;
    auto a = PipelineNodeRegistry::CreateNode("sourceText", Vec2F());
    a->SetConfigString("text", "first");
    auto b = PipelineNodeRegistry::CreateNode("sourceText", Vec2F());
    b->SetConfigString("text", "second");
    auto concat = PipelineNodeRegistry::CreateNode("textConcat", Vec2F());
    concat->SetConfigString("separator", "+");
    concat->SetCustomInputs({ PipelinePort("x", "", PipelinePortType::Text, true), PipelinePort("y", "", PipelinePortType::Text, true) });
    PipelineNodeRegistry::SyncNodeWithSchema(concat);
    graph.nodes = { a, b, concat };
    Link(graph, a, a->outputs[0].id, concat, "x");
    Link(graph, b, b->outputs[0].id, concat, "y");

    PipelineValue out;
    bool done = false;
    auto executor = mmake<PipelineExecutor>();
    executor->onEvent = [&](const PipelineExecEvent& e)
    {
        if (e.type == PipelineExecEvent::Type::NodeOutput && e.nodeId == concat->id) out = e.value;
        if (e.type == PipelineExecEvent::Type::Done || e.type == PipelineExecEvent::Type::Fatal) done = true;
    };
    executor->Execute("inputs", graph, concat->id);
    ASSERT_TRUE(NetPumpUntil([&] { return done; }, 20.0f));
    EXPECT_EQ(out.data, "first+second");
}

// AssetsLine's executor test: style -> kit -> layout; the kit carries the prepared button and the style, the layout renders
// red where the kit's button is and writes both outputs; the layout node runs with its kit input not produced yet too
TEST(PipelineUiNodes, LayoutRendersWithTheKitAndWritesBothOutputs)
{
    WorkDirGuard work;
    o2FileSystem.FolderCreate(PipelineUtils::GetUploadsPath(), true);
    // A red rounded-ish button on a flat green backdrop the kit keys out
    auto picture = PipelineImageOps::Blank(360, 200, Color4(40, 200, 60, 255));
    for (int y = 40; y < 160; y++)
        for (int x = 40; x < 320; x++)
            std::memcpy(PipelineImageOps::Pixel(*picture, x, y), "\xD8\x20\x20\xFF", 4);
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("btn.png"), EncodeBitmapPng(*picture));

    String screen = "screen Menu 1080x1920\n"
        "  background bg anchor=stretch kit=bg_plain\n"
        "  text title anchor=top-stretch pos=0,160 h=140 margin=0,60 style=title \"FOREST QUEST\"\n"
        "  column menu anchor=center size=640x600 gap=30\n"
        "    button play h=140 kit=btn_red \"PLAY\"\n"
        "    button settings h=120 \"SETTINGS\"\n";
    auto parsed = UiDoc::ParseUiml(screen);
    ASSERT_TRUE(parsed.errors.IsEmpty());

    PipelineGraph graph;
    auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F());
    source->SetConfigString("uploadId", "btn.png");
    auto style = PipelineNodeRegistry::CreateNode("textStyle", Vec2F());
    auto kit = PipelineNodeRegistry::CreateNode("uiKit", Vec2F());
    kit->SetCustomInputs({ PipelinePort("kit-btn", "btn", PipelinePortType::Image, true),
                           PipelinePort("kit-st", "title", PipelinePortType::TextStyle, true) });
    PipelineNodeRegistry::SyncNodeWithSchema(kit);
    DataDocument items;
    items.LoadFromData(R"json({"kit-btn":{"name":"btn_red","type":"button","assetType":"sliced","bg":"key","variant":"primary"}})json");
    kit->config["items"] = static_cast<const DataValue&>(items);
    auto layout = PipelineNodeRegistry::CreateNode("uiLayout", Vec2F());
    UiDoc::WriteSteps(layout->config, { UiDoc::MakeAgentStep("menu", "ok", { UiDoc::MakeDocOp(parsed.doc) }, 0, "gpt-6.1-sol") }, {});
    graph.nodes = { source, style, kit, layout };
    Link(graph, source, source->outputs[0].id, kit, "kit-btn");
    Link(graph, style, style->outputs[0].id, kit, "kit-st");
    Link(graph, kit, kit->outputs[0].id, layout, layout->inputs[0].id);

    Map<String, PipelineValue> outputs;
    bool done = false;
    String fatal;
    auto executor = mmake<PipelineExecutor>();
    executor->onEvent = [&](const PipelineExecEvent& e)
    {
        if (e.type == PipelineExecEvent::Type::NodeOutput)
            outputs[e.nodeId + (e.portId.IsEmpty() ? String() : "#" + e.portId)] = e.value;
        if (e.type == PipelineExecEvent::Type::Done) done = true;
        if (e.type == PipelineExecEvent::Type::Fatal) fatal = e.error;
    };
    executor->Execute("uilayout", graph, layout->id);
    ASSERT_TRUE(NetPumpUntil([&] { return done || !fatal.IsEmpty(); }, 60.0f));
    ASSERT_TRUE(fatal.IsEmpty()) << fatal;

    auto kitValue = outputs[kit->id];
    EXPECT_EQ(kitValue.type, PipelinePortType::UiKit);
    auto parsedKit = UiDoc::ParseKit(kitValue.data);
    ASSERT_EQ(parsedKit.elements.Count(), 1);
    EXPECT_EQ(parsedKit.elements[0].name, "btn_red");
    EXPECT_EQ(parsedKit.elements[0].assetType, "sliced");
    EXPECT_TRUE(parsedKit.elements[0].nineSlice.has_value());
    ASSERT_EQ(parsedKit.textStyles.Count(), 1);

    auto image = outputs[layout->id + "#" + layout->outputs[0].id];
    ASSERT_TRUE(image.IsImage());
    auto bitmap = image.GetBitmap();
    ASSERT_TRUE(bitmap);
    EXPECT_EQ(bitmap->GetSize(), Vec2I(1080, 1920));
    EXPECT_EQ(PipelinePngMeta::ReadDpi(image.data).value_or(0), 480.0);
    // The play button (220,660 640x140) in the kit's red
    auto px = PipelineImageOps::Pixel(*bitmap, 240, 730);
    EXPECT_GT((int)px[0], 180);
    EXPECT_LT((int)px[1], 90);

    UiLayout::Json doc;
    ASSERT_TRUE(UiLayout::Json::Parse(outputs[layout->id + "#" + layout->outputs[1].id].data, doc));
    EXPECT_EQ(doc.Get("name")->Str(), "Menu");
}
