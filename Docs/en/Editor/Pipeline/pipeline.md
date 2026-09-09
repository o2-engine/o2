## Pipeline. AI content pipelines window

Edits a `.pipeline` asset: a node graph that turns text, images and sounds into game assets with AI providers and local image/audio processing. Open it from the Assets window (double-click a pipeline asset, `Create/Pipeline` in the context menu makes a new one) or from `View/Show Pipeline`.

![pipeline](pipeline.png)

The window is a canvas with node cards. The toolbar runs the whole graph, stops a run, fits the view and opens the provider keys dialog (Gemini, ElevenLabs, Kling; stored in `Work/Pipelines/PipelineSettings.json`). The status line shows the executor log.

### Canvas
- Right-drag or the wheel pans and zooms (down to 1/40 of the natural size, enough for graphs of a few hundred nodes), `F` fits the graph, the camera is saved into the asset.
- Only the cards and links inside the view are updated and drawn; zoomed out past about 1/3 the cards stop drawing their controls (edits, buttons, sliders, toolbars, resize handles) while what the node produced - previews, drawings, the composer stage - stays in place, so panning a large graph stays smooth.
- Right-click on empty space opens `Add/<category>` with every node type (each item carries the node icon), plus `Paste` and `Select all`. Dragging a link into empty space opens the same menu filtered by compatible ports and connects the new node.
- Links are drawn as curves coloured by port type (text - blue, image - orange, video - purple, audio - teal). Double-click a link to add a bend point, drag the points, `Straighten` in the link menu removes them.
- Drag from an output port to an input port to connect; drag from a connected input to detach and re-route. Ports with a `+` accept extra inputs (composer layers, AI references): drop a link on `+` to add one, rename it in place, remove it with `-`.
- Cards are moved by the header and resized by dragging any edge or corner; a card never gets shorter than its content. Box selection, `Ctrl+C/V/D`, `Delete`, `Ctrl+A`, `Ctrl+Z/Y` work as in the scene editor; every change is an undo action of the window.
- The card frame shows the node state: teal - selected, blue - running or queued, green - result up to date, red - failed (the alert button shows the error). A yellow badge counts provider retries.

### Import from AssetsLine
The import button of the window toolbar reads a pipeline exported by AssetsLine: either the plain pipeline JSON or a bundle made with `Export with results…` (a ZIP with `pipeline.json`, `manifest.json` and the results under `results/`; the older JSON bundles are read too). Node types, configs, ports and links are the same on both sides, so the graph comes in as is; the bundled results become the node previews and freshness markers of the new asset (`Pipelines/<name>.pipeline`), so nothing has to be re-run to see them. Imported drawings of draw nodes keep their pixel size.

Results and the cache are keyed by an id stored in the graph itself (`Work/Pipelines/cache/<id>/`), not by the asset: saving the pipeline under another name or duplicating the asset keeps its results. Assets saved before the id existed adopt their asset UID on the first open.

### Nodes
Sources (text, image, audio), text tools (compose template, concat), AI nodes (text, prompt generator, image generation and edit, extract sprite, remove background, video, sound effects, speech, music), image effects (draw, outline, shadow, gradient, colour, composer), audio processing and finish nodes. Toolbars that do not fit the card width wrap onto the next line (the draw tools, the composer tools). Each card holds the same controls as the AssetsLine node: model dropdowns with presets, prompts, seed inheritance, transparent background (white/black two-pass or chroma key with colour, tolerance, softness and spill sliders) and a result preview with a crop frame.

- **Draw** paints strokes over an optional background image: brush and eraser, colour palette and picker, size and opacity sliders, undo/redo/clear. The strokes are stored in the node config and composited over the background by the node.
- **AI image edit** has the same painter behind `Draw over the image`; **AI extract sprite** adds a region tool whose box is sent to the model.
- **Composer** arranges several image inputs on a fixed work area: select a layer on the stage, move it, scale and rotate it with the frame handles (`Shift` moves along one axis); the layers panel lists them front to back with visibility, rename, duplicate, reorder, reset and delete, and the per-layer settings hold size with aspect lock, opacity and a 9-slice with insets and corner scale. `BG` previews against a colour, `Out BG` fills the rendered output, `PNG` and `Layers` export the result and every layer.
- **Audio and video results** play inside the card: play/pause, seek slider, elapsed and total time, loop toggle and volume slider. Audio goes through the engine sound system; video frames and the audio track are extracted with ffmpeg on first view into `Work/Pipelines/cache/<pipeline>/video/<hash>/` (a frame atlas plus a WAV) and reused afterwards.
- **Finish** nodes pick the folder inside `Assets` (typed or chosen from the folder list behind `…`) and the file name; the card shows the full path the node writes to, whether the file already exists and the size of the incoming result (with the resize target when resizing is on), and the locate button selects the saved asset in the assets window. `Save to Assets` or a run writes the file, then the editor rebuilds assets so it is usable in scenes right away.

Local nodes (text tools, image effects, draw, composer, finish) re-run automatically after a change when their upstream results are cached; provider nodes run on their play button or `Run all`. Results are cached by a content signature that follows the node config and every upstream node, so unchanged nodes are not re-requested; `Clear result` in the node menu drops the cache of one node.
