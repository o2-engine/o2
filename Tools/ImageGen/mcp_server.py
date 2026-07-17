#!/usr/bin/env python3
"""MCP server (stdio, newline-delimited JSON-RPC) exposing the Gemini image tools.

Registered in the repo root .mcp.json as server "imagegen". No dependencies beyond
Pillow/numpy. Tool results contain the saved file path and a downscaled preview image.
"""

import base64
import io
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from PIL import Image

import gemini_image as gi

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
                       "pair. Note: fails on subjects that are themselves nearly white.",
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
        "name": "extract_region",
        "description": "Cut a rectangular region out of an image into a separate sprite PNG. "
                       "With transparent=true the region's main subject is isolated and gets an "
                       "alpha channel (two extra model calls; output resolution may differ).",
        "inputSchema": {
            "type": "object",
            "properties": {
                "image_path": {"type": "string", "description": "Source image"},
                "rect": {"type": "array", "items": {"type": "integer"},
                         "minItems": 4, "maxItems": 4,
                         "description": "Region as [x, y, w, h] in pixels"},
                "out_path": OUT_SCHEMA,
                "transparent": {"type": "boolean",
                                "description": "Isolate subject and build alpha channel"},
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
    crop = gi.crop_rect(Image.open(args["image_path"]), args["rect"])
    if args.get("transparent"):
        crop = gi.isolate_transparent(crop)
    return image_result(crop, args["out_path"])


HANDLERS = {
    "generate_image": tool_generate_image,
    "edit_image": tool_edit_image,
    "generate_transparent_image": tool_generate_transparent_image,
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
