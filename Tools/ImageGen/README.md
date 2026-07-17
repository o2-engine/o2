# ImageGen — Gemini image tools for game assets

CLI tools and an MCP server around Google Gemini image generation (Nano Banana 2,
`gemini-3.1-flash-image`) for producing sprites and assets for the engine. Requires `python3`
with `Pillow` and `numpy`.

API key: put it into `api_key.txt` next to the scripts (gitignored), or export `GEMINI_API_KEY`.

## MCP server

`mcp_server.py` — stdio MCP server (no extra dependencies), registered in the repo root
`.mcp.json` as `imagegen`. Tools: `generate_image`, `edit_image`, `generate_transparent_image`,
`extract_region` — same functionality as the CLIs below; results include the saved path and a
downscaled preview.

## CLI

- `generate_image.py "prompt" --out img.png [--aspect 16:9] [--size 1K|2K|4K] [--ref style.png]` — text-to-image.
- `edit_image.py input.png "instruction" --out img.png [--ref style.png]` — edit an image.
- `generate_transparent.py "prompt" --out sprite.png [--ref style.png] [--keep-steps]` — RGBA
  sprite: renders the subject on white, re-renders on black, recovers per-pixel alpha from the pair.
- `extract_region.py atlas.png --rect x,y,w,h --out part.png [--transparent]` — crop a region;
  with `--transparent` the region's subject is isolated and gets an alpha channel.

All tools accept `--model` to override the model and `-v` to print the system/user prompts.
The system prompts (no watermarks/text, sprite-friendly framing, pure white/black backgrounds
for alpha recovery) are baked into `gemini_image.py`.
