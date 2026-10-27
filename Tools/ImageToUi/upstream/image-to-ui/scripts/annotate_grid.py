"""
Annotate a UI design image with a coordinate grid so an agent can read
positions off it instead of guessing.

Grid layout:
  - Target cell size ~45-50 px (tuned for phone-sized designs); the actual
    cell count is computed from the design's pixel dimensions so the cells
    stay roughly square regardless of aspect ratio.
  - Cells are numbered along the top (col) and left edges (row).
  - Line hue is selected from the design's dominant tone, then shifted to its
    complement and backed by a light/dark halo for local contrast.
  - Lines remain thin and semi-transparent so they don't obscure UI detail.
  - Every 5th line is drawn slightly thicker for easy scanning.

The draw_grid() function is reused by annotate_element.py and
render_comparison.py so every reference image carries the same ruler.

Usage:
    py -B annotate_grid.py --design path/to/design.png --output path/to/design_grid.png
"""

import argparse
import colorsys
import json
import math
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


DEFAULT_CELL_SIZE = 45
MAJOR_EVERY = 5

TONE_SAMPLE_SIZE = 96
HUE_BINS = 24
NEUTRAL_FALLBACK_HUE = 5 / 6  # magenta is uncommon in most UI screenshots
LIGHT_BACKGROUND_THRESHOLD = 0.62

LINE_ALPHA_MINOR = 210
LINE_ALPHA_MAJOR = 245
HALO_ALPHA_MINOR = 165
HALO_ALPHA_MAJOR = 210
LINE_WIDTH_MINOR = 1
LINE_WIDTH_MAJOR = 2
HALO_EXTRA_WIDTH = 2

RESAMPLE_BILINEAR = (
    Image.Resampling.BILINEAR
    if hasattr(Image, "Resampling")
    else Image.BILINEAR
)


def _luminance(rgb) -> float:
    r, g, b = (channel / 255 for channel in rgb)
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def _rgb_hex(rgb) -> str:
    return "#{:02X}{:02X}{:02X}".format(*rgb)


def _analyse_tone(image: Image.Image) -> dict:
    """Return the dominant visible hue and average luminance of an image."""
    sample = image.convert("RGBA")
    sample.thumbnail((TONE_SAMPLE_SIZE, TONE_SAMPLE_SIZE), RESAMPLE_BILINEAR)

    hue_weights = [0.0] * HUE_BINS
    hue_x = [0.0] * HUE_BINS
    hue_y = [0.0] * HUE_BINS
    total_alpha = 0.0
    total_hue_weight = 0.0
    luminance_sum = 0.0
    saturation_sum = 0.0

    pixels = (
        sample.get_flattened_data()
        if hasattr(sample, "get_flattened_data")
        else sample.getdata()
    )
    for r, g, b, a in pixels:
        alpha = a / 255
        if alpha <= 0.02:
            continue
        h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
        hue_weight = alpha * s * max(v, 0.2)
        hue_bin = min(HUE_BINS - 1, int(h * HUE_BINS))
        angle = h * math.tau

        hue_weights[hue_bin] += hue_weight
        hue_x[hue_bin] += math.cos(angle) * hue_weight
        hue_y[hue_bin] += math.sin(angle) * hue_weight
        total_hue_weight += hue_weight
        total_alpha += alpha
        luminance_sum += _luminance((r, g, b)) * alpha
        saturation_sum += s * alpha

    if total_alpha == 0:
        return {
            "dominant_hue": None,
            "hue_confidence": 0.0,
            "average_luminance": 0.0,
            "average_saturation": 0.0,
        }

    dominant_hue = None
    hue_confidence = 0.0
    average_saturation = saturation_sum / total_alpha
    if total_hue_weight > 0 and average_saturation >= 0.08:
        peak = max(range(HUE_BINS), key=hue_weights.__getitem__)
        neighbours = ((peak - 1) % HUE_BINS, peak, (peak + 1) % HUE_BINS)
        local_weight = sum(hue_weights[index] for index in neighbours)
        local_x = sum(hue_x[index] for index in neighbours)
        local_y = sum(hue_y[index] for index in neighbours)
        hue_confidence = local_weight / total_hue_weight
        if local_weight > 0:
            dominant_hue = (math.atan2(local_y, local_x) / math.tau) % 1

    return {
        "dominant_hue": dominant_hue,
        "hue_confidence": hue_confidence,
        "average_luminance": luminance_sum / total_alpha,
        "average_saturation": average_saturation,
    }


def _lift_luminance(rgb, minimum: float = 0.62):
    """Mix a saturated color with white until it remains visible on dark art."""
    current = _luminance(rgb)
    if current >= minimum:
        return rgb
    white_mix = (minimum - current) / (1 - current)
    return tuple(
        round(channel + (255 - channel) * white_mix)
        for channel in rgb
    )


def select_grid_palette(image: Image.Image) -> dict:
    """Choose a complementary line color plus an opposite-luminance halo."""
    tone = _analyse_tone(image)
    source_hue = tone["dominant_hue"]
    line_hue = (
        (source_hue + 0.5) % 1
        if source_hue is not None
        else NEUTRAL_FALLBACK_HUE
    )

    if tone["average_luminance"] >= LIGHT_BACKGROUND_THRESHOLD:
        raw = colorsys.hsv_to_rgb(line_hue, 0.95, 0.46)
        line_rgb = tuple(round(channel * 255) for channel in raw)
        halo_rgb = (255, 255, 255)
        contrast_mode = "dark-on-light"
    else:
        raw = colorsys.hsv_to_rgb(line_hue, 0.95, 1.0)
        line_rgb = _lift_luminance(
            tuple(round(channel * 255) for channel in raw)
        )
        halo_rgb = (0, 0, 0)
        contrast_mode = "bright-on-dark"

    return {
        "strategy": "adaptive-complementary-hue",
        "line_rgb": line_rgb,
        "line_hex": _rgb_hex(line_rgb),
        "halo_rgb": halo_rgb,
        "halo_hex": _rgb_hex(halo_rgb),
        "contrast_mode": contrast_mode,
        "dominant_hue_degrees": (
            round(source_hue * 360, 1) if source_hue is not None else None
        ),
        "hue_confidence": round(tone["hue_confidence"], 4),
        "average_luminance": round(tone["average_luminance"], 4),
        "average_saturation": round(tone["average_saturation"], 4),
    }


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


def compute_grid(W: int, H: int, target_cell: int = DEFAULT_CELL_SIZE):
    """Return (cols, rows, sx, sy) for a grid sized to W x H so each cell is
    approximately target_cell pixels."""
    cols = max(1, round(W / target_cell))
    rows = max(1, round(H / target_cell))
    return cols, rows, W / cols, H / rows


def draw_grid(
    image: Image.Image,
    target_cell: int = DEFAULT_CELL_SIZE,
    palette: dict | None = None,
) -> Image.Image:
    """Overlay a labeled grid on an RGBA image. Returns a new image; does not
    mutate the input. The grid sizing is computed from the image's own
    dimensions, so each call produces a grid intrinsic to that image - handy
    when annotating both the design and a reconstruction of the same canvas.
    """
    base = image.convert("RGBA")
    W, H = base.size
    cols, rows, sx, sy = compute_grid(W, H, target_cell)

    overlay = Image.new("RGBA", base.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    palette = palette or select_grid_palette(base)
    line_rgb = tuple(palette["line_rgb"])
    halo_rgb = tuple(palette["halo_rgb"])
    grid_lines = []

    # Vertical lines
    for c in range(cols + 1):
        x = int(round(c * sx))
        is_major = (c % MAJOR_EVERY == 0) or c == cols
        grid_lines.append(([(x, 0), (x, H)], is_major))

    # Horizontal lines
    for r in range(rows + 1):
        y = int(round(r * sy))
        is_major = (r % MAJOR_EVERY == 0) or r == rows
        grid_lines.append(([(0, y), (W, y)], is_major))

    # Draw minor lines first so major lines stay visually dominant at crossings.
    grid_lines.sort(key=lambda item: item[1])
    for points, is_major in grid_lines:
        line_width = LINE_WIDTH_MAJOR if is_major else LINE_WIDTH_MINOR
        halo_alpha = HALO_ALPHA_MAJOR if is_major else HALO_ALPHA_MINOR
        draw.line(
            points,
            fill=(*halo_rgb, halo_alpha),
            width=line_width + HALO_EXTRA_WIDTH,
        )
    for points, is_major in grid_lines:
        line_width = LINE_WIDTH_MAJOR if is_major else LINE_WIDTH_MINOR
        line_alpha = LINE_ALPHA_MAJOR if is_major else LINE_ALPHA_MINOR
        draw.line(
            points,
            fill=(*line_rgb, line_alpha),
            width=line_width,
        )

    label_size = max(14, int(min(sx, sy) * 0.45))
    font = find_font(label_size)
    label_color = (*line_rgb, 255)
    label_bg = (*halo_rgb, 225)

    # Column labels
    for c in range(0, cols + 1, MAJOR_EVERY):
        x = int(round(c * sx))
        label = str(c)
        try:
            bbox = font.getbbox(label)
            tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
        except Exception:
            tw, th = label_size * len(label), label_size
        pad = 2
        draw.rectangle([x + 2, 2, x + 2 + tw + 2 * pad, 2 + th + 2 * pad],
                       fill=label_bg)
        draw.text((x + 2 + pad, 2 + pad), label, font=font, fill=label_color)

    # Row labels
    for r in range(0, rows + 1, MAJOR_EVERY):
        y = int(round(r * sy))
        label = str(r)
        try:
            bbox = font.getbbox(label)
            tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
        except Exception:
            tw, th = label_size * len(label), label_size
        pad = 2
        draw.rectangle([2, y + 2, 2 + tw + 2 * pad, y + 2 + th + 2 * pad],
                       fill=label_bg)
        draw.text((2 + pad, y + 2 + pad), label, font=font, fill=label_color)

    return Image.alpha_composite(base, overlay)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--design", required=True)
    ap.add_argument("--output", required=True)
    ap.add_argument("--metrics",
                    help="Optional path for grid metrics JSON. Defaults to "
                         "<output-stem>_metrics.json")
    ap.add_argument("--cell-size", type=int, default=DEFAULT_CELL_SIZE,
                    help=f"Target grid cell size in px (default {DEFAULT_CELL_SIZE})")
    args = ap.parse_args()

    design_path = Path(args.design)
    out_path = Path(args.output)
    if not design_path.exists():
        print(f"Design not found: {design_path}", file=sys.stderr)
        sys.exit(1)

    base = Image.open(design_path).convert("RGBA")
    W, H = base.size
    cols, rows, sx, sy = compute_grid(W, H, args.cell_size)
    palette = select_grid_palette(base)
    out = draw_grid(base, args.cell_size, palette)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out.convert("RGB").save(out_path)
    metrics_path = Path(args.metrics) if args.metrics else out_path.with_name(
        f"{out_path.stem}_metrics.json"
    )
    metrics = {
        "design": str(design_path),
        "grid_image": str(out_path),
        "canvas": {"width": W, "height": H},
        "grid": {"cols": cols, "rows": rows},
        "px_per_cell_x": sx,
        "px_per_cell_y": sy,
        "cell_size_target": args.cell_size,
        "grid_style": {
            "strategy": palette["strategy"],
            "line_color": palette["line_hex"],
            "halo_color": palette["halo_hex"],
            "contrast_mode": palette["contrast_mode"],
            "source_tone": {
                "dominant_hue_degrees": palette["dominant_hue_degrees"],
                "hue_confidence": palette["hue_confidence"],
                "average_luminance": palette["average_luminance"],
                "average_saturation": palette["average_saturation"],
            },
        },
    }
    metrics_path.parent.mkdir(parents=True, exist_ok=True)
    metrics_path.write_text(json.dumps(metrics, indent=2) + "\n",
                            encoding="utf-8")
    print(f"Wrote {out_path}")
    print(f"Wrote {metrics_path}")
    print(f"  canvas: {W} x {H}")
    print(f"  grid:   {cols} cols x {rows} rows (~{sx:.1f} x {sy:.1f} px/cell)")
    print(f"  px_per_cell_x: {sx:.1f}")
    print(f"  px_per_cell_y: {sy:.1f}")


if __name__ == "__main__":
    main()
