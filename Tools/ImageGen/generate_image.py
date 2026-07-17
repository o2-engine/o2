#!/usr/bin/env python3
"""Generate a game asset image from a text prompt (Gemini / Nano Banana 2).

Usage: python3 generate_image.py "a cartoon treasure chest" --out chest.png [--aspect 1:1] [--size 1K]
"""

import argparse
import sys

from PIL import Image

import gemini_image as gi


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("prompt", help="what to generate")
    parser.add_argument("--out", required=True, help="output PNG path")
    parser.add_argument("--aspect", choices=gi.ASPECT_RATIOS, default="1:1")
    parser.add_argument("--size", choices=gi.IMAGE_SIZES, default=None, help="output resolution class")
    parser.add_argument("--ref", action="append", default=[], help="style reference image (repeatable)")
    gi.add_common_args(parser)
    args = parser.parse_args()

    image, text = gi.generate(gi.SYSTEM_GENERATE, args.prompt,
                              images=[Image.open(p) for p in args.ref], aspect=args.aspect,
                              size=args.size, model=args.model, verbose=args.verbose)
    print(gi.save_png(image, args.out))
    if text:
        print("model note:", text)


if __name__ == "__main__":
    try:
        main()
    except gi.GeminiError as e:
        sys.exit(f"error: {e}")
