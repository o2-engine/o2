# ImageDiff — pixel comparison of two images

`image_diff.py` compares image B against reference A pixel by pixel
(per-channel tolerance) and writes a difference map: the reference dimmed to
grayscale with differing pixels highlighted in red. When sizes differ, B is
scaled to the reference size first.

CLI:

```
python3 o2/Tools/ImageDiff/image_diff.py <reference.png> <compared.png> [diff.png] \
    [--threshold 16] [--region L T R B]
```

Stats go to stdout as JSON: changed pixel count/share and the bounding box of
the differences.

MCP: server `imagediff` (registered in the repo root `.mcp.json`), tool
`image_diff(a_path, b_path, out_path?, threshold?, region?)` — returns the
stats and a preview of the difference map. Use it to compare UI screenshots
against the PSD mockup references.
