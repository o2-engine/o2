#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/Nodes/UiNodes.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/Providers/AiRouter.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAi.h"
#include "o2Editor/Pipeline/UiDoc/UiDocOps.h"
#include "o2Editor/Pipeline/UiDoc/UiDocOwn.h"
#include "o2Editor/Pipeline/UiDoc/UiStyleAi.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

using namespace o2;
using namespace Editor;

// AssetsLine after 77d5b09: the AI text style node, the layout node's own graphics as outputs, the check keys in the
// signature, what a kit and a graphic's consumer key on (backend/test textStyleExtract.test.ts, uiLayoutOutputs.test.ts)

namespace
{
    struct WorkDir
    {
        String path;

        WorkDir()
        {
            String relative = "./pipeline-ui-nodes2-work-" + (String)(int)(Math::Random()%1000000);
            o2FileSystem.FolderCreate(relative, true);
            path = o2FileSystem.CanonicalizePath(relative) + "/";
            PipelineUtils::SetWorkPathOverride(path);
            o2FileSystem.FolderCreate(PipelineUtils::GetUploadsPath(), true);
        }

        ~WorkDir()
        {
            PipelineUtils::SetWorkPathOverride("");
            o2FileSystem.FolderRemove(path, true);
        }
    };

    struct StubGuard
    {
        ~StubGuard() { UiDoc::SetUiAiTextStub({}); }
    };

    void Link(PipelineGraph& graph, const Ref<PipelineNode>& from, const String& fromPort, const Ref<PipelineNode>& to, const String& toPort)
    {
        auto edge = mmake<PipelineEdge>();
        edge->id = PipelineNode::GenerateId();
        edge->fromNodeId = from->id;
        edge->fromPortId = fromPort;
        edge->toNodeId = to->id;
        edge->toPortId = toPort;
        graph.edges.Add(edge);
    }

    // Runs the graph to the target; the outputs by "<node>" / "<node>#<port>"
    Map<String, PipelineValue> RunGraph(const PipelineGraph& graph, const String& target, Vector<String>* errors = nullptr)
    {
        Map<String, PipelineValue> outputs;
        bool done = false;
        String fatal;
        auto executor = mmake<PipelineExecutor>();
        executor->onEvent = [&](const PipelineExecEvent& e)
        {
            if (e.type == PipelineExecEvent::Type::NodeOutput)
                outputs[e.nodeId + (e.portId.IsEmpty() ? String() : "#" + e.portId)] = e.value;
            if (e.type == PipelineExecEvent::Type::NodeState && e.state == "error" && errors)
                errors->Add(e.error);
            if (e.type == PipelineExecEvent::Type::Done) done = true;
            if (e.type == PipelineExecEvent::Type::Fatal) fatal = e.error;
        };
        executor->Execute("uinodes2", graph, target);
        EXPECT_TRUE(NetPumpUntil([&] { return done || !fatal.IsEmpty(); }, 60.0f));
        if (!errors)
            EXPECT_TRUE(fatal.IsEmpty()) << fatal.Data();
        return outputs;
    }

    String Picture(int w, int h, bool transparent)
    {
        auto bmp = PipelineImageOps::Blank(w, h, transparent ? Color4(0, 0, 0, 0) : Color4(30, 60, 200, 255));
        for (int y = h/3; y < 2*h/3; y++)
            for (int x = w/4; x < 3*w/4; x++)
                std::memcpy(PipelineImageOps::Pixel(*bmp, x, y), "\xFF\xE0\x40\xFF", 4);
        return EncodeBitmapPng(*bmp);
    }

    const char* answer = "```json\n{\"text\":\"PLAY\",\"name\":\"button label\",\"role\":\"button\",\"font\":\"Lilita One\",\"weight\":400,"
                         "\"capHeight\":40,\"screenWidth\":540,\"color\":\"#FFE45C\",\"effects\":[{\"type\":\"stroke\",\"color\":\"#5A2A00\",\"width\":3}],"
                         "\"uppercase\":true,\"description\":\"labels of the main buttons\"}\n```";
}

TEST(PipelineTextStyleExtract, SchemaAsAssetsLine)
{
    auto schema = PipelineNodeRegistry::GetSchema("textStyleExtract");
    ASSERT_TRUE(schema);
    EXPECT_EQ(schema->label, "AI text style");
    EXPECT_EQ(schema->category, PipelineNodeCategory::AI);
    EXPECT_FALSE(schema->instant);
    ASSERT_EQ(schema->inputs.Count(), 1);
    EXPECT_EQ(schema->inputs[0].name, "image");
    ASSERT_EQ(schema->outputs.Count(), 1);
    EXPECT_EQ(schema->outputs[0].name, "style");
    EXPECT_EQ(schema->outputs[0].portType, PipelinePortType::TextStyle);
    EXPECT_EQ(schema->defaultSize, Vec2F(360, 0));
    EXPECT_EQ(PipelineNodeRegistry::CreateNode("textStyleExtract", Vec2F())->GetConfigString("model"), "gpt-6.1-sol");

    auto layout = PipelineNodeRegistry::GetSchema("uiLayout");
    ASSERT_TRUE(layout);
    EXPECT_TRUE(layout->addableInputs.Contains(PipelinePortType::UiKit));
}

TEST(PipelineTextStyleExtract, OneVisionCallTheNameAndRoleAfterIt)
{
    WorkDir work;
    StubGuard guard;
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("label.png"), Picture(600, 300, true));

    int calls = 0;
    String prompt;
    Vector<AiImageRef> images;
    UiDoc::SetUiAiTextStub([&](const String& model, const String& p, const Vector<AiImageRef>& pics) {
        calls++;
        prompt = p;
        images = pics;
        EXPECT_EQ(model, "gpt-6.1-sol");
        AiTextResult r;
        r.ok = true;
        r.text = answer;
        return r;
    });

    PipelineGraph graph;
    auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F());
    source->SetConfigString("uploadId", "label.png");
    auto node = PipelineNodeRegistry::CreateNode("textStyleExtract", Vec2F());
    node->SetConfigString("hint", "  the button label ");
    graph.nodes = { source, node };
    Link(graph, source, source->outputs[0].id, node, node->inputs[0].id);

    auto outputs = RunGraph(graph, node->id);
    EXPECT_EQ(calls, 1);
    // The prompt says the picture's size, the matte and fences the hint as data
    EXPECT_NE(((std::string)prompt).find("The picture is 600x300 px. Its transparent areas are shown as flat #808080"), std::string::npos);
    EXPECT_NE(((std::string)prompt).find("<lettering>\nthe button label\n</lettering>"), std::string::npos);
    ASSERT_EQ(images.Count(), 1);
    UiLayout::UiImage sent;
    ASSERT_TRUE(UiLayout::UiImage::Decode(images[0].data, sent));
    EXPECT_EQ(sent.At(0, 0)[0], 0x80);
    EXPECT_EQ(sent.At(0, 0)[3], 255);

    auto value = outputs[node->id];
    ASSERT_EQ(value.type, PipelinePortType::TextStyle);
    UiLayout::Json style;
    ASSERT_TRUE(UiLayout::Json::Parse(value.data, style));
    EXPECT_EQ(style.Get("name")->Str(), "button_label");
    EXPECT_EQ(style.Get("role")->Str(), "button");
    EXPECT_EQ(style.Get("font")->Str(), "Lilita One");
    // capHeight 40 on a 540-wide screen: 40 / 0.71 * 2 = 112.7 -> 113 px on the 1080 screen
    EXPECT_EQ(style.Get("size")->Num(), 113);
    EXPECT_EQ(style.Get("extracted")->Get("text")->Str(), "PLAY");

    // A name and a role set in the node are a local step: no new call
    node->SetConfigString("name", "big label");
    node->SetConfigString("role", "title");
    outputs = RunGraph(graph, node->id);
    EXPECT_EQ(calls, 1);
    ASSERT_TRUE(UiLayout::Json::Parse(outputs[node->id].data, style));
    EXPECT_EQ(style.Get("name")->Str(), "big_label");
    EXPECT_EQ(style.Get("role")->Str(), "title");
    EXPECT_EQ(style.Get("extracted")->Get("name")->Str(), "button_label");

    // Cleared again: what the model answered
    node->SetConfigString("name", "");
    node->SetConfigString("role", "");
    outputs = RunGraph(graph, node->id);
    EXPECT_EQ(calls, 1);
    ASSERT_TRUE(UiLayout::Json::Parse(outputs[node->id].data, style));
    EXPECT_EQ(style.Get("name")->Str(), "button_label");

    // The style goes into a UI kit as a plain style
    auto kit = PipelineNodeRegistry::CreateNode("uiKit", Vec2F());
    kit->SetCustomInputs({ PipelinePort("kit-st", "style", PipelinePortType::TextStyle, true) });
    PipelineNodeRegistry::SyncNodeWithSchema(kit);
    graph.nodes.Add(kit);
    Link(graph, node, node->outputs[0].id, kit, "kit-st");
    outputs = RunGraph(graph, kit->id);
    EXPECT_EQ(calls, 1);
    auto parsed = UiDoc::ParseKit(outputs[kit->id].data);
    ASSERT_EQ(parsed.textStyles.Count(), 1);
    EXPECT_EQ(parsed.textStyles[0].name, "button_label");
    EXPECT_TRUE(parsed.textStyles[0].extracted.IsNull());
}

TEST(PipelineTextStyleExtract, TheFrameIsWhatTheModelSeesAndTheScreenStaysTheWholePicture)
{
    UiDoc::UiStylePicture whole;
    String png = Picture(800, 400, false);
    ASSERT_TRUE(UiDoc::StyleSourcePicture(png, nullptr, whole));
    EXPECT_EQ(whole.width, 800);
    EXPECT_FALSE(whole.matte);
    EXPECT_EQ(whole.png, png);

    PipelineImageOps::CropRect frame;
    frame.x = 0.25f;
    frame.y = 0.25f;
    frame.w = 0.5f;
    frame.h = 0.5f;
    UiDoc::UiStylePicture part;
    ASSERT_TRUE(UiDoc::StyleSourcePicture(png, &frame, part));
    EXPECT_EQ(part.width, 400);
    EXPECT_EQ(part.height, 200);
    EXPECT_EQ(part.fullWidth, 800);

    // A large picture is sent at most 1024 px on its longer side, the whole picture's width at that scale
    String big = Picture(3000, 1000, false);
    frame.x = 0;
    frame.w = 0.5f;
    ASSERT_TRUE(UiDoc::StyleSourcePicture(big, &frame, part));
    EXPECT_EQ(part.width, 1024);
    EXPECT_EQ(part.fullWidth, 2048);
}

TEST(PipelineTextStyleExtract, NoLetteringFailsTheRun)
{
    WorkDir work;
    StubGuard guard;
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("empty.png"), Picture(64, 64, false));
    UiDoc::SetUiAiTextStub([&](const String&, const String&, const Vector<AiImageRef>&) {
        AiTextResult r;
        r.ok = true;
        r.text = "{\"none\": true}";
        return r;
    });

    PipelineGraph graph;
    auto source = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F());
    source->SetConfigString("uploadId", "empty.png");
    auto node = PipelineNodeRegistry::CreateNode("textStyleExtract", Vec2F());
    graph.nodes = { source, node };
    Link(graph, source, source->outputs[0].id, node, node->inputs[0].id);
    Vector<String> errors;
    RunGraph(graph, node->id, &errors);
    ASSERT_FALSE(errors.IsEmpty());
    EXPECT_EQ(errors[0], "No lettering found in the picture");
}

TEST(PipelineRefCheck, TheCheckKeysCountOnlyWhileItIsOn)
{
    for (const char* type : { "imageExtract", "textStyleExtract" })
    {
        auto node = PipelineNodeRegistry::CreateNode(type, Vec2F());
        String base = PipelineGraph::ComputeNodeSignature(*node, {}, {}, -1);
        node->SetConfigNumber("refCheckMax", 3);
        node->SetConfigString("refCheckModel", "other");
        EXPECT_EQ(PipelineGraph::ComputeNodeSignature(*node, {}, {}, -1), base) << type;
        node->SetConfigBool("refCheck", false);
        EXPECT_EQ(PipelineGraph::ComputeNodeSignature(*node, {}, {}, -1), base) << type;
        node->SetConfigBool("refCheck", true);
        String on = PipelineGraph::ComputeNodeSignature(*node, {}, {}, -1);
        EXPECT_NE(on, base) << type;
        node->SetConfigNumber("refCheckMax", 1);
        EXPECT_NE(PipelineGraph::ComputeNodeSignature(*node, {}, {}, -1), on) << type;
    }

    // The AI text style's stored generation leaves the name, the role and the check out
    auto node = PipelineNodeRegistry::CreateNode("textStyleExtract", Vec2F());
    String content = PipelineExecutor::ContentSignature(*node, {}, -1);
    node->SetConfigString("name", "x");
    node->SetConfigString("role", "title");
    node->SetConfigBool("refCheck", true);
    EXPECT_EQ(PipelineExecutor::ContentSignature(*node, {}, -1), content);
    node->SetConfigString("hint", "the title");
    EXPECT_NE(PipelineExecutor::ContentSignature(*node, {}, -1), content);

    // New UI-only keys of the layout node
    for (const char* key : { "agentImageModel", "agentDraw", "agentTurns", "uiPoints", "uiRefOn", "uiRefMode", "uiRefSplit" })
        EXPECT_TRUE(PipelineGraph::GetUiOnlyConfigKeys().Contains(key)) << key;
}

TEST(PipelineUiLayoutOutputs, GraphicsAreOutputsDerivedFromTheSteps)
{
    WorkDir work;
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("plate.png"), Picture(200, 100, true));

    auto layout = PipelineNodeRegistry::CreateNode("uiLayout", Vec2F());
    ASSERT_EQ(layout->outputs.Count(), 2);
    String imageId = layout->outputs[0].id;

    UiLayout::Json step;
    UiLayout::Json::Parse(R"json({"id":"s1","by":"agent","at":"","ops":[],"assets":[
        {"name":"plate","uploadId":"plate.png","type":"panel","assetType":"sliced","width":200,"height":100,"dpi":460},
        {"name":"Image","uploadId":"plate.png","type":"image","assetType":"simple","width":200,"height":100}]})json", step);
    UiDoc::WriteSteps(layout->config, { UiDoc::UiStep(step) }, {});
    PipelineNodeRegistry::SyncNodeWithSchema(layout);
    ASSERT_EQ(layout->outputs.Count(), 4);
    EXPECT_EQ(layout->outputs[0].id, imageId);
    EXPECT_EQ(layout->outputs[2].name, "plate");
    EXPECT_EQ(layout->outputs[2].id, "own-plate");
    EXPECT_EQ(layout->outputs[2].portType, PipelinePortType::Image);
    EXPECT_EQ(layout->outputs[3].name, "Image_2");
    EXPECT_EQ(layout->outputs[3].id, "own-image_2");

    // A port the node already has keeps its id (a copy re-mints them); an undone graphic's port goes
    layout->outputs[2].id = "minted";
    PipelineNodeRegistry::SyncNodeWithSchema(layout);
    EXPECT_EQ(layout->outputs[2].id, "minted");
    UiDoc::WriteSteps(layout->config, {}, { UiDoc::UiStep(step) });
    PipelineNodeRegistry::SyncNodeWithSchema(layout);
    EXPECT_EQ(layout->outputs.Count(), 2);
}

TEST(PipelineUiLayoutOutputs, AGraphicIsItsStoredPictureAndItsConsumersKeyOnIt)
{
    WorkDir work;
    String plate = Picture(200, 100, true);
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("plate.png"), plate);

    PipelineGraph graph;
    auto layout = PipelineNodeRegistry::CreateNode("uiLayout", Vec2F());
    UiLayout::Json step;
    UiLayout::Json::Parse(R"json({"id":"s1","by":"agent","at":"","ops":[],"assets":[
        {"name":"plate","uploadId":"plate.png","type":"panel","assetType":"sliced","width":200,"height":100,"dpi":460}]})json", step);
    UiDoc::WriteSteps(layout->config, { UiDoc::UiStep(step) }, {});
    PipelineNodeRegistry::SyncNodeWithSchema(layout);
    auto crop = PipelineNodeRegistry::CreateNode("imageCrop", Vec2F());
    graph.nodes = { layout, crop };
    Link(graph, layout, "own-plate", crop, crop->inputs[0].id);

    auto outputs = RunGraph(graph, crop->id);
    auto picture = outputs[layout->id + "#own-plate"];
    ASSERT_TRUE(picture.IsImage());
    EXPECT_EQ(PipelinePngMeta::ReadDpi(picture.data).value_or(0), 460.0);

    // What reads the graphic keys on the upload and its density, not on the layout's steps
    auto sigs = graph.ComputeSignatures();
    String cropSig = sigs[crop->id];
    UiLayout::Json more;
    UiLayout::Json::Parse(R"json({"id":"s2","by":"user","at":"","ops":[{"op":"screen","name":"Other"}]})json", more);
    UiDoc::WriteSteps(layout->config, { UiDoc::UiStep(step), UiDoc::UiStep(more) }, {});
    EXPECT_NE(graph.ComputeSignatures()[layout->id], sigs[layout->id]);
    EXPECT_EQ(graph.ComputeSignatures()[crop->id], cropSig);
}

TEST(PipelineUiKitDensity, AKitBelowAnExtractPartKeysOnThePartDensityRule)
{
    auto extract = PipelineNodeRegistry::CreateNode("imageExtract", Vec2F());
    auto kit = PipelineNodeRegistry::CreateNode("uiKit", Vec2F());
    auto crop = PipelineNodeRegistry::CreateNode("imageCrop", Vec2F());
    EXPECT_EQ(PipelineNodeRegistry::UpstreamSigFor("abc", extract.Get(), "p", *kit), "abc|part-density:3");
    EXPECT_EQ(PipelineNodeRegistry::UpstreamSigFor("abc", extract.Get(), "p", *crop), "abc");
    EXPECT_EQ(PipelineNodeRegistry::UpstreamSigFor("", extract.Get(), "p", *kit), "");
    EXPECT_EQ(PipelineNodeRegistry::UpstreamSigFor("abc", nullptr, "p", *kit), "abc");
}

TEST(PipelineUiKitDevice, APartIsSizedOnTheKitsDeviceAndItsTextureResampled)
{
    // An AI extract part: a 120-px square the model drew twice the source's size, cut from a 200x100 picture
    auto square = PipelineImageOps::Blank(120, 120, Color4(220, 30, 30, 255));
    String part = PipelinePngMeta::WithDpi(EncodeBitmapPng(*square), 960.0, 2.0, 200, 100);

    auto kitNode = PipelineNodeRegistry::CreateNode("uiKit", Vec2F());
    kitNode->SetCustomInputs({ PipelinePort("kit-a", "a", PipelinePortType::Image, true),
                               PipelinePort("kit-st", "st", PipelinePortType::TextStyle, true) });
    PipelineNodeRegistry::SyncNodeWithSchema(kitNode);
    auto styleNode = PipelineNodeRegistry::CreateNode("textStyle", Vec2F());
    Map<String, PipelineValue> byId;
    byId["kit-a"] = PipelineValue::ImageBytes(part);
    byId["kit-st"] = PipelineValue::TextOf(PipelinePortType::TextStyle, PipelineUiNodes::TextStyleOutput(*styleNode));

    // The picture is the device's screen (iPhone 16): the 60-px square is 30 % of the width - 354 px there, 0.77 in. The
    // part has only 120 px, so it is kept as it is at the density it then has
    auto kit = PipelineUiNodes::BuildKit(*kitNode, byId, {});
    ASSERT_TRUE(kit.device.has_value());
    EXPECT_EQ(UiDoc::TargetToJson(*kit.device).Dump(), "{\"id\":\"iphone-16\",\"name\":\"iPhone 16\",\"w\":1179,\"h\":2556,\"dpi\":460}");
    ASSERT_EQ(kit.elements.Count(), 1);
    auto el = kit.elements[0];
    EXPECT_EQ(el.width, 120);
    EXPECT_EQ(el.height, 120);
    EXPECT_EQ(el.dpi.value_or(0), 156.1);
    EXPECT_EQ(el.dpiFrom.value_or(""), "kit");
    EXPECT_EQ(el.sizeFrom.value_or(""), "source");
    EXPECT_EQ(el.share.value_or(0), 0.3);
    EXPECT_TRUE(el.soft);
    EXPECT_FALSE(el.texture.has_value());
    EXPECT_EQ(el.inches->w, 0.769);
    EXPECT_EQ(el.inches->h, 0.769);
    // Text styles come from the 1080-px screen to the device's pixels (by its shorter side): 64 x 1179/1080 = 70
    ASSERT_EQ(kit.textStyles.Count(), 1);
    EXPECT_EQ(kit.textStyles[0].size, 70);

    // A small device: the square is 30 px there, and the texture is resampled to exactly that
    UiDoc::Json small;
    UiDoc::Json::Parse(R"({"w":100,"h":64,"dpi":200})", small);
    kitNode->SetConfigString("kitDevice", "custom");
    UiDoc::SetConfigJson(kitNode->config, "kitDeviceCustom", small);
    kit = PipelineUiNodes::BuildKit(*kitNode, byId, {});
    el = kit.elements[0];
    EXPECT_EQ(kit.device->w, 100);
    EXPECT_EQ(el.width, 30);
    EXPECT_EQ(el.height, 30);
    EXPECT_EQ(el.dpi.value_or(0), 200);
    EXPECT_FALSE(el.soft);
    ASSERT_TRUE(el.texture.has_value());
    EXPECT_EQ(el.texture->w, 120);
    EXPECT_EQ(el.inches->w, 0.15);
    UiLayout::UiImage tex;
    ASSERT_TRUE(UiLayout::UiImage::Decode(PipelineUtils::Base64Decode(el.png), tex));
    EXPECT_EQ(tex.w, 30);
    EXPECT_EQ(tex.h, 30);

    // The element's own density: the same size, twice the pixels
    UiDoc::Json items;
    UiDoc::Json::Parse(R"({"kit-a":{"dpi":400}})", items);
    UiDoc::SetConfigJson(kitNode->config, "items", items);
    el = PipelineUiNodes::BuildKit(*kitNode, byId, {}).elements[0];
    EXPECT_EQ(el.width, 60);
    EXPECT_EQ(el.dpi.value_or(0), 400);
    EXPECT_EQ(el.dpiFrom.value_or(""), "item");
    EXPECT_EQ(el.inches->w, 0.15);

    // A size typed for the element, inches
    UiDoc::Json::Parse(R"({"kit-a":{"size":{"w":0.3,"h":0.2}}})", items);
    UiDoc::SetConfigJson(kitNode->config, "items", items);
    el = PipelineUiNodes::BuildKit(*kitNode, byId, {}).elements[0];
    EXPECT_EQ(el.width, 60);
    EXPECT_EQ(el.height, 40);
    EXPECT_EQ(el.dpi.value_or(0), 200);
    EXPECT_EQ(el.sizeFrom.value_or(""), "item");
    EXPECT_EQ(el.inches->w, 0.3);
    EXPECT_EQ(el.inches->h, 0.2);
}

TEST(PipelineImageEditRefs, ReferencePicturesGoAfterTheOriginalNamedAndNeverCropped)
{
    WorkDir work;
    struct ImageStubGuard
    {
        ~ImageStubGuard() { AiRouter::SetImageStub({}); }
    } imageGuard;

    auto solid = [](int w, int h, Color4 c) { return EncodeBitmapPng(*PipelineImageOps::Blank(w, h, c)); };
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("orig.png"), solid(200, 100, Color4(0x33, 0x66, 0x99, 255)));
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("hat.png"), solid(50, 70, Color4(0xCC, 0x22, 0x22, 255)));
    PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath("other.png"), solid(33, 21, Color4(0x22, 0xCC, 0x22, 255)));

    auto schema = PipelineNodeRegistry::GetSchema("imageEdit");
    ASSERT_TRUE(schema);
    EXPECT_EQ(schema->addableInputs, Vector<PipelinePortType>({ PipelinePortType::Image }));

    PipelineGraph graph;
    auto source = [&](const char* upload) {
        auto n = PipelineNodeRegistry::CreateNode("sourceImage", Vec2F());
        n->SetConfigString("uploadId", upload);
        graph.nodes.Add(n);
        return n;
    };
    auto orig = source("orig.png"), hat = source("hat.png"), other = source("other.png");
    auto edit = PipelineNodeRegistry::CreateNode("imageEdit", Vec2F());
    edit->SetConfigString("model", "gemini-3.1-flash-image");
    edit->SetConfigString("prompt", "put the hat from \"hat\" on the cat");
    edit->SetCustomInputs({ PipelinePort("c1", "hat", PipelinePortType::Image, true), PipelinePort("c2", "", PipelinePortType::Image, true),
                            PipelinePort("c3", "", PipelinePortType::Image, true) });
    PipelineNodeRegistry::SyncNodeWithSchema(edit);
    graph.nodes.Add(edit);
    Link(graph, orig, orig->outputs[0].id, edit, edit->inputs[0].id);
    Link(graph, hat, hat->outputs[0].id, edit, "c1");
    Link(graph, other, other->outputs[0].id, edit, "c2");

    Vector<Vec2I> sizes;
    String prompt;
    AiRouter::SetImageStub([&](const String&, const String& p, const Vector<AiImageRef>& refs, int, bool, const AiImageOptions&) {
        prompt = p;
        sizes.Clear();
        for (auto& r : refs)
        {
            auto bmp = DecodeImageBytes(r.data);
            sizes.Add(bmp ? bmp->GetSize() : Vec2I());
        }
        AiBytesResult result;
        result.ok = true;
        result.mimeType = "image/png";
        result.data = EncodeBitmapPng(*PipelineImageOps::Blank(200, 100, Color4(1, 2, 3, 255)));
        return result;
    });

    RunGraph(graph, edit->id);
    EXPECT_EQ(sizes, Vector<Vec2I>({ Vec2I(200, 100), Vec2I(50, 70), Vec2I(33, 21) })) << "the original first, then the references";
    EXPECT_TRUE(prompt.Contains("After the original come 2 REFERENCE image(s) the user supplied: image 2 is \"hat\", image 3 is \"reference 2\"."));

    // A frame: the original is cropped around it, the references are not
    UiLayout::Json frame;
    UiLayout::Json::Parse(R"({"x":0.25,"y":0.25,"w":0.5,"h":0.5})", frame);
    UiDoc::SetConfigJson(edit->config, "editRegion", frame);
    RunGraph(graph, edit->id);
    ASSERT_EQ(sizes.Count(), 3);
    EXPECT_NE(sizes[0], Vec2I(200, 100));
    EXPECT_EQ(sizes[1], Vec2I(50, 70));
    EXPECT_EQ(sizes[2], Vec2I(33, 21));

    // Labels are quoted: no double quotes in them, two inputs of one name told apart
    UiDoc::SetConfigJson(edit->config, "editRegion", UiLayout::Json());
    edit->SetCustomInputs({ PipelinePort("c1", "big \"hat\"", PipelinePortType::Image, true),
                            PipelinePort("c2", "big  'hat'", PipelinePortType::Image, true) });
    PipelineNodeRegistry::SyncNodeWithSchema(edit);
    RunGraph(graph, edit->id);
    EXPECT_TRUE(prompt.Contains("image 2 is \"big 'hat'\", image 3 is \"big 'hat' (2)\".")) << prompt.Data();
}
