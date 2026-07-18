#!/usr/bin/env python3
"""Cut a rectangular region out of an image into a separate sprite.

Plain crop by default; with --transparent the main subject of the region is isolated
(re-rendered by Gemini on white and black backgrounds, alpha recovered from the pair).

Usage: python3 extract_region.py atlas.png --rect 120,40,256,256 --out sprite.png [--transparent]
"""

import argparse
import sys

from PIL import Image

import gemini_image as gi


def parse_rect(value):
    parts = [int(v) for v in value.split(",")]
    if len(parts) != 4 or parts[2] <= 0 or parts[3] <= 0:
        raise argparse.ArgumentTypeError("expected x,y,w,h with positive w,h")
    return parts


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("image", help="source image")
    parser.add_argument("--rect", required=True, type=parse_rect, help="region as x,y,w,h in pixels")
    parser.add_argument("--out", required=True, help="output PNG path")
    parser.add_argument("--transparent", action="store_true",
                        help="isolate the region's main subject and build an alpha channel")
    gi.add_common_args(parser)
    args = parser.parse_args()

    crop = gi.crop_rect(Image.open(args.image), args.rect)

    if args.transparent:
        print("isolating subject and computing alpha...")
        crop = gi.isolate_transparent(crop, model=args.model, verbose=args.verbose)
    print(gi.save_png(crop, args.out))


if __name__ == "__main__":
    try:
        main()
    except gi.GeminiError as e:
        sys.exit(f"error: {e}")
