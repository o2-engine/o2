#!/usr/bin/env python3
"""MCP server (stdio, newline-delimited JSON-RPC) exposing the Gemini image tools.

Registered in the repo root .mcp.json as server "imagegen". No dependencies beyond
Pillow/numpy. Tool results contain the saved file path and a downscaled preview image.
"""

import base64
import io
import json
import os
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from PIL import Image

import gemini_image as gi
import gemini_video as gv

ASPECT_SCHEMA = {"type": "string", "enum": gi.ASPECT_RATIOS,
                 "description": "Aspect ratio (default 1:1)"}
SIZE_SCHEMA = {"type": "string", "enum": gi.IMAGE_SIZES,
               "description": "Resolution class (default model-chosen, ~1K)"}
REFS_SCHEMA = {"type": "array", "items": {"type": "string"},
               "description": "Paths of style reference images"}
OUT_SCHEMA = {"type": "string", "description": "Output PNG path (absolute preferred)"}

TOOLS = [
    {
        "name": "generate_image",
        "description": "Generate a game asset image (sprite, icon, texture) from a text prompt "
                       "via Gemini (Nano Banana 2) and save it as PNG.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "prompt": {"type": "string", "description": "What to generate"},
                "out_path": OUT_SCHEMA,
                "aspect": ASPECT_SCHEMA,
                "size": SIZE_SCHEMA,
                "ref_paths": REFS_SCHEMA,
            },
            "required": ["prompt", "out_path"],
        },
    },
    {
        "name": "edit_image",
        "description": "Edit an existing image with a text instruction, preserving everything "
                       "else (style, composition, palette). Saves the result as PNG.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "image_path": {"type": "string", "description": "Image to edit"},
                "prompt": {"type": "string", "description": "Editing instruction"},
                "out_path": OUT_SCHEMA,
                "ref_paths": REFS_SCHEMA,
            },
            "required": ["image_path", "prompt", "out_path"],
        },
    },
    {
        "name": "generate_transparent_image",
        "description": "Generate a sprite with a real alpha channel (RGBA PNG): renders the "
                       "subject on white, re-renders on black, recovers per-pixel alpha from the "
                       "pair. If all four corners of the result come out opaque the background "
                       "failed and the render is retried, up to 5 attempts. Note: fails on "
                       "subjects that are themselves nearly white.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "prompt": {"type": "string", "description": "Subject to generate"},
                "out_path": OUT_SCHEMA,
                "aspect": ASPECT_SCHEMA,
                "size": SIZE_SCHEMA,
                "ref_paths": REFS_SCHEMA,
                "keep_steps": {"type": "boolean",
                               "description": "Also save intermediate white/black renders"},
            },
            "required": ["prompt", "out_path"],
        },
    },
    {
        "name": "generate_video",
        "description": "Generate a short game animation video (MP4, no alpha) from a text prompt "
                       "via Veo. Optional reference image becomes the first frame - pass a "
                       "character sprite to animate it (idle, walk, jump...). With "
                       "green_screen=true the background is a flat chroma-key green (#00B140) and "
                       "transparent references are composited onto green, ready for the engine's "
                       "ChromaKey shader. Takes 1-5 minutes.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "prompt": {"type": "string", "description": "Motion/animation description"},
                "out_path": {"type": "string", "description": "Output MP4 path (absolute preferred)"},
                "image_path": {"type": "string",
                               "description": "Reference image used as the first frame"},
                "aspect": {"type": "string", "enum": gv.VIDEO_ASPECTS,
                           "description": "Aspect ratio (default 16:9)"},
                "resolution": {"type": "string", "enum": gv.VIDEO_RESOLUTIONS,
                               "description": "Resolution (default 720p)"},
                "duration_sec": {"type": "integer", "enum": gv.VIDEO_DURATIONS,
                                 "description": "Clip length in seconds (default 8)"},
                "green_screen": {"type": "boolean",
                                 "description": "Uniform green-screen background for chroma keying"},
                "loop": {"type": "boolean", "description": "Ask for a seamless loop"},
                "raw_prompt": {"type": "boolean",
                               "description": "Send the prompt as-is, without the game-animation preamble"},
                "negative_prompt": {"type": "string", "description": "What to avoid"},
                "model": {"type": "string",
                          "description": f"Veo model id (default {gv.DEFAULT_MODEL}, "
                                         f"quality {gv.QUALITY_MODEL})"},
            },
            "required": ["prompt", "out_path"],
        },
    },
    {
        "name": "extract_region",
        "description": "Cut a rectangular region out of an image into a separate sprite PNG. "
                       "With `part` set, the model keeps only the described element exactly as "
                       "it appears in the crop and erases everything else (background and other "
                       "objects) to pure white, preserving the original as much as possible. "
                       "With transparent=true the result additionally gets an alpha channel.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "image_path": {"type": "string", "description": "Source image"},
                "rect": {"type": "array", "items": {"type": "integer"},
                         "minItems": 4, "maxItems": 4,
                         "description": "Region as [x, y, w, h] in pixels"},
                "out_path": OUT_SCHEMA,
                "part": {"type": "string",
                         "description": "Description of the element to extract from the crop; "
                                        "everything else is erased to pure white"},
                "marks_path": {"type": "string",
                               "description": "Optional copy of the image with hand-drawn marks "
                                              "over extra areas to erase (used with `part`)"},
                "transparent": {"type": "boolean",
                                "description": "Build an alpha channel for the result"},
            },
            "required": ["image_path", "rect", "out_path"],
        },
    },
]


def preview_part(image, max_side=320):
    im = image.copy()
    im.thumbnail((max_side, max_side))
    buf = io.BytesIO()
    im.save(buf, format="PNG")
    return {"type": "image", "data": base64.b64encode(buf.getvalue()).decode(),
            "mimeType": "image/png"}


def image_result(image, out_path, note=""):
    text = gi.save_png(image, out_path)
    if note:
        text += "\nmodel note: " + note
    return {"content": [{"type": "text", "text": text}, preview_part(image)]}


def open_refs(args):
    return [Image.open(p) for p in args.get("ref_paths", [])]


def tool_generate_image(args):
    image, note = gi.generate(gi.SYSTEM_GENERATE, args["prompt"], images=open_refs(args),
                              aspect=args.get("aspect", "1:1"), size=args.get("size"))
    return image_result(image, args["out_path"], note)


def tool_edit_image(args):
    images = [Image.open(args["image_path"])] + open_refs(args)
    image, note = gi.generate(gi.SYSTEM_EDIT, args["prompt"], images=images)
    return image_result(image, args["out_path"], note)


def tool_generate_transparent_image(args):
    rgba, white, black = gi.render_transparent(args["prompt"], refs=open_refs(args),
                                               aspect=args.get("aspect", "1:1"),
                                               size=args.get("size"))
    result = image_result(rgba, args["out_path"])
    if args.get("keep_steps"):
        base, _ = os.path.splitext(args["out_path"])
        for img, suffix in ((white, "_white"), (black, "_black")):
            result["content"][0]["text"] += "\n" + gi.save_png(img, base + suffix + ".png")
    return result


def tool_extract_region(args):
    source = Image.open(args["image_path"])
    crop = gi.crop_rect(source, args["rect"])
    if args.get("part"):
        marks = None
        if args.get("marks_path"):
            marks = Image.open(args["marks_path"])
            if marks.size == source.size:
                marks = gi.crop_rect(marks, args["rect"])
        crop = gi.extract_part(crop, args["part"], marks=marks)
        if args.get("transparent"):
            crop = gi.white_to_rgba(crop)
    elif args.get("transparent"):
        crop = gi.isolate_transparent(crop)
    return image_result(crop, args["out_path"])


def video_preview_parts(video_path, times=(0.0, 0.5, 1.0), max_side=256):
    """First/middle/last frame previews via ffmpeg; empty list if ffmpeg is missing."""
    ffprobe, ffmpeg = shutil.which("ffprobe"), shutil.which("ffmpeg")
    if not ffmpeg:
        return []
    duration = 0.0
    if ffprobe:
        probe = subprocess.run([ffprobe, "-v", "quiet", "-show_entries", "format=duration",
                                "-of", "csv=p=0", video_path], capture_output=True, text=True)
        try:
            duration = float(probe.stdout.strip())
        except ValueError:
            pass
    parts = []
    for t in times:
        seek = max(0.0, duration * t - (0.05 if t == 1.0 else 0.0))
        with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
            frame_path = tmp.name
        try:
            run = subprocess.run([ffmpeg, "-v", "quiet", "-y", "-ss", f"{seek:.3f}",
                                  "-i", video_path, "-frames:v", "1", frame_path],
                                 capture_output=True)
            if run.returncode == 0 and os.path.getsize(frame_path):
                parts.append(preview_part(Image.open(frame_path), max_side=max_side))
        finally:
            os.unlink(frame_path)
        if not duration:
            break
    return parts


def tool_generate_video(args):
    image = None
    if args.get("image_path"):
        bg = gv.GREEN_SCREEN if args.get("green_screen") else None
        image = gv.prepare_reference(Image.open(args["image_path"]),
                                     aspect=args.get("aspect", "16:9"), bg_color=bg)
    prompt = gv.build_prompt(args["prompt"], raw=args.get("raw_prompt"),
                             green=args.get("green_screen"), loop=args.get("loop"))
    text = gv.generate_video(prompt, args["out_path"], image=image,
                             model=args.get("model", gv.DEFAULT_MODEL),
                             aspect=args.get("aspect", "16:9"),
                             resolution=args.get("resolution"),
                             duration=args.get("duration_sec"),
                             negative_prompt=args.get("negative_prompt"))
    content = [{"type": "text", "text": text}]
    content += video_preview_parts(args["out_path"])
    return {"content": content}


HANDLERS = {
    "generate_image": tool_generate_image,
    "edit_image": tool_edit_image,
    "generate_transparent_image": tool_generate_transparent_image,
    "generate_video": tool_generate_video,
    "extract_region": tool_extract_region,
}


def respond(request_id, result=None, error=None):
    message = {"jsonrpc": "2.0", "id": request_id}
    if error is not None:
        message["error"] = error
    else:
        message["result"] = result
    sys.stdout.write(json.dumps(message) + "\n")
    sys.stdout.flush()


def handle(request):
    method = request.get("method", "")
    request_id = request.get("id")
    params = request.get("params") or {}

    if method.startswith("notifications/"):
        return
    if method == "initialize":
        respond(request_id, {
            "protocolVersion": params.get("protocolVersion", "2024-11-05"),
            "capabilities": {"tools": {}},
            "serverInfo": {"name": "o2-imagegen", "version": "1.0"},
        })
    elif method == "ping":
        respond(request_id, {})
    elif method == "tools/list":
        respond(request_id, {"tools": TOOLS})
    elif method == "tools/call":
        handler = HANDLERS.get(params.get("name"))
        if handler is None:
            respond(request_id, error={"code": -32602, "message": "unknown tool: " + str(params.get("name"))})
            return
        try:
            respond(request_id, handler(params.get("arguments") or {}))
        except Exception as e:  # tool errors go to the model, not to the process exit
            respond(request_id, {"content": [{"type": "text", "text": f"error: {e}"}],
                                 "isError": True})
    elif request_id is not None:
        respond(request_id, error={"code": -32601, "message": "method not found: " + method})


def main():
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            request = json.loads(line)
        except json.JSONDecodeError:
            continue
        handle(request)


if __name__ == "__main__":
    main()
