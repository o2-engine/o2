"""Audit same-pass render diagnostics and build focused visual evidence.

The audit never renders UI elements again. It reads ``render_trace.json`` and
crops the existing design/reconstruction images only for suspicious elements.
"""

from __future__ import annotations

import argparse
import json
import math
import re
import sys
import time
from pathlib import Path
from typing import Any

from PIL import Image, ImageChops, ImageDraw, ImageFont


ASPECT_WARNING_THRESHOLD = 0.08
PARENT_OVERFLOW_WARNING_PX = 4
TEXT_OVERFLOW_ERROR_PX = 1
MAX_EVIDENCE_ITEMS = 12
SCRIM_NAME_TOKENS = {"backdrop", "dim", "dimmer", "mask", "overlay", "scrim"}


def read_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise SystemExit(f"Invalid JSON: {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise SystemExit(f"JSON root must be an object: {path}")
    return value


def inventory_usage(inventory: dict[str, Any]) -> dict[str, str]:
    usage: dict[str, str] = {}
    basename_counts: dict[str, int] = {}
    for item in inventory.get("assets", []):
        if not isinstance(item, dict):
            continue
        basename = str(item.get("basename", "")).lower()
        basename_counts[basename] = basename_counts.get(basename, 0) + 1
    for item in inventory.get("assets", []):
        if not isinstance(item, dict):
            continue
        path = str(item.get("path", "")).replace("\\", "/").lower()
        basename = str(item.get("basename", "")).lower()
        kind = str(item.get("likely_usage", "image"))
        if path:
            usage[path] = kind
        if basename and basename_counts.get(basename) == 1:
            usage[basename] = kind
    return usage


def issue(
    severity: str,
    code: str,
    item: dict[str, Any],
    message: str,
    details: dict[str, Any] | None = None,
) -> dict[str, Any]:
    result = {
        "severity": severity,
        "code": code,
        "path": item.get("path", "?"),
        "bbox": item.get("bbox"),
        "message": message,
    }
    if details:
        result["details"] = details
    return result


def bbox_overflow(inner: list[int], outer: list[int]) -> dict[str, int]:
    ix, iy, iw, ih = inner
    ox, oy, ow, oh = outer
    return {
        "left": max(0, ox - ix),
        "top": max(0, oy - iy),
        "right": max(0, ix + iw - (ox + ow)),
        "bottom": max(0, iy + ih - (oy + oh)),
    }


def actual_visible_bbox(item: dict[str, Any]) -> list[int | float] | None:
    bbox = item.get("visible_bbox")
    if not isinstance(bbox, list) or len(bbox) != 4:
        return None
    if any(
        not isinstance(value, (int, float))
        or isinstance(value, bool)
        or not math.isfinite(value)
        for value in bbox
    ):
        return None
    if bbox[2] <= 0 or bbox[3] <= 0:
        return None
    return bbox


def trace_canvas_bbox(trace: dict[str, Any]) -> list[int | float] | None:
    canvas = trace.get("canvas")
    if not isinstance(canvas, dict):
        return None
    width = canvas.get("width")
    height = canvas.get("height")
    if (
        not isinstance(width, (int, float))
        or isinstance(width, bool)
        or not math.isfinite(width)
        or width <= 0
        or not isinstance(height, (int, float))
        or isinstance(height, bool)
        or not math.isfinite(height)
        or height <= 0
    ):
        return None
    return [0, 0, width, height]


def covers_canvas(
    bbox: list[int | float], canvas_bbox: list[int | float] | None
) -> bool:
    if canvas_bbox is None:
        return False
    x, y, width, height = bbox
    canvas_x, canvas_y, canvas_width, canvas_height = canvas_bbox
    return (
        x <= canvas_x
        and y <= canvas_y
        and x + width >= canvas_x + canvas_width
        and y + height >= canvas_y + canvas_height
    )


def layer_order(item: dict[str, Any], trace_index: int) -> tuple[float, int]:
    raw_order = item.get("z_order")
    if (
        isinstance(raw_order, (int, float))
        and not isinstance(raw_order, bool)
        and math.isfinite(raw_order)
    ):
        return float(raw_order), trace_index
    return float(trace_index), trace_index


def rectangles_fully_cover(
    target: list[int | float], covers: list[list[int | float]]
) -> bool:
    """Return whether opaque rectangles cover every point in target."""
    x, y, width, height = target
    remaining = [(x, y, x + width, y + height)]
    for cover in covers:
        cover_x, cover_y, cover_width, cover_height = cover
        cover_left = cover_x
        cover_top = cover_y
        cover_right = cover_x + cover_width
        cover_bottom = cover_y + cover_height
        next_remaining = []
        for left, top, right, bottom in remaining:
            intersection_left = max(left, cover_left)
            intersection_top = max(top, cover_top)
            intersection_right = min(right, cover_right)
            intersection_bottom = min(bottom, cover_bottom)
            if (
                intersection_right <= intersection_left
                or intersection_bottom <= intersection_top
            ):
                next_remaining.append((left, top, right, bottom))
                continue
            if top < intersection_top:
                next_remaining.append((left, top, right, intersection_top))
            if intersection_bottom < bottom:
                next_remaining.append((left, intersection_bottom, right, bottom))
            if left < intersection_left:
                next_remaining.append((
                    left,
                    intersection_top,
                    intersection_left,
                    intersection_bottom,
                ))
            if intersection_right < right:
                next_remaining.append((
                    intersection_right,
                    intersection_top,
                    right,
                    intersection_bottom,
                ))
        remaining = next_remaining
        if not remaining:
            return True
    return False


def is_full_canvas_scrim(
    item: dict[str, Any], canvas_bbox: list[int | float] | None
) -> bool:
    visible_bbox = actual_visible_bbox(item)
    if visible_bbox is None or not covers_canvas(visible_bbox, canvas_bbox):
        return False

    raw_identity = f"{item.get('name', '')} {item.get('path', '')}"
    identity = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", raw_identity).casefold()
    tokens = set(re.findall(r"[a-z0-9]+", identity))
    opacity = item.get("opacity")
    translucent = (
        isinstance(opacity, (int, float))
        and not isinstance(opacity, bool)
        and math.isfinite(opacity)
        and 0 <= opacity < 1
    )
    return (
        item.get("type") == "overlay"
        or bool(tokens & SCRIM_NAME_TOKENS)
        or (item.get("type") == "rect" and translucent)
    )


def audit_trace(
    trace: dict[str, Any], inventory: dict[str, Any]
) -> dict[str, Any]:
    usage = inventory_usage(inventory)
    issues: list[dict[str, Any]] = []
    visible_items: list[dict[str, Any]] = []

    for item in trace.get("elements", []):
        if not isinstance(item, dict):
            continue
        if actual_visible_bbox(item) is not None:
            visible_items.append(item)
        asset = item.get("asset")
        if isinstance(asset, dict):
            if not asset.get("found", False):
                issues.append(issue(
                    "error",
                    "asset_missing",
                    item,
                    f"Asset could not be rendered: {asset.get('requested', '?')}",
                ))
            resolved = str(asset.get("resolved") or asset.get("requested") or "")
            kind = usage.get(resolved.replace("\\", "/").lower(), "image")
            aspect_error = float(asset.get("aspect_scale_error", 0) or 0)
            if (
                asset.get("render_mode") == "stretch"
                and kind in {"icon", "portrait"}
                and aspect_error > ASPECT_WARNING_THRESHOLD
            ):
                issues.append(issue(
                    "warning",
                    "atomic_asset_aspect",
                    item,
                    f"{kind} is non-uniformly scaled by {aspect_error:.1%}.",
                    {"asset": resolved, "likely_usage": kind,
                     "aspect_scale_error": aspect_error},
                ))

        text = item.get("text")
        if isinstance(text, dict):
            font = text.get("font") or {}
            if isinstance(font, dict) and font.get("requested") and font.get("fallback"):
                issues.append(issue(
                    "error",
                    "font_fallback",
                    item,
                    f"Requested font {font['requested']!r} resolved to "
                    f"{font.get('resolved') or 'the bitmap fallback'}.",
                    {"font": font},
                ))
            overflow = text.get("ink_overflow") or {}
            max_overflow = max(
                (int(value or 0) for value in overflow.values()),
                default=0,
            )
            if max_overflow > TEXT_OVERFLOW_ERROR_PX:
                issues.append(issue(
                    "error",
                    "text_ink_overflow",
                    item,
                    f"Visible text ink exceeds its box by up to {max_overflow}px.",
                    {"ink_bbox": text.get("ink_bbox"), "overflow": overflow},
                ))

        bbox = item.get("bbox")
        parent_bbox = item.get("parent_bbox")
        if (
            isinstance(bbox, list) and len(bbox) == 4
            and isinstance(parent_bbox, list) and len(parent_bbox) == 4
        ):
            overflow = bbox_overflow(bbox, parent_bbox)
            max_overflow = max(overflow.values())
            if (max_overflow > PARENT_OVERFLOW_WARNING_PX
                    and not item.get("scroll_content") and not item.get("hidden")):
                issues.append(issue(
                    "warning",
                    "parent_overflow",
                    item,
                    f"Element box extends {max_overflow}px beyond its parent.",
                    {"parent_bbox": parent_bbox, "overflow": overflow},
                ))

    canvas_bbox = trace_canvas_bbox(trace)
    ordered_visible_items = [
        (item, layer_order(item, index))
        for index, item in enumerate(visible_items)
    ]
    unoccluded_visible_items = []
    fully_occluded_items = []
    for candidate, candidate_order in ordered_visible_items:
        candidate_bbox = actual_visible_bbox(candidate)
        later_opaque_bboxes = [
            bbox
            for later, later_order in ordered_visible_items
            if later_order > candidate_order
            and later.get("fully_opaque") is True
            and (bbox := actual_visible_bbox(later)) is not None
        ]
        if (
            candidate_bbox is not None
            and rectangles_fully_cover(candidate_bbox, later_opaque_bboxes)
        ):
            fully_occluded_items.append(candidate)
        else:
            unoccluded_visible_items.append(candidate)

    foreground_items = [
        item for item in unoccluded_visible_items
        if not is_full_canvas_scrim(item, canvas_bbox)
    ]
    if not foreground_items:
        excluded_scrims = [
            item.get("path", "?") for item in unoccluded_visible_items
            if is_full_canvas_scrim(item, canvas_bbox)
        ]
        issues.append(issue(
            "error",
            "no_visible_foreground",
            {"path": "root", "bbox": canvas_bbox},
            "Render trace contains no actually visible foreground; logical "
            "nodes and full-canvas scrims do not count.",
            {
                "visible_bbox_count": len(visible_items),
                "fully_occluded_count": len(fully_occluded_items),
                "excluded_fully_occluded": [
                    item.get("path", "?") for item in fully_occluded_items
                ],
                "excluded_full_canvas_scrims": excluded_scrims,
            },
        ))

    error_count = sum(item["severity"] == "error" for item in issues)
    warning_count = sum(item["severity"] == "warning" for item in issues)
    return {
        "valid": error_count == 0,
        "error_count": error_count,
        "warning_count": warning_count,
        "issue_count": len(issues),
        "thresholds": {
            "atomic_asset_aspect": ASPECT_WARNING_THRESHOLD,
            "parent_overflow_px": PARENT_OVERFLOW_WARNING_PX,
            "text_overflow_px": TEXT_OVERFLOW_ERROR_PX,
        },
        "issues": issues,
    }


def find_font(size: int) -> ImageFont.ImageFont:
    for candidate in (
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    ):
        if Path(candidate).is_file():
            try:
                return ImageFont.truetype(candidate, size)
            except OSError:
                pass
    return ImageFont.load_default()


def evidence_crop_box(bbox: list[int] | None, size: tuple[int, int]) -> tuple[int, int, int, int]:
    width, height = size
    if not isinstance(bbox, list) or len(bbox) != 4:
        return (0, 0, width, height)
    x, y, w, h = bbox
    cx, cy = x + w / 2, y + h / 2
    crop_w = min(width, max(120, w + 48))
    crop_h = min(height, max(100, h + 48))
    left = max(0, min(width - crop_w, int(round(cx - crop_w / 2))))
    top = max(0, min(height - crop_h, int(round(cy - crop_h / 2))))
    return (left, top, left + crop_w, top + crop_h)


def fit_panel(image: Image.Image, size: tuple[int, int]) -> Image.Image:
    panel = Image.new("RGB", size, "white")
    copy = image.convert("RGB")
    copy.thumbnail(size, Image.Resampling.LANCZOS)
    panel.paste(copy, ((size[0] - copy.width) // 2, (size[1] - copy.height) // 2))
    return panel


def fit_text(draw: ImageDraw.ImageDraw, value: str, font: ImageFont.ImageFont,
             max_width: int) -> str:
    if draw.textlength(value, font=font) <= max_width:
        return value
    suffix = "..."
    shortened = value
    while shortened and draw.textlength(shortened + suffix, font=font) > max_width:
        shortened = shortened[:-1]
    return shortened + suffix


def build_evidence_sheet(
    design_path: Path,
    reconstruction_path: Path,
    issues: list[dict[str, Any]],
    output_path: Path,
) -> None:
    design = Image.open(design_path).convert("RGBA")
    reconstruction = Image.open(reconstruction_path).convert("RGBA")
    if reconstruction.size != design.size:
        reconstruction = reconstruction.resize(design.size, Image.Resampling.LANCZOS)

    selected = issues[:MAX_EVIDENCE_ITEMS]
    panel_size = (240, 150)
    row_height = 202
    sheet_width = panel_size[0] * 3
    if not selected:
        sheet = Image.new("RGB", (sheet_width, 100), (248, 248, 248))
        ImageDraw.Draw(sheet).text(
            (20, 36), "Visual audit: no automatic suspects", fill=(25, 25, 25),
            font=find_font(18),
        )
    else:
        sheet = Image.new(
            "RGB", (sheet_width, row_height * len(selected)), (245, 245, 245)
        )
        draw = ImageDraw.Draw(sheet)
        title_font = find_font(15)
        label_font = find_font(12)
        for row, item in enumerate(selected):
            y = row * row_height
            crop_box = evidence_crop_box(item.get("bbox"), design.size)
            design_crop = design.crop(crop_box)
            reconstruction_crop = reconstruction.crop(crop_box)
            diff_crop = ImageChops.difference(design_crop, reconstruction_crop)
            for column, (label, crop) in enumerate((
                ("Design", design_crop),
                ("Reconstruction", reconstruction_crop),
                ("Absolute diff", diff_crop),
            )):
                x = column * panel_size[0]
                sheet.paste(fit_panel(crop, panel_size), (x, y + 38))
                draw.text((x + 8, y + 22), label, fill=(70, 70, 70), font=label_font)
            title = f"{item['severity'].upper()} {item['code']}: {item['path']}"
            draw.text(
                (8, y + 3),
                fit_text(draw, title, title_font, sheet_width - 16),
                fill=(20, 20, 20),
                font=title_font,
            )
            draw.line((0, y + row_height - 1, sheet_width, y + row_height - 1),
                      fill=(205, 205, 205), width=1)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(output_path)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", required=True)
    parser.add_argument("--inventory", required=True)
    parser.add_argument("--design", required=True)
    parser.add_argument("--reconstruction", required=True)
    parser.add_argument("--output", required=True, help="visual_audit.json path")
    parser.add_argument("--evidence", help="Optional focused comparison sheet")
    parser.add_argument("--fail-on-error", action="store_true")
    args = parser.parse_args()

    started = time.perf_counter()
    trace_path = Path(args.trace)
    inventory_path = Path(args.inventory)
    design_path = Path(args.design)
    reconstruction_path = Path(args.reconstruction)
    for path, label in (
        (trace_path, "Render trace"),
        (inventory_path, "Asset inventory"),
        (design_path, "Design"),
        (reconstruction_path, "Reconstruction"),
    ):
        if not path.is_file():
            raise SystemExit(f"{label} not found: {path}")

    result = audit_trace(read_json(trace_path), read_json(inventory_path))
    if args.evidence:
        build_evidence_sheet(
            design_path, reconstruction_path, result["issues"], Path(args.evidence)
        )
        result["evidence"] = str(Path(args.evidence))
    result["elapsed_ms"] = round((time.perf_counter() - started) * 1000, 2)
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(
        json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    print(
        f"Visual audit: {result['error_count']} errors, "
        f"{result['warning_count']} warnings ({result['elapsed_ms']} ms)"
    )
    print(f"Report: {output_path}")
    if args.fail_on_error and not result["valid"]:
        sys.exit(1)


if __name__ == "__main__":
    main()
