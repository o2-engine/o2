#!/usr/bin/env python3
"""Generate a sprite with a real alpha channel (Gemini / Nano Banana 2).

Generates the subject on a pure white background, then asks the model to swap the
background to pure black keeping the subject intact, and recovers per-pixel alpha
from the two renders. Output is an RGBA PNG ready to drop into the engine.

Usage: python3 generate_transparent.py "a cartoon health potion" --out potion.png
"""

import argparse
import os
import sys

from PIL import Image

import gemini_image as gi


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("prompt", help="subject to generate")
    parser.add_argument("--out", required=True, help="output RGBA PNG path")
    parser.add_argument("--aspect", choices=gi.ASPECT_RATIOS, default="1:1")
    parser.add_argument("--size", choices=gi.IMAGE_SIZES, default=None, help="output resolution class")
    parser.add_argument("--ref", action="append", default=[], help="style reference image (repeatable)")
    parser.add_argument("--keep-steps", action="store_true",
                        help="also save the intermediate white/black renders next to the output")
    gi.add_common_args(parser)
    args = parser.parse_args()

    print("rendering on white, re-rendering on black, computing alpha...")
    rgba, white, black = gi.render_transparent(args.prompt, refs=[Image.open(p) for p in args.ref],
                                               aspect=args.aspect, size=args.size,
                                               model=args.model, verbose=args.verbose)
    print(gi.save_png(rgba, args.out))

    if args.keep_steps:
        base, _ = os.path.splitext(args.out)
        print(gi.save_png(white, base + "_white.png"))
        print(gi.save_png(black, base + "_black.png"))


if __name__ == "__main__":
    try:
        main()
    except gi.GeminiError as e:
        sys.exit(f"error: {e}")
