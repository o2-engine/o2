"""Create pixel-preserving design crops with absolute-coordinate rulers.

Normal entry point: workflow.py measure --output <task> --region X Y W H.
This helper measures a human-selected region; it never locates elements or
changes ui_structure.json.
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

from PIL import Image, ImageDraw

from annotate_grid import find_font, select_grid_palette


RULER_LEFT = 64
RULER_TOP = 32


def measure_region(
    design: Image.Image,
    region: tuple[int, int, int, int],
    *,
    zoom: int = 3,
    cell_size: int = 10,
) -> tuple[Image.Image, Image.Image, dict]:
    """Return clean/grid crops and their exact map to native design pixels."""
    x, y, width, height = region
    if width <= 0 or height <= 0:
        raise ValueError("Region width and height must be positive.")
    if x < 0 or y < 0 or x + width > design.width or y + height > design.height:
        raise ValueError("Region must be inside the native design canvas.")
    if not 1 <= zoom <= 8:
        raise ValueError("Zoom must be an integer from 1 to 8.")
    if cell_size <= 0:
        raise ValueError("Cell size must be positive, in native design pixels.")

    crop = design.convert("RGBA").crop((x, y, x + width, y + height))
    clean = crop.resize((width * zoom, height * zoom), Image.Resampling.NEAREST)
    palette = select_grid_palette(crop)
    overlay = Image.new("RGBA", clean.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    ticks = {"x": [], "y": []}
    # Fixed native-pixel steps, anchored to the canvas, not a rounded cell count.
    for axis, start, length in (("x", x, width), ("y", y, height)):
        for value in range(math.ceil(start / cell_size) * cell_size,
                           start + length, cell_size):
            pixel = (value - start) * zoom
            ticks[axis].append({"design_px": value, "crop_px": pixel})
            points = (
                [(pixel, 0), (pixel, clean.height - 1)]
                if axis == "x" else [(0, pixel), (clean.width - 1, pixel)]
            )
            draw.line(points, fill=(*palette["halo_rgb"], 130), width=3)
            draw.line(points, fill=(*palette["line_rgb"], 180), width=1)

    gridded = Image.new(
        "RGBA", (clean.width + RULER_LEFT, clean.height + RULER_TOP),
        (245, 245, 245, 255),
    )
    gridded.paste(Image.alpha_composite(clean, overlay), (RULER_LEFT, RULER_TOP))
    ruler = ImageDraw.Draw(gridded)
    font = find_font(12)
    label_step = cell_size * max(1, math.ceil(48 / (cell_size * zoom)))
    for axis, entries in ticks.items():
        for tick in entries:
            value, pixel = tick["design_px"], tick["crop_px"]
            if axis == "x":
                px = RULER_LEFT + pixel
                ruler.line((px, RULER_TOP - 5, px, RULER_TOP - 1), fill="black")
                position = (px, 7)
                anchor = "mt"
            else:
                py = RULER_TOP + pixel
                ruler.line((RULER_LEFT - 5, py, RULER_LEFT - 1, py), fill="black")
                position = (RULER_LEFT - 8, py)
                anchor = "rm"
            if value % label_step == 0:
                ruler.text(position, str(value), font=font, fill="black", anchor=anchor)

    metrics = {
        "canvas": {"width": design.width, "height": design.height},
        "region": {"x": x, "y": y, "width": width, "height": height},
        "zoom": zoom,
        "resampling": "nearest",
        "cell_size_native_px": cell_size,
        "ruler_units": "absolute design pixels",
        "ticks": ticks,
        "images": {
            "design.png": {"content_origin_px": [0, 0], "size": list(clean.size)},
            "design_grid.png": {
                "content_origin_px": [RULER_LEFT, RULER_TOP],
                "size": list(gridded.size),
            },
        },
        "coordinate_mapping": {
            "x": "region.x + (image_x - content_origin_px[0]) / zoom",
            "y": "region.y + (image_y - content_origin_px[1]) / zoom",
            "width": "image_width / zoom",
            "height": "image_height / zoom",
        },
        "purpose": "Design measurement only; not final render-review evidence.",
    }
    return clean, gridded, metrics


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--design", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--region", type=int, nargs=4, required=True,
                        metavar=("X", "Y", "WIDTH", "HEIGHT"))
    parser.add_argument("--zoom", type=int, default=3)
    parser.add_argument("--cell-size", type=int, default=10)
    args = parser.parse_args()
    try:
        with Image.open(args.design) as design:
            clean, grid, metrics = measure_region(
                design, tuple(args.region), zoom=args.zoom, cell_size=args.cell_size,
            )
    except (OSError, ValueError) as exc:
        parser.error(str(exc))

    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    clean.save(output / "design.png")
    grid.save(output / "design_grid.png")
    metrics["design"] = str(Path(args.design).resolve())
    (output / "metrics.json").write_text(
        json.dumps(metrics, ensure_ascii=False, indent=2) + "\n", encoding="utf-8",
    )
    print(f"Wrote measurement crops and metrics: {output}")


if __name__ == "__main__":
    main()
