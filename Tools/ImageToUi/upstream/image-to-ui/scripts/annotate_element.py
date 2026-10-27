"""
Annotate one element, one layout group, or every resolved element in the design
image with colored bounding boxes. The annotated full-image output is what the
agent inspects to judge whether bboxes align with the actual elements in the
design.

Group mode:
  - If --element-path points to a container that has a `layout` block, all of
    its direct children are annotated together with the container itself
    (each with its own colored bbox + label). One vision call covers the
    whole group.

All-elements mode:
  - If --all-elements is passed, every non-full-canvas resolved element is
    annotated in one overview image. Full-canvas `overlay` and `rect` elements
    are also annotated because they are active UI layers. Bboxes are labeled
    with numeric indexes, and the index/path/bbox legend is written to JSON.
    Explicit `list` / `listItem` roles are copied into legend entries. Roles
    are never inferred from layout geometry.
    Use this as the first pass to find large drift cheaply, then inspect only
    suspicious elements separately.

This overview stays on the full canvas so an incorrect draft bbox cannot hide
the actual element. For fine alignment, use workflow.py measure on a region
selected from the design overview, including neighboring landmarks. Its clean
crop and absolute-pixel rulers preserve detail that a scaled overview loses.

Usage:
    py -B annotate_element.py \
        --design design.png \
        --structure ui_structure.json \
        --element-path "root/level_popup/popup_header/close_button" \
        --output out/annotated.png

    py -B annotate_element.py \
        --design design.png \
        --structure ui_structure.json \
        --all-elements \
        --output out/all_elements.png \
        --legend out/all_elements_legend.json

If --element-path is omitted, prints the list of valid paths and exits, so
the agent can discover what to align next.
"""

import argparse
import json
import sys
from pathlib import Path
from typing import List, Tuple

from PIL import Image, ImageDraw, ImageFont

# Make the sibling layout.py importable when this script is run from anywhere
sys.path.insert(0, str(Path(__file__).resolve().parent))
import ui_components as components
import layout as layout_mod  # noqa: E402
from annotate_grid import draw_grid, select_grid_palette  # noqa: E402


# Distinct, high-contrast colors for group members (cycled)
GROUP_COLORS = [
    (255, 64, 64, 255),    # red
    (64, 200, 80, 255),    # green
    (60, 130, 255, 255),   # blue
    (255, 170, 0, 255),    # orange
    (200, 80, 255, 255),   # purple
    (0, 200, 200, 255),    # teal
]
SINGLE_COLOR = (255, 40, 40, 255)
LABEL_BG = (255, 255, 255, 230)
LABEL_FG = (20, 20, 20, 255)


def find_font(size: int) -> ImageFont.FreeTypeFont:
    candidates = [
        "C:/Windows/Fonts/arialbd.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
    ]
    for c in candidates:
        if Path(c).exists():
            try:
                return ImageFont.truetype(c, size)
            except Exception:
                continue
    return ImageFont.load_default()


def draw_bbox(draw: ImageDraw.ImageDraw, bbox: Tuple[int, int, int, int],
              color, label: str, font: ImageFont.FreeTypeFont,
              line_width: int = 3):
    x, y, w, h = bbox
    # Outline
    draw.rectangle([x, y, x + w, y + h], outline=color, width=line_width)
    # Label background pill at the top-left corner of the bbox
    try:
        tbox = font.getbbox(label)
        tw, th = tbox[2] - tbox[0], tbox[3] - tbox[1]
    except Exception:
        tw, th = 8 * len(label), 14
    pad = 4
    lx, ly = x, max(0, y - th - 2 * pad - 2)
    draw.rectangle([lx, ly, lx + tw + 2 * pad, ly + th + 2 * pad],
                   fill=LABEL_BG, outline=color, width=1)
    draw.text((lx + pad, ly + pad), label, font=font, fill=LABEL_FG)


def is_full_canvas(elem_abs, design_w: int, design_h: int) -> bool:
    x, y, w, h = elem_abs
    return x <= 1 and y <= 1 and w >= design_w - 2 and h >= design_h - 2


def should_skip_all_elements_target(elem: dict, bbox, design_w: int, design_h: int) -> bool:
    if not is_full_canvas(bbox, design_w, design_h):
        return False
    return not bool(elem.get("asset") or elem.get("color") or elem.get("text"))


Target = Tuple[str, str, str, Tuple[int, int, int, int], str | None]


def semantic_role(elem: dict) -> str | None:
    role = elem.get("role")
    return role if isinstance(role, str) else None


def legend_entry(target: Target) -> dict:
    path, name, label, bbox, role = target
    entry = {
        "label": label,
        "path": path,
        "name": name,
        "abs_bbox": list(bbox),
    }
    if role is not None:
        entry["role"] = role
    return entry


def gather_targets(structure: dict, path: str) -> List[Target]:
    """Return targets for the element to annotate.

    A layout container includes itself followed by each direct child so the
    requested element path always corresponds to a bbox in the rendered PNG.
    """
    elem = layout_mod.find_by_path(structure, path)
    if elem is None:
        raise SystemExit(f"element path not found: {path}")

    abs_box = elem.get("_abs")
    if abs_box is None:
        raise SystemExit(f"element has no resolved _abs (did you call resolve_positions?): {path}")

    name = elem.get("name", "?")
    out = [(path, name, name, tuple(abs_box), semantic_role(elem))]
    if elem.get("layout") and elem.get("children"):
        for c in elem.get("children", []):
            cabs = c.get("_abs")
            if cabs is None:
                continue
            name = c.get("name", "?")
            out.append((
                f"{path}/{name}",
                name,
                name,
                tuple(cabs),
                semantic_role(c),
            ))
    return out


def gather_all_targets(structure: dict, design_w: int, design_h: int) -> List[Target]:
    """Return every element bbox worth reviewing in DFS order.

    Labels are numeric indexes to keep the overview image readable. The JSON
    summary printed to stdout carries the path/name legend for those indexes.
    """
    out: List[Target] = []
    for path in layout_mod.dfs_paths(structure, include_root=True):
        elem = layout_mod.find_by_path(structure, path)
        if elem is None:
            continue
        abs_box = elem.get("_abs")
        if abs_box is None:
            continue
        bbox = tuple(abs_box)
        if should_skip_all_elements_target(elem, bbox, design_w, design_h):
            continue
        name = elem.get("name", "?")
        label = str(len(out) + 1)
        out.append((path, name, label, bbox, semantic_role(elem)))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--design", required=True)
    ap.add_argument("--structure", required=True)
    ap.add_argument("--element-path", required=False,
                    help="If omitted, prints all valid element paths and exits.")
    ap.add_argument("--all-elements", action="store_true",
                    help="Annotate every non-full-canvas resolved element in one overview image.")
    ap.add_argument("--output", required=False,
                    help="Path to write the annotated full design image.")
    ap.add_argument("--legend", required=False,
                    help="Path to write the target legend JSON. Defaults to "
                         "<output-stem>_legend.json when annotating.")
    ap.add_argument("--print-legend", action="store_true",
                    help="Print the full legend JSON to stdout. By default "
                         "stdout only prints a compact summary.")
    args = ap.parse_args()

    design_path = Path(args.design)
    struct_path = Path(args.structure)
    if not design_path.exists():
        print(f"Design not found: {design_path}", file=sys.stderr); sys.exit(1)
    if not struct_path.exists():
        print(f"Structure not found: {struct_path}", file=sys.stderr); sys.exit(1)

    structure = components.snapshot(json.loads(struct_path.read_text(encoding="utf-8")))
    layout_mod.resolve_positions(structure)

    if args.all_elements and args.element_path:
        print("--all-elements cannot be combined with --element-path",
              file=sys.stderr)
        sys.exit(1)

    if not args.element_path and not args.all_elements:
        # Discovery mode
        for p in layout_mod.dfs_paths(structure, include_root=True):
            print(p)
        return

    if not args.output:
        print("--output is required when annotating elements",
              file=sys.stderr)
        sys.exit(1)

    base = Image.open(design_path).convert("RGBA")
    W, H = base.size

    if args.all_elements:
        targets = gather_all_targets(structure, W, H)
    else:
        targets = gather_targets(structure, args.element_path)

    # Quick sanity check on canvas size
    canvas = structure.get("canvas") or {}
    cw, ch = canvas.get("width"), canvas.get("height")
    if cw and ch and (cw, ch) != (W, H):
        print(f"  [warn] canvas in structure is {cw}x{ch} but design is {W}x{H}; "
              f"bbox positions may be off by that ratio", file=sys.stderr)

    overlay = Image.new("RGBA", base.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    label_font = find_font(max(12, min(W, H) // 90) if args.all_elements
                           else max(18, min(W, H) // 60))
    line_width = 2 if args.all_elements else 4

    for i, (_path, name, label, bbox, _role) in enumerate(targets):
        if is_full_canvas(bbox, W, H):
            color = (180, 180, 180, 230)
            draw_label = f"{label} (full canvas)"
        else:
            color = GROUP_COLORS[i % len(GROUP_COLORS)] if len(targets) > 1 else SINGLE_COLOR
            draw_label = label
        draw_bbox(draw, bbox, color, draw_label, label_font, line_width=line_width)

    full_img = Image.alpha_composite(base, overlay)
    # Overlay the same grid used in Step 1 so the bbox can be read against it.
    full_img = draw_grid(full_img, palette=select_grid_palette(base))
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    full_img.convert("RGB").save(args.output)

    summary = {
        "mode": "all_elements" if args.all_elements else "element",
        "element_path": args.element_path,
        "target_count": len(targets),
        "is_group": (not args.all_elements and len(targets) > 1),
        "targets": [legend_entry(target) for target in targets],
        "design_size": [W, H],
        "output": str(args.output),
    }
    legend_path = Path(args.legend) if args.legend else Path(args.output).with_name(
        f"{Path(args.output).stem}_legend.json"
    )
    legend_path.parent.mkdir(parents=True, exist_ok=True)
    legend_path.write_text(
        json.dumps(summary, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

    compact = {
        "mode": summary["mode"],
        "element_path": summary["element_path"],
        "target_count": summary["target_count"],
        "is_group": summary["is_group"],
        "output": summary["output"],
        "legend": str(legend_path),
    }
    if args.print_legend:
        print(json.dumps(summary, indent=2, ensure_ascii=False))
    else:
        print(json.dumps(compact, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
