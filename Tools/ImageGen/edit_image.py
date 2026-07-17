#!/usr/bin/env python3
"""Edit an existing image with a text instruction (Gemini / Nano Banana 2).

Usage: python3 edit_image.py input.png "make the chest golden" --out edited.png
Extra reference images may be passed with --ref (e.g. a style sample).
"""

import argparse
import sys

from PIL import Image

import gemini_image as gi


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("image", help="input image to edit")
    parser.add_argument("prompt", help="editing instruction")
    parser.add_argument("--out", required=True, help="output PNG path")
    parser.add_argument("--ref", action="append", default=[], help="extra reference image (repeatable)")
    gi.add_common_args(parser)
    args = parser.parse_args()

    images = [Image.open(args.image)] + [Image.open(p) for p in args.ref]
    image, text = gi.generate(gi.SYSTEM_EDIT, args.prompt, images=images,
                              model=args.model, verbose=args.verbose)
    print(gi.save_png(image, args.out))
    if text:
        print("model note:", text)


if __name__ == "__main__":
    try:
        main()
    except gi.GeminiError as e:
        sys.exit(f"error: {e}")
