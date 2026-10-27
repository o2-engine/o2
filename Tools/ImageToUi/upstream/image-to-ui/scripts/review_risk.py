"""Prioritize UI review using visual salience and render residuals.

The scores in this module decide how much review evidence a rendered node
needs. They never change geometry and a high residual is not a correctness
failure by itself.
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Any

import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageEnhance, ImageFont


REPORT_VERSION = 1
MEDIUM_RISK_THRESHOLD = 0.38
HIGH_RISK_THRESHOLD = 0.62
RESIDUAL_NORMALIZATION = 0.33
VISUAL_TYPES = {"image", "text", "rect", "overlay"}
MAX_EXTRA_MEDIUM_EVIDENCE = 12


def clamp01(value: float) -> float:
    return max(0.0, min(1.0, float(value)))


def finite_number(value: Any, default: float = 0.0) -> float:
    if (
        isinstance(value, (int, float))
        and not isinstance(value, bool)
        and math.isfinite(value)
    ):
        return float(value)
    return default


def normalized_bbox(
    value: Any, canvas_size: tuple[int, int]
) -> tuple[int, int, int, int] | None:
    if not isinstance(value, list) or len(value) != 4:
        return None
    x, y, width, height = (finite_number(item) for item in value)
    if width <= 0 or height <= 0:
        return None
    canvas_width, canvas_height = canvas_size
    left = max(0, min(canvas_width, math.floor(x)))
    top = max(0, min(canvas_height, math.floor(y)))
    right = max(left, min(canvas_width, math.ceil(x + width)))
    bottom = max(top, min(canvas_height, math.ceil(y + height)))
    if right <= left or bottom <= top:
        return None
    return left, top, right - left, bottom - top


def item_visible_bbox(
    item: dict[str, Any], canvas_size: tuple[int, int]
) -> tuple[int, int, int, int] | None:
    visible = normalized_bbox(item.get("visible_bbox"), canvas_size)
    if visible is not None:
        return visible
    if item.get("type") in VISUAL_TYPES:
        return normalized_bbox(item.get("bbox"), canvas_size)
    return None


def load_json(path: Path, label: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f"Cannot read {label} {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise ValueError(f"{label} must be a JSON object: {path}")
    return value


def write_json(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(value, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def canvas_size_from_trace(
    trace: dict[str, Any], design: Image.Image
) -> tuple[int, int]:
    canvas = trace.get("canvas")
    if isinstance(canvas, dict):
        width = int(finite_number(canvas.get("width")))
        height = int(finite_number(canvas.get("height")))
        if width > 0 and height > 0:
            if (width, height) != design.size:
                raise ValueError(
                    "Trace canvas does not match the native design size: "
                    f"{(width, height)} != {design.size}"
                )
            return width, height
    return design.size


def composite_reconstruction(
    design: Image.Image, reconstruction: Image.Image
) -> Image.Image:
    if reconstruction.size != design.size:
        raise ValueError(
            "Reconstruction does not match the native design size: "
            f"{reconstruction.size} != {design.size}"
        )
    design_rgb = design.convert("RGB")
    reconstruction_rgba = reconstruction.convert("RGBA")
    alpha = np.asarray(reconstruction_rgba.getchannel("A"), dtype=np.float32)
    if np.all(alpha >= 255):
        return reconstruction_rgba.convert("RGB")
    foreground = np.asarray(
        reconstruction_rgba.convert("RGB"), dtype=np.float32
    )
    background = np.asarray(design_rgb, dtype=np.float32)
    alpha = alpha[..., np.newaxis] / 255.0
    composited = foreground * alpha + background * (1.0 - alpha)
    return Image.fromarray(
        np.clip(np.rint(composited), 0, 255).astype(np.uint8), mode="RGB"
    )


def luminance(array: np.ndarray) -> np.ndarray:
    rgb = array.astype(np.float32) / 255.0
    return (
        rgb[..., 0] * 0.2126
        + rgb[..., 1] * 0.7152
        + rgb[..., 2] * 0.0722
    )


def edge_map(gray: np.ndarray) -> np.ndarray:
    horizontal = np.zeros_like(gray)
    vertical = np.zeros_like(gray)
    if gray.shape[1] > 1:
        horizontal[:, 1:] = np.abs(np.diff(gray, axis=1))
    if gray.shape[0] > 1:
        vertical[1:, :] = np.abs(np.diff(gray, axis=0))
    return np.clip(np.hypot(horizontal, vertical), 0.0, 1.0)


def contrast_score(
    reconstruction: np.ndarray,
    bbox: tuple[int, int, int, int],
) -> float:
    x, y, width, height = bbox
    canvas_height, canvas_width = reconstruction.shape[:2]
    margin = max(4, min(64, int(round(max(width, height) * 0.12))))
    left = max(0, x - margin)
    top = max(0, y - margin)
    right = min(canvas_width, x + width + margin)
    bottom = min(canvas_height, y + height + margin)
    region = luminance(reconstruction[top:bottom, left:right])
    inner_left = x - left
    inner_top = y - top
    inner_right = inner_left + width
    inner_bottom = inner_top + height
    mask = np.ones(region.shape, dtype=bool)
    mask[inner_top:inner_bottom, inner_left:inner_right] = False
    inside = region[inner_top:inner_bottom, inner_left:inner_right]
    outside = region[mask]
    if inside.size == 0:
        return 0.0
    if outside.size:
        reference = float(np.median(outside))
        difference = float(np.mean(np.abs(inside - reference)))
    else:
        difference = float(np.std(inside))
    return clamp01(difference / 0.35)


def residual_metrics(
    design: np.ndarray,
    reconstruction: np.ndarray,
    bbox: tuple[int, int, int, int],
) -> dict[str, float]:
    x, y, width, height = bbox
    design_crop = design[y:y + height, x:x + width].astype(np.float32) / 255.0
    reconstruction_crop = (
        reconstruction[y:y + height, x:x + width].astype(np.float32) / 255.0
    )
    absolute = np.abs(design_crop - reconstruction_crop)
    rgb_mae = float(np.mean(absolute))
    design_luminance = luminance(
        np.clip(np.rint(design_crop * 255), 0, 255).astype(np.uint8)
    )
    reconstruction_luminance = luminance(
        np.clip(np.rint(reconstruction_crop * 255), 0, 255).astype(np.uint8)
    )
    luminance_mae = float(
        np.mean(np.abs(design_luminance - reconstruction_luminance))
    )
    edge_mae = float(np.mean(np.abs(
        edge_map(design_luminance) - edge_map(reconstruction_luminance)
    )))
    changed_fraction = float(np.mean(np.max(absolute, axis=2) >= 0.12))
    raw = (
        rgb_mae * 0.30
        + luminance_mae * 0.25
        + edge_mae * 0.25
        + changed_fraction * 0.20
    )
    return {
        "rgb_mae": round(rgb_mae, 4),
        "luminance_mae": round(luminance_mae, 4),
        "edge_mae": round(edge_mae, 4),
        "changed_fraction": round(changed_fraction, 4),
        "raw": round(raw, 4),
        "score": round(clamp01(raw / RESIDUAL_NORMALIZATION), 4),
    }


def salience_metrics(
    item: dict[str, Any],
    bbox: tuple[int, int, int, int],
    canvas_size: tuple[int, int],
    reconstruction: np.ndarray,
    max_z_order: float,
) -> dict[str, float]:
    _x, _y, width, height = bbox
    canvas_width, canvas_height = canvas_size
    area_ratio = width * height / max(1, canvas_width * canvas_height)
    area_score = clamp01(math.sqrt(area_ratio / 0.06))
    text = item.get("text")
    font_size = (
        finite_number(text.get("font_size"))
        if isinstance(text, dict)
        else 0.0
    )
    font_score = clamp01(
        font_size / max(1.0, min(canvas_size) * 0.055)
    )
    foreground_score = clamp01(
        finite_number(item.get("z_order")) / max(1.0, max_z_order)
    )
    local_contrast = contrast_score(reconstruction, bbox)
    score = clamp01(
        area_score * 0.35
        + font_score * 0.30
        + local_contrast * 0.20
        + foreground_score * 0.15
    )
    return {
        "area_ratio": round(area_ratio, 6),
        "area_score": round(area_score, 4),
        "font_score": round(font_score, 4),
        "contrast_score": round(local_contrast, 4),
        "foreground_score": round(foreground_score, 4),
        "score": round(score, 4),
    }


def reason_codes(
    salience: dict[str, float], residual: dict[str, float]
) -> list[str]:
    reasons: list[str] = []
    if salience["area_score"] >= 0.65:
        reasons.append("salience.large-visible-area")
    if salience["font_score"] >= 0.65:
        reasons.append("salience.large-text")
    if salience["contrast_score"] >= 0.65:
        reasons.append("salience.high-local-contrast")
    if salience["foreground_score"] >= 0.75:
        reasons.append("salience.foreground-layer")
    if residual["rgb_mae"] >= 0.14:
        reasons.append("residual.color")
    if residual["luminance_mae"] >= 0.14:
        reasons.append("residual.luminance")
    if residual["edge_mae"] >= 0.12:
        reasons.append("residual.edges")
    if residual["changed_fraction"] >= 0.35:
        reasons.append("residual.changed-area")
    if not reasons:
        reasons.append(
            "salience.review-priority"
            if salience["score"] >= residual["score"]
            else "residual.review-priority"
        )
    return reasons


def risk_level(score: float) -> str:
    if score >= HIGH_RISK_THRESHOLD:
        return "high"
    if score >= MEDIUM_RISK_THRESHOLD:
        return "medium"
    return "low"


def analyze(
    trace: dict[str, Any],
    design: Image.Image,
    reconstruction: Image.Image,
) -> list[dict[str, Any]]:
    canvas_size = canvas_size_from_trace(trace, design)
    composited = composite_reconstruction(design, reconstruction)
    design_array = np.asarray(design.convert("RGB"), dtype=np.uint8)
    reconstruction_array = np.asarray(composited, dtype=np.uint8)
    elements = [
        item for item in trace.get("elements", [])
        if isinstance(item, dict)
    ]
    max_z_order = max(
        (finite_number(item.get("z_order")) for item in elements),
        default=1.0,
    )
    items: list[dict[str, Any]] = []
    for item in elements:
        path = item.get("path")
        if not isinstance(path, str) or not path:
            continue
        bbox = item_visible_bbox(item, canvas_size)
        if bbox is None:
            continue
        salience = salience_metrics(
            item, bbox, canvas_size, reconstruction_array, max_z_order
        )
        residual = residual_metrics(
            design_array, reconstruction_array, bbox
        )
        score = clamp01(
            salience["score"] * 0.40
            + residual["score"] * 0.45
            + min(salience["score"], residual["score"]) * 0.15
        )
        level = risk_level(score)
        items.append({
            "path": path,
            "name": item.get("name"),
            "type": item.get("type"),
            "bbox": list(bbox),
            "risk_level": level,
            "risk_score": round(score, 4),
            "reasons": reason_codes(salience, residual),
            "required_evidence": (
                "focused"
                if level == "high"
                else "focused-recommended"
                if level == "medium"
                else "overview"
            ),
            "salience": salience,
            "residual": residual,
            "_trace_item": item,
        })
    return sorted(items, key=lambda item: (-item["risk_score"], item["path"]))


def evidence_crop_box(
    bbox: list[int], canvas_size: tuple[int, int]
) -> tuple[int, int, int, int]:
    x, y, width, height = bbox
    canvas_width, canvas_height = canvas_size
    padding = max(8, min(80, int(round(max(width, height) * 0.18))))
    return (
        max(0, x - padding),
        max(0, y - padding),
        min(canvas_width, x + width + padding),
        min(canvas_height, y + height + padding),
    )


def find_font(size: int) -> ImageFont.ImageFont:
    for candidate in (
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    ):
        path = Path(candidate)
        if path.is_file():
            try:
                return ImageFont.truetype(str(path), size)
            except OSError:
                pass
    return ImageFont.load_default()


def fit_panel(image: Image.Image, size: tuple[int, int]) -> Image.Image:
    panel = Image.new("RGB", size, (28, 31, 35))
    copy = image.convert("RGB")
    copy.thumbnail(size, Image.Resampling.LANCZOS)
    panel.paste(
        copy,
        ((size[0] - copy.width) // 2, (size[1] - copy.height) // 2),
    )
    return panel


def outlined_crop(
    image: Image.Image,
    crop_box: tuple[int, int, int, int],
    bbox: list[int],
    color: tuple[int, int, int],
    line_boxes: list[Any] | None = None,
) -> Image.Image:
    crop = image.crop(crop_box).convert("RGB")
    draw = ImageDraw.Draw(crop)
    x, y, width, height = bbox
    left = x - crop_box[0]
    top = y - crop_box[1]
    draw.rectangle(
        (left, top, left + width - 1, top + height - 1),
        outline=color,
        width=2,
    )
    for raw in line_boxes or []:
        if not isinstance(raw, list) or len(raw) != 4:
            continue
        line = normalized_bbox(raw, image.size)
        if line is None:
            continue
        line_x, line_y, line_width, line_height = line
        draw.rectangle(
            (
                line_x - crop_box[0],
                line_y - crop_box[1],
                line_x - crop_box[0] + line_width - 1,
                line_y - crop_box[1] + line_height - 1,
            ),
            outline=(102, 255, 153),
            width=2,
        )
    return crop


def build_evidence(
    design: Image.Image,
    reconstruction: Image.Image,
    items: list[dict[str, Any]],
    output_path: Path,
    legend_path: Path,
) -> set[str]:
    canvas_size = design.size
    composited = composite_reconstruction(design, reconstruction)
    high = [item for item in items if item["risk_level"] == "high"]
    medium = [
        item for item in items if item["risk_level"] == "medium"
    ][:MAX_EXTRA_MEDIUM_EVIDENCE]
    selected = high + medium
    panel_size = (360, 190)
    row_height = 232
    sheet_width = panel_size[0] * 3
    title_font = find_font(15)
    label_font = find_font(12)
    if not selected:
        sheet = Image.new("RGB", (sheet_width, 100), (245, 245, 245))
        ImageDraw.Draw(sheet).text(
            (20, 38),
            "Review risk: no medium/high-priority rendered nodes",
            fill=(25, 25, 25),
            font=title_font,
        )
    else:
        sheet = Image.new(
            "RGB",
            (sheet_width, row_height * len(selected)),
            (242, 242, 242),
        )
        draw = ImageDraw.Draw(sheet)
        for row, item in enumerate(selected):
            row_y = row * row_height
            crop_box = evidence_crop_box(item["bbox"], canvas_size)
            trace_item = item["_trace_item"]
            text = trace_item.get("text")
            line_boxes = (
                text.get("line_ink_bboxes")
                if isinstance(text, dict)
                else None
            )
            design_crop = outlined_crop(
                design, crop_box, item["bbox"], (255, 143, 23)
            )
            reconstruction_crop = outlined_crop(
                composited,
                crop_box,
                item["bbox"],
                (35, 200, 255),
                line_boxes,
            )
            difference_crop = ImageChops.difference(
                design.crop(crop_box).convert("RGB"),
                composited.crop(crop_box).convert("RGB"),
            )
            difference_crop = ImageEnhance.Brightness(
                difference_crop
            ).enhance(2.5)
            for column, (label, crop) in enumerate((
                ("Design", design_crop),
                ("Reconstruction", reconstruction_crop),
                ("Residual x2.5", difference_crop),
            )):
                panel_x = column * panel_size[0]
                sheet.paste(
                    fit_panel(crop, panel_size),
                    (panel_x, row_y + 38),
                )
                draw.text(
                    (panel_x + 8, row_y + 22),
                    label,
                    fill=(65, 65, 65),
                    font=label_font,
                )
            title = (
                f"{item['risk_level'].upper()} {item['risk_score']:.3f} "
                f"{item['path']} | {', '.join(item['reasons'][:3])}"
            )
            draw.text(
                (8, row_y + 3),
                title[:145],
                fill=(25, 25, 25),
                font=title_font,
            )
            draw.line(
                (0, row_y + row_height - 1, sheet_width, row_y + row_height - 1),
                fill=(200, 200, 200),
                width=1,
            )
    output_path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(output_path)
    covered = {item["path"] for item in selected}
    write_json(legend_path, {
        "version": REPORT_VERSION,
        "image": str(output_path),
        "targets": [{
            "path": item["path"],
            "bbox": item["bbox"],
            "risk_level": item["risk_level"],
            "risk_score": item["risk_score"],
            "reasons": item["reasons"],
        } for item in selected],
    })
    return covered


def build_report(
    design_path: Path,
    reconstruction_path: Path,
    evidence_path: Path,
    legend_path: Path,
    items: list[dict[str, Any]],
    covered: set[str],
    canvas_size: tuple[int, int],
) -> dict[str, Any]:
    public_items = []
    for item in items:
        public = {
            key: value for key, value in item.items()
            if not key.startswith("_")
        }
        public["risk_review_covered"] = item["path"] in covered
        public_items.append(public)
    counts = {
        level: sum(item["risk_level"] == level for item in items)
        for level in ("high", "medium", "low")
    }
    return {
        "version": REPORT_VERSION,
        "strategy": "visual-salience-plus-residual",
        "policy": (
            "Scores prioritize evidence only; they never change geometry and "
            "residuals are not standalone correctness failures."
        ),
        "design": str(design_path),
        "reconstruction": str(reconstruction_path),
        "canvas": {"width": canvas_size[0], "height": canvas_size[1]},
        "thresholds": {
            "medium_risk": MEDIUM_RISK_THRESHOLD,
            "high_risk": HIGH_RISK_THRESHOLD,
            "residual_normalization": RESIDUAL_NORMALIZATION,
        },
        "summary": {
            "analyzed": len(items),
            "risk_level_counts": counts,
            "high_risk_paths": [
                item["path"] for item in items
                if item["risk_level"] == "high"
            ],
            "focused_evidence_paths": sorted(covered),
        },
        "evidence": {
            "image": str(evidence_path),
            "legend": str(legend_path),
        },
        "items": public_items,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", required=True)
    parser.add_argument("--design", required=True)
    parser.add_argument("--reconstruction", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--evidence", required=True)
    parser.add_argument("--legend", required=True)
    args = parser.parse_args()

    trace_path = Path(args.trace).expanduser().resolve()
    design_path = Path(args.design).expanduser().resolve()
    reconstruction_path = Path(args.reconstruction).expanduser().resolve()
    output_path = Path(args.output).expanduser().resolve()
    evidence_path = Path(args.evidence).expanduser().resolve()
    legend_path = Path(args.legend).expanduser().resolve()

    trace = load_json(trace_path, "render trace")
    with Image.open(design_path) as source:
        design = source.convert("RGB")
    with Image.open(reconstruction_path) as source:
        reconstruction = source.convert("RGBA")
    items = analyze(trace, design, reconstruction)
    covered = build_evidence(
        design, reconstruction, items, evidence_path, legend_path
    )
    report = build_report(
        design_path,
        reconstruction_path,
        evidence_path,
        legend_path,
        items,
        covered,
        design.size,
    )
    write_json(output_path, report)
    counts = report["summary"]["risk_level_counts"]
    print(
        "Review risk: "
        f"{counts['high']} high, {counts['medium']} medium, "
        f"{counts['low']} low"
    )
    print(f"Report: {output_path}")
    print(f"Evidence: {evidence_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
