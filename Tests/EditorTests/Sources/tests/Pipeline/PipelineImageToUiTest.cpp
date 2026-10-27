#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageToUi.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAi.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutParse.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPrompts.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutRunSteps.h"

using namespace o2;
using namespace Editor;
using namespace Editor::UiLayout;

namespace
{
    String Sha256Hex(const String& text)
    {
        static const char* hex = "0123456789abcdef";
        String digest = PipelineUtils::Sha256(text), out;
        for (int i = 0; i < digest.Length(); i++)
        {
            unsigned char c = (unsigned char)digest[i];
            out += hex[c >> 4];
            out += hex[c & 15];
        }
        return out;
    }

    // The fixture of a web run (design, model answers by tag, expected outputs), outside the repository
    String FixturePath()
    {
        if (const char* env = getenv("O2_IMAGE_TO_UI_FIXTURE"))
            return UiLayout::HasSuffix(String(env), "/") ? String(env) : String(env) + "/";

        for (auto& candidate : { "../../Work/ImageToUiFixture/", "../../../../Work/ImageToUiFixture/", "../../../Work/ImageToUiFixture/" })
        {
            if (o2FileSystem.IsFileExist(String(candidate) + "design.png"))
                return candidate;
        }
        return "";
    }

}

TEST(ImageToUiJson, WritesAndReadsAsJavaScriptDoes)
{
    EXPECT_EQ(Json::FormatNumber(0.7361), "0.7361");
    EXPECT_EQ(Json::FormatNumber(21.58), "21.58");
    EXPECT_EQ(Json::FormatNumber(270), "270");
    EXPECT_EQ(Json::FormatNumber(-3.5), "-3.5");
    EXPECT_EQ(Json::FormatNumber(1e21), "1e+21");
    EXPECT_EQ(Json::FormatNumber(1e-7), "1e-7");
    EXPECT_EQ(Json::FormatNumber(0.000001), "0.000001");
    EXPECT_EQ(Json::FormatNumber(0.1 + 0.2), "0.30000000000000004");

    Json object = Json::Object();
    object["b"] = 1;
    object["a"] = Json::Array();
    object["a"].Add("x\"y");
    object["a"].Add(Json::Object());
    object["c"] = Json::Array();
    EXPECT_EQ(object.Dump(), "{\"b\":1,\"a\":[\"x\\\"y\",{}],\"c\":[]}");
    EXPECT_EQ(object.Dump(2), "{\n  \"b\": 1,\n  \"a\": [\n    \"x\\\"y\",\n    {}\n  ],\n  \"c\": []\n}");

    Json parsed;
    ASSERT_TRUE(Json::Parse("{\"k\": [1, 2.5, true, null, \"\\u00e9\\n\"], \"k\": 7}", parsed));
    EXPECT_EQ(parsed.Count(), 1) << "a repeated key keeps its place and the last value";
    EXPECT_EQ(parsed.Get("k")->Num(), 7);
    EXPECT_FALSE(Json::Parse("{\"a\": 1,}", parsed));

    Json fenced;
    ASSERT_TRUE(JsonOf("Here it is:\n```json\n{\"fixes\": []}\n```\nDone.", fenced));
    EXPECT_TRUE(fenced.Get("fixes")->IsArray());
    EXPECT_EQ(Json("  12 ").ToJsNumber(), 12);
    EXPECT_TRUE(std::isnan(Json("12px").ToJsNumber()));
}

TEST(ImageToUiPrompts, AreTheReferenceStrings)
{
    EXPECT_EQ(Sha256Hex(Prompts::Analyze(471, 1026, "  a word game  ")), "3de72630db49057589de177dfadebb318b17502dae8bf56e8ae2ccc4b83ad55c");
    EXPECT_EQ(Sha256Hex(Prompts::Analyze(471, 1026, "")), "c15ce63dd97c1fb8cbb45d380cd234ef7307129e49eca6f414a6450fe8df6288");
    EXPECT_EQ(Sha256Hex(Prompts::Cut("small square orange tile", { "the letter \"U\"", "the number \"3\"" }, "pure green", "#00FF00")),
              "0a92ae70c947e57d85143ad3e83d6e2c91da3e3f5589ebcdb596aee757cee83e");
    EXPECT_EQ(Sha256Hex(Prompts::Cut("white checkmark symbol", {}, "pure magenta", "#FF00FF")),
              "da76e750b39e4f72f603cc909f018c1ad807e5af577da36b8e8a68eba31934a1");
    EXPECT_EQ(Sha256Hex(Prompts::Background("light beige background")), "615f3c5b981f9c8a206bdadc99d6582dd1a040e6c577daf052fba3df3d1380c0");
    EXPECT_EQ(Sha256Hex(Prompts::Background("")), "cbfc1e62edf0866f7b16fb5ae5664bde918bc8359d78b5244ce1f2eb8927f1b9");
    EXPECT_EQ(Sha256Hex(Prompts::Background("light beige background", true)), "098541c7b8fe77660742909abde7f43d6ed3fa66a4a4403bc9d5e72e42505032");
    EXPECT_EQ(Sha256Hex(Prompts::Background("", true)), "7e2c587825c1856d8edbbbaf6c0957f1c384097a05706e47aea10f51fa233d38");
}

TEST(ImageToUiAi, MemoKeysAreTheReferenceOnes)
{
    EXPECT_EQ(TextMemoKey("gemini-3.1-pro-preview", "Describe it", { "PNGBYTES" }), "uil-t-fc54127ef3f81ba76a716abeadd1f0791e033b1b");
    EXPECT_EQ(ImageMemoKey("gemini-3.1-flash-image", "Cut it", "1:1", 12345, { "REF" }), "uil-i-e2e30c422aec63cd85d1511e59dbcbfc984e1a3e");
    EXPECT_EQ(ImageMemoKey("gemini-3.1-flash-image", "Cut it", "", -1, { "REF" }), "uil-i-e5d1fbbc7d52c734cc40cdcea63a01d6241b3902");
    EXPECT_EQ(PipelineExecutor::BundleKey("abc", "sprite"), "f2dd2997c9950f40690b5e1252dbd826");

    EXPECT_EQ(ImageMimeOf(String(std::string("\xFF\xD8\xFF\xE0", 4))), "image/jpeg") << "the agent's model-sized copies are JPEG";
    EXPECT_EQ(ImageMimeOf("\x89PNG"), "image/png");
}

TEST(ImageToUiParse, FixtureAnalysisIsTheWebModel)
{
    String fixture = FixturePath();
    if (fixture.IsEmpty())
        GTEST_SKIP() << "no image-to-UI fixture: set O2_IMAGE_TO_UI_FIXTURE";

    UiDoc doc;
    ASSERT_TRUE(ParseAnalysis(PipelineUtils::ReadFileBytes(fixture + "answers/analysis.txt"), 471, 1026, doc));
    EXPECT_EQ(doc.els.Count(), 270) << "grids expanded into cells and their texts";
    EXPECT_EQ(doc.looks.Count(), 25);
    EXPECT_EQ(doc.background.kind, "color");
    EXPECT_EQ(doc.background.color, "#F1E5D1");

    String lines;
    char buffer[64];
    for (int i = 0; i < doc.els.Count(); i++)
    {
        auto& e = doc.els[i];
        auto f = [&](double v) { snprintf(buffer, sizeof(buffer), "%.3f", v); return String(buffer); };
        lines += (i ? "\n" : "") + e->id + "|" + e->parent + "|" + e->kind + "|" + e->look + "|" + e->text + "|" + f(e->box.x) + "|" +
            f(e->box.y) + "|" + f(e->box.w) + "|" + f(e->box.h);

        if (!e->parent.IsEmpty())
        {
            int parent = doc.IndexOf(doc.ById(e->parent));
            EXPECT_GE(parent, 0) << e->id.Data();
            EXPECT_LT(parent, i) << "a parent comes before its child: " << e->id.Data();
        }
    }
    EXPECT_EQ(Sha256Hex(lines), "0efb42573068c5f619f38cae00a9d0cbe4fe4f172e2802509b2d824eb5bf7cb5") << "ids, parents, kinds, looks and boxes";
}

TEST(ImageToUiModel, SafeKeysAndHelpers)
{
    EXPECT_EQ(SafeKey("Tile Cream #2"), "tile_cream_2");
    EXPECT_EQ(SafeKey("__a__b__"), "a_b");
    EXPECT_EQ(SafeKey("!!!"), "el");
    EXPECT_EQ(SafeKey("", "look"), "look");
    EXPECT_EQ(SafeKey(String(std::string(60, 'a'))).Length(), 48);
    EXPECT_EQ(Clamp8(127.5), 128);
    EXPECT_EQ(Clamp8(126.5), 126);
    EXPECT_EQ(JsRound(-2.5), -2);
    EXPECT_EQ(Utf16Length("\xD0\x94\xD0\x90"), 2);
    EXPECT_EQ(PipelineImageToUi::SpriteName("preview"), "sprite_preview");
    EXPECT_EQ(PipelineImageToUi::SpriteName("tile_cream"), "tile_cream");
    EXPECT_EQ(PipelineExecutor::DynamicPortId("tile cream/2"), "sp-tile_cream_2");
}

TEST(ImageToUiSlice, NineSliceFromTheCornerRounding)
{
    UiImage img = UiImage::Blank(60, 40, 0, 0, 0, 0);
    for (int y = 0; y < 40; y++)
    {
        for (int x = 0; x < 60; x++)
        {
            int cx = Math::Clamp(x, 6, 53), cy = Math::Clamp(y, 6, 33);
            if ((x - cx)*(x - cx) + (y - cy)*(y - cy) <= 36)
                img.At(x, y)[3] = 255;
        }
    }
    NineSlice nine = NineOf(img);
    EXPECT_EQ(nine.left, 11) << "the diagonal meets the matte 2 px in, round(2*3.4 + 4)";
    EXPECT_EQ(nine.top, 11);
    EXPECT_EQ(nine.right, nine.left);
    EXPECT_EQ(nine.bottom, nine.top);
}

TEST(ImageToUiAssets, FileNamesCarryTheKindAndTheLayoutsRefsAreCollected)
{
    const double screen = 240*320;
    EXPECT_EQ(AssetFileName("tile", "image", 60*60, screen), "icon_tile.png");
    EXPECT_EQ(AssetFileName("hero", "image", screen*0.5, screen), "hero.png");
    EXPECT_EQ(AssetFileName("panel_blue", "panel", 10, screen), "panel_blue.png");
    EXPECT_EQ(AssetFileName("card", "panel", 10, screen), "panel_card.png");
    EXPECT_EQ(AssetFileName("name_field", "input", 10, screen), "panel_name_field.png");
    EXPECT_EQ(AssetFileName("play", "button", 10, screen), "button_play.png");
    EXPECT_EQ(AssetFileName("btn_ok", "button", 10, screen), "btn_ok.png");
    EXPECT_EQ(AssetFileName("hp", "progress", 10, screen), "bar_hp.png");
    EXPECT_EQ(AssetFileName("Avatar Big", "image", 10, screen), "avatar_big.png");
    EXPECT_EQ(AssetFileName("!!!", "image", 10, screen), "icon_sprite.png");

    Json structure;
    ASSERT_TRUE(Json::Parse("{\"root\": {\"asset\": \"a.png\", \"children\": [{\"asset\": \"./b.png\"}, {\"state\": {\"variants\": "
                            "{\"on\": {\"x\": {\"asset\": \"c.png\"}}}}}, {\"asset\": \"a.png\"}]}}", structure));
    EXPECT_EQ(AssetRefs(structure), Vector<String>({ "a.png", "b.png", "c.png" })) << "states and variants included, ./ dropped";
}

TEST(ImageToUiAssets, TheSpritesNoteAndTheNineSliceMeta)
{
    NineSlice nine;
    nine.left = 11;
    nine.top = 12;
    nine.right = 13;
    nine.bottom = 14;
    EXPECT_EQ(NineSliceMeta(nine), "fileFormatVersion: 2\nTextureImporter:\n  spriteBorder: {x: 11, y: 14, z: 13, w: 12}\n");

    const String intro = "About the sliced assets: they were cut from this very design, one PNG per distinct piece of artwork, each at the "
        "size it appears, with the labels, numbers and other text drawn on it removed \xE2\x80\x94 the text belongs in text elements.";
    Vector<AssetFile> files = { { "panel_blue.png", "panel_blue", nine }, { "icon_tile.png", "tile", std::nullopt } };
    UiBackdrop color;
    color.kind = "color";
    color.color = "#F0E6D2";
    EXPECT_EQ(SpritesNote(files, color), intro + " 1 stretchable sprite(s) carry Unity nine-slice borders in their .png.meta "
              "(has_meta_border in the inventory). The backdrop is a flat colour #F0E6D2; there is no sprite for it.");
    UiBackdrop plate;
    plate.kind = "image";
    plate.key = "background";
    EXPECT_EQ(SpritesNote({ files[1] }, plate), intro + " `bg_backdrop.png` is the screen's backdrop with every UI element removed.");
    EXPECT_EQ(SpritesNote({}, UiBackdrop()), intro + " There is no backdrop sprite: leave the area behind the UI empty.");
}

TEST(ImageToUiAssets, WritesAFlatFolderWithUniqueNamesAndMetasForNineSlices)
{
    String dir = "./image-to-ui-assets-" + (String)(int)(Math::Random()%1000000);
    o2FileSystem.FolderCreate(dir, true);

    UiDoc doc;
    auto el = [&](const String& id, const String& kind, const String& look)
    {
        auto e = mmake<UiEl>();
        e->id = id;
        e->kind = kind;
        e->look = look;
        e->box = Box(0, 0, 20, 20);
        doc.els.Add(e);
        return e;
    };
    Vector<LookPlan> plans;
    OrderedMap<Ref<SpriteCut>> sprites;
    auto plan = [&](const String& key, const Ref<UiEl>& rep, std::optional<NineSlice> nine)
    {
        LookPlan p;
        p.key = key;
        p.rep = rep;
        p.instances = { rep };
        plans.Add(p);
        auto cut = mmake<SpriteCut>();
        cut->key = key;
        cut->img = UiImage::Blank(20, 20, 10, 20, 30, 255);
        cut->nine = nine;
        sprites.Set(key, cut);
    };
    NineSlice nine;
    nine.left = nine.right = nine.top = nine.bottom = 6;
    plan("Tile", el("a", "image", "Tile"), std::nullopt);
    plan("tile", el("b", "image", "tile"), std::nullopt);
    plan("card", el("c", "panel", "card"), nine);
    plan("flat", el("d", "panel", "flat"), NineSlice());
    auto backdrop = mmake<SpriteCut>();
    backdrop->img = UiImage::Blank(4, 4, 200, 200, 200, 255);
    sprites.Set("background", backdrop);

    Vector<AssetFile> files = WriteAssets(dir, plans, sprites, 1000*1000);
    Vector<String> names;
    for (auto& f : files)
        names.Add(f.file);
    EXPECT_EQ(names, Vector<String>({ "icon_tile.png", "icon_tile_2.png", "panel_card.png", "panel_flat.png", "bg_backdrop.png" }));
    EXPECT_TRUE(o2FileSystem.IsFileExist(dir + "/panel_card.png.meta"));
    EXPECT_FALSE(o2FileSystem.IsFileExist(dir + "/panel_flat.png.meta")) << "zero borders write no meta";
    EXPECT_FALSE(o2FileSystem.IsFileExist(dir + "/icon_tile.png.meta"));
    EXPECT_EQ(files[4].look, "background");
    o2FileSystem.FolderRemove(dir, true);
}

TEST(ImageToUiConfig, ReadsTheNodeLikeTheServer)
{
    auto node = PipelineNodeRegistry::CreateNode("imageToUi", Vec2F());
    ASSERT_TRUE(node);
    ASSERT_EQ(node->outputs.Count(), 2);
    EXPECT_EQ(node->outputs[0].name, "preview");
    EXPECT_EQ(node->outputs[1].portType, PipelinePortType::Text);
    EXPECT_TRUE(PipelineNodeRegistry::GetSchema("imageToUi")->dynamicOutputs);
    EXPECT_TRUE(PipelineGraph::IsSeededType("imageToUi"));

    auto cfg = PipelineImageToUi::ReadConfig(*node);
    EXPECT_EQ(cfg.model, "gemini-3.1-pro-preview");
    EXPECT_EQ(cfg.imageModel, "gemini-3.1-flash-image");
    EXPECT_EQ(cfg.maxTurns, 60);
    EXPECT_EQ(cfg.screenName, "Screen");
    EXPECT_EQ(cfg.assetPath, "Generated/UI");
    EXPECT_EQ(PipelineImageToUi::TurnOptions(), Vector<int>({ 20, 40, 60, 90, 120 }));

    node->SetConfigString("maxTurns", "500");
    node->SetConfigString("screenName", " Main Menu! ");
    node->SetConfigString("background", "plate");
    cfg = PipelineImageToUi::ReadConfig(*node);
    EXPECT_EQ(cfg.maxTurns, 120);
    EXPECT_EQ(cfg.screenName, "MainMenu");
    EXPECT_EQ(cfg.background, "auto");
    node->SetConfigNumber("maxTurns", 2);
    EXPECT_EQ(PipelineImageToUi::ReadConfig(*node).maxTurns, 4);
    node->SetConfigNumber("maxTurns", 90);
    EXPECT_EQ(PipelineImageToUi::ReadConfig(*node).maxTurns, 90);

    // The view state and the export folder do not change what the node makes
    String sig = PipelineGraph::ComputeNodeSignature(*node, {}, {}, 1);
    node->SetConfigBool("debugView", true);
    node->SetConfigString("debugStep", "turn-3");
    node->SetConfigString("assetPath", "Elsewhere");
    EXPECT_EQ(PipelineGraph::ComputeNodeSignature(*node, {}, {}, 1), sig);
    node->SetConfigNumber("maxTurns", 120);
    EXPECT_NE(PipelineGraph::ComputeNodeSignature(*node, {}, {}, 1), sig);
}

TEST(ImageToUiConfig, TheCardSummarySaysHowTheSkillEnded)
{
    String layout = "{\"canvas\": {\"width\": 240, \"height\": 320, \"name\": \"Test\"}, \"root\": {\"type\": \"container\", \"name\": \"root\", "
        "\"children\": [{\"type\": \"image\", \"asset\": \"a.png\"}, {\"type\": \"image\", \"asset\": \"a.png\"}, {\"type\": \"text\", \"text\": \"A\"}, "
        "{\"type\": \"container\", \"children\": [{\"type\": \"image\", \"asset\": \"b.png\"}]}]}}";
    auto summary = PipelineImageToUi::SummarizeLayout(layout);
    ASSERT_TRUE(summary.has_value());
    EXPECT_EQ(summary->elements, 5);
    EXPECT_EQ(summary->texts, 1);
    EXPECT_EQ(summary->sprites, 2) << "distinct assets";
    EXPECT_EQ(summary->name, "Test");
    EXPECT_FALSE(PipelineImageToUi::SummarizeLayout("{\"canvas\": {}}").has_value());

    auto done = PipelineImageToUi::ReadRunSummary("{\"steps\": [], \"summary\": {\"complete\": true, \"turns\": 7, \"check_count\": 3, \"blockers\": []}}");
    ASSERT_TRUE(done.has_value());
    EXPECT_EQ(PipelineImageToUi::SummaryLine(*summary, &*done, 3), "completed \xC2\xB7 3 checks \xC2\xB7 5 elements \xC2\xB7 3 sprites");
    auto blocked = PipelineImageToUi::ReadRunSummary("{\"summary\": {\"complete\": false, \"check_count\": 1, \"blockers\": [\"a\", \"b\"]}}");
    ASSERT_TRUE(blocked.has_value());
    EXPECT_EQ(PipelineImageToUi::RunLine(*blocked), "not completed \xC2\xB7 2 blockers");
    EXPECT_EQ(PipelineImageToUi::SummaryLine(*summary, nullptr), "5 elements \xC2\xB7 2 sprites");
    EXPECT_FALSE(PipelineImageToUi::ReadRunSummary("{\"steps\": []}").has_value()) << "a run still going has no summary";
}
