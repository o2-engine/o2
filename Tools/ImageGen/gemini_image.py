"""Shared client for Gemini image tools (Nano Banana 2). Used by the CLI scripts in this folder."""

import base64
import io
import json
import os
import sys
import time
import urllib.error
import urllib.request

import numpy as np
from PIL import Image

DEFAULT_MODEL = "gemini-3.1-flash-image"  # Nano Banana 2
API_URL = "https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent?key={key}"
ASPECT_RATIOS = ["1:1", "2:3", "3:2", "3:4", "4:3", "4:5", "5:4", "9:16", "16:9", "21:9"]
IMAGE_SIZES = ["1K", "2K", "4K"]

SYSTEM_GENERATE = """\
You are an art asset generator for a 2D game engine. Produce a single, clean,
production-ready image that exactly matches the user's description.
Rules: no watermarks, no signatures, no captions or text unless explicitly requested -
in particular never render words from the description as text on the image;
no borders or frames; the subject must be fully inside the canvas, never cropped by
the edges; consistent lighting; a crisp, readable silhouette suitable for use as a
game sprite, icon or texture."""

SYSTEM_EDIT = """\
You are an image editing tool in a 2D game engine art pipeline. Apply ONLY the change
the user requests to the provided image. Preserve everything else exactly: composition,
style, palette, lighting, proportions and level of detail. Do not add watermarks,
signatures, text, borders or extra elements. Output a single edited image."""

SYSTEM_WHITE_BG = """\
You are a sprite generator for a 2D game engine. Render exactly one subject as described
by the user, centered on a completely flat, pure white background (#FFFFFF).
Hard rules: the background is uniform pure white - no gradients, no cast shadows, no
vignette, no floor or ground plane, no reflections; the subject does not touch the image
edges and has a clear margin around it; no watermarks, no text. Semi-transparent parts
(glass, glow, thin hair) may blend naturally into the white background."""

SYSTEM_BLACK_BG = """\
You are a precise image processing tool. Replace the background of the provided image with
completely flat, pure black (#000000). Keep the foreground subject absolutely unchanged:
identical position, scale, colors, lighting, edges and details. Do not redraw, restyle,
move or recolor the subject in any way - only the white background becomes pure black.
No watermarks, no text."""

SYSTEM_ISOLATE_WHITE = """\
You are an image processing tool for extracting game sprites. The provided image is a
cropped region containing one main subject. Keep that subject pixel-identical - same
position, scale, colors, lighting and details - and replace everything around it with a
completely flat, pure white background (#FFFFFF). No gradients, no shadows, no watermarks,
no text."""

SYSTEM_EXTRACT_PART = """\
You are an image processing tool for extracting elements from 2D game images. You keep
the requested element exactly as it appears and erase everything else to pure white.
No watermarks, no text, no borders."""


class GeminiError(Exception):
    pass


def load_api_key():
    key = os.environ.get("GEMINI_API_KEY")
    if key:
        return key.strip()
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "api_key.txt")
    if os.path.isfile(path):
        with open(path) as f:
            key = f.read().strip()
        if key:
            return key
    raise GeminiError("no API key - set GEMINI_API_KEY or put the key into " + path)


def _image_part(image):
    buf = io.BytesIO()
    image.save(buf, format="PNG")
    return {"inlineData": {"mimeType": "image/png",
                           "data": base64.b64encode(buf.getvalue()).decode()}}


def generate(system_prompt, user_prompt, images=(), aspect=None, size=None,
             model=DEFAULT_MODEL, retries=3, verbose=False):
    """Calls generateContent, returns (PIL.Image, model_text). Input images go before the prompt."""
    parts = [_image_part(img) for img in images]
    parts.append({"text": user_prompt})
    config = {"responseModalities": ["IMAGE"]}
    image_config = {}
    if aspect:
        image_config["aspectRatio"] = aspect
    if size:
        image_config["imageSize"] = size
    if image_config:
        config["imageConfig"] = image_config
    body = {
        "systemInstruction": {"parts": [{"text": system_prompt}]},
        "contents": [{"role": "user", "parts": parts}],
        "generationConfig": config,
    }
    if verbose:
        print("--- system prompt ---\n" + system_prompt, file=sys.stderr)
        print("--- user prompt ---\n" + user_prompt, file=sys.stderr)

    url = API_URL.format(model=model, key=load_api_key())
    data = json.dumps(body).encode()
    last_error = None
    for attempt in range(retries):
        if attempt:
            time.sleep(2 ** attempt)
        req = urllib.request.Request(url, data=data,
                                     headers={"Content-Type": "application/json"})
        try:
            with urllib.request.urlopen(req, timeout=300) as resp:
                result = json.load(resp)
        except urllib.error.HTTPError as e:
            last_error = f"HTTP {e.code}: {e.read().decode(errors='replace')[:500]}"
            if e.code in (429, 500, 502, 503, 504):
                continue
            raise GeminiError(last_error)
        except urllib.error.URLError as e:
            last_error = str(e)
            continue

        candidates = result.get("candidates")
        if not candidates:
            raise GeminiError("no candidates in response: " + json.dumps(result)[:500])
        image, text = None, []
        for part in candidates[0].get("content", {}).get("parts", []):
            if "inlineData" in part:
                image = Image.open(io.BytesIO(base64.b64decode(part["inlineData"]["data"])))
            elif "text" in part:
                text.append(part["text"])
        if image is None:
            last_error = ("no image in response, finishReason="
                          + str(candidates[0].get("finishReason")) + " " + " ".join(text)[:300])
            continue
        return image, " ".join(text)
    raise GeminiError(last_error or "request failed")


def alpha_from_white_black(white_img, black_img):
    """Recovers RGBA from the same subject rendered on white and on black backgrounds."""
    white = np.asarray(white_img.convert("RGB"), dtype=np.float32)
    black_rgb = black_img.convert("RGB")
    if black_rgb.size != white_img.size:
        black_rgb = black_rgb.resize(white_img.size, Image.LANCZOS)
    black = np.asarray(black_rgb, dtype=np.float32)

    # over white: W = C*a + 255*(1-a); over black: B = C*a  =>  a = 1 - (W-B)/255, C = B/a
    alpha = 255.0 - np.clip(white - black, 0.0, 255.0).mean(axis=2)
    # squash JPEG noise at both ends
    alpha = np.clip((alpha - 8.0) * (255.0 / (247.0 - 8.0)), 0.0, 255.0)
    color = np.clip(black * 255.0 / np.maximum(alpha, 1.0)[..., None], 0.0, 255.0)
    color[alpha == 0.0] = 0.0
    rgba = np.dstack([color, alpha]).astype(np.uint8)
    return Image.fromarray(rgba, "RGBA")


BLACK_BG_INSTRUCTION = ("Replace the white background of this image with pure black (#000000). "
                        "Keep the subject exactly as it is.")
ISOLATE_INSTRUCTION = ("Keep the main subject of this image unchanged and replace everything "
                       "around it with a pure white background (#FFFFFF).")

TRANSPARENT_ATTEMPTS = 5
CORNER_ALPHA_MAX = 16


def has_transparent_corner(rgba):
    """At least one of the four corner pixels is (nearly) fully transparent."""
    alpha = np.asarray(rgba)[..., 3]
    corners = (alpha[0, 0], alpha[0, -1], alpha[-1, 0], alpha[-1, -1])
    return min(int(a) for a in corners) <= CORNER_ALPHA_MAX


def white_to_rgba(white, model=DEFAULT_MODEL, verbose=False):
    """Re-renders a white-background image on black and recovers per-pixel alpha."""
    black, _ = generate(SYSTEM_BLACK_BG, BLACK_BG_INSTRUCTION, images=[white],
                        model=model, verbose=verbose)
    return alpha_from_white_black(white, black)


def render_transparent(prompt, refs=(), aspect=None, size=None, model=DEFAULT_MODEL,
                       verbose=False, attempts=TRANSPARENT_ATTEMPTS):
    """Full transparent-sprite pipeline. Returns (rgba, white_render, black_render).

    When all four corners of the recovered sprite come out opaque the background
    failed - the whole render is retried, up to `attempts` times."""
    for attempt in range(attempts):
        white, _ = generate(SYSTEM_WHITE_BG, prompt, images=refs, aspect=aspect, size=size,
                            model=model, verbose=verbose)
        black, _ = generate(SYSTEM_BLACK_BG, BLACK_BG_INSTRUCTION, images=[white],
                            model=model, verbose=verbose)
        rgba = alpha_from_white_black(white, black)
        if has_transparent_corner(rgba):
            return rgba, white, black
        if verbose:
            print(f"all four corners opaque, retrying ({attempt + 1}/{attempts})",
                  file=sys.stderr)
    raise GeminiError(f"all four corners stayed opaque after {attempts} attempts - "
                      "the model failed to produce a clean background")


def isolate_transparent(image, model=DEFAULT_MODEL, verbose=False):
    """Isolates the main subject of an image and recovers its alpha. Returns an RGBA image."""
    white, _ = generate(SYSTEM_ISOLATE_WHITE, ISOLATE_INSTRUCTION, images=[image.convert("RGB")],
                        model=model, verbose=verbose)
    return white_to_rgba(white, model=model, verbose=verbose)


def extract_part_prompt(part):
    return "\n".join([
        f"This image is a cropped region of a 2D game that contains the element to extract: {part}.",
        "Keep ONLY that element, exactly as it appears — same shape, position, size, colors, "
        "shading, texture and art style. Do NOT move, resize, recolor, restyle, or redraw it.",
        "Erase EVERYTHING else in the crop to flat, solid, pure white (#FFFFFF): the background "
        "AND any other objects, pieces, icons, text or fragments that are not the requested element.",
        "If a SECOND image with hand-drawn marks is provided, the marked areas are EXTRA things "
        "to remove — erase them to white too, and never paint the drawn strokes into the result.",
        "This is NOT background removal — remove the other objects as well. The result must be "
        "almost entirely pure white with only the requested element visible. If the element is "
        "partly hidden, plausibly complete it.",
        "Output only the resulting image.",
    ])


def extract_part(crop, part, marks=None, model=DEFAULT_MODEL, verbose=False):
    """Keeps only the described element of the crop, erasing the rest to pure white.

    Returns the white-background image resized back to the crop's size, so the
    element keeps the original region's geometry 1:1."""
    images = [crop.convert("RGB")]
    if marks is not None:
        images.append(marks.convert("RGB"))
    white, _ = generate(SYSTEM_EXTRACT_PART, extract_part_prompt(part), images=images,
                        model=model, verbose=verbose)
    if white.size != crop.size:
        white = white.resize(crop.size, Image.LANCZOS)
    return white


def crop_rect(image, rect):
    """Crops [x, y, w, h] clamped to image bounds."""
    x, y, w, h = rect
    x0, y0 = max(0, x), max(0, y)
    x1, y1 = min(image.width, x + w), min(image.height, y + h)
    if w <= 0 or h <= 0 or x0 >= x1 or y0 >= y1:
        raise GeminiError(f"rect {rect} is outside the image {image.width}x{image.height}")
    return image.crop((x0, y0, x1, y1))


def save_png(image, out_path):
    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    image.save(out_path, format="PNG")
    return f"saved: {out_path} ({image.size[0]}x{image.size[1]}, {image.mode})"


def add_common_args(parser):
    parser.add_argument("--model", default=DEFAULT_MODEL, help="model id (default: %(default)s)")
    parser.add_argument("-v", "--verbose", action="store_true", help="print the prompts sent to the model")
