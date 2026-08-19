#!/usr/bin/env python3
"""Cut a rectangular region out of an image into a separate sprite.

Plain crop by default. With --part "description" the model keeps only the described
element exactly as it appears and erases everything else in the crop to pure white
(--marks adds an image with hand-drawn marks over extra areas to erase). With
--transparent the result gets an alpha channel (white/black re-render pair).

Usage: python3 extract_region.py atlas.png --rect 120,40,256,256 --out sprite.png
       [--part "the red potion bottle"] [--marks marked.png] [--transparent]
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
    parser.add_argument("--part", help="description of the element to extract; everything else "
                                       "in the crop is erased to pure white")
    parser.add_argument("--marks", help="copy of the image with hand-drawn marks over extra "
                                        "areas to erase (used with --part)")
    parser.add_argument("--transparent", action="store_true",
                        help="build an alpha channel for the result")
    gi.add_common_args(parser)
    args = parser.parse_args()

    source = Image.open(args.image)
    crop = gi.crop_rect(source, args.rect)

    if args.part:
        marks = None
        if args.marks:
            marks = Image.open(args.marks)
            if marks.size == source.size:
                marks = gi.crop_rect(marks, args.rect)
        print("extracting element, erasing the rest to white...")
        crop = gi.extract_part(crop, args.part, marks=marks, model=args.model, verbose=args.verbose)
        if args.transparent:
            print("computing alpha...")
            crop = gi.white_to_rgba(crop, model=args.model, verbose=args.verbose)
    elif args.transparent:
        print("isolating subject and computing alpha...")
        crop = gi.isolate_transparent(crop, model=args.model, verbose=args.verbose)
    print(gi.save_png(crop, args.out))


if __name__ == "__main__":
    try:
        main()
    except gi.GeminiError as e:
        sys.exit(f"error: {e}")
