"""
Validate ui_structure.json before bbox annotation and rendering.

Checks:
  - JSON shape, required fields, positive sizes, and duplicate element paths
  - required anchor alignment metadata on every node
  - explicit list/listItem semantic-role hierarchy
  - canvas size against the design image
  - asset references against the sliced asset directory
  - layout, alignment, color, opacity, and nine-slice field sanity

Usage:
    py -B validate_structure.py --structure ui_structure.json --design design.png --assets sprite_dir --report out/validate_report.json
"""

from __future__ import annotations

import argparse
import json
import math
import re
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any

from PIL import Image

import ui_components as components


IMAGE_EXTS = {".png", ".jpg", ".jpeg"}
ELEMENT_TYPES = {"container", "image", "text", "button", "overlay", "rect"}
LAYOUT_TYPES = {"row", "column", "grid"}
LAYOUT_SPACING = {"even"}
LAYOUT_AXES = {
    "row": ("x", "y"),
    "column": ("y", "x"),
}
AXIS_ALIGN = {
    "x": {"start", "center", "middle", "end", "left", "right"},
    "y": {"start", "center", "middle", "end", "top", "bottom"},
}
LAYOUT_DISTRIBUTION = {"space-between", "space-around", "space-evenly"}
ALIGN_X = {"left", "center", "right", "start", "end"}
ALIGN_Y = {"top", "middle", "bottom", "start", "end", "center"}
TEXT_ALIGN = {"left", "center", "right"}
TEXT_VALIGN = {"top", "middle", "bottom", "start", "end", "center"}
ANCHOR_HORIZONTAL = {"left", "center", "right"}
ANCHOR_VERTICAL = {"top", "middle", "bottom"}
ANCHOR_FIELDS = {"horizontal", "vertical"}
SEMANTIC_ROLES = {"list", "listItem"}
NINE_SLICE_SIDES = ("left", "top", "right", "bottom")
COLOR_RE = re.compile(r"^#[0-9a-fA-F]{6}([0-9a-fA-F]{2})?$")


class Reporter:
    def __init__(self) -> None:
        self.errors: list[str] = []
        self.warnings: list[str] = []

    def error(self, path: str, message: str) -> None:
        self.errors.append(f"{path}: {message}")

    def warn(self, path: str, message: str) -> None:
        self.warnings.append(f"{path}: {message}")


def is_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def positive_number(value: Any) -> bool:
    return is_number(value) and value > 0


def finite_non_negative_number(value: Any) -> bool:
    if not is_number(value) or value < 0:
        return False
    return not isinstance(value, float) or math.isfinite(value)


def asset_key(value: str) -> str:
    return value.replace("\\", "/").lower()


def casefold_asset_key(value: str) -> str:
    return value.replace("\\", "/").casefold()


def read_json(path: Path) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise SystemExit(f"Invalid JSON in {path}: {exc}") from exc
    if not isinstance(data, dict):
        raise SystemExit("Structure root must be a JSON object")
    return data


def build_asset_index(
    assets_dir: Path,
    reporter: Reporter | None = None,
) -> tuple[dict[str, Path], set[str], dict[str, dict[str, int | float]]]:
    by_basename: dict[str, list[Path]] = defaultdict(list)
    index: dict[str, Path] = {}
    borders: dict[str, dict[str, int | float]] = {}
    relative_paths: dict[str, str] = {}

    paths = [
        path
        for path in assets_dir.rglob("*")
        if path.is_file() and path.suffix.casefold() in IMAGE_EXTS
    ]
    paths.sort(
        key=lambda path: (
            path.relative_to(assets_dir).as_posix().casefold(),
            path.relative_to(assets_dir).as_posix(),
        )
    )
    for path in paths:
        rel = path.relative_to(assets_dir).as_posix()
        rel_key = asset_key(rel)
        collision_key = casefold_asset_key(rel)
        name_key = path.name.lower()
        previous = relative_paths.get(collision_key)
        if previous is not None and previous != rel:
            if reporter is not None:
                reporter.error(
                    "assets",
                    f"asset relative path case-fold collision: {previous!r} and {rel!r}",
                )
        else:
            relative_paths[collision_key] = rel
            index[rel_key] = path
        by_basename[name_key].append(path)
        border = read_unity_sprite_border(path)
        if border and rel_key not in borders:
            borders[rel_key] = border

    duplicate_basenames = {name for name, paths in by_basename.items() if len(paths) > 1}
    for name, paths in by_basename.items():
        if len(paths) == 1:
            index[name] = paths[0]
            border = read_unity_sprite_border(paths[0])
            if border:
                borders[name] = border
    return index, duplicate_basenames, borders


def read_unity_sprite_border(path: Path) -> dict[str, int | float] | None:
    meta_path = Path(str(path) + ".meta")
    if not meta_path.exists():
        return None
    try:
        text = meta_path.read_text(encoding="utf-8")
    except OSError:
        return None
    match = re.search(
        r"spriteBorder:\s*\{x:\s*([^,]+),\s*y:\s*([^,]+),\s*z:\s*([^,]+),\s*w:\s*([^}]+)\}",
        text,
    )
    if not match:
        return None
    parsed = tuple(float(value.strip()) for value in match.groups())
    if all(math.isfinite(value) for value in parsed):
        left, bottom, right, top = (int(value) for value in parsed)
    else:
        left, bottom, right, top = parsed
    if left == top == right == bottom == 0:
        return None
    return {"left": left, "top": top, "right": right, "bottom": bottom}


def check_xy_dict(obj: Any, path: str, field: str, reporter: Reporter) -> None:
    if not isinstance(obj, dict):
        reporter.error(path, f"{field} must be an object")
        return
    for key in ("x", "y"):
        if key in obj and not is_number(obj[key]):
            reporter.error(path, f"{field}.{key} must be a number")


def check_size(obj: Any, path: str, reporter: Reporter) -> None:
    if not isinstance(obj, dict):
        reporter.error(path, "size must be an object")
        return
    for key in ("width", "height"):
        if key not in obj:
            reporter.error(path, f"size.{key} is required")
        elif not positive_number(obj[key]):
            reporter.error(path, f"size.{key} must be a positive number")


def check_anchor(obj: Any, path: str, reporter: Reporter) -> None:
    if not isinstance(obj, dict):
        reporter.error(path, "anchor must be an object")
        return
    unknown = sorted(set(obj) - ANCHOR_FIELDS)
    if unknown:
        reporter.error(path, f"anchor has unknown fields: {', '.join(unknown)}")
    allowed_by_field = {
        "horizontal": ANCHOR_HORIZONTAL,
        "vertical": ANCHOR_VERTICAL,
    }
    for field, allowed in allowed_by_field.items():
        if field not in obj:
            reporter.error(path, f"anchor.{field} is required")
        elif not isinstance(obj[field], str) or obj[field] not in allowed:
            reporter.error(
                path,
                f"anchor.{field} must be one of {sorted(allowed)}",
            )


def check_color(value: Any, path: str, field: str, reporter: Reporter) -> None:
    if not isinstance(value, str) or not COLOR_RE.match(value):
        reporter.warn(path, f"{field} should be #RRGGBB or #RRGGBBAA")


def check_axis_alignment(
    layout: dict[str, Any],
    path: str,
    field: str,
    axis: str,
    is_main_axis: bool,
    reporter: Reporter,
) -> None:
    if field not in layout:
        return
    allowed = set(AXIS_ALIGN[axis])
    if is_main_axis:
        allowed.update(LAYOUT_DISTRIBUTION)
    value = layout[field]
    if not isinstance(value, str) or value not in allowed:
        role = "main" if is_main_axis else "cross"
        reporter.error(
            path,
            f"layout.{field} must be one of {sorted(allowed)} for the {axis}-axis {role} axis",
        )


def check_nine_slice(
    value: Any,
    path: str,
    reporter: Reporter,
    asset_path: Path | None,
    sprite_border: dict[str, int | float] | None,
) -> None:
    margins: dict[str, int | float] | None = None
    if isinstance(value, bool):
        if value and sprite_border:
            margins = sprite_border
    elif isinstance(value, str):
        if value not in ("auto", "meta"):
            reporter.error(path, "nineSlice string must be auto or meta")
            return
        if sprite_border:
            margins = sprite_border
    elif is_number(value):
        if not finite_non_negative_number(value):
            reporter.error(path, "nineSlice must be a finite non-negative number")
            return
        margins = {side: value for side in NINE_SLICE_SIDES}
    elif isinstance(value, dict):
        unknown = [key for key in value if key not in NINE_SLICE_SIDES]
        if unknown:
            names = ", ".join(sorted((repr(key) for key in unknown)))
            reporter.error(path, f"nineSlice has unknown margin key(s): {names}")
        valid = True
        for side in NINE_SLICE_SIDES:
            if side in value and not finite_non_negative_number(value[side]):
                reporter.error(
                    path,
                    f"nineSlice.{side} must be a finite non-negative number",
                )
                valid = False
        if not unknown and valid:
            margins = {side: value.get(side, 0) for side in NINE_SLICE_SIDES}
    else:
        reporter.error(path, "nineSlice must be bool, number, auto/meta, or margins object")
        return

    if margins is None:
        return
    valid_margins = True
    for side in NINE_SLICE_SIDES:
        if not finite_non_negative_number(margins.get(side)):
            reporter.error(
                path,
                f"nineSlice.{side} must be a finite non-negative number",
            )
            valid_margins = False
    if not valid_margins or asset_path is None:
        return

    try:
        with Image.open(asset_path) as image:
            source_width, source_height = image.size
    except OSError:
        return
    if margins["left"] + margins["right"] > source_width:
        reporter.error(
            path,
            f"nineSlice horizontal margins exceed source width {source_width}",
        )
    if margins["top"] + margins["bottom"] > source_height:
        reporter.error(
            path,
            f"nineSlice vertical margins exceed source height {source_height}",
        )


def check_layout(layout: Any, path: str, reporter: Reporter) -> None:
    if not isinstance(layout, dict):
        reporter.error(path, "layout must be an object")
        return
    if "type" not in layout:
        reporter.error(path, "layout.type is required")
        ltype = None
    else:
        ltype = layout["type"]
        if not isinstance(ltype, str) or ltype not in LAYOUT_TYPES:
            reporter.error(path, "layout.type must be row or column or grid")
    spacing = layout.get("spacing", 0)
    if ltype == "grid":
        columns = layout.get("columns")
        if isinstance(columns, bool) or not isinstance(columns, int) or columns < 1 or columns > 2147483647:
            reporter.error(path, "layout.columns must be a positive integer")
        cell = layout.get("cellSize")
        if not isinstance(cell, dict) or any(not is_number(cell.get(k)) or not math.isfinite(cell[k]) or cell[k] <= 0 for k in ("width", "height")):
            reporter.error(path, "layout.cellSize must have positive width and height")
        grid_spacing = layout.get("spacing", {})
        if not isinstance(grid_spacing, dict) or any(not is_number(grid_spacing.get(k, 0)) or not math.isfinite(grid_spacing.get(k, 0)) for k in ("x", "y")):
            reporter.error(path, "grid layout.spacing must be a finite {x,y} object")
        check_axis_alignment(layout, path, "align", "x", False, reporter)
        check_axis_alignment(layout, path, "vAlign", "y", False, reporter)
    elif isinstance(spacing, str) and spacing not in LAYOUT_SPACING:
        reporter.error(path, "layout.spacing string must be even")
    elif ltype != "grid" and not isinstance(spacing, str) and not is_number(spacing):
        reporter.error(path, "layout.spacing must be a number or even")
    if "padding" in layout:
        padding = layout["padding"]
        if is_number(padding):
            pass
        elif isinstance(padding, dict):
            for key in ("x", "y"):
                if key in padding and not is_number(padding[key]):
                    reporter.error(path, f"layout.padding.{key} must be a number")
        else:
            reporter.error(path, "layout.padding must be a number or {x,y}")
    if isinstance(ltype, str) and ltype in LAYOUT_AXES:
        main_axis, cross_axis = LAYOUT_AXES[ltype]
        check_axis_alignment(layout, path, "align", main_axis, True, reporter)
        check_axis_alignment(layout, path, "vAlign", cross_axis, False, reporter)


def validate_tree(
    elem: Any,
    path: str,
    parent_layout_type: str | None,
    parent_role: str | None,
    reporter: Reporter,
    asset_index: dict[str, Path],
    duplicate_basenames: set[str],
    asset_borders: dict[str, dict[str, int | float]],
    seen_paths: set[str],
    stats: dict[str, int],
) -> None:
    if not isinstance(elem, dict):
        reporter.error(path, "element must be an object")
        return

    name = elem.get("name")
    if not isinstance(name, str) or not name:
        reporter.error(path, "name is required")
        name = "?"
    elif "/" in name or "\\" in name or name in {".", ".."}:
        reporter.error(
            path,
            "name must be a stable path segment; path separators and '.'/'..' "
            "are not allowed",
        )
    etype = elem.get("type")
    if etype not in ELEMENT_TYPES:
        reporter.error(path, f"type must be one of {sorted(ELEMENT_TYPES)}")
    role = elem.get("role")
    semantic_role: str | None = None
    if "role" in elem:
        if not isinstance(role, str) or role not in SEMANTIC_ROLES:
            reporter.error(path, "role must be list or listItem")
        else:
            semantic_role = role
            if role == "list":
                stats["lists"] += 1
            else:
                stats["list_items"] += 1
    if semantic_role == "listItem" and parent_role != "list":
        reporter.error(path, "role listItem requires a direct role list parent")
    if parent_role == "list" and semantic_role != "listItem":
        reporter.error(path, "direct children of role list must use role listItem")
    if "size" not in elem:
        reporter.error(path, "size is required")
    else:
        check_size(elem["size"], path, reporter)
    if "anchor" not in elem:
        reporter.error(path, "anchor is required")
    else:
        check_anchor(elem["anchor"], path, reporter)
        stats["anchored"] += 1

    if path in seen_paths:
        reporter.error(path, "duplicate element path")
    seen_paths.add(path)
    stats["elements"] += 1
    if etype == "text":
        stats["texts"] += 1
    if elem.get("layout"):
        stats["layouts"] += 1
    if "position" in elem:
        stats["positioned"] += 1
        check_xy_dict(elem["position"], path, "position", reporter)
    elif parent_layout_type is None and path != "root" and not (elem.get("align") or elem.get("vAlign") or "responsive" in elem):
        reporter.warn(path, "missing position outside layout/alignment derivation")

    if parent_layout_type == "row" and "align" in elem:
        reporter.warn(path, "child align is ignored in row layout; use vAlign or offset")
    if parent_layout_type == "column" and "vAlign" in elem:
        reporter.warn(path, "child vAlign is ignored in column layout; use align or offset")
    if parent_layout_type is not None and "position" in elem:
        reporter.warn(path, "position is ignored inside parent layout; use offset")

    if "align" in elem and elem["align"] not in ALIGN_X:
        reporter.warn(path, f"unknown align: {elem['align']}")
    if "vAlign" in elem and elem["vAlign"] not in ALIGN_Y:
        reporter.warn(path, f"unknown vAlign: {elem['vAlign']}")
    if "offset" in elem:
        check_xy_dict(elem["offset"], path, "offset", reporter)
    if "opacity" in elem:
        opacity = elem["opacity"]
        if not is_number(opacity) or opacity < 0 or opacity > 1:
            reporter.error(path, "opacity must be between 0 and 1")
    if "color" in elem:
        check_color(elem["color"], path, "color", reporter)
    if "hueShift" in elem:
        hue_shift = elem["hueShift"]
        if (
            not is_number(hue_shift)
            or (isinstance(hue_shift, float) and not math.isfinite(hue_shift))
            or hue_shift < -360
            or hue_shift > 360
        ):
            reporter.error(
                path, "hueShift must be a finite number between -360 and 360"
            )
    if etype == "overlay" and not (elem.get("asset") or elem.get("color")):
        reporter.error(path, "overlay must have color or asset")
    if etype == "rect":
        if not elem.get("color"):
            reporter.error(path, "rect must have color")
        if elem.get("asset"):
            reporter.warn(path, "rect should not use asset; use image for sliced sprites")

    asset_path: Path | None = None
    sprite_border: dict[str, int | float] | None = None
    asset = elem.get("asset")
    if asset:
        stats["asset_refs"] += 1
        if not isinstance(asset, str):
            reporter.error(path, "asset must be a string")
        else:
            key = asset_key(asset)
            asset_path = asset_index.get(key)
            sprite_border = asset_borders.get(key)
            if "/" not in key and key in duplicate_basenames:
                reporter.error(path, f"asset basename is ambiguous, use relative path: {asset}")
            elif key not in asset_index:
                reporter.error(path, f"asset not found: {asset}")
            nine_slice = elem.get("nineSlice")
            uses_metadata = nine_slice is True or (
                isinstance(nine_slice, str) and nine_slice in ("meta", "auto")
            )
            if uses_metadata and sprite_border is None:
                reporter.warn(path, f"nineSlice {nine_slice!r} has no Unity spriteBorder metadata; renderer will infer margins")
    elif etype == "image":
        reporter.warn(path, "image element has no asset")

    if "nineSlice" in elem:
        check_nine_slice(
            elem["nineSlice"],
            path,
            reporter,
            asset_path,
            sprite_border,
        )

    if etype == "text":
        text = elem.get("text")
        if not isinstance(text, str) or not text.strip():
            reporter.error(path, "text must be a non-empty string")
        if "fontSize" in elem and not positive_number(elem["fontSize"]):
            reporter.error(path, "fontSize must be positive")
        if "lineHeight" in elem and not positive_number(elem["lineHeight"]):
            reporter.error(path, "lineHeight must be positive")
        if "textScaleX" in elem:
            text_scale_x = elem["textScaleX"]
            if (
                not is_number(text_scale_x)
                or text_scale_x <= 0
                or (
                    isinstance(text_scale_x, float)
                    and not math.isfinite(text_scale_x)
                )
            ):
                reporter.error(
                    path, "textScaleX must be a positive finite number"
                )
        if "strokeWidth" in elem:
            stroke_width = elem["strokeWidth"]
            if not is_number(stroke_width) or stroke_width < 0:
                reporter.error(path, "strokeWidth must be a non-negative number")
        if "alignment" in elem and elem["alignment"] not in TEXT_ALIGN:
            reporter.warn(path, f"unknown text alignment: {elem['alignment']}")
        if "textVAlign" in elem and elem["textVAlign"] not in TEXT_VALIGN:
            reporter.warn(path, f"unknown textVAlign: {elem['textVAlign']}")

    layout_type = None
    if "layout" in elem:
        check_layout(elem["layout"], path, reporter)
        if isinstance(elem["layout"], dict):
            candidate_layout_type = elem["layout"].get("type")
            if (
                isinstance(candidate_layout_type, str)
                and candidate_layout_type in LAYOUT_TYPES
            ):
                layout_type = candidate_layout_type

    children = elem.get("children") or []
    if not isinstance(children, list):
        reporter.error(path, "children must be a list")
        return
    if semantic_role == "list":
        if etype != "container":
            reporter.error(path, "role list is only valid on container elements")
        if layout_type is None:
            reporter.error(path, "role list requires a row or column or grid layout")
        if not children:
            reporter.error(path, "role list requires at least one listItem child")
    if (
        path != "root"
        and etype in {"container", "button"}
        and not children
        and not (elem.get("asset") or elem.get("color"))
    ):
        reporter.error(
            path,
            f"leaf {etype} must have children or its own visual (asset or color)",
        )

    sibling_names: dict[str, int] = defaultdict(int)
    for child in children:
        if isinstance(child, dict):
            child_name = child.get("name")
            if isinstance(child_name, str):
                sibling_names[child_name] += 1
    for child_name, count in sibling_names.items():
        if count > 1:
            reporter.error(path, f"duplicate child name: {child_name}")

    for child in children:
        child_name = child.get("name", "?") if isinstance(child, dict) else "?"
        if layout_type == "grid" and isinstance(child, dict):
            cell = elem["layout"].get("cellSize")
            if isinstance(cell, dict) and child.get("size") != cell:
                reporter.error(path + "/" + child_name, "grid child size must match layout.cellSize; put differently sized artwork inside the cell")
            if "align" in child or "vAlign" in child:
                reporter.error(path + "/" + child_name, "grid owns cell alignment; align artwork inside the cell")
        validate_tree(
            child,
            f"{path}/{child_name}",
            layout_type,
            semantic_role,
            reporter,
            asset_index,
            duplicate_basenames,
            asset_borders,
            seen_paths,
            stats,
        )


def validate_structure(structure: dict[str, Any], design_path: Path | None, assets_dir: Path) -> tuple[Reporter, dict[str, int]]:
    reporter = Reporter()
    stats = {
        "anchored": 0,
        "elements": 0,
        "positioned": 0,
        "asset_refs": 0,
        "texts": 0,
        "layouts": 0,
        "lists": 0,
        "list_items": 0,
    }

    canvas = structure.get("canvas")
    if not isinstance(canvas, dict):
        reporter.error("canvas", "canvas object is required")
    else:
        for key in ("width", "height"):
            if key not in canvas or not positive_number(canvas[key]):
                reporter.error("canvas", f"{key} must be a positive number")
        if design_path is not None and design_path.exists() and positive_number(canvas.get("width")) and positive_number(canvas.get("height")):
            with Image.open(design_path) as img:
                if (int(canvas["width"]), int(canvas["height"])) != img.size:
                    reporter.error(
                        "canvas",
                        f"canvas {canvas['width']}x{canvas['height']} does not match design {img.width}x{img.height}",
                    )

    if "root" not in structure:
        reporter.error("root", "root object is required")
        return reporter, stats

    asset_index, duplicate_basenames, asset_borders = build_asset_index(
        assets_dir,
        reporter,
    )
    validate_tree(
        structure["root"],
        "root",
        None,
        None,
        reporter,
        asset_index,
        duplicate_basenames,
        asset_borders,
        set(),
        stats,
    )
    if not reporter.errors:
        try:
            components.validate_components(structure)
            for variant_path, merged, parent_layout, parent_role in components.state_patch_nodes(structure):
                validate_tree(
                    merged, variant_path, parent_layout, parent_role, reporter,
                    asset_index, duplicate_basenames, asset_borders, set(),
                    {key: 0 for key in stats},
                )
            import layout
            layout.resolve_positions(components.snapshot(structure))
        except (components.ComponentError, ValueError) as exc:
            reporter.error("components", str(exc))
    return reporter, stats


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--structure", required=True, help="Path to ui_structure.json")
    ap.add_argument("--design", required=True, help="Path to the source design image")
    ap.add_argument("--assets", required=True, help="Directory containing sliced PNG assets")
    ap.add_argument("--json", action="store_true", help="Print machine-readable validation result")
    ap.add_argument("--report", help="Path to write the full validation JSON report")
    ap.add_argument("--warnings-as-errors", action="store_true",
                    help="Exit non-zero when warnings are present")
    args = ap.parse_args()

    structure_path = Path(args.structure)
    design_path = Path(args.design)
    assets_dir = Path(args.assets)
    if not structure_path.exists():
        raise SystemExit(f"Structure not found: {structure_path}")
    if not design_path.exists():
        raise SystemExit(f"Design image not found: {design_path}")
    if not assets_dir.exists():
        raise SystemExit(f"Assets directory not found: {assets_dir}")

    structure = read_json(structure_path)
    reporter, stats = validate_structure(structure, design_path, assets_dir)
    result = {
        "valid": not reporter.errors and not (args.warnings_as_errors and reporter.warnings),
        "error_count": len(reporter.errors),
        "warning_count": len(reporter.warnings),
        "stats": dict(stats),
        "errors": reporter.errors,
        "warnings": reporter.warnings,
    }

    if args.report:
        report_path = Path(args.report)
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(
            json.dumps(result, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )

    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
    else:
        print(f"Validated {structure_path}")
        print(f"  elements: {stats['elements']}")
        print(f"  asset refs: {stats['asset_refs']}")
        print(f"  layouts: {stats['layouts']}")
        print(f"  lists: {stats['lists']}")
        print(f"  list items: {stats['list_items']}")
        print(f"  text elements: {stats['texts']}")
        print(f"  errors: {len(reporter.errors)}")
        print(f"  warnings: {len(reporter.warnings)}")
        if args.report:
            print(f"  report: {args.report}")
        for item in reporter.errors[:10]:
            print(f"  [error] {item}")
        if len(reporter.errors) > 10:
            print(f"  ... {len(reporter.errors) - 10} more errors in report")
        for item in reporter.warnings[:10]:
            print(f"  [warn] {item}")
        if len(reporter.warnings) > 10:
            print(f"  ... {len(reporter.warnings) - 10} more warnings in report")
        if result["valid"]:
            print("Structure is valid")

    sys.exit(0 if result["valid"] else 1)


if __name__ == "__main__":
    main()
