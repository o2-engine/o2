#!/usr/bin/env python3
"""MCP server (stdio, newline-delimited JSON-RPC) exposing the PSD tools.

Registered in the repo root .mcp.json as server "psd". Requires psd-tools
(pip install psd-tools). Renders return a downscaled preview image so the
model can see the document.
"""

import base64
import io
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import psd_lib
import psd_to_o2

PSD_SCHEMA = {"type": "string", "description": "Path to the .psd file"}
HIDDEN_SCHEMA = {"type": "boolean", "description": "Include hidden layers (default false)"}

TOOLS = [
    {
        "name": "psd_structure",
        "description": "Read the PSD layer tree: names, kinds, hierarchy, visibility, opacity, "
                       "blend modes, pixel bounding boxes and sizes. Layers are listed "
                       "bottom-to-top (draw order).",
        "inputSchema": {
            "type": "object",
            "properties": {"psd_path": PSD_SCHEMA, "include_hidden": HIDDEN_SCHEMA},
            "required": ["psd_path"],
        },
    },
    {
        "name": "psd_render",
        "description": "Composite the whole PSD into a PNG and return a preview for viewing. "
                       "Use scale to downsize large mockups.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "psd_path": PSD_SCHEMA,
                "out_path": {"type": "string", "description": "Output PNG path"},
                "scale": {"type": "number", "description": "Uniform scale factor (default 1.0)"},
            },
            "required": ["psd_path", "out_path"],
        },
    },
    {
        "name": "psd_extract_layers",
        "description": "Save raster layers of the PSD as individual RGBA PNG files named after "
                       "the layers. Optionally filter by layer names or slash paths "
                       "(e.g. \"Panel/Buttons/PlayBtn\").",
        "inputSchema": {
            "type": "object",
            "properties": {
                "psd_path": PSD_SCHEMA,
                "out_dir": {"type": "string", "description": "Output folder for the PNGs"},
                "layers": {"type": "array", "items": {"type": "string"},
                           "description": "Only these layer names/paths (default: all)"},
                "include_hidden": HIDDEN_SCHEMA,
            },
            "required": ["psd_path", "out_dir"],
        },
    },
    {
        "name": "psd_layer_positions",
        "description": "Flat list of layer placements: PSD pixel bbox/center/size plus the "
                       "position in o2 world space (origin at canvas center, y up).",
        "inputSchema": {
            "type": "object",
            "properties": {"psd_path": PSD_SCHEMA, "include_hidden": HIDDEN_SCHEMA},
            "required": ["psd_path"],
        },
    },
    {
        "name": "psd_to_o2_prefab",
        "description": "Build an o2 actor prefab (.proto) replicating the PSD layout: groups "
                       "become container actors, raster layers become actors with "
                       "ImageComponent; hierarchy, order, positions and opacity are preserved. "
                       "Extracts layer images with .meta files into a folder under the assets "
                       "root and writes the prefab there.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "psd_path": PSD_SCHEMA,
                "out_dir": {"type": "string",
                            "description": "Folder under the assets root for images and the .proto, "
                                           "e.g. \"PsdImport/MainMenu\""},
                "assets_root": {"type": "string",
                                "description": "Project assets root (default \"Assets\")"},
                "atlas": {"type": "string",
                          "description": "Atlas asset for extracted images relative to assets root "
                                         "(default \"Basic.atlas\", \"\" for standalone textures)"},
                "scale": {"type": "number", "description": "Transform scale factor (default 1.0)"},
                "include_hidden": HIDDEN_SCHEMA,
                "name": {"type": "string", "description": "Prefab name (default: PSD file name)"},
            },
            "required": ["psd_path", "out_dir"],
        },
    },
]


def preview_part(image, max_side=640):
    im = image.copy()
    im.thumbnail((max_side, max_side))
    buf = io.BytesIO()
    im.save(buf, format="PNG")
    return {"type": "image", "data": base64.b64encode(buf.getvalue()).decode(),
            "mimeType": "image/png"}


def text_result(payload):
    return {"content": [{"type": "text", "text": json.dumps(payload, indent=1, ensure_ascii=False)}]}


def tool_psd_structure(args):
    return text_result(psd_lib.structure(args["psd_path"], args.get("include_hidden", False)))


def tool_psd_render(args):
    image = psd_lib.render(args["psd_path"], args["out_path"], args.get("scale"))
    text = f"saved {args['out_path']} ({image.width}x{image.height})"
    return {"content": [{"type": "text", "text": text}, preview_part(image)]}


def tool_psd_extract_layers(args):
    saved = psd_lib.extract_layers(args["psd_path"], args["out_dir"], args.get("layers"),
                                   args.get("include_hidden", False))
    return text_result(saved)


def tool_psd_layer_positions(args):
    psd, layers = psd_lib.flat_layers(args["psd_path"], args.get("include_hidden", False),
                                      groups=True)
    result = []
    for path, layer in layers:
        left, top, right, bottom = layer.bbox
        cx, cy = (left + right) * 0.5, (top + bottom) * 0.5
        result.append({
            "path": path,
            "kind": psd_lib.layer_kind(layer),
            "bbox": [int(left), int(top), int(right), int(bottom)],
            "center": [round(cx, 1), round(cy, 1)],
            "size": [int(right - left), int(bottom - top)],
            "o2_position": [round(cx - psd.width * 0.5, 1), round(psd.height * 0.5 - cy, 1)],
        })
    return text_result({"canvas": [psd.width, psd.height], "layers": result})


def tool_psd_to_o2_prefab(args):
    result = psd_to_o2.build_prefab(
        args["psd_path"], args["out_dir"], args.get("assets_root", "Assets"),
        args.get("atlas", "Basic.atlas") or None, args.get("scale", 1.0),
        args.get("include_hidden", False), args.get("name"))
    return text_result(result)


HANDLERS = {
    "psd_structure": tool_psd_structure,
    "psd_render": tool_psd_render,
    "psd_extract_layers": tool_psd_extract_layers,
    "psd_layer_positions": tool_psd_layer_positions,
    "psd_to_o2_prefab": tool_psd_to_o2_prefab,
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
            "serverInfo": {"name": "o2-psd", "version": "1.0"},
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
