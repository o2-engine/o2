# Pipelines

AI content pipelines: node graphs that turn text, images and sounds into assets through provider APIs and local processing. Sources live in `o2/Editor/Sources/o2Editor/Pipeline` (namespace `Editor`); the editor side is the Pipeline window. The runtime only carries `o2::PipelineAsset`, a `.pipeline` asset holding the graph as a `DataDocument`; `PipelineGraph::LoadFromAsset` / `SaveToAsset` convert between the document and the editor graph. The document is the AssetsLine pipeline JSON (see Format).

## Graph

- **Editor::PipelineGraph** — `nodes`, `edges`, saved camera. `Validate()` reports type mismatches and cycles, `WouldMakeCycle(from, to)`, `FindEdgeToPort`, `GetOutgoingEdges`, `RemoveDanglingEdges`, `ResolveSeeds()` (image nodes inherit the seed of the upstream image node when `inheritSeed` is set), `ComputeSignatures()` — a content hash per node built from its config (UI-only keys excluded, see `GetUiOnlyConfigKeys()`), the signatures of upstream nodes and the seed.
- **Editor::PipelineNode** — `id`, `nodeType`, `position`/`size` (y down, as in the web editor), `inputs`/`outputs` (`Editor::PipelinePort`: `id`, `name`, `portType`, `custom`), `config` — a `DataDocument` with `GetConfigString/Number/Bool/Value`, `SetConfig*`, `RemoveConfig`, `HasConfig`. Extra inputs added by the user are kept in `customInputs` (`GetCustomInputs/SetCustomInputs`); `PipelineNodeRegistry::SyncNodeWithSchema` rebuilds the port list from the schema and the custom inputs.
- **Editor::PipelinePortType** — `Text`, `Image`, `Video`, `Audio`; a link connects ports of one type only.
- **o2::PipelineAsset** (framework) — the `.pipeline` asset holding the graph document; the editor reads it into a `PipelineGraph` and writes it back on every change.

## Format
A `.pipeline` file is the AssetsLine `Pipeline` document: `schemaVersion`, `id`, `name`, `nodes` (`id`, `type`, `position`, `size` as `{width, height}`, `config`, `inputs`/`outputs` of `{id, name, type, color?, custom?}`), `edges` (`id`, endpoints, `points`), `camera` (`{x, y, scale}`: screen = world * scale + (x, y)). `PipelineGraph::LoadFromJson` / `SaveToJson` (and `LoadFromJsonString` / `ToJsonString`, used by undo and the clipboard too) read and write it; members the editor does not use - `annotations`, `icon`, a port's `color`, anything newer - are kept in `extra` of the graph, node and edge and written back untouched. Whole numbers are written as integers and the rest rounded to 1/1000, so a document read and written without an edit stays byte-stable. The camera converts through a fixed nominal view (`GetNominalViewSize`). The older o2 format (`{"graph": ...}`, reflection) is recognised by `IsLegacyDocument`, read, and saved in the new format on the next save.

Numbers both editors derive from the document agree: the implicit seed is AssetsLine's (32-bit FNV-1a of the node id, `Number()` semantics for `config.seed`, inheritance off only for a literal `false`), and the UI-only config keys are the same list on both sides. A finish node without `assetPath` writes to `Generated/<filename>` (`PipelineNodeRegistry::GetFinishAssetPath`). A paste gives nodes and ports new ids and remaps the ids config refers to (`PipelineNode::RemapConfigPortIds`: custom inputs, composer layers, extract regions).

## Node types

`Editor::PipelineNodeRegistry` holds every `Editor::PipelineNodeSchema` (`type`, `label`, `category`, `description`, `inputs`, `outputs`, `addableInputs`, `instant`, `perPortRun`, `defaultSize`) with its implementation (`Editor::PipelineNodeBase::Run` — a `Coroutine<PipelineRunResult>` receiving the execution context and the input values). `AllSchemas()`, `GetSchema(type)`, `CreateNode(type, position)`, `IsFinishType`. Besides `Run` an implementation may override `InitNode(node)` (the config a fresh node starts with), and, for a `perPortRun` node, `SyncPorts(node)` (it owns its output list, so `SyncNodeWithSchema` leaves the schema outputs alone), `PortCacheExcludedKeys()` and `PortCacheVariant(node, portId)` - the config keys the per-port signature drops and what it folds back in for one port.

`Editor::PipelineRegions` is the part list of the extract node: `PipelineExtractRegion` (`id` - the id of the output port carrying it, `name` - what to extract and the port name, a normalised box), `Read`/`Write` on the node config (a node saved before regions existed reads as one part built from its `prompt` and `roi`), `PortNames` (unique, non-empty), `SyncPorts` (rebuilds the outputs keeping the ids, so links survive a rename), `RegionOfPort`, `autoSplitPrompt` and `ParseAutoSplit(answer)` (the model answers with `box_2d` as `[ymin, xmin, ymax, xmax]` on a 0..1000 grid).

Categories: sources (`sourceText`, `sourceImage`, `sourceAudio`), transforms (`textCompose`, `textConcat`, `drawImage`, `imageOutline`, `imageShadow`, `imageGradient`, `imageColor`, `audioProcess`), AI (`aiText`, `textEdit`, `promptGen`, `nanoBananaGen`, `imageEdit`, `imageExtract`, `removeBackground`, `aiRemoveBg`, `videoGen`, `sfxGen`, `ttsSpeech`, `musicGen`), outputs (`composer`, `finishImage`, `finishText`, `finishVideo`, `finishAudio`). `instant` nodes run locally and are re-applied automatically by the editor, from the results their inputs show (`PipelineExecutor::ExecuteSingle`): a node generated again keeps its signature, so a stored result of the effect could be made from the old picture. Cached-only runs still compute sources, which only read their file.

Providers (`o2/Pipeline/Providers`): `GeminiProvider` (text, image generation and edit, Imagen, Veo, Lyria, TTS), `ElevenLabsProvider` (speech, sound effects, music), `VideoProviders` (Kling). Keys come from `Editor::PipelineSettings` (`Work/Pipelines/PipelineSettings.json`). Requests use `o2Network.RequestAsync` with retries reported as `Retry` events.

Image helpers: `Editor::PipelineImageOps` (resize, `Fit` - scale into a size keeping the aspect ratio on a transparent canvas, crop, crop to content, chroma key, two-pass matte, outline, shadow, gradient, colour adjust, flip, rotate, nine-slice, `ComposeLayers`, `Blank`, `Pixel` accessors — bitmaps are RGBA with image-space `Pixel(bmp, x, y)`), `Editor::PipelineTransparency` (white/black two-pass and chroma post-steps), `Editor::PipelineAudio`. `ResolveComposerLayers(node)` lists composer layers back to front (image inputs, duplicates from `dupLayers`, order from `layerOrder`).

## Execution

`Editor::PipelineExecutor::Execute(pipelineId, graph, targetNodeId, dirtyNodeIds, cachedOnly)` runs the upstream chain of the target in dependency order. Results are cached under `Work/Pipelines/cache/<pipelineId>/{content,previews,ran}` by node signature: a node whose signature matches the cached one is served from disk, `cachedOnly` serves provider nodes from their cache entry or, failing that, from their last rendered preview (without marking them ran), and refuses only nodes that have neither (`Not cached`). `ResolveSourceValue(node, assetsPath, value, error)` (`Nodes/PipelineNodesCommon.h`) is the one place a source node turns its config into a value: the source nodes run through it and the editor uses it to give sources their value without a run. `PipelineUtils::PrettyModelName(id)` maps model ids to the names the model lists show. `PipelineGraph::GetRunTargets()` returns what a whole-graph run targets: every node with no outgoing edges, finish nodes first, skipping sources nothing consumes. A `perPortRun` node is evaluated output by output: only the ports the branch consumes are produced (the target computes all of its own), each under its own signature (the shared config minus `PortCacheExcludedKeys`, plus `PortCacheVariant`) and with its own preview (`GetPortPreviewPath`, `LoadPortPreview`, `NodeOutput` carrying `portId`), so one part failing or changing leaves the others in place; a consumer of such an output hashes the part it reads (`PipelineGraph::ComputePortSignature`, `OutputSigKey`), so two identically configured nodes reading two parts never share a cache slot. `ExecuteSingle` runs one node, `Cancel()` stops after the current node, `assetsPathOverride` redirects finish nodes (tests). Events (`Editor::PipelineExecEvent`): `NodeState` (`queued`, `running`, `done`, `error`), `NodeOutput` (the `Editor::PipelineValue` with `previewPath`/`srcPreviewPath`), `Retry`, `Log`, `Done`, `Fatal`. `LoadPreview` and `ClearNodeCache` manage the cache from the editor.

Finish nodes write the result to `<assets>/<assetPath>.<ext>` (optionally cropped and resized; `keepAspect` fits the image into `resizeW`x`resizeH` instead of squashing it) and set `assetsChanged`; in the editor the executor then calls `o2Assets.RebuildAssets()`.

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

Every entry point that can build card widgets between frames (`PipelineEditor::Update`, `OnExecutorEvent`, `PipelineNodeWidget::OnOutputChanged/OnConfigChanged/ApplyRuntime`, `PipelineNodeBody::RebuildBody`, the composer's layer panel) pushes `PushEditorScopeOnStack`: a widget built outside the editor scope is registered as a scene object, which polluted the scene and crashed it when the card was rebuilt.

## AssetsLine sync
`Editor::AssetsLineSync` (`o2/Editor/Sources/o2Editor/Pipeline/Sync`) keeps the pipeline assets of one folder and a linked AssetsLine project in step; `EditorApplication` owns it and calls `Update` every frame, the Pipeline window gives it `isPipelineFileBusy` (the edited asset has unsaved changes), `onPipelineFileChanged` (reload the edited asset) and `onResultsChanged` (reload previews), and autosaves a synced pipeline a second after an edit. The link is `o2::AssetsLineConfig` in `o2Config.assetsLine` (`enabled`, `serverUrl`, `projectId`, `projectName`, `folder`, `writeFinishAssets`, `pollInterval`); the token and the sync state are per user in `Work/Pipelines/sync/` (`credentials.json`, `state.json`, `base/<id>.json`).

`AssetsLineClient` sends the requests (`Authorization: Bearer`, `X-Project-Id`, `X-Sync-Client`) through `o2Network`. Connecting: `StartConnect` asks the server for a code and opens its approval page, then polls until the token comes; `ConnectWithToken` checks a hand-made token with `/api/sync/me`.

A pass (`SyncPass`, coroutine) reads `/api/sync/state`, then:
- **documents** (`SyncDocuments`): scans the folder (a file without an id adopts its meta UID, a copied file gets new ids, an asset-backed source gets an upload id from its content), compares every pipeline with its base (the last version both sides had, camera ignored) and the stored revision: pushes a local change with `baseRev` (the server merges it with changes made meanwhile when the base comes along; a merged answer is written back unless the file changed again), pulls a remote change, renames and deletes on either side; files the editor holds unsaved are skipped;
- **results** (`SyncResults`, when the server's result counter or a local file moved): maps AssetsLine result keys (`<node>`, `<node>.src`, `<node>#<port>`, `<node>.raw` for the raw render of a chroma node) to the preview and content files of `Work/Pipelines/cache/<id>/`, uploads what changed here and downloads what changed there (both changed: the fresh one wins), exchanges source uploads, marks fresh on the other side what is fresh on this one - a node fresh on the server gets its download copied into the content cache under the key a local run looks it up by (`PipelineExecutor::ContentSignature`); a node generated again on either side keeps its signature, so a result that arrives replaces the kept generation, and a result sent from here is marked fresh again for the server to do the same - and writes finish results into the assets (downloaded, or computed by a cached-only run of the finish node).

Tests: `PipelineFormat.*` (format, seeds, round trip of the AssetsLine documents found on the machine), `AssetsLineSync_.*` (end to end against a real server: set `O2_ASSETSLINE_TEST_URL`, a throwaway one is started by `AssetsLine/backend/scripts/test-instance.sh start`), `PipelineSyncUiFixture.*` in `o2EditorUITests` (autosave, reload, the dialog).

## Import
`PipelineImport::ParseBytes` reads an AssetsLine export by content: a pipeline JSON, a JSON bundle with data URL results (`Parse`) or a ZIP bundle (`ParseZip`: `pipeline.json`, an optional `manifest.json` listing `results/<file>` per node with its media type and mime, files not in the manifest keyed by `results/<nodeId>.<ext>`) into a `PipelineGraph` with a fresh `id` and a map of `PipelineValue` results; `StoreResults` writes them as previews, content cache entries and freshness markers of a pipeline id; `ImportFile` creates the asset under a folder, stores the results under the graph id and rebuilds the assets. `PipelineZip` is the small archive reader and writer behind it (stored and deflate entries, zlib).

`O2_PIPELINE_RUN=<file>` runs a `.pipeline` file headlessly through the `PipelineRunAsset.RunFile` test of `o2EditorTests`: every run target of the graph is executed against the real providers, so an art pipeline can be driven from the command line instead of the Pipeline window (`O2_PIPELINE_NODE` limits the run to one node, `O2_PIPELINE_ASSETS` says where finish nodes write, `O2_PIPELINE_WORK` where the cache lives, `O2_PIPELINE_TIMEOUT` the seconds budget of one target).

`O2_PIPELINE_IMPORT=<file>` makes the Pipeline window import that AssetsLine export on its first update (a development aid for reproducing import problems without the file dialog); `PipelineWindow::ImportFile` catches exceptions and reports them in the status line.

The graph `id` is the cache key the editor uses (`PipelineEditor::GetPipelineId`); an asset without one adopts its UID on open, so results cached by UID before the field existed stay reachable, and a graph saved into another asset keeps its cache.
