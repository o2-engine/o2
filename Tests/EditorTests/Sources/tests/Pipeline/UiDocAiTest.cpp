#include "o2Editor/stdafx.h"

#include <functional>
#include <mutex>

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAi.h"
#include "UiDocTestKit.h"

using namespace UiDocTest;
using Editor::AiBytesResult;
using Editor::AiImageOptions;
using Editor::AiImageRef;
using Editor::AiTextResult;
using Editor::PipelineExecContext;
namespace PipelineUtils = Editor::PipelineUtils;
namespace AiRouter = Editor::AiRouter;

// The UI AI with the models stubbed (backend/test/uiLayout.test.ts "the AI routes", dpi.test.ts "the agent and the
// reference"): what goes into the prompts, which pictures go along, how the answers land in the node's config
namespace
{
    // The text model as the tests see it: every prompt and its picture count, answered by a function
    struct TextModel
    {
        std::mutex                                           lock;
        Vector<String>                                       prompts;
        Vector<int>                                          imageCounts;
        std::function<String(const String&, int, int)>       answer; // prompt, pictures, call index

        TextModel()
        {
            SetUiAiTextStub([this](const String& model, const String& prompt, const Vector<AiImageRef>& images) {
                std::lock_guard<std::mutex> guard(lock);
                prompts.Add(prompt);
                imageCounts.Add(images.Count());
                AiTextResult r;
                r.ok = true;
                r.text = answer ? answer(prompt, images.Count(), prompts.Count()) : String();
                return r;
            });
        }

        ~TextModel() { SetUiAiTextStub({}); }
    };

    // Runs a coroutine to its end off the main thread
    template<typename T>
    T RunToEnd(Coroutine<T> co)
    {
        co.Start(JobThread::Any);
        co.Wait();
        return co.GetResult();
    }

    Ref<PipelineExecContext> Ctx()
    {
        auto ctx = mmake<PipelineExecContext>();
        return ctx;
    }

    String Picture(const char* name)
    {
        return PipelineUtils::Base64Decode(S(M(G("prep"), "pics"), name));
    }

    // The web test's kit: the prepared red button (primary) and the title style
    UiKit TestKit()
    {
        Json raw;
        Json::Parse("{\"version\":1,\"name\":\"k\",\"elements\":[],\"textStyles\":[{\"name\":\"title\",\"role\":\"title\","
                    "\"font\":\"Russo One\",\"size\":96,\"color\":\"#FFE066\",\"gradient\":\"#FF8C00\","
                    "\"stroke\":{\"color\":\"#5A2A00\",\"width\":6}}]}", raw);
        UiKit kit = ParseKit(raw.Dump());
        UiPrepInput in;
        in.data = Picture("green");
        in.assetType = "sliced";
        in.key = true;
        UiPrepOutput p;
        if (PrepareElement(in, p))
        {
            UiKitElement el;
            el.name = "btn_red";
            el.type = "button";
            el.assetType = "sliced";
            el.variant = "primary";
            el.width = p.width;
            el.height = p.height;
            el.nineSlice = p.nineSlice;
            el.png = PipelineUtils::Base64Encode(p.png);
            kit.elements.Add(el);
        }
        return kit;
    }

    Json StepsConfig(const Vector<UiStep>& steps)
    {
        Json config = Json::Object();
        config["steps"] = StepsToJson(steps);
        return config;
    }

    // The web tests' first step: the SCREEN as a whole document
    UiStep ScreenStep()
    {
        UiStep s;
        s.data["id"] = "s1";
        s.data["by"] = "agent";
        s.data["prompt"] = "menu";
        s.data["reply"] = "ok";
        s.data["at"] = "";
        Json ops = Json::Array();
        ops.Add(MakeDocOp(Parse(Screen())));
        s.data["ops"] = ops;
        return s;
    }

    bool Has(const String& text, const char* part) { return ((const std::string&)text).find(part) != std::string::npos; }
}

TEST(UiDocAi, AgentPromptCarriesTheFormatTheKitTheLayoutAndAPictureTheAnswerBecomesAStep)
{
    TextModel tm;
    tm.answer = [](const String&, int images, int) {
        return "\xD0\x94\xD0\xBE\xD0\xB1\xD0\xB0\xD0\xB2\xD0\xB8\xD0\xBB (" + JsNum(images) + " img).\n```uiml-edit\n@add menu after=play\n"
               "  button shop h=120 \"SHOP\"\n```";
    };
    UiAgentRequest req;
    req.config = StepsConfig({ ScreenStep() });
    req.config["agentModel"] = "gemini-3.1-pro-preview";
    req.kit = TestKit();
    req.prompt = "add a shop button";
    req.selected = "play";
    UiAgentResponse r = RunToEnd(AskUiAgent(Ctx(), req));
    ASSERT_TRUE(r.ok) << r.error.Data();
    EXPECT_TRUE(r.errors.IsEmpty());
    EXPECT_STR_EQ(r.step.By(), "agent");
    EXPECT_STR_EQ(*r.step.Prompt(), "add a shop button");
    EXPECT_STR_EQ(*r.step.Model(), "gemini-3.1-pro-preview");
    EXPECT_TRUE(StartsWith(*r.step.Reply(), "\xD0\x94\xD0\xBE\xD0\xB1\xD0\xB0\xD0\xB2\xD0\xB8\xD0\xBB (2 img)")); // the screen and the kit sheet
    EXPECT_STR_EQ(OpKind(r.step.Ops()[0]), "add");
    ASSERT_EQ(tm.prompts.Count(), 1);
    const String& prompt = tm.prompts[0];
    EXPECT_TRUE(Has(prompt, "btn_red: button/primary, sliced"));
    EXPECT_TRUE(Has(prompt, "- title: title, Russo One"));
    EXPECT_TRUE(Has(prompt, "button play h=140 kit=btn_red \"PLAY\""));
    EXPECT_TRUE(Has(prompt, "play: 220,660 640x140"));
    EXPECT_TRUE(Has(prompt, "block \"play\" selected"));
    EXPECT_TRUE(Has(prompt, "is the kit sheet"));
    EXPECT_FALSE(r.step.data.Has("review")); // nothing to check: the kit's only graphic is used
    UiDoc next = DocAt(StepsConfig({ ScreenStep(), r.step })).doc;
    const UiNode* menu = FindNode(next, "menu");
    ASSERT_TRUE(menu && menu->children.size() == 4);
    EXPECT_STR_EQ(menu->children[1].id, "shop");
    EXPECT_STR_EQ(menu->children[2].id, "settings");
}

TEST(UiDocAi, WhenKitGraphicsGoUnusedOneCheckPassFixesTheKitUsage)
{
    TextModel tm;
    tm.answer = [](const String& prompt, int, int call) -> String {
        if (call == 1)
            return "Done.\n```uiml\nscreen Menu 1080x1920\n  panel top anchor=top-stretch h=200 color=#AA5522\n"
                   "    text heart w=100 h=100 \"\xE2\x99\xA5\"\n    icon fav anchor=center size=120x120\n```";
        return Has(prompt, "Kit graphics not used: icon_star") ? "Star from the kit.\n```uiml-edit\n@update fav kit=icon_star\n```" : "";
    };
    UiKit kit;
    UiKitElement star;
    star.name = "icon_star";
    star.type = "icon";
    star.assetType = "simple";
    star.width = 40;
    star.height = 20;
    star.png = S(G("structure"), "png");
    kit.elements = { star };
    UiAgentRequest req;
    req.kit = kit;
    req.prompt = "menu";
    UiAgentResponse r = RunToEnd(AskUiAgent(Ctx(), req));
    ASSERT_TRUE(r.ok) << r.error.Data();
    ASSERT_EQ(tm.prompts.Count(), 2); // the answer and one check
    EXPECT_EQ(r.calls, 2);
    EXPECT_TRUE(Has(tm.prompts[1], "Blocks that draw an icon with characters: heart"));
    EXPECT_EQ(tm.imageCounts[1], 2); // the result rendered with the kit and the kit sheet
    String reply;
    double edits = 0;
    ASSERT_TRUE(r.step.Review(reply, edits));
    EXPECT_STR_EQ(reply, "Star from the kit.");
    EXPECT_EQ(edits, 1);
    UiDoc doc = DocAt(StepsConfig({ r.step })).doc;
    EXPECT_STR_EQ(*doc.root.children[0].children[1].kit, "icon_star");

    // Switched off: no second call
    TextModel again;
    again.answer = tm.answer;
    req.config["agentReview"] = false;
    UiAgentResponse off = RunToEnd(AskUiAgent(Ctx(), req));
    EXPECT_EQ(again.prompts.Count(), 1);
    EXPECT_FALSE(off.step.data.Has("review"));
}

TEST(UiDocAi, AnEmptyLayoutStartsAtTheReferencePicturesSizeAndDpi)
{
    TextModel tm;
    tm.answer = [](const String&, int, int) { return String("Done.\n```uiml-edit\n@add root\n  button play anchor=center size=300x100 \"Play\"\n```"); };
    UiAgentRequest req;
    UiAgentInput shot;
    shot.name = "screen";
    shot.png = UiImage::Blank(750, 1334, 17, 34, 51, 255).Png();
    shot.dpi = 326;
    req.references = { shot };
    req.prompt = "rebuild this screen";
    UiAgentResponse r = RunToEnd(AskUiAgent(Ctx(), req));
    ASSERT_TRUE(r.ok);
    EXPECT_STR_EQ(r.step.Ops()[0].Dump(), "{\"op\":\"screen\",\"width\":750,\"height\":1334,\"dpi\":326}");
    EXPECT_TRUE(Has(tm.prompts[0], "(empty) screen Screen 750x1334 dpi=326"));
    EXPECT_EQ(tm.imageCounts[0], 1); // no screenshot of an empty screen, no kit: the reference only
    UiDoc doc = ApplyOps(ParseUiml("").doc, r.step.Ops()).doc;
    EXPECT_EQ(doc.width, 750);
    EXPECT_EQ(doc.height, 1334);
    EXPECT_EQ(*doc.dpi, 326);

    // The DPI tag of the PNG itself when the caller gives none
    String png = UiImage::Blank(10, 10).Png();
    std::string chunk = std::string("\0\0\0\x12tEXtAssetsLine:dpi\0" "326", 8 + 18) + std::string(4, '\0');
    std::string tagged = ((std::string)png).substr(0, 33) + chunk + ((std::string)png).substr(33);
    ASSERT_TRUE(PngDpiTag(String(tagged)).has_value());
    EXPECT_EQ(*PngDpiTag(String(tagged)), 326);
    EXPECT_FALSE(PngDpiTag(png).has_value());
}

TEST(UiDocAi, AStyleFromTheModelKeepsItsNameAndTheReferencesGoAlong)
{
    TextModel tm;
    tm.answer = [](const String&, int images, int) {
        return "Here:\n```json\n{\"role\":\"title\",\"font\":\"Russo One\",\"weight\":400,\"size\":110,\"color\":\"#FFE45C\",\"gradient\":\"#FF8A00\","
               "\"stroke\":{\"color\":\"#3A1800\",\"width\":8},\"uppercase\":true,\"description\":\"main title (" + JsNum(images) + " ref)\"}\n```";
    };
    UiStyleRequest req;
    Json::Parse("{\"name\":\"title\",\"role\":\"title\",\"font\":\"Russo One\",\"size\":96,\"color\":\"#FFE066\",\"gradient\":\"#FF8C00\","
                "\"stroke\":{\"color\":\"#5A2A00\",\"width\":6}}", req.config);
    req.references = { Picture("green") };
    req.prompt = "bold game title";
    UiStyleResponse r = RunToEnd(GenerateUiTextStyle(Ctx(), req));
    ASSERT_TRUE(r.ok) << r.error.Data();
    EXPECT_STR_EQ(r.style.name, "title");
    EXPECT_STR_EQ(r.style.font, "Russo One");
    EXPECT_EQ(r.style.size, 110);
    ASSERT_TRUE(r.style.stroke.has_value());
    EXPECT_STR_EQ(r.style.stroke->color, "#3A1800");
    EXPECT_EQ(r.style.stroke->width, 8);
    EXPECT_STR_EQ(*r.style.description, "main title (1 ref)");
    EXPECT_TRUE(Has(tm.prompts[0], "Russo One \xE2\x80\x94 weights 400"));
    EXPECT_TRUE(Has(tm.prompts[0], "bold game title"));

    DataDocument config;
    ToDataValue(req.config, config);
    StoreTextStyle(config, r.style);
    TextStyle stored = TextStyleFromConfig(config);
    EXPECT_STR_EQ(stored.name, "title");
    EXPECT_EQ(stored.size, 110);

    tm.answer = [](const String&, int, int) { return String("no style here"); };
    EXPECT_FALSE(RunToEnd(GenerateUiTextStyle(Ctx(), req)).ok);
}

TEST(UiDocAi, KitAnalysisNamesTypesAndKeysFlatBackdrops)
{
    TextModel tm;
    tm.answer = [](const String&, int images, int) {
        return "[{\"index\":1,\"name\":\"Red Button!\",\"type\":\"button\",\"assetType\":\"sliced\",\"variant\":\"primary\","
               "\"description\":\"main action (" + JsNum(images) + ")\",\"background\":\"flat\"}]";
    };
    UiKitAnalyzeRequest req;
    req.pictures = { UiKitPictureIn{ "kit-btn", "btn", Picture("green") } };
    UiKitAnalyzeResponse r = RunToEnd(AnalyzeUiKit(Ctx(), req));
    ASSERT_TRUE(r.ok) << r.error.Data();
    ASSERT_EQ(r.items.Count(), 1);
    const Json& it = *r.items.Get("kit-btn");
    EXPECT_STR_EQ(S(it, "name"), "Red_Button");
    EXPECT_STR_EQ(S(it, "type"), "button");
    EXPECT_STR_EQ(S(it, "bg"), "key");
    EXPECT_STR_EQ(S(it, "description"), "main action (1)");
    EXPECT_TRUE(M(it, "ai").Bool());
    EXPECT_FALSE(it.Has("nineSlice")); // the pixels find its stretch
    EXPECT_TRUE(Has(tm.prompts[0], "1. input named \"btn\", 360x200 px"));

    // Nothing to analyse
    UiKitAnalyzeRequest none;
    EXPECT_FALSE(RunToEnd(AnalyzeUiKit(Ctx(), none)).ok);
}

TEST(UiDocAi, TheAisNineSliceIsTakenWhereThePixelsShowNoStretch)
{
    TextModel tm;
    tm.answer = [](const String&, int, int) {
        return String("[{\"index\":1,\"name\":\"btn_grad\",\"type\":\"button\",\"assetType\":\"sliced\",\"description\":\"x\","
                      "\"background\":\"transparent\",\"nineSlice\":{\"left\":31,\"top\":30,\"right\":32,\"bottom\":30}}]");
    };
    UiKitAnalyzeRequest req;
    req.pictures = { UiKitPictureIn{ "kit-btn", "btn", Picture("striped") } };
    UiKitAnalyzeResponse r = RunToEnd(AnalyzeUiKit(Ctx(), req));
    ASSERT_TRUE(r.ok);
    const Json& it = *r.items.Get("kit-btn");
    EXPECT_STR_EQ(S(it, "nineFrom"), "ai");
    EXPECT_EQ(M(M(it, "nineSlice"), "left").Num(), 31); // x from the AI (the picture went unscaled)
    EXPECT_EQ(M(M(it, "nineSlice"), "right").Num(), 32);
    double top = M(M(it, "nineSlice"), "top").Num();
    EXPECT_TRUE(top >= 20 && top <= 40) << top; // y from the pixels

    // A picture whose stretch the pixels find keeps automatic borders
    tm.answer = [](const String&, int, int) {
        return String("[{\"index\":1,\"name\":\"btn\",\"type\":\"button\",\"assetType\":\"sliced\",\"background\":\"flat\","
                      "\"nineSlice\":{\"left\":5,\"top\":5,\"right\":5,\"bottom\":5}}]");
    };
    req.pictures = { UiKitPictureIn{ "kit-btn", "btn", Picture("green") } };
    UiKitAnalyzeResponse auto_ = RunToEnd(AnalyzeUiKit(Ctx(), req));
    const Json& kept = *auto_.items.Get("kit-btn");
    EXPECT_FALSE(kept.Has("nineSlice"));
    EXPECT_FALSE(kept.Has("nineFrom"));
}

TEST(UiDocAi, AComplexBackgroundIsRemovedByTheImageModelAndStoredAsAnUpload)
{
    String work = PipelineUtils::GetWorkPath();
    String temp = o2FileSystem.GetParentPath(work) + "/uidoc-ai-test-work/";
    PipelineUtils::SetWorkPathOverride(temp);
    struct Restore { ~Restore() { PipelineUtils::SetWorkPathOverride(""); AiRouter::SetImageStub({}); } } restore;

    // The image model paints the backdrop over with the key colour around a red square (a model that draws alpha itself
    // is asked for it and leaves it clear)
    int imageCalls = 0;
    bool keyed = false;
    AiRouter::SetImageStub([&](const String&, const String&, const Vector<AiImageRef>&, int, bool transparent, const AiImageOptions&) {
        imageCalls++;
        keyed = !transparent;
        UiImage img = UiImage::Blank(200, 120, 0x00, 0xB1, 0x40, transparent ? 0 : 255);
        for (int y = 30; y < 90; y++)
            for (int x = 50; x < 150; x++)
            {
                UInt8* p = img.At(x, y);
                p[0] = 220; p[1] = 40; p[2] = 40;
            }
        AiBytesResult r;
        r.ok = true;
        r.data = img.Png();
        r.mimeType = "image/png";
        return r;
    });
    TextModel tm;
    tm.answer = [](const String&, int, int) {
        return String("[{\"index\":1,\"name\":\"hero\",\"type\":\"image\",\"assetType\":\"simple\",\"description\":\"a hero\",\"background\":\"complex\"}]");
    };
    UiKitAnalyzeRequest req;
    String source = Picture("noisy");
    req.pictures = { UiKitPictureIn{ "p1", "hero", source } };
    req.removeBg = true;
    req.imageModel = "gemini-2.5-flash-image"; // draws no alpha: the key colour path
    UiKitAnalyzeResponse r = RunToEnd(AnalyzeUiKit(Ctx(), req));
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(imageCalls, 1);
    EXPECT_TRUE(keyed);
    const Json& it = *r.items.Get("p1");
    EXPECT_STR_EQ(S(it, "bg"), "ai");
    String uploadId = S(M(it, "prepared"), "uploadId");
    EXPECT_STR_EQ(S(M(it, "prepared"), "from"), InputHash(source));
    UiImage cut;
    ASSERT_TRUE(UiImage::Decode(PipelineUtils::ReadFileBytes(PipelineUtils::GetUploadsPath() + uploadId), cut));
    EXPECT_EQ(cut.At(5, 5)[3], 0);       // the key colour went
    EXPECT_GT(cut.At(100, 60)[3], 200);  // the subject stays

    // Without "remove backgrounds": a note, the input as it is
    req.removeBg = false;
    UiKitAnalyzeResponse kept = RunToEnd(AnalyzeUiKit(Ctx(), req));
    EXPECT_STR_EQ(S(*kept.items.Get("p1"), "bg"), "keep");
    EXPECT_EQ(kept.notes.Count(), 1);
}

TEST(UiDocAi, TheAnalysisMergesIntoTheKitsItems)
{
    Json config;
    ASSERT_TRUE(Json::Parse("{\"items\":{\"a\":{\"name\":\"old\",\"nineSlice\":{\"left\":7,\"top\":7,\"right\":7,\"bottom\":7}},"
                            "\"b\":{\"bg\":\"ai\",\"prepared\":{\"uploadId\":\"u.png\",\"from\":\"h\"}},"
                            "\"c\":{\"nineSlice\":{\"left\":1,\"top\":1,\"right\":1,\"bottom\":1},\"nineFrom\":\"ai\"}},\"kitModel\":\"m\"}", config));
    Json items;
    ASSERT_TRUE(Json::Parse("{\"a\":{\"name\":\"new\",\"bg\":\"keep\",\"ai\":true,\"background\":\"transparent\","
                            "\"nineSlice\":{\"left\":2,\"top\":2,\"right\":2,\"bottom\":2},\"nineFrom\":\"ai\"},"
                            "\"b\":{\"name\":\"b\",\"bg\":\"keep\",\"ai\":true,\"background\":\"complex\"},"
                            "\"c\":{\"name\":\"c\",\"bg\":\"key\",\"ai\":true,\"background\":\"flat\"},"
                            "\"d\":{\"name\":\"d\",\"bg\":\"keep\",\"ai\":true,\"background\":\"transparent\","
                            "\"nineSlice\":{\"left\":3,\"top\":3,\"right\":3,\"bottom\":3},\"nineFrom\":\"ai\"}}", items));
    Json merged = MergeKitAnalysis(config, items);
    const Json& m = M(merged, "items");
    // Borders set by hand stay
    EXPECT_STR_EQ(M(m, "a").Dump(), "{\"name\":\"new\",\"nineSlice\":{\"left\":7,\"top\":7,\"right\":7,\"bottom\":7},\"bg\":\"keep\",\"ai\":true}");
    // A background removed earlier stays removed
    EXPECT_STR_EQ(M(m, "b").Dump(), "{\"bg\":\"ai\",\"prepared\":{\"uploadId\":\"u.png\",\"from\":\"h\"},\"name\":\"b\",\"ai\":true,\"nineSlice\":null}");
    // The AI's earlier borders go when this answer has none; new AI borders are taken
    EXPECT_STR_EQ(M(m, "c").Dump(), "{\"nineSlice\":null,\"name\":\"c\",\"bg\":\"key\",\"ai\":true}");
    EXPECT_STR_EQ(M(m, "d").Dump(), "{\"name\":\"d\",\"bg\":\"keep\",\"ai\":true,\"nineSlice\":{\"left\":3,\"top\":3,\"right\":3,\"bottom\":3},\"nineFrom\":\"ai\"}");
    EXPECT_STR_EQ(S(merged, "kitModel"), "m");

    DataDocument doc;
    ToDataValue(config, doc);
    MergeKitAnalysis(doc, items);
    EXPECT_STR_EQ(FromDataValue(doc).Dump(), merged.Dump());
}

TEST(UiDocAi, TheKitAnalysisPromptIsTheWebs)
{
    EXPECT_STR_EQ(KitAnalyzePrompt(2, { "btn", "Panel One" }, { "768x427", "300x120" }),
    "You are preparing graphics for the UI kit of a game or app: the pictures will be used as UI elements (buttons, panels, frames, icons, bars\xE2\x80\xA6).\n"
    "2 picture(s) follow, in this order:\n"
    "1. input named \"btn\", 768x427 px\n"
    "2. input named \"Panel One\", 300x120 px\n"
    "\n"
    "For each picture decide:\n"
    "- name: short snake_case name for the kit (btn_green, panel_dark, frame_gold, icon_coin, bar_hp_fill); keep the input name when it is already a good one\n"
    "- type: one of background (full-screen backdrop behind everything); panel (stretchable surface that holds content (window, card, popup body)); frame (border / outline drawn around content); button (pressable button surface (the label is drawn on top)); icon (small symbol kept at its aspect (coin, gear, heart, close X)); image (illustration or picture kept at its aspect); progressBar (track (background) of a progress / health / xp bar); progressFill (fill of a progress bar, cut to the value); toggle (on/off switch or checkbox); slider (slider track); sliderHandle (slider knob); input (text field box); tab (tab header); badge (small counter / notification bubble); divider (separator line); header (title ribbon or banner); slot (item cell of an inventory or a grid); other (anything else)\n"
    "- assetType: one of simple, sliced, tiled \xE2\x80\x94 sliced (nine-slice: corners keep their size, the middle stretches) for surfaces that change size: buttons, panels, frames, bars, fields, tabs; simple for icons, pictures, characters and full backgrounds; tiled for repeating patterns\n"
    "- variant: optional, e.g. primary, secondary, disabled, selected, small (empty when not obvious)\n"
    "- description: one short sentence \xE2\x80\x94 what it is and where in an interface it is used\n"
    "- background: transparent (already cut out: transparent or checker-free around the element) | flat (one plain colour fills everything around the element) | complex (scenery, gradient or texture behind the element)\n"
    "- nineSlice (sliced only): how many pixels from each edge of the picture (at the size given above) belong to the corners and borders that must keep their size \xE2\x80\x94 everything between them is stretched. Put each line just inside the flat middle, past the rounding, bevel, rim or ornament; null for simple / tiled\n"
    "\n"
    "Answer with JSON only, an array in the same order:\n"
    "[{\"index\":1,\"name\":\"...\",\"type\":\"...\",\"assetType\":\"...\",\"variant\":\"\",\"description\":\"...\",\"background\":\"...\",\"nineSlice\":{\"left\":0,\"top\":0,\"right\":0,\"bottom\":0}}]");
    EXPECT_EQ(ParseJsonArrayAnswer("x ```json\n[1, {\"a\": 2}]\n``` y").Count(), 2);
    EXPECT_EQ(ParseJsonArrayAnswer("nothing").Count(), 0);
}

TEST(UiDocAi, StepsGoIntoTheConfigAsTheCardsWriteThem)
{
    DataDocument config;
    config.SetObject();
    UiStep step = MakeAgentStep("p", "r", { MakeDocOp(Parse(Screen())) }, 1762768800000, "m");
    StoreAgentStep(config, step);
    Json c = FromDataValue(config);
    EXPECT_EQ(M(c, "steps").Count(), 1);
    EXPECT_STR_EQ(M(c, "uiRedo").Dump(), "[]");
    EXPECT_STR_EQ(S(c, "uiDraft"), "");

    Json set = Json::Object();
    set["text"] = "GO";
    CommitUserAction(config, { MakeUpdateOp("play", set) }, "play: text", 1762768801000);
    EXPECT_STR_EQ(*FindNode(DocAt(config).doc, "play")->text, "GO");
    ASSERT_TRUE(UndoInConfig(config));
    EXPECT_STR_EQ(*FindNode(DocAt(config).doc, "play")->text, "PLAY");
    EXPECT_EQ(RedoOf(config).Count(), 1);
    ASSERT_TRUE(RedoInConfig(config));
    EXPECT_STR_EQ(*FindNode(DocAt(config).doc, "play")->text, "GO");
    EXPECT_FALSE(RedoInConfig(config));
}
