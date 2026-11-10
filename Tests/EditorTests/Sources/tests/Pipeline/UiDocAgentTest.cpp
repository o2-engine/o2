#include "o2Editor/stdafx.h"

#include "UiDocTestKit.h"

using namespace UiDocTest;

namespace
{
    Json OpsJson(const Vector<UiOp>& ops)
    {
        Json out = Json::Array();
        for (auto& op : ops)
            out.Add(op);
        return out;
    }

    // The kit a golden structure case names: A (the web test's two graphics), KIT (the golden kit with real pictures), or its JSON
    UiKit StructureKit(const Json& tag)
    {
        const Json& g = G("structure");
        if (tag.Str() == "A")
            return ParseKit(S(g, "kitA"));
        if (tag.Str() == "KIT")
        {
            UiKit kit = GoldenKit();
            for (auto& e : kit.elements)
                e.png = S(g, "png");
            return kit;
        }
        return ParseKit(tag.Str());
    }
}

// --- the web's tests (backend/test/uiLayout.test.ts) ------------------------------------------------------------------

TEST(UiDocAgent, TheAnswerMessagePlusEditsOrAWholeScreenFencedOrNot)
{
    UiDoc doc = Parse(Screen());
    auto a = ParseAgentAnswer("\xD0\x9F\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB8\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xBE\xD0\xB2\xD0\xB0\xD0\xBB "
                              "\xD0\xBA\xD0\xBD\xD0\xBE\xD0\xBF\xD0\xBA\xD1\x83.\n```uiml-edit\n@update play \"GO\"\n```", doc);
    EXPECT_STR_EQ(a.reply, "\xD0\x9F\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB8\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xBE\xD0\xB2\xD0\xB0\xD0\xBB "
                           "\xD0\xBA\xD0\xBD\xD0\xBE\xD0\xBF\xD0\xBA\xD1\x83.");
    EXPECT_STR_EQ(OpsJson(a.ops).Dump(), "[{\"op\":\"update\",\"id\":\"play\",\"set\":{\"text\":\"GO\"}}]");
    auto b = ParseAgentAnswer("Here it is:\n```uiml\nscreen Shop 1080x1920\n  panel p anchor=center size=800x900\n```", doc);
    EXPECT_STR_EQ(OpKind(b.ops[0]), "doc");
    auto c = ParseAgentAnswer("Done\n@remove close", doc);
    EXPECT_STR_EQ(OpsJson(c.ops).Dump(), "[{\"op\":\"remove\",\"ids\":[\"close\"]}]");
    EXPECT_STR_EQ(c.reply, "Done");
    EXPECT_TRUE(ParseAgentAnswer("It has 3 buttons.", doc).ops.IsEmpty());
}

TEST(UiDocAgent, TheLayoutWithItsKitAsUiStructure)
{
    // The export test's structure checks (the Unity / o2 exporters themselves are not part of this port)
    UiKit kit = ParseKit("{\"elements\": [{\"name\": \"btn_red\", \"type\": \"button\", \"assetType\": \"sliced\", \"width\": 60, \"height\": 40, "
                         "\"nineSlice\": {\"left\": 10, \"top\": 10, \"right\": 10, \"bottom\": 10}, \"png\": \"AAAA\"}]}");
    auto st = ToUiStructure(Parse(Screen()), &kit);
    Json structure;
    ASSERT_TRUE(Json::Parse(st.json, structure));
    const Json* menu = nullptr;
    for (auto& c : M(M(structure, "root"), "children").Items())
    {
        if (S(c, "name") == "menu")
            menu = &c;
    }
    ASSERT_TRUE(menu);
    const Json& play = *M(*menu, "children").At(0);
    EXPECT_STR_EQ(S(play, "type"), "button");
    EXPECT_STR_EQ(S(play, "asset"), "btn_red.png");
    EXPECT_TRUE(play.Has("nineSlice"));
    EXPECT_STR_EQ(S(*M(play, "children").At(0), "type"), "text");
    EXPECT_STR_EQ(S(*M(play, "children").At(0), "text"), "PLAY");
    for (auto& c : M(M(structure, "root"), "children").Items())
    {
        if (S(c, "name") == "close")
            EXPECT_EQ(M(M(M(c, "responsive"), "min"), "x").Num(), 1);
    }
    ASSERT_EQ(st.sprites.Count(), 1);
    EXPECT_STR_EQ(st.sprites[0].file, "btn_red.png");
}

// --- golden ------------------------------------------------------------------------------------------------------

TEST(UiDocAgent, GoldenAgentPromptsByteForByte)
{
    const Json& g = G("agent");
    UiKit kit = GoldenKit();
    UiKit empty;
    empty.name = "k";
    for (auto& c : M(g, "contexts").Items())
    {
        UiAgentContext ctx;
        ctx.doc = S(c, "doc") == "SCREEN" ? Parse(Screen()) : Parse(S(c, "doc"));
        ctx.kit = M(c, "kit").IsNull() ? nullptr : S(c, "kit") == "KIT" ? &kit : &empty;
        ctx.steps = StepsFrom(M(c, "steps"));
        ctx.prompt = S(c, "prompt");
        for (auto& r : M(c, "references").Items())
        {
            UiAgentReference ref;
            ref.name = S(r, "name");
            ref.kind = S(r, "kind");
            ref.text = S(r, "text");
            ctx.references.Add(ref);
        }
        ctx.selected = S(c, "selected");
        ctx.withScreenshot = M(c, "withScreenshot").Bool();
        ctx.withKitSheet = M(c, "withKitSheet").Bool();
        EXPECT_STR_EQ(BuildAgentPrompt(ctx), S(c, "prompt_text"));
    }

    UiDoc doc = Parse(Screen());
    UiKitReport report = KitReport(doc, &kit);
    report.glyphs = { "heart", "star" };
    EXPECT_STR_EQ(BuildReviewPrompt(doc, kit, " \xD0\xBC\xD0\xB5\xD0\xBD\xD1\x8E ", report), S(G("review"), "prompt"));
    EXPECT_STR_EQ(BuildReviewPrompt(Parse(S(*G("uiml").At(2), "src")), empty, "r", UiKitReport()), S(G("review"), "empty"));
}

TEST(UiDocAgent, GoldenAgentAnswers)
{
    UiDoc doc = Parse(Screen());
    for (auto& c : G("answers").Items())
    {
        SCOPED_TRACE(S(c, "a").Data());
        auto a = ParseAgentAnswer(S(c, "a"), doc);
        EXPECT_STR_EQ(a.reply, S(c, "reply"));
        EXPECT_STR_EQ(OpsJson(a.ops).Dump(), S(c, "ops"));
        EXPECT_EQ(a.errors, Strings(M(c, "errors")));
    }
}

TEST(UiDocAgent, GoldenUiStructureSpritesAndFonts)
{
    for (auto& c : M(G("structure"), "cases").Items())
    {
        SCOPED_TRACE(S(c, "name").Data());
        UiDoc doc = Parse(S(c, "src"));
        UiKit kit;
        if (!M(c, "kit").IsNull())
            kit = StructureKit(M(c, "kit"));
        auto st = ToUiStructure(doc, M(c, "kit").IsNull() ? nullptr : &kit);
        EXPECT_STR_EQ(st.structure.Dump(), S(c, "json"));
        if (!M(c, "pretty").IsNull())
            EXPECT_STR_EQ(st.json, S(c, "pretty"));

        Json sprites = Json::Array();
        for (auto& s : st.sprites)
        {
            Json row = Json::Array();
            row.Add(Json(s.file));
            row.Add(Json(s.width));
            row.Add(Json(s.height));
            row.Add(s.nineSlice ? NineToJson(*s.nineSlice) : Json(String("__undef__")));
            row.Add(s.density ? Json(*s.density) : Json(String("__undef__")));
            row.Add(s.dpi ? Json(*s.dpi) : Json(String("__undef__")));
            row.Add(Json(s.png.Length()));
            sprites.Add(row);
        }
        EXPECT_STR_EQ(sprites.Dump(), M(c, "sprites").Dump());
        EXPECT_EQ(st.fonts, Strings(M(c, "fonts")));
        EXPECT_EQ(st.warnings, Strings(M(c, "warnings")));
    }
}
