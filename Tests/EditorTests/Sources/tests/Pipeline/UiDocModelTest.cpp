#include "o2Editor/stdafx.h"

#include "UiDocTestKit.h"

using namespace UiDocTest;

// --- the web's tests (backend/test/uiLayout.test.ts: layout text and edits) -------------------------------

TEST(UiDocModel, UimlReadsAndWritesTheSameTree)
{
    auto parsed = ParseUiml(Screen());
    EXPECT_TRUE(parsed.errors.IsEmpty());
    const UiDoc& doc = parsed.doc;
    EXPECT_EQ(doc.name, "Menu");
    EXPECT_EQ(doc.width, 1080);
    auto again = ParseUiml(DocToUiml(doc));
    EXPECT_STR_EQ(DocToJson(again.doc).Dump(), DocToJson(doc).Dump());

    const UiNode* menu = FindNode(doc, "menu");
    ASSERT_TRUE(menu);
    EXPECT_EQ(menu->type, "group");
    EXPECT_EQ(*menu->layout, "column");
    EXPECT_EQ(*menu->children[0].kit, "btn_red");
    EXPECT_EQ(*menu->children[0].text, "PLAY");
}

TEST(UiDocModel, AnchorSpellingsResolveToTheSamePresets)
{
    EXPECT_EQ(AnchorName(ParseAnchor(String("top-stretch"))), "top-stretch");
    EXPECT_EQ(AnchorName(ParseAnchor(String("stretch top"))), "top-stretch");
    EXPECT_EQ(AnchorName(ParseAnchor(String("left-stretch"))), "stretch-left");
    EXPECT_EQ(AnchorName(ParseAnchor(String("bottom"))), "bottom-center");
    EXPECT_EQ(AnchorName(ParseAnchor(String("fill"))), "stretch");
    EXPECT_EQ(AnchorName(ParseAnchor(String("centre"))), "center");
}

// --- golden: the web's outputs for the same inputs ------------------------------------------------------------

TEST(UiDocModel, GoldenUimlParseWriteAndRoundTrip)
{
    for (auto& c : G("uiml").Items())
    {
        SCOPED_TRACE(S(c, "name").Data());
        const Json& b = M(c, "base");
        UiDoc base;
        if (b.IsObject())
        {
            base = EmptyDoc(S(b, "name"), M(b, "width").Num(), M(b, "height").Num(), M(b, "dpi").Num());
        }
        auto parsed = ParseUiml(S(c, "src"), b.IsObject() ? &base : nullptr);
        EXPECT_STR_EQ(DocToJson(parsed.doc).Dump(), S(c, "doc"));
        String text = DocToUiml(parsed.doc);
        EXPECT_STR_EQ(text, S(c, "uiml"));
        EXPECT_EQ(parsed.errors, Strings(M(c, "errors")));
        EXPECT_EQ(DocToUiml(ParseUiml(text).doc) == text, M(c, "again").Bool());
    }
}

TEST(UiDocModel, GoldenTokens)
{
    for (auto& c : G("tokens").Items())
    {
        auto toks = TokenizeUiml(S(c, "line"));
        const Json& expected = M(c, "toks");
        ASSERT_EQ(toks.Count(), expected.Count()) << S(c, "line").Data();
        for (int i = 0; i < toks.Count(); i++)
        {
            const Json& e = *expected.At(i);
            EXPECT_EQ(toks[i].key, e.At(0)->IsNull() ? String() : e.At(0)->Str());
            EXPECT_EQ(toks[i].value, e.At(1)->Str());
            EXPECT_EQ(toks[i].quoted, e.At(2)->Bool());
        }
    }
}

TEST(UiDocModel, GoldenAnchors)
{
    for (auto& c : G("anchors").Items())
        EXPECT_STR_EQ(AnchorName(ParseAnchor(c.At(0)->Str())), c.At(1)->Str()) << c.At(0)->Str().Data();
}

TEST(UiDocModel, GoldenCleanProp)
{
    for (auto& c : G("cleanProp").Items())
    {
        Json cleaned;
        bool kept = CleanProp(S(c, "k"), &M(c, "v"), cleaned);
        const Json& expected = M(c, "c");
        SCOPED_TRACE((S(c, "k") + " = " + M(c, "v").Dump()).Data());
        EXPECT_EQ(kept, !IsUndef(expected));
        if (kept)
            EXPECT_STR_EQ(cleaned.Dump(), expected.Dump());
    }
}

TEST(UiDocModel, GoldenNormalizeNodeAndDoc)
{
    // The web normalises the raw nodes one after another against one id book
    UiIdBook book;
    const Json& raws = M(G("nodes"), "raws");
    const Json& outs = M(G("nodes"), "outs");
    ASSERT_EQ(raws.Count(), outs.Count());
    for (int i = 0; i < outs.Count(); i++)
        EXPECT_STR_EQ(NodeToJson(NormalizeNode(*raws.At(i), book)).Dump(), outs.At(i)->Str()) << i;

    for (auto& c : G("docs").Items())
        EXPECT_STR_EQ(DocToJson(NormalizeDoc(M(c, "raw"))).Dump(), S(c, "doc")) << M(c, "raw").Dump().Data();
}

TEST(UiDocModel, GoldenConstantsAndJavaScriptHelpers)
{
    const Json& misc = G("misc");
    EXPECT_EQ(UiLayoutUiOnly(), Strings(M(misc, "uiOnly")));
    EXPECT_EQ(AnchorPresets(), Strings(M(misc, "presets")));
    EXPECT_EQ(UiNodeTypes(), Strings(M(misc, "types")));
    EXPECT_EQ(NodeKeys(), Strings(M(misc, "keys")));
    for (auto& c : M(misc, "base36").Items())
        EXPECT_EQ(ToBase36((Int64)c.At(0)->Num()), c.At(1)->Str());
    for (auto& c : M(misc, "iso").Items())
        EXPECT_EQ(IsoTime(c.At(0)->Num()), c.At(1)->Str());
    for (auto& c : M(misc, "parse").Items())
    {
        double t = ParseIsoTime(c.At(0)->Str());
        if (c.At(1)->IsNull())
            EXPECT_TRUE(std::isnan(t)) << c.At(0)->Str().Data();
        else
            EXPECT_EQ(t, c.At(1)->Num()) << c.At(0)->Str().Data();
    }

    String id = NewStepId();
    EXPECT_EQ(id[0], 's');
    EXPECT_GE(id.Length(), 7);

    EXPECT_EQ(JsRound(0.49999999999999994), 0);
    EXPECT_EQ(JsRound(2.5), 3);
    EXPECT_EQ(JsRound(-2.5), -2);
    EXPECT_TRUE(std::signbit(JsRound(-0.2)));
    EXPECT_EQ(JsNum(0.1 + 0.2), "0.30000000000000004");
    EXPECT_EQ(JsNum(1e21), "1e+21");
    EXPECT_EQ(JsNum(120), "120");
    EXPECT_EQ(JsSlice("a\xF0\x9F\x98\x80" "b", 0, 3), "a\xF0\x9F\x98\x80");
    EXPECT_EQ(JsSlice("a\xF0\x9F\x98\x80" "b", 0, 2), "a");
    EXPECT_EQ(Utf16Length("\xD0\x9F\xF0\x9F\x98\x80"), 3);
    EXPECT_EQ(JsToUpper("stra\xC3\x9F" "e \xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82 \xD1\x91"), "STRASSE \xD0\x9F\xD0\xA0\xD0\x98\xD0\x92\xD0\x95\xD0\xA2 \xD0\x81");
    EXPECT_EQ(JsTrim("\xC2\xA0 x\t\xE3\x80\x80"), "x");
}

TEST(UiDocModel, JsonOfANodeKeepsTheOrderItsKeysWereSetIn)
{
    // As on a JavaScript object: a key an edit adds goes after the others, children included
    UiDoc doc = Parse("screen S 100x100\n  row r w=10\n    button b");
    UiOp op = MakeUpdateOp("r", [] { Json set = Json::Object(); set["x"] = 5; set["w"] = 20; return set; }());
    auto res = ApplyOps(doc, { op });
    EXPECT_STR_EQ(NodeToJson(*FindNode(res.doc, "r"), false).Dump(), "{\"id\":\"r\",\"type\":\"group\",\"w\":20,\"layout\":\"row\",\"x\":5}");
    EXPECT_EQ(FindNode(res.doc, "r")->keys.Last(), "x");

    DataDocument data;
    ToDataValue(DocToJson(res.doc), data);
    EXPECT_STR_EQ(FromDataValue(data).Dump(), DocToJson(res.doc).Dump());
}

TEST(UiDocModel, StepsKeepTheirValuesThroughANodesConfig)
{
    // A node's config is an o2 DataDocument written by rapidjson: the steps come back with the same members, order and numbers
    UiDoc doc = Parse(Screen());
    Json set = Json::Object();
    set["x"] = 0.25;
    set["y"] = -12.34;
    set["w"] = 510;
    set["opacity"] = 1e-7;
    set["text"] = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82 \"q\"\n";
    UiStep step = MakeAgentStep("p", "r", { MakeDocOp(doc), MakeUpdateOp("play", set, { "kit" }) }, 1762768800000, "m");
    Json config = Json::Object();
    config["steps"] = StepsToJson({ step });

    DataDocument data;
    ToDataValue(config, data);
    String text = data.SaveAsString();
    DataDocument loaded;
    ASSERT_TRUE(loaded.LoadFromData(text));
    Json back = FromDataValue(loaded);
    EXPECT_STR_EQ(back.Dump(), config.Dump());
    EXPECT_STR_EQ(DocToJson(DocAt(back).doc).Dump(), DocToJson(DocAt(config).doc).Dump());
    // Whole numbers stay integers in o2's text ("width": 1080, not 1080.0)
    EXPECT_EQ(((std::string)text).find("1080.0"), std::string::npos);

    // o2's writer (rapidjson's Grisu2) does not always print the shortest exact form of a 17-digit double: 0.1 + 0.2 comes
    // back an ulp off. Numbers of up to 15 significant digits (everything the layout rounds) are exact
    DataDocument odd;
    ToDataValue(Json(0.1 + 0.2), odd);
    DataDocument oddBack;
    ASSERT_TRUE(oddBack.LoadFromData(odd.SaveAsString()));
    EXPECT_NEAR((double)oddBack, 0.1 + 0.2, 1e-15);
}
