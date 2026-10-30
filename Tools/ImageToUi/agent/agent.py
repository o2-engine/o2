#!/usr/bin/env python3
"""The agent side of the Image-To-UI skill, for AssetsLine's editors.

The upstream skill (../upstream/image-to-ui, XuToWei/Image-To-UI, unmodified)
is written for a coding agent with a shell: the agent reads SKILL.md, runs
`workflow.py prepare / measure / check / target / preview / finalize`, opens
the images and reports those commands write, writes ui_structure.json and
alignment_review.md, and is done when finalize writes completion_report.json
with "complete": true.

This program is that agent's environment for a vision model. Every turn the
model gets the skill's instructions (SKILL.md, plus every reference file it
has opened — they stay open, as in an agent's context), the task, the current
files it wrote, what it did so far, the result of its last action and the
newest pictures, and answers with ONE action. The action runs with the skill's
own scripts, unchanged; their result is the next turn's observation.

The model is called by the host editor (the AssetsLine server, the o2 editor,
the Unity plugin) over a line protocol, so keys, routing, billing and the
memo of paid answers stay the host's:

  agent → host (stdout):  @@bridge {"type": "model", "id": 3, "tag": "turn-3",
                                     "prompt": "...", "images": ["/abs/a.jpg", ...]}
  host → agent (stdin):   {"id": 3, "text": "..."}   or   {"id": 3, "error": "..."}
  agent → host (stdout):  @@bridge {"type": "event", "message": "..."}     (no reply)
                          @@bridge {"type": "result", ...}                 (last line)

Everything else the agent prints goes to stderr. The debug trace is written
into --trace-dir in the editors' TraceFile format (trace.json + pictures),
appending to steps the host already wrote there (the sprite cuts).

Exit codes: 0 — a checked structure exists (complete or not, see the result),
2 — no usable structure, 3 — Python dependencies missing, 4 — bad arguments.
"""
from __future__ import annotations

import argparse
import copy
import json
import os
import re
import shutil
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

HERE = Path(__file__).resolve().parent
DEFAULT_SKILL = HERE.parent / "upstream" / "image-to-ui"
BRIDGE = "@@bridge "

WRITABLE = ("ui_structure.json", "alignment_review.md", "component_review.md")
TEXT_VIEW_LIMIT = 60_000
READ_BUDGET = 150_000
PAGES_SHOWN = 4


def page_count(size: tuple[int, int]) -> int:
    w, h = size
    if h <= w * 2.2:
        return 1
    return -(-h // round(w * 1.6))


def page_span(size: tuple[int, int], part: int) -> tuple[int, int]:
    w, h = size
    page = round(w * 1.6)
    top = min((part - 1) * page, max(0, h - 1))
    return top, min(h, top + page)
SUMMARY_LIMIT = 700


def log(*parts: Any) -> None:
    print(*parts, file=sys.stderr, flush=True)


def utc_now() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


# --- bridge -------------------------------------------------------------------


class Bridge:
    """Model calls go to the host; its answer comes back on stdin."""

    def __init__(self) -> None:
        self.n = 0

    def send(self, payload: dict[str, Any]) -> None:
        sys.stdout.write(BRIDGE + json.dumps(payload, ensure_ascii=False) + "\n")
        sys.stdout.flush()

    def event(self, message: str) -> None:
        self.send({"type": "event", "message": message})

    def model(self, prompt: str, images: list[Path], tag: str) -> str:
        self.n += 1
        self.send({"type": "model", "id": self.n, "tag": tag, "prompt": prompt,
                   "images": [str(p) for p in images]})
        while True:
            line = sys.stdin.readline()
            if not line:
                raise RuntimeError("the host closed the bridge")
            line = line.strip()
            if not line:
                continue
            reply = json.loads(line)
            if reply.get("id") != self.n:
                continue
            if "error" in reply:
                raise RuntimeError(f"model call failed: {reply['error']}")
            return str(reply.get("text", ""))


# --- trace --------------------------------------------------------------------


class Trace:
    """trace.json in the editors' format; pictures are copied next to it, so
    every check's evidence stays visible after the next check replaces it.
    A round of edits appends to the trace of the run it continues: `prefix`
    keeps its step ids and picture names apart ("edit2-turn-3",
    "edit2-a001-…"), `title_prefix` labels its steps ("Edit 2 · …")."""

    def __init__(self, directory: Path | None, prefix: str = "", title_prefix: str = "") -> None:
        self.dir = directory
        self.prefix = prefix
        self.title_prefix = title_prefix
        self.n = 0
        self.data: dict[str, Any] = {"version": 1, "startedAt": utc_now(), "steps": []}
        if not directory:
            return
        directory.mkdir(parents=True, exist_ok=True)
        existing = directory / "trace.json"
        if existing.is_file():
            try:
                self.data = json.loads(existing.read_text("utf-8"))
            except (OSError, ValueError):
                pass

    def image(self, title: str, path: Path) -> dict[str, Any] | None:
        if not path.is_file():
            return None
        size = image_size(path)
        if not self.dir or not size:
            return None
        self.n += 1
        name = f"{self.prefix}a{self.n:03d}-{re.sub(r'[^A-Za-z0-9._-]+', '_', path.name)}"
        shutil.copyfile(path, self.dir / name)
        return {"title": title, "file": name, "w": size[0], "h": size[1]}

    def step(self, **step: Any) -> None:
        if self.prefix:
            step["id"] = self.prefix + str(step.get("id", ""))
        if self.title_prefix:
            step["title"] = self.title_prefix + str(step.get("title", ""))
        step.setdefault("images", [])
        step["images"] = [i for i in step["images"] if i]
        self.data["steps"].append(step)
        self.write()

    def write(self) -> None:
        if not self.dir:
            return
        tmp = self.dir / f"trace.json.{os.getpid()}.tmp"
        tmp.write_text(json.dumps(self.data, ensure_ascii=False, indent=1), "utf-8")
        os.replace(tmp, self.dir / "trace.json")


def image_size(path: Path) -> tuple[int, int] | None:
    try:
        from PIL import Image
        with Image.open(path) as img:
            return img.size
    except Exception:
        return None


# --- the skill's commands ----------------------------------------------------


class Workflow:
    def __init__(self, skill: Path, task: Path, design: Path, assets: Path) -> None:
        self.skill = skill
        self.task = task
        self.design = design
        self.assets = assets

    def run(self, script: str, *args: str, timeout: int = 900) -> tuple[int, str]:
        cmd = [sys.executable, "-B", str(self.skill / "scripts" / script), *args]
        try:
            done = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout,
                                  cwd=str(self.task.parent))
        except subprocess.TimeoutExpired:
            return 124, f"{script} timed out after {timeout} s"
        return done.returncode, self.clean((done.stdout or "") + (done.stderr or ""))

    def workflow(self, command: str, *args: str) -> tuple[int, str]:
        return self.run("workflow.py", command, "--output", str(self.task), *args)

    def clean(self, text: str) -> str:
        """Absolute host paths would make every prompt machine-specific (and
        the host's memo of paid answers useless); the model reads names."""
        for path, name in ((self.task, "<task-dir>"), (self.assets, "<assets-dir>"),
                           (self.skill, "<skill>"), (self.design, "<design>")):
            text = text.replace(str(path), name)
        return text


# --- observations --------------------------------------------------------------


class Observation:
    def __init__(self, title: str, text: str, images: list[tuple[str, Path]] | None = None,
                 kind: str = "agent", status: str = "ok", completed: bool = False,
                 trace_only: list[tuple[str, Path]] | None = None) -> None:
        self.title = title
        self.text = text
        self.images = images or []
        # Kept in the debug trace, not shown to the model (the editors put
        # design | reconstruction under one divider after every check).
        self.trace_only = trace_only or []
        self.kind = kind
        self.status = status
        self.completed = completed


def read_json(path: Path) -> dict[str, Any] | None:
    try:
        return json.loads(path.read_text("utf-8"))
    except (OSError, ValueError):
        return None


def tail(text: str, lines: int = 60) -> str:
    rows = [r for r in text.splitlines() if r.strip()]
    return "\n".join(rows[-lines:])


def issues_text(report: dict[str, Any] | None, limit: int = 60) -> str:
    if not report:
        return ""
    out: list[str] = []
    for key in ("errors", "warnings", "issues"):
        for issue in (report.get(key) or [])[:limit]:
            if isinstance(issue, dict):
                parts = [str(issue.get(k)) for k in ("severity", "code", "path") if issue.get(k)]
                out.append(f"- {key[:-1] if key != 'issues' else ''} {' '.join(parts)}: {issue.get('message', '')}".replace("  ", " "))
            else:
                out.append(f"- {key[:-1]}: {issue}")
    return "\n".join(out)


def check_summary(task: Path) -> str:
    parts: list[str] = []
    validation = read_json(task / "validate_report.json")
    if validation:
        parts.append(f"validate_report.json: valid={validation.get('valid')} errors={validation.get('error_count')} "
                     f"warnings={validation.get('warning_count')} stats={json.dumps(validation.get('stats', {}))}")
        listed = issues_text(validation)
        if listed:
            parts.append(listed)
    audit = read_json(task / "visual_audit.json")
    if audit:
        parts.append(f"visual_audit.json: valid={audit.get('valid')} errors={audit.get('error_count')} "
                     f"warnings={audit.get('warning_count')}")
        listed = issues_text(audit)
        if listed:
            parts.append(listed)
    risk = read_json(task / "review_risk.json")
    if risk:
        summary = risk.get("summary", {})
        parts.append(f"review_risk.json: {json.dumps(summary.get('risk_level_counts', {}))}, "
                     f"thresholds {json.dumps(risk.get('thresholds', {}))}")
        rows = [i for i in risk.get("items", []) if i.get("risk_level") in ("high", "medium")]
        for item in rows[:40]:
            parts.append(f"- {item.get('risk_level')} {item.get('risk_score')} `{item.get('path')}` "
                         f"bbox={item.get('bbox')} reasons={','.join(item.get('reasons', []))}"
                         f"{' (in risk_review.png)' if item.get('risk_review_covered') else ''}")
        if len(rows) > 40:
            parts.append(f"... {len(rows) - 40} more medium/high rows in review_risk.json")
    legend = read_json(task / "all_elements_legend.json")
    if legend:
        parts.append(f"all_elements_legend.json: {legend.get('target_count')} element paths "
                     "(every one needs a row in alignment_review.md)")
    return "\n".join(parts)


# --- the agent -----------------------------------------------------------------


PROTOCOL = """\
There is no shell here. Instead of running a command, end every reply with ONE
fenced block tagged `action` that holds a JSON object. The environment carries
it out with the skill's own scripts, unchanged, and shows you the result in
your next turn. One action per reply; keep the note before it short.

Command → action:
- `workflow.py prepare --design … --assets … --output …` → already done for you
  (its result is your first observation). {"action": "prepare"} runs it again.
- `workflow.py measure --output <task-dir> --region X Y W H --zoom Z --cell-size C`
  → {"action": "measure", "region": [X, Y, W, H], "zoom": 3, "cellSize": 10}
- `workflow.py check --output <task-dir> [--transparent-bg] [--allow-warnings]`
  → {"action": "check", "transparentBg": false, "allowWarnings": false}
- `workflow.py target --output <task-dir> --element-path P … [--transparent-bg] [--allow-warnings]`
  → {"action": "target", "paths": ["root/popup/close", "root/popup/rewards"]}
- `workflow.py preview --output <task-dir> --name N [--size W H] [--state PATH=VARIANT]
  [--progress PATH=VALUE] [--scroll PATH=X,Y]`
  → {"action": "preview", "name": "N", "size": [W, H], "state": ["PATH=VARIANT"],
     "progress": ["PATH=0.5"], "scroll": ["PATH=0,120"]}  (only "name" is required)
- `workflow.py finalize --output <task-dir>` → {"action": "finalize"}
- `backfill_anchors.py --structure <task-dir>/ui_structure.json --output <task-dir>/ui_structure.json [--overwrite]`
  → {"action": "backfill_anchors", "overwrite": false}
- `scale_structure.py` is not needed: there is no older draft.
- Opening a file → {"action": "view", "path": "<path>"}. Paths are relative to
  the task folder ("risk_review.png", "assets/assets_inventory.json",
  "measurements/region_10_20_200_100/design_grid.png"); "@assets/<path>" is a
  file of the sliced-asset directory (paths as in assets_inventory.json);
  "@skill/<path>" is a file of the skill ("@skill/references/schema.md");
  "@design" is the design at native resolution. A tall sheet comes in parts;
  {"action": "view", "path": "risk_review.png", "part": 5} shows one. An image is shown to you; a
  text file is shown in full (cut at 60 000 characters). A reference file you
  open stays in front of you for the rest of the task: where SKILL.md says
  "Read references/<name>.md", open "@skill/references/<name>.md"
  (schema.md and assets.md are open already).
- Writing a file → {"action": "write", "path": "ui_structure.json"}
  (or "alignment_review.md", "component_review.md"), followed by a SECOND
  fenced block tagged `content` holding the complete new text of the file.
- Editing ui_structure.json in place → {"action": "patch", "ops": [ … ]} with
  RFC 6902 JSON Patch operations (add / remove / replace / move / copy / test;
  paths are JSON Pointers such as "/root/children/2/position/x"). Use it for
  small edits; write the whole file for large ones.
- Replacing a piece of a file you wrote → {"action": "edit", "path": "alignment_review.md",
  "old": "<exact text>", "new": "<replacement>"} — "old" must be in the file
  exactly once (add "all": true to replace every occurrence). Use it for the
  binding line or a few review rows instead of writing the whole file again.
- Giving up → {"action": "stop", "reason": "…"}, only when nothing more can be done.

What finalize (workflow.py) parses: in alignment_review.md the element path and
every cited PNG must be in backticks — | `root/popup/close` | aligned | … |
`risk_review.png` | — a name without backticks counts as no evidence; and the
binding comment must be the one printed by your LATEST check or target (it is
repeated for you under "Latest review binding"). Any file of the skill can be
read with view "@skill/…" (its scripts too) when a message is unclear.

Every action costs one turn. The task is done when finalize writes
completion_report.json with "complete": true."""


def task_text(args: argparse.Namespace, default_prompt: str) -> str:
    lines = [
        default_prompt or "Use $image-to-ui to reconstruct this design as structured UI JSON.",
        "",
        "Inputs (already prepared):",
        f"- design: `{args.design.name}` (\"@design\"), {args.design_w}×{args.design_h} px",
        f"- sliced assets: the `<assets-dir>` ({args.asset_count} PNG files), inventoried into `assets/`",
        "- task folder: `<task-dir>` (all paths below are relative to it)",
    ]
    if args.sprites_note:
        lines += ["", args.sprites_note]
    if args.fonts:
        lines += ["", "Fonts available for text (use these exact `fontFamily` values; any other font is a "
                  "font_fallback error): " + ", ".join(f"`{f}`" for f in args.fonts)]
    if args.name:
        lines += ["", f"Use \"{args.name}\" as the canvas name."]
    if args.hint:
        lines += ["", "Notes from the user:", args.hint]
    return "\n".join(lines)


# --- a round of edits ------------------------------------------------------------
# After the task was done the user can ask for changes: notes, each with a
# point on the design or on the reconstruction (canvas pixels, the two have the
# same size) or with none. The host passes them as --revise <request.json>:
# {"round": 2, "items": [{"text": "…", "at": {"on": "result", "x": 120, "y": 40}}],
#  "earlier": [{"round": 1, "items": […]}]}. The agent then continues in the same
# task folder: it checks the layout as it is, shows the model the request with
# the points drawn on both pictures and enlarged, and runs the usual loop until
# finalize passes again.

PICTURE = {"design": "the design", "result": "the reconstruction"}


def elements_at(legend: dict[str, Any] | None, x: int, y: int, limit: int = 3) -> list[str]:
    """The innermost elements under a point, from all_elements_legend.json
    (abs_bbox is [x, y, w, h])."""
    hits: list[tuple[int, str, list[int]]] = []
    for target in (legend or {}).get("targets") or []:
        box = target.get("abs_bbox") or []
        if len(box) != 4:
            continue
        bx, by, bw, bh = (int(v) for v in box)
        if bx <= x < bx + bw and by <= y < by + bh:
            hits.append((bw * bh, str(target.get("path")), [bx, by, bw, bh]))
    hits.sort(key=lambda h: h[0])
    return [f"`{path}` (x {b[0]}, y {b[1]}, w {b[2]}, h {b[3]})" for _, path, b in hits[:limit]]


def note_lines(items: list[dict[str, Any]], legend: dict[str, Any] | None = None) -> list[str]:
    lines: list[str] = []
    for i, item in enumerate(items, start=1):
        at = item.get("at")
        if at:
            lines.append(f"{i}. At ({at['x']}, {at['y']}) on {PICTURE.get(at.get('on'), 'the reconstruction')}: “{item['text']}”")
            under = elements_at(legend, int(at["x"]), int(at["y"])) if legend else []
            if under:
                lines.append("   Under the point, innermost first: " + "; ".join(under))
        else:
            lines.append(f"{i}. (no point) “{item['text']}”")
    return lines


def request_text(request: dict[str, Any], legend: dict[str, Any] | None, pictures: list[str]) -> str:
    n = int(request.get("round") or 1)
    lines = [
        f"# Changes the user asked for (round {n} of edits)",
        "",
        "You completed this task before: `ui_structure.json` and `alignment_review.md` in the task folder "
        "are as you left them, and the layout has just been checked again as it is. The user has looked at "
        "the result and asks for the changes below. A point is in canvas pixels (the design and the "
        "reconstruction have the same size, origin top left); each note says which picture the user "
        "pointed at.",
        "",
        *note_lines(request.get("items") or [], legend),
    ]
    if pictures:
        lines += ["", "Pictures of the request (view them again any time): " + ", ".join(f"`{p}`" for p in pictures)
                  + " — the design and the reconstruction with the points numbered, then both around each point, enlarged."]
    lines += [
        "",
        "What to do: make the changes the notes ask for and what they imply, and keep everything else as it is. "
        "Measure first where a position or a size is involved. Where a note asks for something the design "
        "does not show (another colour, a bigger button), do what the note says and record it in "
        "`metadata.approximations` ({\"path\": …, \"reason\": \"requested by the user: …\", \"accepted\": true}) "
        "so the checks accept the difference. Then run check, bring `alignment_review.md` up to date — the rows "
        "of the elements you changed, and the binding of your latest check (the `edit` action replaces a line) — "
        "and finalize. The round is done when finalize reports \"complete\": true again.",
    ]
    earlier = [r for r in request.get("earlier") or [] if r.get("items")]
    if earlier:
        lines += ["", "Changes made in earlier rounds (they are in the layout already; keep them):"]
        for r in earlier:
            lines.append(f"Round {r.get('round')}:")
            lines += ["  " + line for line in note_lines(r["items"])]
    return "\n".join(lines)


def request_pictures(design: Path, reconstruction: Path | None, items: list[dict[str, Any]], out: Path) -> list[tuple[str, Path]]:
    """The design and the reconstruction with the points drawn and numbered,
    and one sheet per point: both pictures around it, enlarged."""
    from PIL import Image, ImageDraw, ImageFont
    out.mkdir(parents=True, exist_ok=True)
    base = Image.open(design).convert("RGB")
    w, h = base.size
    recon = None
    if reconstruction and reconstruction.is_file():
        with Image.open(reconstruction) as img:
            layer = img.convert("RGBA").resize((w, h)) if img.size != (w, h) else img.convert("RGBA")
        recon = Image.new("RGBA", (w, h), (255, 255, 255, 255))
        recon.alpha_composite(layer)
        recon = recon.convert("RGB")
    pins = [(i, item["at"]) for i, item in enumerate(items, start=1) if item.get("at")]
    radius = max(9, round(max(w, h) / 70))

    def font(size: int):
        try:
            return ImageFont.load_default(size=size)
        except TypeError:  # Pillow < 10.1
            return ImageFont.load_default()

    def marked(img: Image.Image, r: int) -> Image.Image:
        img = img.copy()
        d = ImageDraw.Draw(img)
        f = font(max(10, round(r * 1.25)))
        for i, at in pins:
            x, y = int(at["x"]), int(at["y"])
            d.ellipse([x - r, y - r, x + r, y + r], fill=(229, 57, 53), outline=(255, 255, 255), width=max(2, r // 4))
            d.text((x, y), str(i), fill=(255, 255, 255), font=f, anchor="mm")
        return img

    shown: list[tuple[str, Path]] = []
    marked(base, radius).save(out / "design.png")
    shown.append(("@request/design.png — the design, the points numbered", out / "design.png"))
    if recon is not None:
        marked(recon, radius).save(out / "reconstruction.png")
        shown.append(("@request/reconstruction.png — the reconstruction, the points numbered", out / "reconstruction.png"))
    side = max(80, min(360, round(max(w, h) * 0.16)))
    for i, at in pins:
        x, y = int(at["x"]), int(at["y"])
        x0 = max(0, min(w - side, x - side // 2)) if w > side else 0
        y0 = max(0, min(h - side, y - side // 2)) if h > side else 0
        box = (x0, y0, min(w, x0 + side), min(h, y0 + side))
        zoom = max(1.0, min(4.0, 420 / side))
        parts = [("design", base)] + ([("reconstruction", recon)] if recon is not None else [])
        tiles = []
        for title, img in parts:
            tile = img.crop(box).resize((round((box[2] - box[0]) * zoom), round((box[3] - box[1]) * zoom)), Image.LANCZOS)
            d = ImageDraw.Draw(tile)
            px, py = (x - box[0]) * zoom, (y - box[1]) * zoom
            # Four ticks pointing at the spot, clear of it: what is there stays visible.
            near, far = 14 * zoom, 14 * zoom + 14
            for c, wd in (((255, 255, 255), 5), ((229, 57, 53), 2)):
                d.line([(px - far, py), (px - near, py)], fill=c, width=wd)
                d.line([(px + near, py), (px + far, py)], fill=c, width=wd)
                d.line([(px, py - far), (px, py - near)], fill=c, width=wd)
                d.line([(px, py + near), (px, py + far)], fill=c, width=wd)
            tiles.append((title, tile))
        gap, cap = 12, 22
        sheet = Image.new("RGB", (sum(t.width for _, t in tiles) + gap * (len(tiles) - 1), max(t.height for _, t in tiles) + cap),
                          (30, 30, 40))
        d = ImageDraw.Draw(sheet)
        left = 0
        for title, tile in tiles:
            sheet.paste(tile, (left, cap))
            d.text((left + 4, 4), f"{i} · {title} · ({x}, {y})", fill=(235, 235, 240), font=font(13))
            left += tile.width + gap
        sheet.save(out / f"point_{i}.png")
        shown.append((f"@request/point_{i}.png — point {i}, design | reconstruction, enlarged", out / f"point_{i}.png"))
    return shown


def parse_reply(text: str) -> tuple[dict[str, Any] | None, str | None, str | None]:
    """→ (action, content, error)."""
    # A fence opens at a line start; the content block may itself hold ``` runs
    # only inside a line, never at its start, so the closing fence is "\n```".
    blocks = re.findall(r"(?:^|\n)```([A-Za-z0-9_-]*)[^\n]*\n(.*?)\n```", text, flags=re.S)
    tags = [tag.lower() for tag, _ in blocks]
    at = tags.index("action") if "action" in tags else None
    if at is None:
        for i, (tag, body) in enumerate(blocks):
            if tags[i] in ("json", ""):
                try:
                    obj = json.loads(body)
                except ValueError:
                    continue
                if isinstance(obj, dict) and "action" in obj:
                    at = i
                    break
    if at is None:
        # A model that writes the action as plain text ("Action: {…}").
        loose = loose_action(text)
        if loose is None:
            return None, None, "no ```action block"
        content = blocks[-1][1] if loose.get("action") == "write" and blocks else None
        return loose, content, None
    try:
        action = json.loads(blocks[at][1])
    except ValueError as exc:
        return None, None, f"the action block is not valid JSON ({exc})"
    if not isinstance(action, dict) or not isinstance(action.get("action"), str):
        return None, None, 'the action block must be a JSON object with an "action" field'
    content = None
    if "content" in tags:
        content = blocks[tags.index("content")][1]
    elif action["action"] == "write":
        # A model that labels the file block by its language instead.
        later = [body for i, (_, body) in enumerate(blocks) if i != at and tags[i] in ("json", "markdown", "md", "")]
        if later:
            content = later[-1]
    return action, content, None


def loose_action(text: str) -> dict[str, Any] | None:
    """The last JSON object outside fences that has an "action" field."""
    bare = re.sub(r"(?:^|\n)```.*?\n```", "\n", text, flags=re.S)
    found = None
    for start in [m.start() for m in re.finditer(r"\{", bare)]:
        depth = 0
        for end in range(start, len(bare)):
            if bare[end] == "{":
                depth += 1
            elif bare[end] == "}":
                depth -= 1
                if depth == 0:
                    try:
                        obj = json.loads(bare[start:end + 1])
                    except ValueError:
                        break
                    if isinstance(obj, dict) and isinstance(obj.get("action"), str):
                        found = obj
                    break
    return found


def json_pointer_parts(pointer: str) -> list[str]:
    if pointer == "":
        return []
    if not pointer.startswith("/"):
        raise ValueError(f"bad JSON Pointer {pointer!r}")
    return [p.replace("~1", "/").replace("~0", "~") for p in pointer[1:].split("/")]


def apply_patch(doc: Any, ops: list[dict[str, Any]]) -> Any:
    doc = copy.deepcopy(doc)

    def walk(parts: list[str]) -> tuple[Any, str]:
        parent = doc
        for part in parts[:-1]:
            parent = parent[int(part)] if isinstance(parent, list) else parent[part]
        return parent, parts[-1]

    def get(pointer: str) -> Any:
        node = doc
        for part in json_pointer_parts(pointer):
            node = node[int(part)] if isinstance(node, list) else node[part]
        return node

    for op in ops:
        kind, path = op.get("op"), op.get("path", "")
        parts = json_pointer_parts(path)
        if kind == "test":
            if get(path) != op.get("value"):
                raise ValueError(f"test failed at {path}")
            continue
        if kind in ("move", "copy"):
            value = copy.deepcopy(get(op["from"]))
            if kind == "move":
                apply_remove(doc, json_pointer_parts(op["from"]))
            kind, op = "add", {"op": "add", "path": path, "value": value}
        if not parts:
            if kind in ("add", "replace"):
                doc = copy.deepcopy(op.get("value"))
                continue
            raise ValueError("cannot remove the whole document")
        parent, key = walk(parts)
        if kind == "add":
            if isinstance(parent, list):
                parent.insert(len(parent) if key == "-" else int(key), copy.deepcopy(op.get("value")))
            else:
                parent[key] = copy.deepcopy(op.get("value"))
        elif kind == "replace":
            if isinstance(parent, list):
                parent[int(key)] = copy.deepcopy(op.get("value"))
            else:
                if key not in parent:
                    raise ValueError(f"replace: {path} does not exist")
                parent[key] = copy.deepcopy(op.get("value"))
        elif kind == "remove":
            apply_remove(doc, parts)
        else:
            raise ValueError(f"unknown op {kind!r}")
    return doc


def apply_remove(doc: Any, parts: list[str]) -> None:
    parent = doc
    for part in parts[:-1]:
        parent = parent[int(part)] if isinstance(parent, list) else parent[part]
    key = parts[-1]
    if isinstance(parent, list):
        del parent[int(key)]
    else:
        del parent[key]


class Agent:
    def __init__(self, args: argparse.Namespace, bridge: Bridge, trace: Trace) -> None:
        self.args = args
        self.bridge = bridge
        self.trace = trace
        self.skill: Path = args.skill
        self.task: Path = args.output
        self.wf = Workflow(args.skill, args.output, args.design, args.assets)
        self.view_dir: Path = args.scratch
        self.view_dir.mkdir(parents=True, exist_ok=True)
        self.skill_md = (self.skill / "SKILL.md").read_text("utf-8")
        yaml_path = self.skill / "agents" / "openai.yaml"
        default_prompt = ""
        if yaml_path.is_file():
            m = re.search(r'default_prompt:\s*"(.*)"', yaml_path.read_text("utf-8"))
            default_prompt = m.group(1) if m else ""
        self.task_md = task_text(args, default_prompt)
        # Reference files that stay open. SKILL.md makes two of them part of
        # steps 1–2 for every task ("Read references/assets.md when selecting
        # sprites", "Read references/schema.md for fields and examples"), so
        # they are open from the start, as a diligent agent would have them;
        # the others the model opens when SKILL.md sends it there.
        self.opened: dict[str, str] = {}
        for name in ("references/schema.md", "references/assets.md"):
            path = self.skill / name
            if path.is_file():
                self.opened[name] = path.read_text("utf-8")
        self.history: list[str] = []              # one compact line block per turn
        # Text files the model has read, latest reading of each, as an agent
        # keeps what it read in its context (a report it re-reads replaces
        # the earlier one). Large ones are cut; the oldest drop out first.
        self.read: dict[str, tuple[int, str]] = {}
        self.turn = 0
        self.binding = ""                         # the Review binding of the latest check / target
        self.last_finalize = ""                   # what the latest finalize said, while it is current
        self.recent_images: list[tuple[str, Path]] = []
        self.checks = 0
        self.completed = False
        self.blockers: list[str] = []
        self.error = ""
        self.n_view = 0
        # A round of edits (--revise): the user's notes, and where their pictures go.
        self.request: dict[str, Any] | None = getattr(args, "request", None)
        self.request_dir = self.view_dir / "request"

    # -- model view of a picture: big evidence sheets are scaled to fit what a
    # -- model looks at (the files themselves stay native in the task folder).
    def model_image(self, path: Path, part: int | None = None) -> Path:
        from PIL import Image
        limit = self.args.max_image_side
        self.n_view += 1
        out = self.view_dir / f"v{self.n_view:03d}-{path.stem}{f'-p{part}' if part else ''}.jpg"
        with Image.open(path) as img:
            if part:
                top, bottom = page_span(img.size, part)
                img = img.crop((0, top, img.width, bottom))
            img = img.convert("RGBA")
            bg = Image.new("RGBA", img.size, (30, 30, 40, 255))
            bg.alpha_composite(img)
            img = bg.convert("RGB")
            scale = min(1.0, limit / max(img.size))
            if scale < 1.0:
                img = img.resize((max(1, round(img.width * scale)), max(1, round(img.height * scale))), Image.LANCZOS)
            img.save(out, "JPEG", quality=90)
        return out

    # -- one prompt -----------------------------------------------------------
    def compose(self, turn: int, obs: Observation) -> tuple[str, list[Path], list[str]]:
        sections = [
            "You are the coding agent of this task, running the `image-to-ui` skill below. "
            "Follow SKILL.md exactly, as you would with a shell; only the way you run "
            "commands and edit files differs (see \"How to act here\").",
            "# SKILL.md (image-to-ui)\n\n" + self.skill_md,
        ]
        for name, text in self.opened.items():
            sections.append(f"# {name} (opened)\n\n{text}")
        sections.append("# The task\n\n" + self.task_md)
        if self.read:
            sections.append("# Files you have read (the latest reading of each; re-open a file to see it as it is now)\n\n"
                            + "\n\n".join(f"## {name} (read on turn {turn})\n```\n{text}\n```" for name, (turn, text) in self.read.items()))
        sections.append("# How to act here\n\n" + PROTOCOL)
        for name in WRITABLE:
            path = self.task / name
            if path.is_file():
                body = path.read_text("utf-8")
                sections.append(f"# Current `{name}` ({len(body)} characters)\n\n```\n{body}\n```")
        if self.binding:
            sections.append("# Latest review binding (printed by your last successful check or target)\n\n" + self.binding)
        if self.last_finalize:
            sections.append("# Your latest finalize was blocked by (still the case until you change something)\n\n" + self.last_finalize)
        if self.history:
            sections.append("# What you have done so far\n\n" + "\n\n".join(self.history))
        sections.append(f"# Result of your last action — {obs.title}\n\n{obs.text}")

        images: list[tuple[str, Path]] = [("design_grid.png — the design with the measurement grid (from prepare)",
                                           self.task / "design_grid.png")]
        fresh = obs.images or self.recent_images
        if obs.images:
            self.recent_images = obs.images
        for entry in fresh:
            if entry[1].resolve() != (self.task / "design_grid.png").resolve():
                images.append(entry)
        # A sheet far taller than wide (risk_review.png grows a row per risky
        # element) would be a sliver at a model's size: it goes in pages.
        shown: list[tuple[str, Path, int | None]] = []
        for title, path, *rest in images:
            if not path.is_file():
                continue
            wanted = rest[0] if rest else None
            n = page_count(image_size(path) or (1, 1))
            if n <= 1:
                shown.append((title, path, None))
            elif wanted:
                shown.append((f"{title} — part {wanted} of {n}", path, min(wanted, n)))
            else:
                for k in range(1, min(n, PAGES_SHOWN) + 1):
                    shown.append((f"{title} — part {k} of {n}", path, k))
                if n > PAGES_SHOWN:
                    shown.append((f"(parts {PAGES_SHOWN + 1}–{n} of {title} not attached: "
                                  f'view {{"path": "{title}", "part": k}})', path, -1))
        notes = [t for t, _, k in shown if k == -1]
        shown = [x for x in shown if x[2] != -1][: self.args.max_images]
        images = shown
        listing = "\n".join(f"{i + 1}. {t}" for i, (t, _, _) in enumerate(images))
        sections.append("# Images attached to this message, in order\n\n" + listing + ("\n" + "\n".join(notes) if notes else ""))
        sections.append(f"# Your turn {turn} of {self.args.max_turns}\n\n"
                        "Look at the result and the pictures, say briefly what you decide, then give "
                        "exactly one ```action block (and a ```content block when writing a file).")
        return "\n\n".join(sections), [self.model_image(p, k) for _, p, k in images], [t for t, _, _ in images]

    # -- actions ----------------------------------------------------------------
    def resolve_view(self, value: str) -> Path:
        if value in ("@design", "design"):
            return self.args.design
        if value.startswith("@assets/"):
            base, rel = self.args.assets, value[len("@assets/"):]
        elif value.startswith("@skill/"):
            base, rel = self.skill, value[len("@skill/"):]
        elif value.startswith("@request/"):
            base, rel = self.request_dir, value[len("@request/"):]
        else:
            base, rel = self.task, value.lstrip("/")
        path = (base / rel).resolve()
        if base.resolve() not in path.parents and path != base.resolve():
            raise ValueError(f"{value} is outside the folders you can open")
        return path

    def act(self, action: dict[str, Any], content: str | None) -> Observation:
        obs = self.act_raw(action, content)
        # Reports quote absolute paths too (completion_report's evidence list).
        obs.text = self.wf.clean(obs.text)
        return obs

    def act_raw(self, action: dict[str, Any], content: str | None) -> Observation:
        name = action["action"]
        if name == "view":
            part = action.get("part")
            return self.do_view(str(action.get("path", "")), int(part) if isinstance(part, (int, float)) and part >= 1 else None)
        if name == "write":
            return self.do_write(str(action.get("path", "")), content)
        if name == "patch":
            return self.do_patch(action.get("ops"))
        if name == "edit":
            return self.do_edit(action)
        if name == "prepare":
            return self.do_prepare()
        if name == "measure":
            return self.do_measure(action)
        if name == "check":
            return self.do_check(action)
        if name == "target":
            return self.do_target(action)
        if name == "preview":
            return self.do_preview(action)
        if name == "backfill_anchors":
            return self.do_backfill(action)
        if name == "finalize":
            return self.do_finalize()
        if name == "stop":
            return Observation("stop", f"Stopped: {action.get('reason', '')}", kind="agent", status="warn")
        return Observation(f"unknown action {name!r}", f"There is no action {name!r}. See \"How to act here\".",
                           status="warn")

    def do_prepare(self) -> Observation:
        code, out = self.wf.run("workflow.py", "prepare", "--design", str(self.args.design),
                                "--assets", str(self.args.assets), "--output", str(self.task))
        images = [("design_grid.png", self.task / "design_grid.png")]
        for sheet in sorted((self.task / "assets").glob("assets_contact_sheet_usage_*.png")):
            images.append((f"assets/{sheet.name}", sheet))
        metrics = read_json(self.task / "design_grid_metrics.json") or {}
        text = f"$ workflow.py prepare (exit {code})\n{tail(out, 40)}"
        if code == 0:
            text += ("\n\nFiles: design_grid.png, design_grid_metrics.json "
                     f"(px_per_cell_x={metrics.get('px_per_cell_x')}, px_per_cell_y={metrics.get('px_per_cell_y')}), "
                     "assets/assets_inventory.json, assets/assets_contact_sheet.png, "
                     + ", ".join(f"assets/{p.name}" for p in sorted((self.task / "assets").glob('assets_contact_sheet_usage_*.png')))
                     + ". Open assets/assets_inventory.json for exact asset paths and sizes.")
        return Observation("prepare", text, images, kind="prepare", status="ok" if code == 0 else "error")

    def prepare_keeping(self) -> Observation:
        """prepare takes an empty folder or one with its own state; a task folder
        that lost its state keeps only the files the model wrote: set aside,
        prepared afresh, put back (check makes everything else again)."""
        kept = {name: (self.task / name).read_bytes() for name in WRITABLE if (self.task / name).is_file()}
        if self.task.exists():
            shutil.rmtree(self.task)
        obs = self.do_prepare()
        self.task.mkdir(parents=True, exist_ok=True)
        for name, data in kept.items():
            (self.task / name).write_bytes(data)
        return obs

    def do_view(self, value: str, part: int | None = None) -> Observation:
        try:
            path = self.resolve_view(value)
        except ValueError as exc:
            return Observation(f"view {value}", str(exc), status="warn")
        if not path.is_file():
            return Observation(f"view {value}", f"No such file: {value}", status="warn")
        if path.suffix.lower() in (".png", ".jpg", ".jpeg", ".webp"):
            size = image_size(path)
            entry: tuple = (value, path, part) if part else (value, path)
            return Observation(f"view {value}", f"{value}: image {size[0]}×{size[1]} — attached." if size else value,
                               [entry], kind="view")
        text = self.wf.clean(path.read_text("utf-8", errors="replace"))
        rel = value.replace("@skill/", "")
        if value.startswith("@skill/") and path.suffix.lower() == ".md":
            self.opened[rel] = text
            return Observation(f"view {value}", f"{rel} is now open (shown above, it stays open).", kind="view")
        cut = text if len(text) <= TEXT_VIEW_LIMIT else text[:TEXT_VIEW_LIMIT] + f"\n… cut ({len(text)} characters)"
        self.read.pop(value, None)
        self.read[value] = (self.turn, cut)
        while sum(len(t) for _, t in self.read.values()) > READ_BUDGET and len(self.read) > 1:
            self.read.pop(next(iter(self.read)))
        return Observation(f"view {value}", f"{value} ({len(text)} characters) is now in front of you under "
                           "\"Files you have read\".", kind="view")

    def do_write(self, rel: str, content: str | None) -> Observation:
        if rel not in WRITABLE:
            return Observation(f"write {rel}", f"You can write only {', '.join(WRITABLE)}.", status="warn")
        if content is None:
            return Observation(f"write {rel}", "No ```content block with the file's text.", status="warn")
        if rel.endswith(".json"):
            try:
                json.loads(content)
            except ValueError as exc:
                # Written anyway: check reports invalid JSON as the skill would.
                (self.task / rel).write_text(content, "utf-8")
                return Observation(f"write {rel}", f"Wrote {rel} ({len(content)} characters), but it is not valid JSON: {exc}",
                                   kind="write", status="warn")
        (self.task / rel).write_text(content, "utf-8")
        return Observation(f"write {rel}", f"Wrote {rel} ({len(content)} characters).", kind="write")

    def do_patch(self, ops: Any) -> Observation:
        path = self.task / "ui_structure.json"
        if not isinstance(ops, list):
            return Observation("patch", '"ops" must be a list of JSON Patch operations.', status="warn")
        try:
            doc = json.loads(path.read_text("utf-8"))
            doc = apply_patch(doc, ops)
        except (OSError, ValueError, KeyError, IndexError, TypeError) as exc:
            return Observation("patch", f"Patch not applied: {exc}", status="warn")
        path.write_text(json.dumps(doc, ensure_ascii=False, indent=2) + "\n", "utf-8")
        return Observation("patch", f"Applied {len(ops)} operation(s) to ui_structure.json.", kind="write")

    def do_edit(self, a: dict[str, Any]) -> Observation:
        rel = str(a.get("path", ""))
        if rel not in WRITABLE:
            return Observation(f"edit {rel}", f"You can edit only {', '.join(WRITABLE)}.", status="warn")
        path = self.task / rel
        if not path.is_file():
            return Observation(f"edit {rel}", f"There is no {rel} yet: write it first.", status="warn")
        old, new = a.get("old"), a.get("new")
        if not isinstance(old, str) or not old or not isinstance(new, str):
            return Observation(f"edit {rel}", '"old" (not empty) and "new" must be strings.', status="warn")
        text = path.read_text("utf-8")
        count = text.count(old)
        if not count:
            return Observation(f"edit {rel}", f'"old" is not in {rel}: it must match exactly, spaces and backticks included.',
                               status="warn")
        every = a.get("all") is True
        if count > 1 and not every:
            return Observation(f"edit {rel}", f'"old" is in {rel} {count} times: make it longer so it is unique, '
                               'or add "all": true.', status="warn")
        text = text.replace(old, new) if every else text.replace(old, new, 1)
        path.write_text(text, "utf-8")
        done = f"Replaced {count if every else 1} occurrence(s) in {rel}."
        if rel.endswith(".json"):
            try:
                json.loads(text)
            except ValueError as exc:
                return Observation(f"edit {rel}", f"{done} The file is not valid JSON now: {exc}", kind="write", status="warn")
        return Observation(f"edit {rel}", done, kind="write")

    def do_measure(self, a: dict[str, Any]) -> Observation:
        region = [str(int(v)) for v in (a.get("region") or [])][:4]
        if len(region) != 4:
            return Observation("measure", '"region" must be [X, Y, W, H].', status="warn")
        code, out = self.wf.workflow("measure", "--region", *region, "--zoom", str(int(a.get("zoom", 3))),
                                     "--cell-size", str(int(a.get("cellSize", a.get("cell_size", 10)))))
        folder = self.task / "measurements" / ("region_" + "_".join(region))
        images = [(f"measurements/{folder.name}/design_grid.png (ruler)", folder / "design_grid.png"),
                  (f"measurements/{folder.name}/design.png (clean zoom)", folder / "design.png")]
        metrics = read_json(folder / "metrics.json")
        text = f"$ workflow.py measure --region {' '.join(region)} (exit {code})\n{tail(out, 20)}"
        if metrics:
            text += f"\nmetrics.json: {json.dumps(metrics)}"
        return Observation(f"measure {' '.join(region)}", text, images if code == 0 else [], kind="measure",
                           status="ok" if code == 0 else "warn")

    def do_check(self, a: dict[str, Any]) -> Observation:
        flags = (["--transparent-bg"] if a.get("transparentBg") else []) + (["--allow-warnings"] if a.get("allowWarnings") else [])
        code, out = self.wf.workflow("check", *flags)
        self.last_finalize = ""
        if code == 0:
            self.checks += 1
            self.note_binding(out)
        text = f"$ workflow.py check {' '.join(flags)} (exit {code})\n{tail(out, 40)}\n\n{check_summary(self.task)}"
        images = [(name, self.task / name) for name in ("comparison.png", "all_elements.png", "risk_review.png")]
        return Observation("check" + (" passed" if code == 0 else " failed"), text,
                           [i for i in images if i[1].is_file()], kind="check", status="ok" if code == 0 else "warn",
                           trace_only=self.reconstruction(code))

    def do_target(self, a: dict[str, Any]) -> Observation:
        paths = [str(p) for p in (a.get("paths") or a.get("element_paths") or [])]
        if not paths:
            return Observation("target", '"paths" must list element paths.', status="warn")
        args: list[str] = []
        for p in paths:
            args += ["--element-path", p]
        if a.get("transparentBg"):
            args.append("--transparent-bg")
        if a.get("allowWarnings"):
            args.append("--allow-warnings")
        code, out = self.wf.workflow("target", *args)
        if code == 0:
            self.note_binding(out)
        # The names workflow.py gives them (safe_slug, deduplicated by index).
        made: list[Path] = []
        used: set[str] = set()
        for index, element_path in enumerate(paths, start=1):
            slug = (re.sub(r"[^A-Za-z0-9._-]+", "_", element_path).strip("._-") or "element")[-120:]
            if slug in used:
                slug = f"{slug}_{index}"
            used.add(slug)
            made.append(self.task / f"target_{slug}.png")
        text = f"$ workflow.py target {' '.join(paths)} (exit {code})\n{tail(out, 30)}"
        if code == 0:
            text += "\n\n" + check_summary(self.task)
        images = [(p.name, p) for p in made if code == 0 and p.is_file()]
        return Observation(f"target {len(paths)}", text, images[:6], kind="target", status="ok" if code == 0 else "warn",
                           trace_only=self.reconstruction(code))

    def note_binding(self, out: str) -> None:
        m = re.findall(r"<!-- image-to-ui-review-binding:[^>]*-->", out)
        if m:
            self.binding = m[-1]

    def reconstruction(self, code: int) -> list[tuple[str, Path]]:
        path = self.task / "reconstruction.png"
        return [("reconstruction.png", path)] if code == 0 and path.is_file() else []

    def do_preview(self, a: dict[str, Any]) -> Observation:
        name = str(a.get("name") or "scenario")
        args = ["--name", name]
        if a.get("size"):
            args += ["--size", *[str(int(v)) for v in a["size"][:2]]]
        for key in ("state", "progress", "scroll"):
            for value in a.get(key) or []:
                args += [f"--{key}", str(value)]
        code, out = self.wf.workflow("preview", *args)
        folder = self.task / "previews" / re.sub(r"[^A-Za-z0-9._-]+", "_", name).strip("._-")
        images = [(f"previews/{folder.name}/{p.name}", p) for p in sorted(folder.glob("*.png"))]
        report = read_json(folder / "preview_report.json")
        text = f"$ workflow.py preview {' '.join(args)} (exit {code})\n{tail(out, 30)}"
        if report:
            text += "\npreview_report.json: " + json.dumps(report)[:4000]
        return Observation(f"preview {name}", text, images[:4], kind="preview", status="ok" if code == 0 else "warn")

    def do_backfill(self, a: dict[str, Any]) -> Observation:
        path = str(self.task / "ui_structure.json")
        args = ["--structure", path, "--output", path] + (["--overwrite"] if a.get("overwrite") else [])
        code, out = self.wf.run("backfill_anchors.py", *args)
        return Observation("backfill_anchors", f"$ backfill_anchors.py (exit {code})\n{tail(out, 30)}", kind="write",
                           status="ok" if code == 0 else "warn")

    def do_finalize(self) -> Observation:
        code, out = self.wf.workflow("finalize")
        report = read_json(self.task / "completion_report.json") or {}
        self.blockers = [str(b) for b in report.get("blockers", [])]
        self.last_finalize = self.wf.clean("\n".join(f"- {b[:2500]}" for b in self.blockers)) if self.blockers else ""
        if code != 0 and not self.blockers:
            self.last_finalize = self.wf.clean(tail(out, 6))
        self.completed = code == 0 and bool(report.get("complete"))
        text = f"$ workflow.py finalize (exit {code})\n{tail(out, 30)}"
        if report:
            text += (f"\ncompletion_report.json: complete={report.get('complete')}, "
                     f"iterations={json.dumps(report.get('iterations', {}))}, "
                     f"review_coverage={json.dumps(report.get('review_coverage', {}))[:3000]}")
            if self.blockers:
                text += "\nBlockers:\n" + "\n".join(f"- {b}" for b in self.blockers)
        detail = self.task / "detail_comparison.png"
        return Observation("finalize: complete" if self.completed else "finalize: blocked", text,
                           [("detail_comparison.png", detail)] if detail.is_file() else [], kind="finalize",
                           status="ok" if self.completed else "warn", completed=self.completed)

    # -- the loop ---------------------------------------------------------------
    def run(self) -> int:
        t0 = time.time()
        if self.request is not None and not (self.task / "ui_structure.json").is_file():
            self.bridge.send({"type": "result", "ok": False, "error": "nothing to revise",
                              "detail": "the task folder has no ui_structure.json to change"})
            return 2
        # A round of edits continues a prepared task folder; prepare again only
        # when it lost its state (the inventory and grid it needs) — keeping
        # what the model wrote, which prepare's empty-folder rule would refuse.
        if self.request is None or not (self.task / "workflow_state.json").is_file():
            obs = self.do_prepare() if self.request is None else self.prepare_keeping()
            self.trace.step(id="prepare", title="Prepare: inventory + measurement grid", kind="prepare",
                            text=obs.text, status=obs.status,
                            images=[self.trace.image(t, p) for t, p, *_ in obs.images])
            if obs.status == "error":
                self.bridge.send({"type": "result", "ok": False, "error": "prepare failed", "detail": obs.text[-2000:]})
                return 2
        if self.request is not None:
            obs = self.begin_round()
        turn = 0
        for turn in range(1, self.args.max_turns + 1):
            self.turn = turn
            prompt, images, titles = self.compose(turn, obs)
            self.bridge.event(f"turn {turn}/{self.args.max_turns}")
            started = time.time()
            reply = None
            for attempt in (1, 2):
                try:
                    reply = self.bridge.model(prompt, images, f"turn-{turn}" if attempt == 1 else f"turn-{turn}-retry")
                    break
                except RuntimeError as exc:
                    self.error = str(exc)
                    if attempt == 2 or "closed the bridge" in self.error:
                        break
                    time.sleep(5)
            if reply is None:
                self.trace.step(id=f"turn-{turn}", title=f"Turn {turn}: model call failed", kind="agent",
                                status="error", note=self.error)
                break
            self.error = ""
            action, content, error = parse_reply(reply)
            note = re.sub(r"```.*?```", "", reply, flags=re.S).strip()
            if error:
                obs = Observation("no action", f"Your reply had no usable action: {error}. Reply with one "
                                  "```action block as described in \"How to act here\".", status="warn")
            else:
                try:
                    obs = self.act(action, content)
                except Exception as exc:  # an action must never end the task
                    obs = Observation(f"{action.get('action')} failed", f"{type(exc).__name__}: {exc}", status="warn")
            label = json.dumps(action, ensure_ascii=False) if action else "(none)"
            self.history.append(f"Turn {turn}: {note[:SUMMARY_LIMIT]}\nAction: {label[:SUMMARY_LIMIT]}\n"
                                f"Result: {obs.title} — {obs.text[:SUMMARY_LIMIT].strip()}")
            self.trace.step(
                id=f"turn-{turn}", title=f"Turn {turn}: {obs.title}", kind=obs.kind, status=obs.status,
                ms=int((time.time() - started) * 1000), note=note[:4000] or None,
                text=obs.text + (f"\n\n--- written ---\n{content}" if content and action and action.get("action") == "write" else ""),
                data={"action": action, "imagesShown": titles},
                images=[self.trace.image(t, p) for t, p, *_ in [*obs.images, *obs.trace_only]],
            )
            if obs.completed or (action and action.get("action") == "stop"):
                break
        result = self.result(turn, time.time() - t0)
        self.bridge.send(result)
        return 0 if result["ok"] else 2

    def begin_round(self) -> Observation:
        """A round of edits starts from the layout as it is: check it (fresh
        evidence and binding), then put the request in front of the model —
        in the task for every turn, with its pictures on the first."""
        request = self.request or {}
        items = request.get("items") or []
        check = self.do_check({})
        self.trace.step(id="check", title=f"The layout to change: {check.title}", kind="check", text=check.text,
                        status=check.status, images=[self.trace.image(t, p) for t, p, *_ in [*check.images, *check.trace_only]])
        pictures = request_pictures(self.args.design, self.task / "reconstruction.png", items, self.request_dir)
        names = [title.split(" — ")[0] for title, _ in pictures]
        text = request_text(request, read_json(self.task / "all_elements_legend.json"), names)
        self.task_md += "\n\n" + text
        self.trace.step(id="request", title=f"The user's request: {len(items)} note(s)", kind="request",
                        text=text, data={"request": request}, images=[self.trace.image(t, p) for t, p in pictures])
        shown = pictures[:2 + PAGES_SHOWN] + [i for i in check.images if i[0] == "comparison.png"]
        return Observation("the layout to change, checked again", "Your layout, checked again before the changes "
                           f"(the request is under \"The task\"):\n{check.text}", shown, kind="check", status=check.status)

    def result(self, turns: int, seconds: float) -> dict[str, Any]:
        structure = self.task / "ui_structure.json"
        state = read_json(self.task / "workflow_state.json") or {}
        checked = state.get("status") in ("checked", "completed", "finalize_failed") and (self.task / "reconstruction.png").is_file()
        report = read_json(self.task / "completion_report.json") or {}
        summary = {
            "complete": self.completed, "turns": turns, "seconds": round(seconds, 1),
            "check_count": state.get("check_count", 0), "structure_revision": state.get("structure_revision", 0),
            "target_count": state.get("target_count", 0), "status": state.get("status"),
            "blockers": self.blockers or report.get("blockers", []),
        }
        self.trace.data["summary"] = summary
        self.trace.step(id="agent-done", title=("Completed" if self.completed else "Not completed")
                        + f" after {turns} turn(s), {summary['check_count']} check(s)",
                        kind="output", status="ok" if self.completed else "warn", data=summary)
        out: dict[str, Any] = {"type": "result", "ok": structure.is_file() and checked, "summary": summary,
                               "structure": str(structure), "reconstruction": str(self.task / "reconstruction.png")}
        if self.error:
            out["error"] = self.error
        return out


def main() -> int:
    p = argparse.ArgumentParser(description="Run the image-to-ui skill with a model as the agent.")
    p.add_argument("--design", type=Path, required=True)
    p.add_argument("--assets", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True, help="the task folder (fresh)")
    p.add_argument("--skill", type=Path, default=DEFAULT_SKILL)
    p.add_argument("--trace-dir", type=Path)
    p.add_argument("--scratch", type=Path, help="where model-sized copies of pictures go")
    p.add_argument("--max-turns", type=int, default=60)
    p.add_argument("--max-images", type=int, default=10)
    p.add_argument("--max-image-side", type=int, default=2048)
    p.add_argument("--hint", default="")
    p.add_argument("--name", default="")
    p.add_argument("--font", action="append", dest="fonts", default=[], help="fontFamily value available to the model")
    p.add_argument("--sprites-note", default="")
    p.add_argument("--revise", type=Path, help="a round of edits (request JSON): continue the task folder")
    args = p.parse_args()
    args.request = None
    if args.revise:
        try:
            args.request = json.loads(args.revise.read_text("utf-8"))
        except (OSError, ValueError) as exc:
            Bridge().send({"type": "result", "ok": False, "error": "bad_arguments", "detail": f"--revise: {exc}"})
            return 4
    try:
        import numpy  # noqa: F401
        from PIL import Image
    except ImportError as exc:
        Bridge().send({"type": "result", "ok": False, "error": "missing_dependencies",
                       "detail": f"Python {sys.version.split()[0]} at {sys.executable} lacks {exc.name}: "
                                 "install Pillow and numpy (python3 -m pip install pillow numpy)"})
        return 3
    for attr in ("design", "assets", "output", "skill"):
        setattr(args, attr, getattr(args, attr).resolve())
    if not args.design.is_file() or not args.assets.is_dir() or not (args.skill / "scripts" / "workflow.py").is_file():
        Bridge().send({"type": "result", "ok": False, "error": "bad_arguments", "detail": "design, assets or skill missing"})
        return 4
    with Image.open(args.design) as img:
        args.design_w, args.design_h = img.size
    args.asset_count = sum(1 for f in args.assets.rglob("*.png"))
    args.scratch = (args.scratch or args.output.parent / "agent-view").resolve()
    args.max_turns = max(1, args.max_turns)
    n = int((args.request or {}).get("round") or 1)
    trace = Trace(args.trace_dir.resolve() if args.trace_dir else None,
                  *((f"edit{n}-", f"Edit {n} · ") if args.request is not None else ()))
    return Agent(args, Bridge(), trace).run()


if __name__ == "__main__":
    sys.exit(main())
