# AssetGenerator

Local web utility for generating 2D game assets via AI models. Runs as a
Dockerised backend + React frontend, opened in the host browser.

## Quick start

Requires Docker Desktop (Windows/Mac) or Docker Engine + Compose plugin (Linux).

```
# Windows
start.bat

# macOS / Linux
chmod +x start.command stop.command
./start.command
```

Frontend opens at <http://localhost:5173>, backend listens on <http://localhost:8765>.

Stop with `stop.bat` / `./stop.command`.

## Layout

```
o2/AssetGenerator/
  docker-compose.yml
  .env / .env.example
  start.* / stop.*
  data/               runtime state — settings.json (gitignored)
  shared/             types shared between backend and frontend
  backend/            Fastify + TypeScript
  frontend/           React + Vite + TypeScript
```

By default the host folder mounted into the backend as `/workspace` is two
levels up (`../..`), i.e. the PetStory repo root. Override in `.env`:

```
PROJECT_ROOT_HOST=C:/work/MyOtherProject
```

After changing `PROJECT_ROOT_HOST`, run `docker compose down && start.*`.

`Assets/` and `ContentDatabase/` paths inside settings are then resolved
relative to `/workspace`.

## Settings

Open the gear icon in the top-right and fill in:

- API keys: Gemini (required for Nano Banana 2 image and Gemini text), Claude
  and OpenAI (stubs reserved for future).
- Directories: paths to `Assets/` (output) and `ContentDatabase/` (sources,
  saved pipelines), relative to the mount root.

Stored in `data/settings.json` (gitignored). Empty Gemini key falls back to
`GEMINI_API_KEY` env var if set.

## Tools

### Node Image Editor

Graphical pipeline editor. Right-click on the canvas to add nodes from a
context menu. Connect output → input by dragging between ports (types must
match: `text` or `image`). Drop an edge on empty space to create-and-connect
a compatible node in one gesture.

Press Play on a `finish (save)` node to run. Execution is **pull-based**:
finish recursively pulls each connected input, evaluating producers in the
right order; node rings light up as they activate and fade out when done.

Pipelines are saved as JSON to
`<ContentDatabase>/NodeImageGen/Pipelines/<name>.json`.

Built-in nodes:

| type | purpose |
|---|---|
| `finish` | Save the connected text/image input to disk; press Play here to run |
| `sourceText` | Inline text source |
| `sourceImage` | Read an image file from `Assets/` or `ContentDatabase/` |
| `promptCompose` | Build text by substituting `{key}` values from config or text inputs |
| `nanoBananaGen` | Gemini image generation (Nano Banana 2) with optional reference |
| `removeBackground` | Two-pass alpha extraction from white-bg + black-bg shots |

### UX

- Wheel: zoom canvas to cursor
- Right mouse drag: pan canvas
- Right click on background / node / edge: context menu (add nodes, copy/paste/delete)
- Left drag on port: create edge — drop on compatible port to connect, drop
  on empty space to open a "create node here" menu filtered by port type
- Left click on node: select; Shift+click: add to selection; drag: move
- Left drag on background: rubber-band selection
- All node configuration is edited inline inside the node
- Hotkeys: Ctrl+C / Ctrl+V / Ctrl+D / Ctrl+A / Ctrl+S / Del

Undo/Redo are not yet implemented (planned for next iteration).

## Architecture

Backend: Fastify routes for settings, pipelines (file CRUD), assets (file
listing/read/write), execute (SSE-streamed pipeline run), node-types
(palette schemas). Node implementations live in `backend/src/nodes/`,
registered in `registry.ts`.

Frontend: React + Zustand for editor state, SVG-based canvas. Pipeline data
model in `shared/types.ts` is reused on both sides.
