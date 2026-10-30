"""agent.py end to end with a scripted "model" — the real upstream scripts
run every action. A tiny synthetic design is built from two sprites on a
backdrop, so a structure that places them exactly checks clean and the whole
skill can be driven to `complete: true`."""
from __future__ import annotations

import json
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image, ImageDraw

AGENT = Path(__file__).resolve().parents[1] / "agent.py"
SKILL = Path(__file__).resolve().parents[2] / "upstream" / "image-to-ui"
BRIDGE = "@@bridge "


def make_fixture(root: Path) -> tuple[Path, Path]:
    assets = root / "sprites"
    (assets / "panels").mkdir(parents=True)
    (assets / "icons").mkdir(parents=True)
    bg = Image.new("RGBA", (320, 200), (40, 60, 90, 255))
    d = ImageDraw.Draw(bg)
    for y in range(200):
        d.line([(0, y), (319, y)], fill=(40, 60 + y // 4, 90, 255))
    bg.save(assets / "panels" / "background.png")
    panel = Image.new("RGBA", (160, 80), (0, 0, 0, 0))
    ImageDraw.Draw(panel).rounded_rectangle([0, 0, 159, 79], radius=12, fill=(230, 200, 120, 255),
                                            outline=(120, 80, 30, 255), width=4)
    panel.save(assets / "panels" / "panel_card.png")
    icon = Image.new("RGBA", (40, 40), (0, 0, 0, 0))
    ImageDraw.Draw(icon).ellipse([2, 2, 37, 37], fill=(200, 40, 60, 255), outline=(255, 255, 255, 255), width=3)
    icon.save(assets / "icons" / "icon_gem.png")
    design = bg.copy()
    design.alpha_composite(panel, (80, 60))
    design.alpha_composite(icon, (140, 80))
    design_path = root / "design.png"
    design.convert("RGB").save(design_path)
    return design_path, assets


STRUCTURE = {
    "canvas": {"width": 320, "height": 200, "name": "Fixture"},
    "root": {
        "type": "container", "name": "root", "position": {"x": 0, "y": 0},
        "size": {"width": 320, "height": 200}, "anchor": {"horizontal": "left", "vertical": "top"},
        "children": [
            {"type": "image", "name": "background", "asset": "panels/background.png", "position": {"x": 0, "y": 0},
             "size": {"width": 320, "height": 200}, "anchor": {"horizontal": "center", "vertical": "middle"}},
            {"type": "container", "name": "card", "position": {"x": 80, "y": 60}, "size": {"width": 160, "height": 80},
             "anchor": {"horizontal": "center", "vertical": "middle"},
             "children": [
                 {"type": "image", "name": "frame", "asset": "panels/panel_card.png", "position": {"x": 0, "y": 0},
                  "size": {"width": 160, "height": 80}, "anchor": {"horizontal": "center", "vertical": "middle"}},
                 {"type": "image", "name": "gem", "asset": "icons/icon_gem.png", "position": {"x": 60, "y": 20},
                  "size": {"width": 40, "height": 40}, "anchor": {"horizontal": "center", "vertical": "middle"}},
             ]},
        ],
    },
    "metadata": {},
}


def action(obj: dict, content: str | None = None) -> str:
    text = "Next step.\n```action\n" + json.dumps(obj) + "\n```"
    if content is not None:
        text += "\n```content\n" + content + "\n```"
    return text


class ScriptedModel:
    """Plays the agent: write → check → review every legend path → finalize."""

    def __init__(self, task: Path) -> None:
        self.task = task
        self.prompts: list[str] = []
        self.images: list[list[str]] = []

    def __call__(self, prompt: str, images: list[str]) -> str:
        self.prompts.append(prompt)
        self.images.append(images)
        turn = len(self.prompts)
        if turn == 1:
            return action({"action": "view", "path": "@skill/references/alignment.md"})
        if turn == 2:
            return "garbage without an action"
        if turn == 3:
            # The action as plain text, not in a fence.
            return 'I read the grid metrics.\n\nAction: {"action": "view", "path": "design_grid_metrics.json"}'
        if turn == 4:
            return action({"action": "write", "path": "ui_structure.json"}, json.dumps(STRUCTURE, indent=2))
        if turn == 5:
            return action({"action": "patch", "ops": [{"op": "replace", "path": "/canvas/name", "value": "FixtureScreen"}]})
        if turn == 6:
            return action({"action": "check"})
        if turn == 7:
            binding = re.search(r"<!-- image-to-ui-review-binding:[^>]*-->", prompt).group(0)
            legend = json.loads((self.task / "all_elements_legend.json").read_text())
            risk = json.loads((self.task / "review_risk.json").read_text())
            high = {i["path"] for i in risk["items"] if i.get("risk_level") == "high"}
            rows = ["| Path | Status | Issue / change | Recheck |", "| --- | --- | --- | --- |"]
            for item in legend["targets"]:
                evidence = "`risk_review.png`" if item["path"] in high else "`all_elements.png`, `comparison.png`"
                rows.append(f"| `{item['path']}` | aligned | matches the design | {evidence} |")
            return action({"action": "write", "path": "alignment_review.md"}, binding + "\n\n" + "\n".join(rows) + "\n")
        if turn == 8:
            return action({"action": "finalize"})
        return action({"action": "stop", "reason": "script exhausted"})


def run_agent(root: Path, design: Path, assets: Path, model, extra: list[str] | None = None) -> tuple[int, dict, list[str]]:
    task = root / "task"
    proc = subprocess.Popen(
        [sys.executable, str(AGENT), "--design", str(design), "--assets", str(assets), "--output", str(task),
         "--trace-dir", str(root / "trace"), "--max-turns", "10", *(extra or [])],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
    result: dict = {}
    events: list[str] = []
    assert proc.stdout and proc.stdin
    for line in proc.stdout:
        if not line.startswith(BRIDGE):
            continue
        msg = json.loads(line[len(BRIDGE):])
        if msg["type"] == "model":
            for p in msg["images"]:
                assert Path(p).is_file(), p
            reply = {"id": msg["id"], "text": model(msg["prompt"], msg["images"])}
            proc.stdin.write(json.dumps(reply) + "\n")
            proc.stdin.flush()
        elif msg["type"] == "event":
            events.append(msg["message"])
        elif msg["type"] == "result":
            result = msg
    proc.stdin.close()
    proc.stdout.close()
    code = proc.wait(timeout=600)
    return code, result, events


class AgentRunTests(unittest.TestCase):
    def test_scripted_run_completes_the_skill(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            design, assets = make_fixture(root)
            model = ScriptedModel(root / "task")
            code, result, events = run_agent(root, design, assets, model, ["--font", "fonts/Roboto-Regular.ttf"])
            self.assertEqual(code, 0, result)
            self.assertTrue(result["ok"])
            self.assertTrue(result["summary"]["complete"], result["summary"])
            self.assertEqual(result["summary"]["turns"], 8)
            report = json.loads((root / "task" / "completion_report.json").read_text())
            self.assertTrue(report["complete"])
            self.assertEqual(report["blockers"], [])

            first = model.prompts[0]
            self.assertIn("# SKILL.md (image-to-ui)", first)
            self.assertIn("design_grid.png", first)
            self.assertIn("fonts/Roboto-Regular.ttf", first)
            for prompt in model.prompts:
                self.assertNotIn(str(root), prompt, "host paths must not reach the model")
            # SKILL.md's mandatory references are open from the start; one the
            # model opens stays open.
            self.assertIn("# references/schema.md (opened)", first)
            self.assertIn("# references/assets.md (opened)", first)
            self.assertNotIn("# references/alignment.md (opened)", first)
            self.assertIn("# references/schema.md (opened)", model.prompts[7])
            # A file it read stays in front of it (the plain-text action worked).
            self.assertIn("## design_grid_metrics.json (read on turn 3)", model.prompts[3])
            self.assertIn("## design_grid_metrics.json (read on turn 3)", model.prompts[7])
            self.assertIn("# references/alignment.md (opened)", model.prompts[1])
            # A reply without an action is answered, not fatal.
            self.assertIn("no usable action", model.prompts[2])
            # The written file is in front of the model, and the patch landed.
            self.assertIn('"FixtureScreen"', model.prompts[5])
            # The latest binding is repeated for the review.
            self.assertIn("# Latest review binding", model.prompts[6])
            self.assertNotIn("# Latest review binding", model.prompts[5])
            # After check: the evidence pictures are attached.
            self.assertGreaterEqual(len(model.images[6]), 3)
            self.assertIn("comparison.png", model.prompts[6])

            trace = json.loads((root / "trace" / "trace.json").read_text())
            kinds = [s["kind"] for s in trace["steps"]]
            self.assertEqual(kinds[0], "prepare")
            self.assertIn("check", kinds)
            self.assertIn("finalize", kinds)
            self.assertEqual(kinds[-1], "output")
            for step in trace["steps"]:
                for img in step["images"]:
                    self.assertTrue((root / "trace" / img["file"]).is_file(), img)
            check = next(s for s in trace["steps"] if s["kind"] == "check")
            self.assertEqual([i["title"] for i in check["images"]],
                             ["comparison.png", "all_elements.png", "risk_review.png", "reconstruction.png"])
            # The reconstruction is for the editors' divider, not for the model.
            self.assertEqual(len(model.images[6]), 4)  # design grid + the three check pictures
            self.assertTrue(events and events[0].startswith("turn 1/"))

    def test_a_round_of_edits_continues_the_task(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            design, assets = make_fixture(root)
            task = root / "task"
            code, result, _ = run_agent(root, design, assets, ScriptedModel(task))
            self.assertTrue(result["summary"]["complete"], result["summary"])
            base_steps = json.loads((root / "trace" / "trace.json").read_text())["steps"]
            base_files = {i["file"] for s in base_steps for i in s["images"]}

            request = {"round": 1, "items": [
                {"text": "call this gem ruby", "at": {"on": "result", "x": 160, "y": 100}},
                {"text": "keep the colours"},
            ]}
            (root / "request.json").write_text(json.dumps(request))
            prompts: list[str] = []
            images: list[list[str]] = []

            def reviser(prompt: str, imgs: list[str]) -> str:
                prompts.append(prompt)
                images.append(imgs)
                turn = len(prompts)
                if turn == 1:
                    return action({"action": "patch", "ops": [
                        {"op": "test", "path": "/root/children/1/children/1/name", "value": "gem"},
                        {"op": "replace", "path": "/root/children/1/children/1/name", "value": "ruby"}]})
                if turn == 2:
                    return action({"action": "check"})
                if turn == 3:
                    return action({"action": "edit", "path": "alignment_review.md",
                                   "old": "`root/card/gem`", "new": "`root/card/ruby`"})
                if turn == 4:
                    old = re.search(r"<!-- image-to-ui-review-binding:[^>]*-->", (task / "alignment_review.md").read_text()).group(0)
                    new = re.search(r"# Latest review binding[^\n]*\n\n(<!--[^>]*-->)", prompt).group(1)
                    return action({"action": "edit", "path": "alignment_review.md", "old": old, "new": new})
                if turn == 5:
                    return action({"action": "finalize"})
                return action({"action": "stop", "reason": "script exhausted"})

            # A task folder that lost its state: prepared afresh around what the model wrote.
            (task / "workflow_state.json").unlink()
            code, result, _ = run_agent(root, design, assets, reviser, ["--revise", str(root / "request.json")])
            self.assertEqual(code, 0, result)
            self.assertTrue(result["summary"]["complete"], result["summary"])
            self.assertEqual(result["summary"]["turns"], 5)
            structure = json.loads((task / "ui_structure.json").read_text())
            self.assertEqual(structure["root"]["children"][1]["children"][1]["name"], "ruby")
            self.assertIn("`root/card/ruby`", (task / "alignment_review.md").read_text())

            first = prompts[0]
            self.assertIn("# Changes the user asked for (round 1 of edits)", first)
            self.assertIn("At (160, 100) on the reconstruction: “call this gem ruby”", first)
            self.assertIn("Under the point, innermost first: `root/card/gem`", first)
            self.assertIn("(no point) “keep the colours”", first)
            self.assertIn("checked again before the changes", first)
            # The request stays in the task; its pictures come with the first turn.
            self.assertIn("# Changes the user asked for", prompts[4])
            self.assertEqual(len(images[0]), 5)  # grid, design + reconstruction with the point, the point enlarged, comparison
            self.assertIn("@request/point_1.png", first)
            for prompt in prompts:
                self.assertNotIn(str(root), prompt, "host paths must not reach the model")
            # It appended to the run's trace under its own ids and file names,
            # without touching the pictures the run left there.
            steps = json.loads((root / "trace" / "trace.json").read_text())["steps"]
            self.assertEqual([s["id"] for s in steps[:len(base_steps)]], [s["id"] for s in base_steps])
            added = steps[len(base_steps):]
            # The folder had lost its state: prepared again first, then the round as usual.
            self.assertEqual([s["id"] for s in added[:4]], ["edit1-prepare", "edit1-check", "edit1-request", "edit1-turn-1"])
            self.assertTrue(all(s["title"].startswith("Edit 1 · ") for s in added))
            self.assertEqual(added[2]["kind"], "request")
            self.assertEqual(len(added[2]["images"]), 3)
            for s in added:
                for img in s["images"]:
                    self.assertTrue(img["file"].startswith("edit1-"), img)
                    self.assertNotIn(img["file"], base_files)
            for s in steps:
                for img in s["images"]:
                    self.assertTrue((root / "trace" / img["file"]).is_file(), img)

    def test_edit_needs_an_exact_unique_match(self) -> None:
        sys.path.insert(0, str(AGENT.parent))
        from agent import Agent  # noqa: E402
        with tempfile.TemporaryDirectory() as tmp:
            agent = Agent.__new__(Agent)
            agent.task = Path(tmp)
            (agent.task / "alignment_review.md").write_text("| `a` | aligned |\n| `b` | aligned |\n")
            self.assertEqual(agent.do_edit({"path": "alignment_review.md", "old": "aligned", "new": "adjusted"}).status, "warn")
            self.assertEqual(agent.do_edit({"path": "alignment_review.md", "old": "`c`", "new": "`d`"}).status, "warn")
            self.assertEqual(agent.do_edit({"path": "notes.txt", "old": "a", "new": "b"}).status, "warn")
            self.assertEqual(agent.do_edit({"path": "alignment_review.md", "old": "`a` | aligned", "new": "`a` | adjusted"}).status, "ok")
            self.assertEqual(agent.do_edit({"path": "alignment_review.md", "old": "|", "new": "¦", "all": True}).text,
                             "Replaced 6 occurrence(s) in alignment_review.md.")

    def test_patch_operations(self) -> None:
        sys.path.insert(0, str(AGENT.parent))
        from agent import apply_patch, parse_reply  # noqa: E402
        doc = {"a": [1, 2, 3], "b": {"c": 1}}
        out = apply_patch(doc, [{"op": "add", "path": "/a/-", "value": 4}, {"op": "remove", "path": "/a/0"},
                                {"op": "replace", "path": "/b/c", "value": 2}, {"op": "move", "from": "/b", "path": "/d"},
                                {"op": "copy", "from": "/d", "path": "/e"}, {"op": "test", "path": "/e/c", "value": 2}])
        self.assertEqual(out, {"a": [2, 3, 4], "d": {"c": 2}, "e": {"c": 2}})
        self.assertEqual(doc["a"], [1, 2, 3])
        with self.assertRaises(ValueError):
            apply_patch(doc, [{"op": "replace", "path": "/missing", "value": 1}])
        a, c, e = parse_reply('note\n```action\n{"action": "write", "path": "alignment_review.md"}\n```\n'
                              '```content\n| `root` | aligned | x | `a.png` |\n```')
        self.assertEqual((a["action"], c.strip(), e), ("write", "| `root` | aligned | x | `a.png` |", None))
        a, c, e = parse_reply('Plan.\nAction: {"action": "target", "paths": ["root/a"]}')
        self.assertEqual(a, {"action": "target", "paths": ["root/a"]})
        a, c, e = parse_reply('```json\n{"action": "check"}\n```')
        self.assertEqual(a, {"action": "check"})
        self.assertIsNone(parse_reply("nothing")[0])

    def test_tall_sheets_come_in_parts(self) -> None:
        sys.path.insert(0, str(AGENT.parent))
        from agent import page_count, page_span  # noqa: E402
        self.assertEqual(page_count((1672, 941)), 1)       # a comparison
        self.assertEqual(page_count((1080, 2300)), 1)       # a short risk review
        self.assertEqual(page_count((1080, 23200)), 14)     # 86 risky rows
        spans = [page_span((1080, 23200), k) for k in range(1, 15)]
        self.assertEqual(spans[0], (0, 1728))
        self.assertEqual(spans[-1][1], 23200)
        self.assertTrue(all(a[1] == b[0] for a, b in zip(spans, spans[1:])))

    def test_missing_skill_is_a_bad_argument(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            design, assets = make_fixture(root)
            code, result, _ = run_agent(root, design, assets, lambda p, i: "", ["--skill", str(root / "nope")])
            self.assertEqual(code, 4)
            self.assertEqual(result["error"], "bad_arguments")


if __name__ == "__main__":
    unittest.main()
