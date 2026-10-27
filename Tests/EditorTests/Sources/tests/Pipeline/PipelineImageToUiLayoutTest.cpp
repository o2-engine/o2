#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "ImageToUiTestKit.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgent.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutWidgets.h"

using namespace o2;
using namespace Editor;
using namespace Editor::UiLayout;

namespace
{
    // Every layout feature of the skill's schema: row (even spacing, padding, a child's cross alignment and offset), column
    // (fixed spacing, centred), grid (padding, alignment, two spacings), align / vAlign / offset, responsive, a scrolled
    // viewport, a progress bar with a formatted label and a state block
    const char* structureText = R"({
  "canvas": {"width": 400, "height": 300, "name": "Layouts"},
  "root": {"type": "container", "name": "root", "position": {"x": 0, "y": 0}, "size": {"width": 400, "height": 300},
    "anchor": {"horizontal": "left", "vertical": "top"},
    "children": [
      {"type": "container", "name": "row", "position": {"x": 10, "y": 10}, "size": {"width": 300, "height": 60},
        "anchor": {"horizontal": "left", "vertical": "top"},
        "layout": {"type": "row", "spacing": "even", "padding": {"x": 5, "y": 4}, "vAlign": "middle"},
        "children": [
          {"type": "image", "name": "a", "size": {"width": 50, "height": 40}, "anchor": {"horizontal": "left", "vertical": "top"}, "asset": "a.png"},
          {"type": "image", "name": "b", "size": {"width": 51, "height": 30}, "vAlign": "bottom", "offset": {"x": 1, "y": -2},
            "anchor": {"horizontal": "left", "vertical": "top"}, "asset": "a.png"},
          {"type": "image", "name": "c", "size": {"width": 40, "height": 40}, "anchor": {"horizontal": "left", "vertical": "top"}, "asset": "b.png", "nineSlice": "meta"}
        ]},
      {"type": "container", "name": "col", "position": {"x": 10, "y": 80}, "size": {"width": 120, "height": 120},
        "anchor": {"horizontal": "left", "vertical": "top"}, "layout": {"type": "column", "spacing": 7, "align": "center"},
        "children": [
          {"type": "rect", "name": "p", "size": {"width": 100, "height": 30}, "color": "#336699", "anchor": {"horizontal": "left", "vertical": "top"}},
          {"type": "rect", "name": "q", "size": {"width": 60, "height": 30}, "align": "right", "color": "#336699", "anchor": {"horizontal": "left", "vertical": "top"}}
        ]},
      {"type": "container", "name": "grid", "position": {"x": 150, "y": 80}, "size": {"width": 200, "height": 120},
        "anchor": {"horizontal": "center", "vertical": "middle"},
        "layout": {"type": "grid", "columns": 3, "cellSize": {"width": 40, "height": 30}, "spacing": {"x": 6, "y": 5},
          "padding": {"x": 3, "y": 2}, "align": "center", "vAlign": "bottom"},
        "children": [
          {"type": "image", "name": "g1", "size": {"width": 40, "height": 30}, "anchor": {"horizontal": "left", "vertical": "top"}, "asset": "a.png", "nineSlice": true},
          {"type": "image", "name": "g2", "size": {"width": 40, "height": 30}, "anchor": {"horizontal": "left", "vertical": "top"}, "asset": "a.png", "nineSlice": 3},
          {"type": "image", "name": "g3", "size": {"width": 40, "height": 30}, "anchor": {"horizontal": "left", "vertical": "top"}, "asset": "a.png",
            "nineSlice": {"left": 4, "right": 5}},
          {"type": "image", "name": "g4", "size": {"width": 40, "height": 30}, "anchor": {"horizontal": "left", "vertical": "top"}, "asset": "a.png"},
          {"type": "image", "name": "g5", "size": {"width": 40, "height": 30}, "anchor": {"horizontal": "left", "vertical": "top"}, "asset": "a.png"}
        ]},
      {"type": "image", "name": "centered", "size": {"width": 33, "height": 21}, "align": "center", "vAlign": "bottom", "offset": {"x": 3, "y": -4},
        "anchor": {"horizontal": "center", "vertical": "bottom"}, "asset": "a.png"},
      {"type": "image", "name": "resp", "size": {"width": 120, "height": 40}, "anchor": {"horizontal": "right", "vertical": "bottom"}, "asset": "a.png",
        "responsive": {"min": {"x": 1, "y": 1}, "max": {"x": 1, "y": 1}, "offsetMin": {"x": -136.5, "y": -56}, "offsetMax": {"x": -16, "y": -16}}},
      {"type": "container", "name": "view", "position": {"x": 200, "y": 220}, "size": {"width": 100, "height": 50},
        "anchor": {"horizontal": "left", "vertical": "top"},
        "scroll": {"content": "content", "direction": "vertical", "offset": {"x": 0, "y": 30}},
        "children": [
          {"type": "container", "name": "content", "position": {"x": 0, "y": 0}, "size": {"width": 100, "height": 120},
            "anchor": {"horizontal": "left", "vertical": "top"},
            "children": [{"type": "rect", "name": "r", "position": {"x": 2, "y": 70}, "size": {"width": 20, "height": 10}, "color": "#FF0000",
              "anchor": {"horizontal": "left", "vertical": "top"}}]}
        ]},
      {"type": "container", "name": "bar", "position": {"x": 10, "y": 260}, "size": {"width": 120, "height": 20},
        "anchor": {"horizontal": "left", "vertical": "top"},
        "progress": {"min": 0, "max": 100, "value": 65, "fill": "fill", "direction": "left-to-right", "label": "label", "format": "{value:.0f}/{max:.0f}"},
        "children": [
          {"type": "rect", "name": "track", "position": {"x": 0, "y": 0}, "size": {"width": 120, "height": 20}, "color": "#202630",
            "anchor": {"horizontal": "left", "vertical": "top"}},
          {"type": "rect", "name": "fill", "position": {"x": 2, "y": 2}, "size": {"width": 116, "height": 16}, "color": "#35C779",
            "anchor": {"horizontal": "left", "vertical": "top"}},
          {"type": "text", "name": "label", "text": "x", "position": {"x": 0, "y": 0}, "size": {"width": 120, "height": 20}, "fontSize": 14,
            "anchor": {"horizontal": "center", "vertical": "middle"}, "alignment": "center"}
        ]},
      {"type": "container", "name": "tab", "position": {"x": 300, "y": 10}, "size": {"width": 80, "height": 30},
        "anchor": {"horizontal": "right", "vertical": "top"},
        "state": {"current": "selected", "variants": {"normal": {}, "selected": {"bg": {"asset": "b.png"}, "label": {"color": "#FFE8A0"}}}},
        "children": [
          {"type": "image", "name": "bg", "position": {"x": 0, "y": 0}, "size": {"width": 80, "height": 30}, "asset": "a.png",
            "anchor": {"horizontal": "left", "vertical": "top"}},
          {"type": "text", "name": "label", "text": "Tab", "position": {"x": 0, "y": 0}, "size": {"width": 80, "height": 30},
            "fontFamily": "fonts/Roboto-Bold.ttf", "fontSize": 16, "color": "#FFFFFF", "strokeColor": "#000000", "strokeWidth": 2,
            "anchor": {"horizontal": "center", "vertical": "middle"}, "alignment": "center", "textVAlign": "top"}
        ]}
    ]},
  "metadata": {}
})";

    // What the skill's own Python makes of a structure: every node's path, _abs box, text, asset and colour after the snapshot
    bool SkillResolution(const String& structure, Json& out, String& log)
    {
        String home = ImageToUiHome();
        Vector<String> pythons = PythonCandidates("");
        if (home.IsEmpty() || pythons.IsEmpty())
            return false;

        String file = AbsolutePath(".") + "/image-to-ui-layout-" + (String)(int)(Math::Random()%1000000) + ".json";
        PipelineUtils::WriteFileBytes(file, structure);
        String script =
            "import json, sys\n"
            "sys.path.insert(0, sys.argv[1])\n"
            "import layout, ui_components as c\n"
            "s = c.snapshot(json.load(open(sys.argv[2])))\n"
            "layout.resolve_positions(s)\n"
            "print(json.dumps([{'path': p, 'abs': n['_abs'], 'text': n.get('text'), 'asset': n.get('asset'), 'color': n.get('color')}\n"
            "                  for p, n in c.nodes(s['root'])]))\n";
        int code = ImageToUiTestKit::RunToEnd(pythons[0], { "-B", "-c", script, home + "/upstream/image-to-ui/scripts", file }, log);
        o2FileSystem.FileDelete(file);
        return code == 0 && Json::Parse(log, out);
    }

    const UiPlanNode* NodeAt(const UiWidgetPlan& plan, const String& path)
    {
        for (auto& node : plan.nodes)
        {
            if (node.path == path)
                return &node;
        }
        return nullptr;
    }
}

// The o2 prototype is built from the boxes the skill's layout.py resolves: every node lands where the skill drew it
TEST(ImageToUiLayout, ResolvesEveryBoxLikeTheSkill)
{
    Json structure;
    ASSERT_TRUE(Json::Parse(structureText, structure));
    Vector<UiResolvedNode> boxes;
    String error;
    ASSERT_TRUE(ResolveStructure(SnapshotStructure(structure), boxes, &error)) << error.Data();

    Json skill;
    String log;
    if (!SkillResolution(structureText, skill, log))
        GTEST_SKIP() << "no Python or no image-to-UI bundle: " << log.Data();

    ASSERT_EQ(skill.Count(), boxes.Count()) << log.Data();
    for (int i = 0; i < boxes.Count(); i++)
    {
        const Json& expected = skill.Items()[i];
        EXPECT_EQ(boxes[i].path, expected.Get("path")->Str());
        const Json* abs = expected.Get("abs");
        Box b(abs->At(0)->Num(), abs->At(1)->Num(), abs->At(2)->Num(), abs->At(3)->Num());
        EXPECT_EQ(boxes[i].abs, b) << boxes[i].path.Data() << ": " << boxes[i].abs.x << "," << boxes[i].abs.y << " " << boxes[i].abs.w << "x"
                                   << boxes[i].abs.h << " vs " << b.x << "," << b.y << " " << b.w << "x" << b.h;
    }

    // The snapshot: the current state and the progress label, as the skill renders them
    Json snapshot = SnapshotStructure(structure);
    for (auto& expected : skill.Items())
    {
        String path = expected.Get("path")->Str();
        Function<const Json*(const Json&, const String&)> find = [&](const Json& node, const String& at) -> const Json*
        {
            if (at == path)
                return &node;
            if (const Json* children = node.Get("children"))
            {
                for (auto& c : children->Items())
                {
                    if (auto found = find(c, at + "/" + c.Get("name")->Str()))
                        return found;
                }
            }
            return nullptr;
        };
        const Json* mine = find(*snapshot.Get("root"), "root");
        ASSERT_TRUE(mine) << path.Data();
        for (auto key : { "text", "asset", "color" })
        {
            const Json* theirs = expected.Get(key);
            const Json* ours = mine->Get(key);
            EXPECT_EQ(ours ? ours->Dump() : String("null"), theirs ? theirs->Dump() : String("null")) << path.Data() << " " << key;
        }
    }
}

TEST(ImageToUiLayout, ProgressLabelsFormatAsPython)
{
    String out;
    ASSERT_TRUE(FormatProgress("{percent:.0f}%", 65, 0, 100, out));
    EXPECT_EQ(out, "65%");
    ASSERT_TRUE(FormatProgress("{value:.0f}/{max:.0f}", 3, 0, 10, out));
    EXPECT_EQ(out, "3/10");
    ASSERT_TRUE(FormatProgress("{value} of {max} ({percent})", 1, 0, 4, out));
    EXPECT_EQ(out, "1 of 4 (25.0)");
    ASSERT_TRUE(FormatProgress("{{x}} {value:.2f}", 0.5, 0, 1, out));
    EXPECT_EQ(out, "{x} 0.50");
    EXPECT_FALSE(FormatProgress("{other}", 1, 0, 2, out));
    EXPECT_FALSE(FormatProgress("{value:>8}", 1, 0, 2, out));
}

// What the prototype is made of: widget kinds, anchors, sprites with their nine-slice borders, texts, the progress fill, the
// baked state, and the warnings for what o2 does not reproduce
TEST(ImageToUiLayout, PlansTheWidgetsOfTheSkillsSchema)
{
    Json structure;
    ASSERT_TRUE(Json::Parse(structureText, structure));
    Map<String, UiSpriteInfo> sprites;
    UiSpriteInfo a;
    a.w = a.h = 40;
    NineSlice border;
    border.left = border.right = 7;
    border.top = border.bottom = 6;
    a.nine = border;
    sprites["a.png"] = a;
    UiSpriteInfo b;
    b.w = b.h = 10;
    sprites["b.png"] = b;

    UiWidgetPlan plan;
    String error;
    ASSERT_TRUE(PlanWidgets(structure, sprites, plan, &error)) << error.Data();
    EXPECT_EQ(plan.rootName, "Layouts");
    EXPECT_EQ(plan.size, Vec2F(400, 300));
    ASSERT_GT(plan.nodes.Count(), 20);
    EXPECT_EQ(plan.nodes[0].parent, -1);
    EXPECT_EQ(plan.nodes[0].widgetPath, "Layouts");

    const UiPlanNode* c = NodeAt(plan, "root/row/c");
    ASSERT_TRUE(c);
    EXPECT_EQ(c->widget, "Image");
    EXPECT_EQ(c->widgetPath, "Layouts/row/c");
    ASSERT_TRUE(c->nine.has_value());
    EXPECT_EQ(c->nine->left, 4) << "no meta: a quarter of the smaller side, at least 4";

    auto g1 = NodeAt(plan, "root/grid/g1"), g2 = NodeAt(plan, "root/grid/g2"), g3 = NodeAt(plan, "root/grid/g3"), g4 = NodeAt(plan, "root/grid/g4");
    ASSERT_TRUE(g1 && g2 && g3 && g4);
    EXPECT_EQ(g1->nine->left, 7) << "true takes the .png.meta borders";
    EXPECT_EQ(g1->nine->top, 6);
    EXPECT_EQ(g2->nine->bottom, 3);
    EXPECT_EQ(g3->nine->right, 5);
    EXPECT_EQ(g3->nine->top, 0);
    EXPECT_FALSE(g4->nine.has_value());
    EXPECT_EQ(NodeAt(plan, "root/grid")->widget, "Widget") << "two spacings: the cells are baked";

    auto p = NodeAt(plan, "root/col/p");
    ASSERT_TRUE(p);
    EXPECT_EQ(p->type, "rect");
    EXPECT_EQ(p->color, "#336699");
    EXPECT_EQ(p->rel, Box(0, 26, 100, 30)) << "the column centres its items along it";
    EXPECT_EQ(NodeAt(plan, "root/col/q")->rel, Box(60, 63, 60, 30)) << "an item's own align moves it across";

    // A box attached to its anchor point: the grid hangs from the root's centre, y up in o2
    auto grid = NodeAt(plan, "root/grid");
    EXPECT_EQ(grid->anchorMin, Vec2F(0.5f, 0.5f));
    EXPECT_EQ(grid->offsetMin, Vec2F(150 - 200, 300 - 80 - 120 - 150));
    EXPECT_EQ(grid->offsetMax, Vec2F(150 + 200 - 200, 300 - 80 - 150));

    // A responsive block is o2's own anchors
    auto resp = NodeAt(plan, "root/resp");
    ASSERT_TRUE(resp);
    EXPECT_EQ(resp->anchorMin, Vec2F(1, 0));
    EXPECT_EQ(resp->anchorMax, Vec2F(1, 0));
    EXPECT_EQ(resp->offsetMin, Vec2F(-136.5f, 16));
    EXPECT_EQ(resp->offsetMax, Vec2F(-16, 56));

    auto fill = NodeAt(plan, "root/bar/fill");
    ASSERT_TRUE(fill);
    EXPECT_EQ(fill->widget, "HorizontalProgress");
    EXPECT_EQ(fill->progressOrientation, "Right");
    EXPECT_NEAR(fill->progressValue, 0.65, 1e-9);
    EXPECT_EQ(NodeAt(plan, "root/bar/label")->text, "65/100");
    EXPECT_EQ(NodeAt(plan, "root/bar/label")->fontFile, "Roboto-Regular.ttf");
    EXPECT_EQ(NodeAt(plan, "root/bar/label")->textVAlign, "middle") << "the renderer's default";

    auto bg = NodeAt(plan, "root/tab/bg"), label = NodeAt(plan, "root/tab/label");
    ASSERT_TRUE(bg && label);
    EXPECT_EQ(bg->asset, "b.png") << "the current state is baked";
    EXPECT_EQ(label->color, "#FFE8A0");
    EXPECT_EQ(label->widget, "Label");
    EXPECT_EQ(label->fontFile, "Roboto-Bold.ttf");
    EXPECT_EQ(label->strokeColor, "#000000");
    EXPECT_EQ(label->strokeWidth, 2);
    EXPECT_EQ(label->textVAlign, "top");

    auto has = [&](const String& text) { return plan.warnings.Any([&](const String& w) { return w.Contains(text); }); };
    EXPECT_TRUE(has("root/row: row layout baked as positions"));
    EXPECT_TRUE(has("root/grid: grid spacing 6x5 baked"));
    EXPECT_TRUE(has("root/tab: state \"selected\" baked"));
    EXPECT_TRUE(has("root/view: scroll viewport baked"));
    EXPECT_FALSE(has("missing sprite"));

    // One spacing on both axes: a native GridLayout from the first cell
    Json native;
    ASSERT_TRUE(Json::Parse(R"({"canvas": {"width": 100, "height": 100}, "root": {"type": "container", "name": "root",
        "size": {"width": 100, "height": 100}, "children": [{"type": "container", "name": "g", "position": {"x": 0, "y": 0},
        "size": {"width": 100, "height": 100}, "layout": {"type": "grid", "columns": 2, "cellSize": {"width": 20, "height": 20},
        "spacing": {"x": 4, "y": 4}, "padding": 6}, "children": [{"type": "rect", "name": "x", "size": {"width": 20, "height": 20}, "color": "#000000"},
        {"type": "rect", "name": "y", "size": {"width": 20, "height": 20}, "color": "#000000"},
        {"type": "rect", "name": "z", "size": {"width": 20, "height": 20}, "color": "#000000"}]}]}})", native));
    UiWidgetPlan nativePlan;
    ASSERT_TRUE(PlanWidgets(native, {}, nativePlan));
    auto g = NodeAt(nativePlan, "root/g");
    ASSERT_TRUE(g);
    EXPECT_EQ(g->widget, "GridLayout");
    EXPECT_EQ(g->gridColumns, 2);
    EXPECT_EQ(g->gridSpacing, 4);
    EXPECT_EQ(NodeAt(nativePlan, "root/g/z")->rel, Box(6, 30, 20, 20));

    Json bad;
    ASSERT_TRUE(Json::Parse(R"({"root": {"name": "root", "size": {"width": 10, "height": 10}, "layout": {"type": "flex"}, "children": [{}]}})", bad));
    EXPECT_FALSE(PlanWidgets(bad, {}, nativePlan, &error));
    EXPECT_TRUE(error.Contains("row or column or grid"));
}

// The host's side of the agent: its command line, where the Python and the bundle are found, the child process with pipes
TEST(ImageToUiAgent, CommandLineBundleAndProcess)
{
    AgentRun run;
    run.design = "/w/input/design.png";
    run.assets = "/w/sprites";
    run.task = "/w/task";
    run.scratch = "/w/agent-view";
    run.traceDir = "/w/trace";
    run.maxTurns = 60;
    run.name = "Main";
    run.hint = "- a word game";
    run.note = "About the sliced assets: ...";
    run.fonts = { "fonts/Roboto-Regular.ttf", "fonts/Roboto-Bold.ttf" };
    EXPECT_EQ(AgentArguments("/b", run), Vector<String>({ "/b/agent/agent.py", "--design", "/w/input/design.png", "--assets", "/w/sprites",
        "--output", "/w/task", "--scratch", "/w/agent-view", "--skill", "/b/upstream/image-to-ui", "--max-turns", "60", "--name=Main",
        "--hint=- a word game", "--sprites-note=About the sliced assets: ...", "--font", "fonts/Roboto-Regular.ttf", "--font",
        "fonts/Roboto-Bold.ttf", "--trace-dir", "/w/trace" })) << "a hint that starts with a dash is still the hint";

    String home = ImageToUiHome();
    ASSERT_FALSE(home.IsEmpty()) << "o2/Tools/ImageToUi is missing: run tools/sync-image-to-ui.sh";
    EXPECT_TRUE(o2FileSystem.IsFileExist(home + "/upstream/image-to-ui/scripts/workflow.py"));
    EXPECT_TRUE(o2FileSystem.IsFileExist(home + "/agent/agent.py"));
    EXPECT_TRUE(home.StartsWith("/"));

    EXPECT_EQ(PythonCandidates("/no/such/python"), Vector<String>({ "/no/such/python" })) << "a configured path is taken as it is";
    Vector<String> found = PythonCandidates("");
    if (!found.IsEmpty())
        EXPECT_TRUE(found[0].EndsWith("python3"));

#if defined(_WIN32)
    GTEST_SKIP() << "the agent runs on POSIX hosts";
#endif
    // stdin in, stdout lines out, stderr kept, the exit code
    String out;
    int code = ImageToUiTestKit::RunToEnd("/bin/sh", { "-c", "read line; echo \"got $line\"; echo second; echo oops >&2; exit 5" }, out, "hello\n");
    EXPECT_EQ(code, 5);
    EXPECT_EQ(out, "got hello\nsecond\n");

    AgentProcess sleeper;
    String reason;
    ASSERT_TRUE(sleeper.Start("/bin/sh", { "-c", "sleep 30" }, "/", reason)) << reason.Data();
    int exit = 0;
    EXPECT_FALSE(sleeper.Poll(exit));
    sleeper.Kill();
    EXPECT_TRUE(sleeper.Poll(exit));
    EXPECT_GE(exit, 128) << "ended by a signal";

    AgentProcess missing;
    EXPECT_FALSE(missing.Start("/no/such/binary", {}, "", reason));
    EXPECT_FALSE(reason.IsEmpty());
}
