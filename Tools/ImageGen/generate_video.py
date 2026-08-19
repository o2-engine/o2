#!/usr/bin/env python3
"""Generate a game animation video from a text prompt via Veo (Gemini API).

Usage: python3 generate_video.py "idle animation of a puppy" --out idle.mp4 \
           [--image ref.png] [--aspect 16:9] [--duration 8] [--green] [--loop]
"""

import argparse
import sys

from PIL import Image

import gemini_image as gi
import gemini_video as gv


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("prompt", help="what to animate")
    parser.add_argument("--out", required=True, help="output MP4 path")
    parser.add_argument("--image", help="reference image used as the first frame")
    parser.add_argument("--aspect", choices=gv.VIDEO_ASPECTS, default="16:9")
    parser.add_argument("--resolution", choices=gv.VIDEO_RESOLUTIONS, default=None)
    parser.add_argument("--duration", type=int, choices=gv.VIDEO_DURATIONS, default=None,
                        help="clip length in seconds (default model-chosen, 8)")
    parser.add_argument("--green", action="store_true",
                        help="chroma-key mode: green-screen background, alpha refs composited onto green")
    parser.add_argument("--loop", action="store_true", help="ask for a seamless loop")
    parser.add_argument("--raw", action="store_true",
                        help="send the prompt as-is, without the game-animation preamble")
    parser.add_argument("--negative", help="negative prompt")
    parser.add_argument("--timeout", type=int, default=600, help="poll timeout, seconds")
    parser.add_argument("--model", default=gv.DEFAULT_MODEL,
                        help=f"model id (default: %(default)s; quality: {gv.QUALITY_MODEL})")
    parser.add_argument("-v", "--verbose", action="store_true", help="print prompt and progress")
    args = parser.parse_args()

    image = None
    if args.image:
        bg = gv.GREEN_SCREEN if args.green else None
        image = gv.prepare_reference(Image.open(args.image), aspect=args.aspect, bg_color=bg)

    prompt = gv.build_prompt(args.prompt, raw=args.raw, green=args.green, loop=args.loop)
    print(gv.generate_video(prompt, args.out, image=image, model=args.model,
                            aspect=args.aspect, resolution=args.resolution,
                            duration=args.duration, negative_prompt=args.negative,
                            timeout=args.timeout, verbose=args.verbose))


if __name__ == "__main__":
    try:
        main()
    except gi.GeminiError as e:
        sys.exit(f"error: {e}")
