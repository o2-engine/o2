"""
Scale a ui_structure.json from one canvas size to another.

Use this when a draft structure was authored against a reduced screenshot
(for example 720x1560) but the design image used for verification is native
resolution (for example 1080x2340). The script scales layout coordinates,
sizes, offsets, padding, numeric layout spacing, and text sizes. It
intentionally does not scale nineSlice margins because those are source-asset
pixels.

Usage:
    py -B scale_structure.py \
        --structure ui_structure.json \
        --target-design design.png \
        --output ui_structure_native.json

Or pass --width/--height instead of --target-design.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

from PIL import Image


XY_FIELDS = {"position", "offset", "padding", "offsetMin", "offsetMax"}
SIZE_FIELDS = {"size", "cellSize"}
TEXT_FIELDS = {"fontSize", "lineHeight", "strokeWidth"}


def _scale_number(value: Any, scale: float) -> Any:
    if isinstance(value, bool):
        return value
    if isinstance(value, (int, float)):
        return int(round(float(value) * scale))
    return value


def _scale_axis_dict(obj: dict, sx: float, sy: float) -> dict:
    out = dict(obj)
    if "x" in out:
        out["x"] = _scale_number(out["x"], sx)
    if "y" in out:
        out["y"] = _scale_number(out["y"], sy)
    if "width" in out:
        out["width"] = _scale_number(out["width"], sx)
    if "height" in out:
        out["height"] = _scale_number(out["height"], sy)
    return out


def scale_structure(obj: Any, sx: float, sy: float,
                    context: str | None = None) -> Any:
    if isinstance(obj, list):
        return [scale_structure(v, sx, sy, context) for v in obj]

    if isinstance(obj, dict):
        if context in XY_FIELDS | SIZE_FIELDS:
            return _scale_axis_dict(obj, sx, sy)

        if context == "safeArea":
            return {key: _scale_number(value, sx if key in {"left", "right"} else sy)
                    for key, value in obj.items()}
        out = {}
        layout_type = obj.get("type", "row") if context == "layout" else None
        for key, value in obj.items():
            if key == "nineSlice":
                out[key] = value
            elif context == "layout" and key == "spacing":
                if layout_type == "grid" and isinstance(value, dict):
                    out[key] = _scale_axis_dict(value, sx, sy)
                elif layout_type == "column":
                    out[key] = _scale_number(value, sy)
                elif layout_type == "row":
                    out[key] = _scale_number(value, sx)
                else:
                    out[key] = _scale_number(value, (sx + sy) / 2.0)
            elif key in TEXT_FIELDS:
                out[key] = _scale_number(value, (sx + sy) / 2.0)
            else:
                out[key] = scale_structure(value, sx, sy, key)
        return out

    return obj


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--structure", required=True, help="Input ui_structure.json")
    ap.add_argument("--output", required=True, help="Output scaled JSON")
    target = ap.add_mutually_exclusive_group(required=True)
    target.add_argument("--target-design", help="Design image whose size is the target canvas")
    target.add_argument("--width", type=int, help="Target canvas width")
    ap.add_argument("--height", type=int, help="Target canvas height when --width is used")
    args = ap.parse_args()

    in_path = Path(args.structure)
    out_path = Path(args.output)
    structure = json.loads(in_path.read_text(encoding="utf-8"))

    canvas = structure.get("canvas") or {}
    src_w = int(canvas.get("width", 0))
    src_h = int(canvas.get("height", 0))
    if src_w <= 0 or src_h <= 0:
        raise SystemExit("Input structure must have canvas.width and canvas.height")

    if args.target_design:
        with Image.open(args.target_design) as img:
            dst_w, dst_h = img.size
    else:
        if not args.height:
            raise SystemExit("--height is required when --width is used")
        dst_w, dst_h = args.width, args.height

    sx = dst_w / src_w
    sy = dst_h / src_h
    scaled = scale_structure(structure, sx, sy)
    scaled.setdefault("canvas", {})
    scaled["canvas"]["width"] = dst_w
    scaled["canvas"]["height"] = dst_h
    if "root" in scaled:
        scaled["root"].setdefault("position", {"x": 0, "y": 0})
        scaled["root"]["size"] = {"width": dst_w, "height": dst_h}

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(scaled, ensure_ascii=False, indent=2) + "\n",
                        encoding="utf-8")
    print(f"Scaled {in_path} -> {out_path}")
    print(f"  {src_w}x{src_h} -> {dst_w}x{dst_h}")
    print(f"  scale: x={sx:.4f}, y={sy:.4f}")


if __name__ == "__main__":
    main()
