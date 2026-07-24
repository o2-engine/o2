#!/usr/bin/env python3
"""MCP server (stdio, newline-delimited JSON-RPC) exposing the image diff tool.

Registered in the repo root .mcp.json as server "imagediff". Compares two
images pixel by pixel and returns stats plus a preview of the difference map
(reference dimmed to grayscale, differing pixels highlighted in red).
"""

import base64
import io
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from PIL import Image

import image_diff

TOOLS = [
    {
        "name": "image_diff",
        "description": "Compare image B against reference A pixel by pixel. Returns changed "
                       "pixel stats and a difference map preview: the reference dimmed to "
                       "grayscale with differing pixels highlighted in red. B is scaled to "
                       "the size of A when sizes differ.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "a_path": {"type": "string", "description": "Reference image path"},
                "b_path": {"type": "string", "description": "Compared image path"},
                "out_path": {"type": "string",
                             "description": "Where to save the diff highlight PNG (optional)"},
                "threshold": {"type": "integer",
                              "description": "Per-channel difference tolerance 0-255 (default 16)"},
                "region": {"type": "array", "items": {"type": "integer"},
                           "minItems": 4, "maxItems": 4,
                           "description": "Optional [left, top, right, bottom] box of the "
                                          "reference to compare"},
            },
            "required": ["a_path", "b_path"],
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


def tool_image_diff(args):
    out_path = args.get("out_path")
    stats = image_diff.compare(args["a_path"], args["b_path"], out_path,
                               args.get("threshold", 16), args.get("region"))

    content = [{"type": "text", "text": json.dumps(stats, indent=1)}]
    if out_path and os.path.exists(out_path):
        content.append(preview_part(Image.open(out_path)))

    return {"content": content}


HANDLERS = {
    "image_diff": tool_image_diff,
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
            "serverInfo": {"name": "o2-imagediff", "version": "1.0"},
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
