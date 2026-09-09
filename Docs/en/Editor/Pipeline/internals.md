# Pipelines

AI content pipelines: node graphs that turn text, images and sounds into assets through provider APIs and local processing. Sources live in `o2/Editor/Sources/o2Editor/Pipeline` (namespace `Editor`); the editor side is the Pipeline window. The runtime only carries `o2::PipelineAsset`, a `.pipeline` asset holding the graph as a `DataDocument`; `PipelineGraph::LoadFromAsset` / `SaveToAsset` convert between the document and the editor graph.

## Graph

- **Editor::PipelineGraph** — `nodes`, `edges`, saved camera. `Validate()` reports type mismatches and cycles, `WouldMakeCycle(from, to)`, `FindEdgeToPort`, `GetOutgoingEdges`, `RemoveDanglingEdges`, `ResolveSeeds()` (image nodes inherit the seed of the upstream image node when `inheritSeed` is set), `ComputeSignatures()` — a content hash per node built from its config (UI-only keys excluded, see `GetUiOnlyConfigKeys()`), the signatures of upstream nodes and the seed.
- **Editor::PipelineNode** — `id`, `nodeType`, `position`/`size` (y down, as in the web editor), `inputs`/`outputs` (`Editor::PipelinePort`: `id`, `name`, `portType`, `custom`), `config` — a `DataDocument` with `GetConfigString/Number/Bool/Value`, `SetConfig*`, `RemoveConfig`, `HasConfig`. Extra inputs added by the user are kept in `customInputs` (`GetCustomInputs/SetCustomInputs`); `PipelineNodeRegistry::SyncNodeWithSchema` rebuilds the port list from the schema and the custom inputs.
- **Editor::PipelinePortType** — `Text`, `Image`, `Video`, `Audio`; a link connects ports of one type only.
- **o2::PipelineAsset** (framework) — the `.pipeline` asset holding the graph document; the editor reads it into a `PipelineGraph` and writes it back on every change.

## Node types

`Editor::PipelineNodeRegistry` holds every `Editor::PipelineNodeSchema` (`type`, `label`, `category`, `description`, `inputs`, `outputs`, `addableInputs`, `instant`, `defaultSize`) with its implementation (`Editor::PipelineNodeBase::Run` — a `Coroutine<PipelineRunResult>` receiving the execution context and the input values). `AllSchemas()`, `GetSchema(type)`, `CreateNode(type, position)`, `IsFinishType`.

Categories: sources (`sourceText`, `sourceImage`, `sourceAudio`), transforms (`textCompose`, `textConcat`, `drawImage`, `imageOutline`, `imageShadow`, `imageGradient`, `imageColor`, `audioProcess`), AI (`aiText`, `textEdit`, `promptGen`, `nanoBananaGen`, `imageEdit`, `imageExtract`, `removeBackground`, `videoGen`, `sfxGen`, `ttsSpeech`, `musicGen`), outputs (`composer`, `finishImage`, `finishText`, `finishVideo`, `finishAudio`). `instant` nodes run locally and are re-applied automatically by the editor.

Providers (`o2/Pipeline/Providers`): `GeminiProvider` (text, image generation and edit, Imagen, Veo, Lyria, TTS), `ElevenLabsProvider` (speech, sound effects, music), `VideoProviders` (Kling). Keys come from `Editor::PipelineSettings` (`Work/Pipelines/PipelineSettings.json`). Requests use `o2Network.RequestAsync` with retries reported as `Retry` events.

Image helpers: `Editor::PipelineImageOps` (resize, crop, crop to content, chroma key, two-pass matte, outline, shadow, gradient, colour adjust, flip, rotate, nine-slice, `ComposeLayers`, `Blank`, `Pixel` accessors — bitmaps are RGBA with image-space `Pixel(bmp, x, y)`), `Editor::PipelineTransparency` (white/black two-pass and chroma post-steps), `Editor::PipelineAudio`. `ResolveComposerLayers(node)` lists composer layers back to front (image inputs, duplicates from `dupLayers`, order from `layerOrder`).

## Execution

`Editor::PipelineExecutor::Execute(pipelineId, graph, targetNodeId, dirtyNodeIds, cachedOnly)` runs the upstream chain of the target in dependency order. Results are cached under `Work/Pipelines/cache/<pipelineId>/{content,previews,ran}` by node signature: a node whose signature matches the cached one is served from disk, `cachedOnly` refuses provider nodes that are not cached (`Not cached`). `ExecuteSingle` runs one node, `Cancel()` stops after the current node, `assetsPathOverride` redirects finish nodes (tests). Events (`Editor::PipelineExecEvent`): `NodeState` (`queued`, `running`, `done`, `error`), `NodeOutput` (the `Editor::PipelineValue` with `previewPath`/`srcPreviewPath`), `Retry`, `Log`, `Done`, `Fatal`. `LoadPreview` and `ClearNodeCache` manage the cache from the editor.

Finish nodes write the result to `<assets>/<assetPath>.<ext>` (optionally cropped and resized) and set `assetsChanged`; in the editor the executor then calls `o2Assets.RebuildAssets()`.

`Editor::PipelineValue` — `type`, `data` (text or encoded bytes), `mimeType`; `Text`, `Image(bitmap)`, `ImageBytes`, `Bytes` constructors, `GetBitmap()`; `DecodeImageBytes` decodes PNG/JPEG bytes. `Editor::PipelineUtils` — work paths (`SetWorkPathOverride`), uploads, data URLs, base64, FNV hashing, hex colours, file bytes.

<details>
<summary>Example</summary>

```cpp
PipelineGraph graph;
auto text = PipelineNodeRegistry::CreateNode("sourceText", Vec2F());
text->SetConfigString("text", "pixel art coin");
auto gen = PipelineNodeRegistry::CreateNode("nanoBananaGen", Vec2F(300, 0));
auto finish = PipelineNodeRegistry::CreateNode("finishImage", Vec2F(600, 0));
finish->SetConfigString("assetPath", "Generated/coin");
graph.nodes = { text, gen, finish };
// edges: text.out -> gen.prompt, gen.out -> finish.in

auto executor = mmake<PipelineExecutor>();
executor->onEvent = [](const PipelineExecEvent& e) { if (e.type == PipelineExecEvent::Type::Log) o2Debug.Log(e.message); };
executor->Execute("coin", graph, finish->id, {}, false);
```
</details>

## Import
`PipelineImport::ParseBytes` reads an AssetsLine export by content: a pipeline JSON, a JSON bundle with data URL results (`Parse`) or a ZIP bundle (`ParseZip`: `pipeline.json`, an optional `manifest.json` listing `results/<file>` per node with its media type and mime, files not in the manifest keyed by `results/<nodeId>.<ext>`) into a `PipelineGraph` with a fresh `id` and a map of `PipelineValue` results; `StoreResults` writes them as previews, content cache entries and freshness markers of a pipeline id; `ImportFile` creates the asset under a folder, stores the results under the graph id and rebuilds the assets. `PipelineZip` is the small archive reader and writer behind it (stored and deflate entries, zlib).

The graph `id` is the cache key the editor uses (`PipelineEditor::GetPipelineId`); an asset without one adopts its UID on open, so results cached by UID before the field existed stay reachable, and a graph saved into another asset keeps its cache.
