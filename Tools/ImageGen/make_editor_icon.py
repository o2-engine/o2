#!/usr/bin/env python3
"""Post-process a generated o2 editor asset icon to match the editor icon set.

Takes a white-background render (e.g. from generate_image.py), keys the background out
with a border flood fill (the white/black alpha trick fails for near-white icons),
scales the body to the set's size, centers it and adds the set's soft drop shadow
(fitted to existing icons: gaussian sigma 4, offset +2px, intensity 0.148).

Usage: python3 make_editor_icon.py render.png --out UI4_big_xxx_icon.png [--body 25] [--canvas 40]
"""

import argparse
import sys
from collections import deque

import numpy as np
from PIL import Image, ImageFilter

import gemini_image as gi


def flood_background_alpha(image, tolerance=12):
    rgba = np.asarray(image.convert("RGBA"), dtype=np.float32)
    a = rgba[..., 3:4] / 255.0
    rgb = (rgba[..., :3] * a + 255.0 * (1.0 - a)).astype(np.uint8)
    # key on the border's dominant color: generated backgrounds are not always pure white
    border = np.concatenate([rgb[0], rgb[-1], rgb[:, 0], rgb[:, -1]])
    key = np.median(border, axis=0)
    near_white = (np.abs(rgb.astype(np.int16) - key).max(axis=2) <= tolerance)
    h, w = near_white.shape
    bg = np.zeros((h, w), bool)
    dq = deque()
    for x in range(w):
        for y in (0, h - 1):
            if near_white[y, x]:
                bg[y, x] = True; dq.append((y, x))
    for y in range(h):
        for x in (0, w - 1):
            if near_white[y, x]:
                bg[y, x] = True; dq.append((y, x))
    while dq:
        y, x = dq.popleft()
        for ny, nx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
            if 0 <= ny < h and 0 <= nx < w and near_white[ny, nx] and not bg[ny, nx]:
                bg[ny, nx] = True; dq.append((ny, nx))
    alpha = np.where(bg, 0, 255).astype(np.uint8)
    out = np.dstack([rgb, alpha])
    out[bg] = 0
    return Image.fromarray(out, "RGBA")


def make_icon(image, body_max=25, canvas_size=40, center=19.0,
              shadow_sigma=4.0, shadow_k=0.148, shadow_offset=2):
    a = np.asarray(image)[..., 3]
    ys, xs = np.where(a > 8)
    image = image.crop((xs.min(), ys.min(), xs.max() + 1, ys.max() + 1))
    scale = body_max / max(image.width, image.height)
    image = image.resize((max(1, round(image.width * scale)), max(1, round(image.height * scale))),
                         Image.LANCZOS)
    x0 = round(center - (image.width - 1) / 2.0)
    y0 = round(center - (image.height - 1) / 2.0)

    # blur on a padded canvas so the shadow is not clipped by the 40x40 bounds
    pad = int(shadow_sigma * 3)
    side = canvas_size + pad * 2
    body = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    body.alpha_composite(image, (x0 + pad, y0 + pad))
    blurred = np.asarray(body.getchannel("A").filter(ImageFilter.GaussianBlur(shadow_sigma)), np.float32)
    shadow_a = np.clip(np.roll(blurred * shadow_k, (shadow_offset, shadow_offset), axis=(0, 1)),
                       0, 255).astype(np.uint8)
    shadow = np.zeros((side, side, 4), np.uint8)
    shadow[..., 3] = shadow_a
    canvas = Image.fromarray(shadow, "RGBA")
    canvas.alpha_composite(body)
    return canvas.crop((pad, pad, pad + canvas_size, pad + canvas_size))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("image", help="white-background render of the icon")
    parser.add_argument("--out", required=True, help="output icon PNG path")
    parser.add_argument("--body", type=int, default=25, help="body size, px of max dimension (default 25)")
    parser.add_argument("--canvas", type=int, default=40, help="canvas size (default 40)")
    args = parser.parse_args()

    icon = make_icon(flood_background_alpha(Image.open(args.image)),
                     body_max=args.body, canvas_size=args.canvas,
                     center=(args.canvas - 2) / 2.0)
    print(gi.save_png(icon, args.out))


if __name__ == "__main__":
    try:
        main()
    except gi.GeminiError as e:
        sys.exit(f"error: {e}")
