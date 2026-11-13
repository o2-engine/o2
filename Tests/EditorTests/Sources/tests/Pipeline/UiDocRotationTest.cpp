#include "o2Editor/stdafx.h"

#include <cmath>
#include <functional>

#include "UiDocTestKit.h"

#include "o2/Render/Sprite.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFromStructure.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRotate.h"
#include "o2Editor/Pipeline/UiDoc/UiDocStructure.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgentRun.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutDraft.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutParse.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutWidgets.h"

// Turned blocks (port of AssetsLine backend/test/uiRotation.test.ts): a block's `rotation` (degrees clockwise) turns it about
// the centre of its box, the blocks in it with it; the layout of the boxes does not change. The text form, the drawing, the
// hit test, the agent's draft of a tilted element, and the export carry it. The SVG checks of the web are pixel checks here
// (this renderer draws pixels), and o2's own prototype export turns the widget too (the web's o2 export only warns)
namespace
{
    using namespace UiDocTest;
    namespace L = Editor::UiLayout;

    UiDoc TurnedDoc()
    {
        return ParseUiml("screen S 400x400\n"
                         "  panel card anchor=center size=200x100 rotation=90\n"
                         "    button ok anchor=top-left pos=10,10 size=40x20 \"OK\"\n"
                         "  image tile anchor=top-left pos=300,300 size=60x60 rot=-15").doc;
    }

    std::optional<double> TurnOfJson(const String& json)
    {
        Json v;
        Json::Parse(json, v);
        return CleanTurn(&v);
    }

    Json Parsed(const String& text)
    {
        Json out;
        Json::Parse(text, out);
        return out;
    }

    // A pixel of a rendered picture as RGB
    std::array<int, 3> Rgb(const UiImage& img, int x, int y)
    {
        const UInt8* p = img.At(x, y);
        return { p[0], p[1], p[2] };
    }

    // A widget's resources with nothing but white sprites (the turn is the point)
    class PlainResources: public L::UiWidgetResources
    {
    public:
        Ref<Sprite> SpriteOf(const String& asset) override { return nullptr; }
        Ref<Sprite> WhiteSprite() override { return mmake<Sprite>(); }
        void SetFont(const Ref<Label>& label, const String& fontFile) override {}
        void SetLook(const Ref<Label>& label, const L::UiTextLook& look) override {}
    };
}

TEST(UiDocRotation, TheTurnIsAPropertyCleanedWrittenReadBackSetAndRemovedByEditsTheRootNeverTurns)
{
    std::optional<double> want[] = { 10, 170, 180, 180, std::nullopt, 12.3, std::nullopt };
    const char* values[] = { "370", "-190", "180", "-180", "0.04", "\"12.345\"", "\"x\"" };
    for (int i = 0; i < 7; i++)
        EXPECT_EQ(TurnOfJson(values[i]), want[i]) << values[i];

    UiDoc d = TurnedDoc();
    EXPECT_EQ(FindNode(d, "card")->rotation, std::optional<double>(90));
    EXPECT_EQ(FindNode(d, "tile")->rotation, std::optional<double>(-15)) << "rot= is a spelling of rotation=";
    EXPECT_TRUE(DocToUiml(d).Contains("panel card anchor=center size=200x100 rotation=90"));
    EXPECT_STR_EQ(DocToJson(ParseUiml(DocToUiml(d)).doc).Dump(), DocToJson(d).Dump());

    auto e = ParseEdits("@update tile rotation=30\n@update card rotation=-", d);
    UiDoc after = ApplyOps(d, e.ops).doc;
    EXPECT_EQ(FindNode(after, "tile")->rotation, std::optional<double>(30));
    EXPECT_EQ(FindNode(after, "card")->rotation, std::nullopt);

    EXPECT_EQ(NormalizeDoc(Parsed(R"({"name":"s","width":100,"height":100,"root":{"id":"root","type":"screen","rotation":45,"children":[]}})")).root.rotation,
              std::nullopt);
    UiDoc angled = NormalizeDoc(Parsed(R"({"name":"s","width":100,"height":100,"root":{"id":"root","type":"screen","children":[{"id":"a","type":"panel","angle":33}]}})"));
    EXPECT_EQ(angled.root.children[0].rotation, std::optional<double>(33));

    // An edit never turns the screen itself
    UiDoc rootTurn = ApplyOps(d, ParseEdits("@update root rotation=45", d).ops).doc;
    EXPECT_EQ(rootTurn.root.rotation, std::nullopt);
}

TEST(UiDocRotation, TheBoxesAreSolvedAsIfNothingTurnedTheScreenSeesTheTurn)
{
    UiDoc d = TurnedDoc();
    UiRects rects = SolveLayout(d);
    EXPECT_EQ(rects.Get("card"), UiRect(100, 150, 200, 100)) << "the layout is the same";
    EXPECT_EQ(rects.Get("ok"), UiRect(110, 160, 40, 20));

    // The card turned 90 degrees about (200, 200): its unturned top-left corner area (110..150, 160..180) is now at the right
    UiPoint c = Apply(WorldMatrix(d, rects, "ok"), { 130, 170 });
    EXPECT_NEAR(c.x, 230, 1e-9);
    EXPECT_NEAR(c.y, 130, 1e-9);
    EXPECT_STR_EQ(HitTest(d, rects, 230, 130), "ok") << "the button where it is drawn";
    EXPECT_STR_EQ(HitTest(d, rects, 130, 170), "") << "not where its box was";
    EXPECT_STR_EQ(HitTest(d, rects, 200, 110), "card") << "the card turned upright: 100 wide, 200 tall";
    EXPECT_TRUE(InBlock(d, rects, "card", { 200, 290 }));
    EXPECT_FALSE(InBlock(d, rects, "card", { 120, 200 }));
    EXPECT_EQ(WorldTurn(d, "ok"), 90);
    EXPECT_EQ(WorldTurn(d, "ok", false), 90);

    auto cs = Corners(rects.Get("card"), WorldMatrix(d, rects, "card"));
    int want[4][2] = { { 250, 100 }, { 250, 300 }, { 150, 300 }, { 150, 100 } };
    for (int i = 0; i < 4; i++)
    {
        EXPECT_EQ((int)JsRound(cs[i].x), want[i][0]) << i;
        EXPECT_EQ((int)JsRound(cs[i].y), want[i][1]) << i;
    }
}

TEST(UiDocRotation, DrawnTurnedAboutItsCentreItsChildrenInsideTheTurn)
{
    // A 200 x 20 bar turned 90 degrees is a 20 x 200 one
    UiKit kit = ParseKit(R"({"version":1,"name":"k","elements":[],"textStyles":[]})");
    UiDoc bar = ParseUiml("screen S 300x300 color=#000000\n  panel bar anchor=center size=200x20 kit=none color=#FFFFFF rotation=90").doc;
    UiRenderOptions o;
    o.mode = "kit";
    o.kit = &kit;
    UiImage png = RenderLayout(bar, o);
    ASSERT_EQ(png.w, 300);
    auto white = [&](int x, int y) { return png.At(x, y)[0] > 200; };
    EXPECT_TRUE(white(150, 60) && white(150, 240) && !white(60, 150) && !white(240, 150));

    // The child turns with its parent: a red block in the corner of a blue card turned 90 degrees about (200, 200)
    UiDoc nested = ParseUiml("screen S 400x400 color=#000000\n"
                             "  panel card anchor=center size=200x100 kit=none color=#0000FF rotation=90\n"
                             "    panel ok anchor=top-left pos=10,10 size=40x20 kit=none color=#FF0000").doc;
    UiImage img = RenderLayout(nested, o);
    EXPECT_EQ(Rgb(img, 230, 130), (std::array<int, 3>{ 255, 0, 0 })) << "the red block where it turned to";
    EXPECT_EQ(Rgb(img, 200, 110), (std::array<int, 3>{ 0, 0, 255 })) << "the card upright: 100 wide, 200 tall";
    EXPECT_EQ(Rgb(img, 130, 170), (std::array<int, 3>{ 0, 0, 0 })) << "nothing where the red block's box was";
    EXPECT_EQ(Rgb(img, 120, 200), (std::array<int, 3>{ 0, 0, 0 })) << "nor where the card's was";

    // A turn near the screen's edge keeps its whole block (it is drawn on a layer around its own centre)
    UiDoc edge = ParseUiml("screen S 400x200 color=#000000\n  panel long anchor=top-left pos=-150,90 size=400x20 kit=none color=#FFFFFF rotation=90").doc;
    UiImage e = RenderLayout(edge, o);
    EXPECT_GT(e.At(50, 10)[0], 200) << "the part of the bar that was off the screen before the turn";
    EXPECT_GT(e.At(50, 190)[0], 200);
    EXPECT_LT(e.At(150, 100)[0], 50);
}

TEST(UiDocRotation, ATiltedElementOfTheReferenceItsUprightSizeTheDraftTurnsItTheSkillPassKeepsTheTurn)
{
    // A 100 x 100 square turned 30 degrees shows a 136.6 box
    UiSize b = TurnedBounds(100, 100, 30);
    EXPECT_NEAR(b.w, 136.6, 0.1);
    EXPECT_NEAR(b.h, 136.6, 0.1);
    UiSize back = UnturnedSize(b.w, b.h, 30, 1);
    EXPECT_NEAR(back.w, 100, 0.01) << "exact with the graphic's proportions";
    EXPECT_NEAR(back.h, 100, 0.01);
    UiSize tb = TurnedBounds(120, 60, 15);
    UiSize free = UnturnedSize(tb.w, tb.h, 15);
    EXPECT_NEAR(free.w, 120, 0.01) << "solved when it is well posed";
    EXPECT_NEAR(free.h, 60, 0.01);
    UiSize none = UnturnedSize(80, 50, 0);
    EXPECT_EQ(none.w, 80);
    EXPECT_EQ(none.h, 50);
    // Upside down or a quarter turn: solved like the small angles they are, not taken as a square
    for (double deg : { 170.0, 100.0 })
    {
        UiSize bounds = TurnedBounds(120, 60, deg);
        UiSize solved = UnturnedSize(bounds.w, bounds.h, deg);
        EXPECT_NEAR(solved.w, 120, 0.01) << deg;
        EXPECT_NEAR(solved.h, 60, 0.01) << deg;
    }

    L::UiDoc an;
    String error;
    ASSERT_TRUE(L::ParseAnalysis(R"({"background":{"kind":"color","color":"#203040"},"elements":[)"
                                 R"({"id":"tile_a","parent":"","kind":"image","box":[100,100,237,237],"kit":"tile","rotation":-30},)"
                                 R"({"id":"tile_a_letter","parent":"tile_a","kind":"text","box":[150,150,190,190],"text":"A","color":"#000000"},)"
                                 R"({"id":"steady","parent":"","kind":"image","box":[300,300,360,360],"kit":"tile","rotation":1}]})",
                                 400, 400, an, &error)) << error.Data();
    ASSERT_TRUE(an.ById("tile_a") && an.ById("steady"));
    EXPECT_EQ(an.ById("tile_a")->rotation, std::optional<double>(-30));
    EXPECT_EQ(an.ById("steady")->rotation, std::nullopt) << "a degree is a wobble, not a tilt";

    UiKit kit = ParseKit(R"({"version":1,"name":"k","elements":[{"name":"tile","type":"image","assetType":"simple","width":50,"height":50,"png":""}],"textStyles":[]})");
    L::DraftOptions o;
    o.kit = &kit;
    o.dpi = 480;
    o.name = "S";
    o.backgroundColor = "#203040";
    UiDoc draft = L::DraftFromAnalysis(an, o).doc;
    const UiNode* tile = FindNode(draft, "tile_a");
    ASSERT_TRUE(tile);
    EXPECT_EQ(tile->rotation, std::optional<double>(-30));
    // The analysis gives boxes in thousandths of the picture: [100..237] of 400 px is 40..94.8, a 54.8 box - the box of a
    // 40-px square turned 30 degrees, about the same centre
    UiRect r = SolveLayout(draft).Get("tile_a");
    EXPECT_LE(std::abs(r.w - 40), 1);
    EXPECT_LE(std::abs(r.h - 40), 1);
    EXPECT_LE(std::abs(r.x + r.w/2 - 67.4), 1);

    // The skill rebuilds the screen upright and keeps the ids: the turn comes back from the draft
    UiDoc rebuilt = ParseUiml("screen S 400x400\n  image tile_a anchor=top-left pos=110,110 size=110x110 kit=tile\n  image other size=10x10").doc;
    UiDoc kept = L::KeepTurns(rebuilt, draft);
    EXPECT_EQ(FindNode(kept, "tile_a")->rotation, std::optional<double>(-30));
    EXPECT_EQ(FindNode(kept, "other")->rotation, std::nullopt);
    EXPECT_EQ(FindNode(rebuilt, "tile_a")->rotation, std::nullopt) << "the input is not changed";
}

TEST(UiDocRotation, TheStructureCarriesItAndReadsItBack)
{
    UiDoc d = TurnedDoc();
    UiStructureResult st = ToUiStructure(d, nullptr);
    Json json = Parsed(st.json);
    std::function<const Json*(const Json&, const String&)> find = [&](const Json& n, const String& name) -> const Json* {
        if (n.Get("name") && n.Get("name")->Str() == name)
            return &n;
        if (const Json* kids = n.Get("children"))
        {
            for (auto& c : kids->Items())
            {
                if (const Json* f = find(c, name))
                    return f;
            }
        }
        return nullptr;
    };
    const Json* root = json.Get("root") ? json.Get("root") : &json;
    ASSERT_TRUE(find(*root, "card") && find(*root, "ok"));
    EXPECT_EQ(find(*root, "card")->Get("rotation")->Num(), 90);
    EXPECT_EQ(find(*root, "ok")->Get("rotation"), nullptr);

    FromStructureInput in;
    in.structure = json;
    in.dpi = 480;
    EXPECT_EQ(FindNode(FromUiStructure(in).doc, "tile")->rotation, std::optional<double>(-15));
}

TEST(UiDocRotation, TheO2PrototypeTurnsTheWidgetAboutACentredPivot)
{
    UiDoc d = ParseUiml("screen S 400x400\n"
                        "  panel card anchor=center size=200x100 kit=none color=#3366FF rotation=90\n"
                        "    panel ok anchor=top-left pos=10,10 size=40x20 kit=none color=#FF0000").doc;
    L::UiWidgetPlan plan;
    String error;
    ASSERT_TRUE(L::PlanWidgets(Parsed(ToUiStructure(d, nullptr).json), {}, plan, &error)) << error.Data();
    for (auto& w : plan.warnings)
        EXPECT_FALSE(w.Contains("rotation")) << w.Data();

    PlainResources resources;
    L::UiWidgetTree tree = L::BuildWidgets(plan, resources, L::UiWidgetOptions());
    ASSERT_TRUE(tree.root);
    L::UpdateWidgetTree(tree.root);
    Ref<Widget> card, ok;
    for (auto& w : tree.widgets)
    {
        if (w->GetName() == "card") card = w;
        if (w->GetName() == "ok") ok = w;
    }
    ASSERT_TRUE(card && ok);
    EXPECT_NEAR(card->layout->GetAngleDegrees(), -90.0f, 1e-3f) << "counter-clockwise, y up";
    EXPECT_EQ(card->layout->GetPivot2D(), Vec2F(0.5f, 0.5f));
    EXPECT_NEAR(ok->layout->GetAngleDegrees(), 0.0f, 1e-3f) << "the child keeps its own (none) and turns with the card";

    // The screen is 400 x 400 about the origin, y up: the design point (x, y) is (x - 200, 200 - y). The card stands upright
    // (100 x 200) about the centre - its x axis points down the screen; the red block's centre (130, 170) turned 90 degrees
    // clockwise about (200, 200) is (230, 130)
    Basis cb = card->transform->GetWorldBasis();
    EXPECT_NEAR(cb.xv.x, 0.0f, 0.01f);
    EXPECT_NEAR(cb.xv.y, -200.0f, 0.01f);
    EXPECT_NEAR(cb.yv.x, 100.0f, 0.01f);
    EXPECT_NEAR(cb.yv.y, 0.0f, 0.01f);
    Vec2F cardCentre = cb.origin + (cb.xv + cb.yv)*0.5f;
    EXPECT_NEAR(cardCentre.x, 0.0f, 0.01f);
    EXPECT_NEAR(cardCentre.y, 0.0f, 0.01f);
    Basis ob = ok->transform->GetWorldBasis();
    Vec2F okCentre = ob.origin + (ob.xv + ob.yv)*0.5f;
    EXPECT_NEAR(okCentre.x, 30.0f, 0.01f);
    EXPECT_NEAR(okCentre.y, 70.0f, 0.01f);
    EXPECT_NEAR(ob.xv.y, -40.0f, 0.01f) << "the 40 x 20 block stands 20 x 40";
}
