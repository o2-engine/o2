"""Shared client for Veo video generation via the Gemini API. Used by generate_video.py and mcp_server.py."""

import base64
import io
import json
import sys
import time
import urllib.error
import urllib.request

from PIL import Image

import gemini_image as gi

DEFAULT_MODEL = "veo-3.1-fast-generate-preview"
QUALITY_MODEL = "veo-3.1-generate-preview"
VIDEO_ASPECTS = ["16:9", "9:16"]
VIDEO_RESOLUTIONS = ["720p", "1080p"]
VIDEO_DURATIONS = [4, 6, 8]
GREEN_SCREEN = (0, 177, 64)  # #00B140, keyed by the engine's ChromaKey shader

START_URL = "https://generativelanguage.googleapis.com/v1beta/models/{model}:predictLongRunning?key={key}"
POLL_URL = "https://generativelanguage.googleapis.com/v1beta/{name}?key={key}"

# Veo has no systemInstruction, so the game-animation rules are prepended to the prompt itself.
PROMPT_PREFIX = (
    "A 2D game animation clip. Fixed locked-off camera: no camera motion, no zoom, no pan, "
    "no cuts, no scene changes. No text, no watermarks, no UI. The subject stays fully inside "
    "the frame and keeps its exact art style, proportions and colors for the whole clip. ")

GREEN_SCREEN_SUFFIX = (
    " The background is a perfectly uniform flat chroma-key green screen (#00B140) filling the "
    "entire frame; it stays exactly the same solid green for the whole clip - no shadows, no "
    "gradients, no floor, nothing else ever appears in the background.")

LOOP_SUFFIX = (
    " The animation is a seamless loop: the last frame matches the first frame exactly - the "
    "subject returns to its starting pose at the end.")


def build_prompt(prompt, raw=False, green=False, loop=False):
    if raw:
        return prompt
    text = PROMPT_PREFIX + prompt
    if green:
        text += GREEN_SCREEN_SUFFIX
    if loop:
        text += LOOP_SUFFIX
    return text


def prepare_reference(image, aspect=None, bg_color=None):
    """First-frame reference: composites alpha onto bg_color and pads the canvas to the aspect."""
    if image.mode != "RGB":
        rgba = image.convert("RGBA")
        bg = Image.new("RGBA", rgba.size, (bg_color or (255, 255, 255)) + (255,))
        image = Image.alpha_composite(bg, rgba).convert("RGB")
    if not aspect:
        return image
    aw, ah = (int(v) for v in aspect.split(":"))
    w, h = image.size
    cw, ch = max(w, (h * aw + ah - 1) // ah), max(h, (w * ah + aw - 1) // aw)
    if (cw, ch) == (w, h):
        return image
    canvas = Image.new("RGB", (cw, ch), bg_color or (255, 255, 255))
    canvas.paste(image, ((cw - w) // 2, (ch - h) // 2))
    return canvas


def _request_json(url, body=None):
    data = json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(url, data=data,
                                 headers={"Content-Type": "application/json"})
    try:
        with urllib.request.urlopen(req, timeout=120) as resp:
            return json.load(resp)
    except urllib.error.HTTPError as e:
        raise gi.GeminiError(f"HTTP {e.code}: {e.read().decode(errors='replace')[:800]}")


def start_generation(prompt, image=None, model=DEFAULT_MODEL, aspect=None,
                     resolution=None, duration=None, negative_prompt=None, verbose=False):
    """Starts a long-running Veo generation, returns the operation name."""
    instance = {"prompt": prompt}
    if image is not None:
        buf = io.BytesIO()
        image.save(buf, format="PNG")
        instance["image"] = {"bytesBase64Encoded": base64.b64encode(buf.getvalue()).decode(),
                             "mimeType": "image/png"}
    parameters = {}
    if aspect:
        parameters["aspectRatio"] = aspect
    if resolution:
        parameters["resolution"] = resolution
    if duration:
        parameters["durationSeconds"] = int(duration)
    if negative_prompt:
        parameters["negativePrompt"] = negative_prompt
    body = {"instances": [instance]}
    if parameters:
        body["parameters"] = parameters
    if verbose:
        print("--- video prompt ---\n" + prompt, file=sys.stderr)
    result = _request_json(START_URL.format(model=model, key=gi.load_api_key()), body)
    name = result.get("name")
    if not name:
        raise gi.GeminiError("no operation name in response: " + json.dumps(result)[:500])
    return name


def poll_operation(name, timeout=600, interval=10, verbose=False):
    """Polls the operation until done, returns its response dict."""
    url = POLL_URL.format(name=name, key=gi.load_api_key())
    start = time.time()
    while True:
        result = _request_json(url)
        if result.get("done"):
            if "error" in result:
                raise gi.GeminiError("generation failed: " + json.dumps(result["error"])[:500])
            return result.get("response") or {}
        if time.time() - start > timeout:
            raise gi.GeminiError(f"timed out after {timeout}s, operation still running: {name}")
        if verbose:
            print(f"... generating ({int(time.time() - start)}s)", file=sys.stderr)
        time.sleep(interval)


def _find_videos(node, out):
    if isinstance(node, dict):
        if isinstance(node.get("bytesBase64Encoded"), str):
            out.append(("bytes", node["bytesBase64Encoded"]))
        if isinstance(node.get("uri"), str) and node["uri"].startswith("http"):
            out.append(("uri", node["uri"]))
        for value in node.values():
            _find_videos(value, out)
    elif isinstance(node, list):
        for value in node:
            _find_videos(value, out)


def extract_video_bytes(response):
    """Pulls the generated video out of a finished operation response."""
    found = []
    _find_videos(response, found)
    for kind, value in found:
        if kind == "bytes":
            return base64.b64decode(value)
    for kind, value in found:
        if kind == "uri":
            req = urllib.request.Request(value, headers={"x-goog-api-key": gi.load_api_key()})
            try:
                with urllib.request.urlopen(req, timeout=300) as resp:
                    return resp.read()
            except urllib.error.HTTPError as e:
                raise gi.GeminiError(f"video download failed, HTTP {e.code}")
    filtered = json.dumps(response)[:500]
    raise gi.GeminiError("no video in response (possibly filtered): " + filtered)


def save_video(data, out_path):
    import os
    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    with open(out_path, "wb") as f:
        f.write(data)
    return f"saved: {out_path} ({len(data) / 1024 / 1024:.1f} MB)"


def generate_video(prompt, out_path, image=None, model=DEFAULT_MODEL, aspect="16:9",
                   resolution=None, duration=None, negative_prompt=None,
                   timeout=600, verbose=False):
    """Full pipeline: start, poll, download, save. Returns the status string."""
    name = start_generation(prompt, image=image, model=model, aspect=aspect,
                            resolution=resolution, duration=duration,
                            negative_prompt=negative_prompt, verbose=verbose)
    if verbose:
        print("operation: " + name, file=sys.stderr)
    response = poll_operation(name, timeout=timeout, verbose=verbose)
    return save_video(extract_video_bytes(response), out_path)
