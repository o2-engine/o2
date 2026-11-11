#include "o2Editor/stdafx.h"

#include <map>
#include <regex>

#include "UiDocTestKit.h"

using namespace UiDocTest;

namespace
{
    // The diff as the golden script writes it
    Json DiffJson(const DocDiff& d)
    {
        Json out = Json::Object();
        out["added"] = JsonStrings(d.added);
        out["removed"] = JsonStrings(d.removed);
        out["changed"] = JsonStrings(d.changed);
        out["moved"] = JsonStrings(d.moved);
        out["screen"] = d.screen;
        return out;
    }

    Json OpsJson(const Vector<UiOp>& ops)
    {
        Json out = Json::Array();
        for (auto& op : ops)
            out.Add(op);
        return out;
    }

    // Step ids (random) renamed S1, S2... in order of first appearance, as JSON.parse's reviver meets them (depth first)
    struct IdNames
    {
        std::map<std::string, std::string> names;

        void Rename(Json& v)
        {
            if (v.IsArray())
            {
                for (auto& item : v.Items())
                    Rename(item);
            }
            else if (v.IsObject())
            {
                static const std::regex stepId("^s[0-9a-z]{6,}$");
                for (auto& m : v.Members())
                {
                    Rename(m.second);
                    if ((m.first == "id" || m.first == "partOf") && m.second.IsString() &&
                        std::regex_match((const std::string&)m.second.Str(), stepId))
                    {
                        std::string id = m.second.Str();
                        if (!names.count(id))
                            names[id] = "S" + std::to_string(names.size() + 1);
                        m.second = Json(String(names[id]));
                    }
                }
            }
        }

        Json Named(const Vector<UiStep>& steps)
        {
            Json out = StepsToJson(steps);
            Rename(out);
            return out;
        }
    };

    Json PartsJson(const Vector<UiStepPart>& parts)
    {
        Json out = Json::Array();
        for (auto& p : parts)
        {
            Json part = Json::Object();
            part["label"] = p.label;
            part["n"] = p.n;
            part["at"] = p.at;
            out.Add(part);
        }
        return out;
    }

    Vector<UiOp> Update(const char* id, const Json& set)
    {
        return { MakeUpdateOp(id, set) };
    }

    Json Set(const char* key, const Json& value)
    {
        Json out = Json::Object();
        out[key] = value;
        return out;
    }
}

// --- the web's tests (backend/test/uiLayout.test.ts) ----------------------------------------------------------------

TEST(UiDocOps, EditsApplyByIdReportWhatDoesNotExistAndTheDiffNamesWhatChanged)
{
    UiDoc doc = Parse(Screen());
    auto edits = ParseEdits("@update play text=\"START\" kit=btn_blue\n@add menu after=play\n  button shop h=120 \"SHOP\"\n@remove settings\n"
                            "@move coin to=bar index=0\n@update nope text=\"x\"\n@screen size=1080x2340", doc);
    EXPECT_TRUE(edits.errors.IsEmpty());
    auto res = ApplyOps(doc, edits.ops);
    EXPECT_EQ(res.errors, Vector<String>({ "@update: no block \"nope\"" }));
    const UiNode* menu = FindNode(res.doc, "menu");
    ASSERT_TRUE(menu);
    ASSERT_EQ(menu->children.size(), 3u);
    EXPECT_EQ(menu->children[0].id, "play");
    EXPECT_EQ(menu->children[1].id, "shop");
    EXPECT_EQ(menu->children[2].id, "coins");
    EXPECT_EQ(*menu->children[0].text, "START");
    EXPECT_EQ(FindNode(res.doc, "bar")->children[0].id, "coin");
    EXPECT_EQ(res.doc.height, 2340);

    DocDiff d = DiffDocs(doc, res.doc);
    EXPECT_EQ(d.added, Vector<String>({ "shop" }));
    EXPECT_EQ(d.removed, Vector<String>({ "settings" }));
    EXPECT_TRUE(d.changed.Contains("play"));
    EXPECT_EQ(d.moved, Vector<String>({ "coin" }));
    EXPECT_TRUE(d.screen);
}

TEST(UiDocOps, TextThatWouldBreakABlockIsIgnored)
{
    // Unsetting id/type, "children=", object keys as types, one-number size (AssetsLine 77d5b09)
    UiDoc doc = Parse(Screen());
    auto edits = ParseEdits("@update play id=- type=- children=- text=\"GO\"", doc);
    EXPECT_TRUE(edits.errors.IsEmpty());
    UiDoc after = ApplyOps(doc, edits.ops).doc;
    const UiNode* play = FindNode(after, "play");
    ASSERT_TRUE(play);
    EXPECT_EQ(play->type, "button");
    EXPECT_EQ(*play->text, "GO");

    // A node an unset slipped through to (stored by an older editor) keeps them too
    Json unset = Json::Array();
    for (const char* k : { "id", "type", "children" })
        unset.Add(Json(k));
    Json old = Json::Object();
    old["op"] = "update";
    old["id"] = "play";
    old["unset"] = unset;
    UiDoc oldDoc = ApplyOps(doc, { old }).doc;
    ASSERT_TRUE(FindNode(oldDoc, "play"));
    EXPECT_EQ(FindNode(oldDoc, "play")->type, "button");
    EXPECT_EQ(FindNode(oldDoc, "menu")->children.size(), FindNode(doc, "menu")->children.size());

    // "constructor" is a key of every object, not a block type
    String type, layout;
    EXPECT_FALSE(ResolveType("constructor", type, layout));
    EXPECT_FALSE(ParseUiml("screen S 100x100\n  constructor x\n").errors.IsEmpty());

    // "children=" on a line with an indented child under it
    auto nested = ParseUiml("screen S 100x100\n  group g children=x\n    button b\n");
    EXPECT_TRUE(nested.errors.IsEmpty());
    ASSERT_TRUE(FindNode(nested.doc, "g"));
    ASSERT_EQ(FindNode(nested.doc, "g")->children.size(), 1u);
    EXPECT_EQ(FindNode(nested.doc, "g")->children[0].id, "b");
    // ... and on a line with none
    auto bare = ParseUiml("screen S 100x100\n  group g children=x\n");
    EXPECT_TRUE(bare.errors.IsEmpty());
    EXPECT_TRUE(FindNode(bare.doc, "g")->children.empty());
    EXPECT_STR_EQ(NodeToJson(*FindNode(bare.doc, "g")).Dump(), "{\"id\":\"g\",\"type\":\"group\"}");

    // A one-number size sets the width only, the same before and after the step is stored
    auto one = ParseEdits("@update play size=10", doc).ops;
    ASSERT_EQ(one.Count(), 1);
    EXPECT_STR_EQ(one[0].Dump(), "{\"op\":\"update\",\"id\":\"play\",\"set\":{\"w\":10}}");
    UiDoc sized = ApplyOps(doc, one).doc;
    EXPECT_EQ(*FindNode(sized, "play")->w, 10);
    EXPECT_EQ(FindNode(sized, "play")->h, FindNode(doc, "play")->h);
    auto pos = ParseEdits("@update close pos=12", doc).ops;
    EXPECT_STR_EQ(pos[0].Dump(), "{\"op\":\"update\",\"id\":\"close\",\"set\":{\"x\":12}}");
    EXPECT_EQ(FindNode(ApplyOps(doc, pos).doc, "close")->y, FindNode(doc, "close")->y);
}

TEST(UiDocOps, ANewBlockNeverTakesAnIdThatExists)
{
    UiDoc doc = Parse(Screen());
    auto res = ApplyOps(doc, ParseEdits("@add root\n  button play \"again\"", doc).ops);
    Vector<String> ids;
    bool dup = false;
    Walk(res.doc.root, [&](const UiNode& n, const UiNode*, int) { dup = dup || ids.Contains(n.id); ids.Add(n.id); });
    EXPECT_FALSE(dup);
    EXPECT_TRUE(ids.Contains("play_2"));
}

TEST(UiDocOps, TheNodeKeepsStepsAndTheLayoutIsThemAppliedInOrder)
{
    UiStep s1, s2;
    s1.data["id"] = "s1"; s1.data["by"] = "agent"; s1.data["prompt"] = "menu"; s1.data["reply"] = "ok"; s1.data["at"] = "";
    s1.data["ops"] = OpsJson({ MakeDocOp(Parse(Screen())) });
    s2.data["id"] = "s2"; s2.data["by"] = "user"; s2.data["at"] = "";
    s2.data["ops"] = OpsJson(Update("play", Set("text", Json("GO"))));
    Json config = Json::Object();
    config["steps"] = StepsToJson({ s1, s2 });
    EXPECT_EQ(*FindNode(DocAt(config).doc, "menu")->children[0].text, "GO");
    EXPECT_EQ(*FindNode(DocAt(config, 1).doc, "menu")->children[0].text, "PLAY");
    EXPECT_TRUE(DocAt(Json::Object()).doc.root.children.empty());
}

// --- the web's tests (backend/test/uiEdit.test.ts: a run of hand edits is one step) ----------------------------------

namespace
{
    const Int64 T0 = 1762768800000; // 2026-11-10T10:00:00Z

    UiStep AgentStep()
    {
        UiStep s;
        s.data["id"] = "ag"; s.data["by"] = "agent"; s.data["prompt"] = "make it"; s.data["reply"] = "done";
        Json node = Json::Object();
        node["id"] = "x";
        node["type"] = "button";
        s.data["ops"] = OpsJson({ MakeAddOp("root", { node }) });
        s.data["at"] = IsoTime((double)T0);
        return s;
    }

    Vector<String> Labels(const UiStep& s)
    {
        Vector<String> out;
        for (auto& p : StepParts(s))
            out.Add(p.label);
        return out;
    }

    Json ConfigOf(const Vector<UiStep>& steps)
    {
        Json config = Json::Object();
        config["steps"] = StepsToJson(steps);
        return config;
    }
}

TEST(UiDocOps, ActionsJoinTheStepAndTypingIntoOneBlockFolds)
{
    Vector<UiStep> steps = { AgentStep() };
    steps = PushUserAction(steps, Update("x", Set("w", Json(10))), "x: size", T0 + 1000);
    steps = PushUserAction(steps, Update("x", Set("text", Json("A"))), "x: text", T0 + 10000);
    steps = PushUserAction(steps, Update("x", Set("text", Json("AB"))), "x: text", T0 + 11000);
    Json node = Json::Object();
    node["id"] = "y";
    node["type"] = "icon";
    steps = PushUserAction(steps, { MakeAddOp("root", { node }) }, "+ y", T0 + 20000);
    ASSERT_EQ(steps.Count(), 2);
    const UiStep& hand = steps[1];
    EXPECT_EQ(Labels(hand), Vector<String>({ "x: size", "x: text", "+ y" }));
    EXPECT_EQ(hand.Ops().Count(), 3);
    EXPECT_EQ(UserStepText(hand), "x: size; x: text; + y");
    EXPECT_EQ(*FindNode(DocAt(ConfigOf(steps)).doc, "x")->text, "AB");

    // Another field, or a gesture on the stage, is an action of its own however soon it comes
    auto other = PushUserAction(steps, Update("x", Set("text", Json("ABC"))), "x: tint", T0 + 20500);
    EXPECT_EQ(StepParts(other[1]).Count(), 4);
    auto g1 = PushUserAction(steps, Update("y", Set("x", Json(5))), "y: move", T0 + 21000, false);
    auto g2 = PushUserAction(g1, Update("y", Set("x", Json(9))), "y: move", T0 + 21500, false);
    EXPECT_EQ(StepParts(g2[1]).Count(), 5);
    // A long pause starts a new step; the agent's answer does too
    auto later = PushUserAction(steps, Update("x", Set("h", Json(5))), "x: size", T0 + 20000 + (Int64)HAND_STEP_IDLE_MS + 1);
    EXPECT_EQ(later.Count(), 3);
}

TEST(UiDocOps, UndoTakesOneActionBackRedoPutsItIntoItsStepAgain)
{
    Vector<UiStep> steps = { AgentStep() };
    steps = PushUserAction(steps, Update("x", Set("w", Json(10))), "x: size", T0 + 1000);
    steps = PushUserAction(steps, Update("x", Set("h", Json(20))), "x: height", T0 + 9000);
    steps = PushUserAction(steps, Update("x", Set("text", Json("Go"))), "x: text", T0 + 20000);
    UiUndoResult u;
    ASSERT_TRUE(UndoAction(steps, {}, u));
    EXPECT_EQ(u.steps.Count(), 2);
    EXPECT_EQ(StepParts(u.steps[1]).Count(), 2);
    EXPECT_FALSE(FindNode(DocAt(ConfigOf(u.steps)).doc, "x")->text.has_value());
    ASSERT_TRUE(UndoAction(u.steps, u.redo, u));
    ASSERT_TRUE(UndoAction(u.steps, u.redo, u));
    EXPECT_EQ(u.steps.Count(), 1);
    UiUndoResult r;
    ASSERT_TRUE(RedoAction(u.steps, u.redo, r));
    ASSERT_TRUE(RedoAction(r.steps, r.redo, r));
    ASSERT_TRUE(RedoAction(r.steps, r.redo, r));
    EXPECT_EQ(r.redo.Count(), 0);
    EXPECT_EQ(r.steps.Count(), 2);
    EXPECT_EQ(Labels(r.steps[1]), Vector<String>({ "x: size", "x: height", "x: text" }));
    EXPECT_STR_EQ(DocToJson(DocAt(ConfigOf(r.steps)).doc).Dump(), DocToJson(DocAt(ConfigOf(steps)).doc).Dump());

    // Old user steps (no parts) are one action
    UiStep old;
    old.data["id"] = "o"; old.data["by"] = "user"; old.data["prompt"] = "edit";
    old.data["ops"] = OpsJson(Update("x", Set("w", Json(1))));
    old.data["at"] = AgentStep().At();
    ASSERT_TRUE(UndoAction({ AgentStep(), old }, {}, u));
    EXPECT_STR_EQ(StepsToJson(u.steps).Dump(), StepsToJson({ AgentStep() }).Dump());
    EXPECT_FALSE(UndoAction({}, {}, u));
    EXPECT_FALSE(RedoAction({}, {}, r));
}

// --- golden ------------------------------------------------------------------------------------------------------------

TEST(UiDocOps, GoldenEditsParsedTextAppliedAndDiffed)
{
    for (auto& c : G("edits").Items())
    {
        SCOPED_TRACE(S(c, "name").Data());
        UiDoc doc = Parse(S(c, "base"));
        auto edits = ParseEdits(S(c, "src"), doc);
        EXPECT_STR_EQ(OpsJson(edits.ops).Dump(), S(c, "ops"));
        EXPECT_EQ(edits.errors, Strings(M(c, "errors")));
        EXPECT_STR_EQ(OpsToText(edits.ops), S(c, "text"));
        auto res = ApplyOps(doc, edits.ops);
        EXPECT_STR_EQ(DocToJson(res.doc).Dump(), S(c, "doc"));
        EXPECT_EQ(res.errors, Strings(M(c, "applyErrors")));
        DocDiff d = DiffDocs(doc, res.doc);
        EXPECT_STR_EQ(DiffJson(d).Dump(), M(c, "diff").Dump());
        EXPECT_STR_EQ(DiffSummary(d), S(c, "summary"));
        EXPECT_STR_EQ(DocToUiml(res.doc), S(c, "uiml"));
    }
}

TEST(UiDocOps, GoldenStoredOpsTheWebTolerates)
{
    UiDoc doc = Parse(Screen());
    Vector<UiOp> before;
    for (auto& c : G("rawOps").Items())
    {
        const Json& op = M(c, "op");
        SCOPED_TRACE(op.Dump().Data());
        UiDoc start = ApplyOps(doc, before).doc;
        auto res = ApplyOps(start, { op });
        EXPECT_STR_EQ(DocToJson(res.doc).Dump(), S(c, "doc"));
        EXPECT_EQ(res.errors, Strings(M(c, "errors")));
        if (!M(c, "text").IsNull())
            EXPECT_STR_EQ(OpsToText({ op }), S(c, "text"));
        before.Add(op);
    }
}

TEST(UiDocOps, GoldenHandEditHistoryStepByStep)
{
    const Json& g = G("steps");
    Int64 t0 = (Int64)M(g, "t0").Num();
    Vector<UiStep> steps = { UiStep(M(g, "agent")) };
    Vector<UiStep> redo;
    IdNames names;
    const Json& acts = M(g, "acts");
    const Json& trace = M(g, "trace");
    ASSERT_EQ(acts.Count(), trace.Count());
    for (int i = 0; i < acts.Count(); i++)
    {
        const Json& a = *acts.At(i);
        String kind = S(a, "kind");
        SCOPED_TRACE((String("act ") + JsNum(i) + " " + kind).Data());
        if (kind == "push")
            steps = PushUserAction(steps, M(a, "ops").Items(), S(a, "label"), t0 + (Int64)M(a, "dt").Num(), M(a, "fold").Bool());
        else
        {
            UiUndoResult r;
            if (kind == "undo" ? UndoAction(steps, redo, r) : RedoAction(steps, redo, r))
            {
                steps = r.steps;
                redo = r.redo;
            }
        }

        const Json& t = *trace.At(i);
        EXPECT_STR_EQ(names.Named(steps).Dump(), M(t, "steps").Dump());
        EXPECT_STR_EQ(names.Named(redo).Dump(), M(t, "redo").Dump());
        Json parts = Json::Array();
        Json texts = Json::Array();
        for (auto& s : steps)
        {
            parts.Add(PartsJson(StepParts(s)));
            texts.Add(Json(s.By() == "user" ? UserStepText(s) : String()));
        }
        EXPECT_STR_EQ(parts.Dump(), M(t, "parts").Dump());
        EXPECT_STR_EQ(texts.Dump(), M(t, "text").Dump());
        EXPECT_STR_EQ(DocToJson(DocAt(ConfigOf(steps)).doc).Dump(), S(t, "doc"));
    }
}

TEST(UiDocOps, GoldenStepPartsAndDocAt)
{
    for (auto& c : G("stepParts").Items())
    {
        UiStep s(M(c, "s"));
        SCOPED_TRACE(M(c, "s").Dump().Data());
        Json expected = M(c, "parts");
        // The web hands back stored parts as they are; a part without `at` reads as ""
        for (auto& p : expected.Items())
        {
            if (!p.Has("at"))
                p["at"] = "";
        }
        EXPECT_STR_EQ(PartsJson(StepParts(s)).Dump(), expected.Dump());
        EXPECT_STR_EQ(UserStepText(s), S(c, "text"));
    }

    const Json& g = G("docAt");
    for (auto& c : M(g, "counts").Items())
    {
        const Json& count = M(c, "c");
        auto res = count.IsNull() ? DocAt(M(g, "config")) : DocAt(M(g, "config"), (int)count.Num());
        EXPECT_STR_EQ(DocToJson(res.doc).Dump(), S(c, "doc")) << count.Dump().Data();
        EXPECT_EQ(res.errors, Strings(M(c, "errors")));
    }
}
