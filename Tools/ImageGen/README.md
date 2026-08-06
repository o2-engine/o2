# ImageGen — Gemini image and video tools for game assets

CLI tools and an MCP server around Google Gemini image generation (Nano Banana 2,
`gemini-3.1-flash-image`) and Veo video generation (`veo-3.1-*`) for producing sprites, assets
and animation clips for the engine. Requires `python3` with `Pillow` and `numpy`.

API key: put it into `api_key.txt` next to the scripts (gitignored), or export `GEMINI_API_KEY`.

## MCP server

`mcp_server.py` — stdio MCP server (no extra dependencies), registered in the repo root
`.mcp.json` as `imagegen`. Tools: `generate_image`, `edit_image`, `generate_transparent_image`,
`generate_video`, `extract_region` — same functionality as the CLIs below; results include the
saved path and a downscaled preview (for video — first/middle/last frames, needs `ffmpeg`).

## CLI

- `generate_image.py "prompt" --out img.png [--aspect 16:9] [--size 1K|2K|4K] [--ref style.png]` — text-to-image.
- `edit_image.py input.png "instruction" --out img.png [--ref style.png]` — edit an image.
- `generate_transparent.py "prompt" --out sprite.png [--ref style.png] [--keep-steps]` — RGBA
  sprite: renders the subject on white, re-renders on black, recovers per-pixel alpha from the pair.
  If all four corners of the result come out opaque the background failed and the render is
  retried, up to 5 attempts.
- `extract_region.py atlas.png --rect x,y,w,h --out part.png [--part "description"]
  [--marks marked.png] [--transparent]` — crop a region; with `--part` the model keeps only the
  described element exactly as it appears and erases everything else in the crop to pure white
  (`--marks` — a copy of the image with hand-drawn marks over extra areas to erase); with
  `--transparent` the result gets an alpha channel.
- `generate_video.py "prompt" --out clip.mp4 [--image ref.png] [--aspect 16:9|9:16]
  [--resolution 720p|1080p] [--duration 4|6|8] [--green] [--loop] [--raw] [--negative "..."]` —
  text/image-to-video via Veo (default `veo-3.1-fast-generate-preview`, quality
  `veo-3.1-generate-preview`). `--image` becomes the first frame — pass a character sprite to
  animate it. `--green` is the chroma-key mode for in-game animations: transparent references
  are composited onto flat green (#00B140), the canvas is padded to the aspect with the same
  green, and the prompt demands the background stay uniform green — the result plays through
  `VideoComponent` with the engine's ChromaKey shader (key #00B140; measure the decoded key
  color, AVFoundation BT.709 vs pl_mpeg BT.601 shift it slightly). `--loop` asks for a
  seamless loop (best effort, not guaranteed frame-exact). Generation is a long-running
  operation, typically 1–5 minutes.

All tools accept `--model` to override the model and `-v` to print the system/user prompts.
The system prompts (no watermarks/text, sprite-friendly framing, pure white/black backgrounds
for alpha recovery, fixed-camera game-animation preamble for video) are baked into
`gemini_image.py` / `gemini_video.py`.

Veo output is MP4 (H.264 720p/1080p, 24 fps, with an audio track — the engine plays video
silent). MP4 plays via hardware decoders on Mac/iOS/Windows/Android/WASM; for the pl_mpeg
path (Linux, `.mpg`) convert: `ffmpeg -i clip.mp4 -c:v mpeg1video -q:v 4 -an clip.mpg`.
