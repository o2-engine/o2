#include "o2Editor/stdafx.h"

#include <cmath>

#include "UiDocTestKit.h"

#include "o2Editor/Pipeline/UiDoc/UiDocFromStructure.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRotate.h"
#include "o2Editor/Pipeline/UiDoc/UiDocSolve.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgentRun.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutMarkup.h"

#include "UiDocGolden5.inc"

// Round 5 of the golden checks: turned layout blocks (AssetsLine shared/uiDoc/rotate.ts) - the turn as a property, the
// maths, the hit test through turns, ui_structure.json's `rotation` and back, keepTurns after the skill
// pass and the markup's rule for tilted elements; the web's outputs for the same inputs, made by
// UiDocGolden/cases5.mts. Numbers that go through sin / cos are compared to 1e-9 (V8's libm and this one may differ in the
// last bit); everything written out is compared exactly
namespace
{
    using namespace UiDocTest;
    namespace L = Editor::UiLayout;

    const Json& G5(const char* key)
    {
        static const Json golden = []() {
            std::string text;
            for (const char* part : kUiDocGolden5Parts)
                text += part;
            Json out;
            String error;
            if (!Json::Parse(String(text), out, &error))
                ADD_FAILURE() << "the golden data does not parse: " << error.Data();
            return out;
        }();
        static const Json none;
        const Json* v = golden.Get(key);
        return v ? *v : none;
    }

    Json ParseText(const String& text)
    {
        Json out;
        Json::Parse(text, out);
        return out;
    }

    bool Close(double a, double b)
    {
        return std::abs(a - b) <= 1e-9*std::max(1.0, std::abs(b));
    }

    std::optional<double> OptOf(const Json& v)
    {
        return v.IsNull() ? std::nullopt : std::optional<double>(v.Num());
    }

    String OpsJson(const Vector<UiOp>& ops)
    {
        Json arr = Json::Array();
        for (auto& op : ops)
            arr.Add(op);
        return arr.Dump();
    }
}

TEST(UiDocGolden5, CleanTurnIsTheWebs)
{
    ASSERT_GT(G5("cleanTurn").Count(), 20);
    for (auto& c : G5("cleanTurn").Items())
    {
        auto turn = CleanTurn(c.At(0));
        auto want = OptOf(*c.At(1));
        EXPECT_EQ(turn.has_value(), want.has_value()) << c.Dump().Data();
        if (turn && want)
            EXPECT_EQ(*turn, *want) << c.Dump().Data();
    }
}

TEST(UiDocGolden5, TurnedBoundsAndUnturnedSizesAreTheWebs)
{
    for (auto& c : G5("bounds").Items())
    {
        UiSize b = TurnedBounds(c.At(0)->Num(), c.At(1)->Num(), c.At(2)->Num());
        EXPECT_TRUE(Close(b.w, M(*c.At(3), "w").Num()) && Close(b.h, M(*c.At(3), "h").Num())) << c.Dump().Data();
    }
    int n = 0;
    for (auto& c : G5("unturned").Items())
    {
        UiSize s = UnturnedSize(c.At(0)->Num(), c.At(1)->Num(), c.At(2)->Num(), OptOf(*c.At(3)));
        EXPECT_TRUE(Close(s.w, M(*c.At(4), "w").Num()) && Close(s.h, M(*c.At(4), "h").Num()))
            << c.Dump().Data() << " -> " << s.w << " x " << s.h;
        n++;
    }
    EXPECT_EQ(n, 600);
}

TEST(UiDocGolden5, TurnedDocumentsAreTheWebs)
{
    ASSERT_EQ(G5("docs").Count(), 2);
    for (auto& c : G5("docs").Items())
    {
        UiDoc doc = ParseUiml(S(c, "src")).doc;
        EXPECT_STR_EQ(DocToJson(doc).Dump(), S(c, "doc"));
        EXPECT_STR_EQ(DocToUiml(doc), S(c, "uiml"));
        UiRects rects = SolveLayout(doc);
        for (auto& b : M(c, "blocks").Items())
        {
            String id = S(b, "id");
            EXPECT_EQ(rects.Get(id), RectOf(M(b, "rect"))) << id.Data();
            UiMat m = WorldMatrix(doc, rects, id), mp = WorldMatrix(doc, rects, id, false);
            for (int i = 0; i < 6; i++)
            {
                EXPECT_TRUE(Close(m[i], M(b, "m").At(i)->Num())) << id.Data() << " m[" << i << "] " << m[i];
                EXPECT_TRUE(Close(mp[i], M(b, "mParent").At(i)->Num())) << id.Data() << " mParent[" << i << "]";
            }
            EXPECT_EQ(WorldTurn(doc, id), M(b, "turn").Num()) << id.Data();
            EXPECT_EQ(WorldTurn(doc, id, false), M(b, "turnParent").Num()) << id.Data();
            auto cs = Corners(rects.Get(id), m);
            for (int i = 0; i < 4; i++)
            {
                const Json& w = *M(b, "corners").At(i);
                EXPECT_TRUE(Close(cs[i].x, M(w, "x").Num()) && Close(cs[i].y, M(w, "y").Num())) << id.Data() << " corner " << i;
            }
        }

        int hits = 0;
        for (auto& h : M(c, "hits").Items())
        {
            String got = HitTest(doc, rects, h.At(0)->Num(), h.At(1)->Num());
            EXPECT_STR_EQ(got, h.At(2)->Str()) << h.Dump().Data();
            hits += got.IsEmpty() ? 0 : 1;
        }
        EXPECT_GT(hits, 100);

        // The same grid of points as the web's, each block's points on it
        Json inside = Json::Array();
        for (auto& id : AllIds(doc))
        {
            if (id == "root")
                continue;
            for (double y = 1.13; y < doc.height; y += 37.7)
            {
                for (double x = 1.91; x < doc.width; x += 41.3)
                {
                    if (InBlock(doc, rects, id, { x, y }))
                    {
                        Json p = Json::Array();
                        p.Add(Json(id));
                        p.Add(Json(x));
                        p.Add(Json(y));
                        inside.Add(p);
                    }
                }
            }
        }
        EXPECT_STR_EQ(inside.Dump(), M(c, "inside").Dump());
    }
}

TEST(UiDocGolden5, NormalizingAndEditsKeepTheTurnLikeTheWeb)
{
    for (auto& c : G5("normalize").Items())
        EXPECT_STR_EQ(DocToJson(NormalizeDoc(ParseText(c.At(0)->Str()))).Dump(), c.At(1)->Str()) << c.At(0)->Str().Data();

    UiDoc doc = ParseUiml(S(*G5("docs").At(0), "src")).doc;
    ASSERT_EQ(G5("edits").Count(), 6);
    for (auto& c : G5("edits").Items())
    {
        auto e = ParseEdits(S(c, "text"), doc);
        EXPECT_STR_EQ(OpsJson(e.ops), S(c, "ops")) << S(c, "text").Data();
        EXPECT_EQ(e.errors, Strings(M(c, "errors")));
        UiDoc after = ApplyOps(doc, e.ops).doc;
        EXPECT_STR_EQ(DocToJson(after).Dump(), S(c, "doc")) << S(c, "text").Data();
        EXPECT_STR_EQ(DocToUiml(after), S(c, "uiml"));
    }
}

TEST(UiDocGolden5, TheStructureCarriesTheTurnAndReadsItBack)
{
    for (auto& c : G5("structure").Items())
    {
        UiDoc doc = ParseUiml(S(c, "src")).doc;
        auto st = ToUiStructure(doc, nullptr);
        EXPECT_STR_EQ(st.json, S(c, "json"));
        FromStructureInput in;
        in.structure = ParseText(st.json);
        in.dpi = 480;
        EXPECT_STR_EQ(DocToJson(FromUiStructure(in).doc).Dump(), S(c, "back"));
    }
}

TEST(UiDocGolden5, KeepTurnsAndTheMarkupRuleAreTheWebs)
{
    for (auto& c : G5("keepTurns").Items())
    {
        UiDoc doc = ParseUiml(S(c, "src")).doc;
        UiDoc draft = NormalizeDoc(ParseText(S(c, "draft")));
        String before = DocToJson(doc).Dump();
        EXPECT_STR_EQ(DocToJson(L::KeepTurns(doc, draft)).Dump(), S(c, "doc")) << S(c, "src").Data();
        EXPECT_STR_EQ(DocToJson(doc).Dump(), before) << "the input is not changed";
    }
    EXPECT_STR_EQ(L::MarkupFlatRule(), G5("flatRule").Str());
}
