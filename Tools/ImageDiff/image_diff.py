#!/usr/bin/env python3
"""Pixel comparison of two images with a highlighted difference map.

Compares image B against reference A pixel by pixel (per-channel tolerance),
writes a diff image: the reference dimmed to grayscale with differing pixels
highlighted in red. When sizes differ, B is scaled to the size of A first.

CLI: image_diff.py <a> <b> <out_diff.png> [--threshold 16] [--region l t r b]
Exit code 0; stats go to stdout as JSON.
"""

import argparse
import json

from PIL import Image, ImageChops


def compare(a_path, b_path, out_path=None, threshold=16, region=None):
    """Returns diff stats dict; when out_path is set, writes the highlight image.

    region: optional (left, top, right, bottom) box in pixels of A to compare.
    """
    a = Image.open(a_path).convert("RGBA")
    b = Image.open(b_path).convert("RGBA")

    scaled = False
    if b.size != a.size:
        b = b.resize(a.size, Image.LANCZOS)
        scaled = True

    if region:
        a = a.crop(tuple(region))
        b = b.crop(tuple(region))

    diff = ImageChops.difference(a.convert("RGB"), b.convert("RGB"))

    # Per-pixel max channel difference against the threshold
    r, g, bl = diff.split()
    mask = ImageChops.lighter(ImageChops.lighter(r, g), bl)
    mask = mask.point(lambda v: 255 if v > threshold else 0)

    histogram = mask.histogram()
    total = a.size[0] * a.size[1]
    changed = histogram[255] if len(histogram) > 255 else 0

    bbox = mask.getbbox()

    if out_path:
        base = a.convert("L").convert("RGB").point(lambda v: v // 2)
        highlight = Image.new("RGB", a.size, (255, 40, 40))
        result = Image.composite(highlight, base, mask)
        result.save(out_path)

    return {
        "size": list(a.size),
        "total_pixels": total,
        "changed_pixels": changed,
        "changed_share": round(changed / total, 5) if total else 0.0,
        "diff_bbox": list(bbox) if bbox else None,
        "b_scaled_to_a": scaled,
        "threshold": threshold,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("a", help="Reference image")
    parser.add_argument("b", help="Compared image (scaled to the reference size if differs)")
    parser.add_argument("out", nargs="?", default=None, help="Diff highlight PNG path")
    parser.add_argument("--threshold", type=int, default=16,
                        help="Per-channel difference tolerance 0-255 (default 16)")
    parser.add_argument("--region", type=int, nargs=4, metavar=("L", "T", "R", "B"),
                        default=None, help="Compare only this box of the reference")
    args = parser.parse_args()

    stats = compare(args.a, args.b, args.out, args.threshold, args.region)
    print(json.dumps(stats, indent=1))


if __name__ == "__main__":
    main()
