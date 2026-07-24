#!/usr/bin/env python3
"""Generates reference docs for PSD sources: a PNG render and a markdown
structure description next to every .psd under a folder, plus an index.

CLI: psd_docs.py <folder> [--max-side 2048] [--force]
"""

import argparse
import json
import os
import re

import psd_lib


def fmt_pct(value255):
    return f"{round(value255 / 255 * 100)}%"


def layer_line(node, canvas, depth):
    cx = (node["bbox"][0] + node["bbox"][2]) / 2
    cy = (node["bbox"][1] + node["bbox"][3]) / 2
    o2x, o2y = cx - canvas[0] / 2, canvas[1] / 2 - cy

    parts = [f"{'  ' * depth}- **{node['name']}**"]
    parts.append("(group)" if node["kind"] == "group" else f"({node['kind']})")
    parts.append(f"{node['size'][0]}x{node['size'][1]} @ psd({round(cx)}, {round(cy)}) o2({round(o2x)}, {round(o2y)})")
    if not node["visible"]:
        parts.append("[hidden]")
    if node["opacity"] != 255:
        parts.append(f"[opacity {fmt_pct(node['opacity'])}]")
    if node["blend_mode"] not in ("normal", "pass_through"):
        parts.append(f"[{node['blend_mode']}]")
    return " ".join(parts)


def walk_lines(nodes, canvas, depth, lines, counts):
    for node in nodes:
        counts["group" if node["kind"] == "group" else "layer"] += 1
        lines.append(layer_line(node, canvas, depth))
        if "children" in node:
            walk_lines(node["children"], canvas, depth + 1, lines, counts)


def read_description(md_path):
    """Keeps the hand-written Description section across regenerations."""
    try:
        with open(md_path) as f:
            match = re.search(r"## Description\n\n(.*?)\n\n## ", f.read(), re.DOTALL)
            if match and match.group(1).strip() != "(to be filled)":
                return match.group(1)
    except OSError:
        pass
    return "(to be filled)"


def document(psd_path, max_side, force):
    stem = os.path.splitext(psd_path)[0]
    png_path = stem + ".png"
    md_path = stem + ".md"

    if not force and os.path.isfile(md_path) and os.path.isfile(png_path) and \
            os.path.getmtime(md_path) > os.path.getmtime(psd_path):
        return None

    description = read_description(md_path)

    data = psd_lib.structure(psd_path, include_hidden=True)
    canvas = data["canvas"]

    scale = min(1.0, max_side / max(canvas)) if max(canvas) > max_side else 1.0
    render_error = None
    try:
        psd_lib.render(psd_path, png_path, scale if scale != 1.0 else None)
    except Exception as e:
        render_error = str(e)

    lines, counts = [], {"layer": 0, "group": 0}
    walk_lines(data["layers"], canvas, 0, lines, counts)

    name = os.path.basename(psd_path)
    with open(md_path, "w") as f:
        f.write(f"# {name}\n\n")
        f.write(f"Canvas {canvas[0]}x{canvas[1]} px, {counts['layer']} layers in {counts['group']} groups.\n")
        if render_error:
            f.write(f"Render failed: {render_error}\n")
        else:
            f.write(f"Render: [{os.path.basename(png_path)}]({os.path.basename(png_path)})")
            f.write(f" (scaled to {round(scale * 100)}%)\n" if scale != 1.0 else "\n")
        f.write("\nCoordinates: `psd(x, y)` is the layer bbox center in PSD pixels (top-left origin, y down);\n")
        f.write("`o2(x, y)` is the same point in o2 world space (canvas center origin, y up).\n")
        f.write(f"\n## Description\n\n{description}\n")
        f.write("\n## Layers (bottom to top)\n\n")
        f.write("\n".join(lines) + "\n")

    return {"psd": psd_path, "md": md_path, "png": None if render_error else png_path,
            "canvas": canvas, "layers": counts["layer"], "groups": counts["group"],
            "render_error": render_error}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("folder")
    parser.add_argument("--max-side", type=int, default=2048,
                        help="Max render side in px, larger canvases are downscaled (default 2048)")
    parser.add_argument("--force", action="store_true", help="Regenerate up-to-date docs too")
    parser.add_argument("--single", action="store_true",
                        help="Treat the folder argument as one .psd file (used by the per-file workers)")
    args = parser.parse_args()

    if args.single:
        result = document(args.folder, args.max_side, args.force)
        print(json.dumps(result or {"psd": args.folder, "skipped": True}))
        return

    # every file runs in its own worker process: one huge PSD crashing the
    # interpreter (OOM) must not take the rest of the batch with it
    import subprocess
    import sys

    results = []
    for root, dirs, files in os.walk(args.folder):
        for file in sorted(files):
            if not file.lower().endswith(".psd"):
                continue
            path = os.path.join(root, file)
            print("processing", path, flush=True)

            cmd = [sys.executable, os.path.abspath(__file__), path,
                   "--max-side", str(args.max_side), "--single"]
            if args.force:
                cmd.append("--force")
            try:
                proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
            except subprocess.TimeoutExpired:
                print("  TIMEOUT", flush=True)
                results.append({"psd": path, "error": "timeout"})
                continue

            if proc.returncode == 0:
                try:
                    results.append(json.loads(proc.stdout.strip().splitlines()[-1]))
                except (ValueError, IndexError):
                    results.append({"psd": path, "error": "bad worker output"})
            else:
                error = (proc.stderr or "").strip().splitlines()
                error = error[-1] if error else f"worker died (code {proc.returncode})"
                print("  FAILED:", error, flush=True)
                results.append({"psd": path, "error": error})

    print(json.dumps(results, indent=1))


if __name__ == "__main__":
    main()
