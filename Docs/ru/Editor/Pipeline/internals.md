# Пайплайны

AI-пайплайны контента: графы нод, которые превращают текст, картинки и звуки в ассеты через API провайдеров и локальную обработку. Исходники - в `o2/Editor/Sources/o2Editor/Pipeline` (пространство имён `Editor`); редакторская часть - окно Pipeline. В рантайме остаётся только `o2::PipelineAsset` - ассет `.pipeline`, хранящий граф как `DataDocument`; `PipelineGraph::LoadFromAsset` / `SaveToAsset` переводят документ в граф редактора и обратно.

## Граф

- **Editor::PipelineGraph** — `nodes`, `edges`, сохранённая камера. `Validate()` сообщает о несовпадении типов и циклах, `WouldMakeCycle(from, to)`, `FindEdgeToPort`, `GetOutgoingEdges`, `RemoveDanglingEdges`, `ResolveSeeds()` (ноды картинок наследуют seed ноды-картинки выше по графу при `inheritSeed`), `ComputeSignatures()` — хеш содержимого каждой ноды из её конфига (без UI-ключей, см. `GetUiOnlyConfigKeys()`), сигнатур нод выше по графу и seed.
- **Editor::PipelineNode** — `id`, `nodeType`, `position`/`size` (ось y вниз, как в веб-редакторе), `inputs`/`outputs` (`Editor::PipelinePort`: `id`, `name`, `portType`, `custom`), `config` — `DataDocument` с `GetConfigString/Number/Bool/Value`, `SetConfig*`, `RemoveConfig`, `HasConfig`. Добавленные пользователем входы хранятся в `customInputs` (`GetCustomInputs/SetCustomInputs`); `PipelineNodeRegistry::SyncNodeWithSchema` пересобирает список портов из схемы и пользовательских входов.
- **Editor::PipelinePortType** — `Text`, `Image`, `Video`, `Audio`; связь соединяет только порты одного типа.
- **o2::PipelineAsset** (движок) — ассет `.pipeline` с документом графа; редактор читает его в `PipelineGraph` и записывает обратно при каждом изменении.

## Типы нод

`Editor::PipelineNodeRegistry` хранит все `Editor::PipelineNodeSchema` (`type`, `label`, `category`, `description`, `inputs`, `outputs`, `addableInputs`, `instant`, `defaultSize`) вместе с реализацией (`Editor::PipelineNodeBase::Run` — `Coroutine<PipelineRunResult>`, получающая контекст выполнения и значения входов). `AllSchemas()`, `GetSchema(type)`, `CreateNode(type, position)`, `IsFinishType`.

Категории: источники (`sourceText`, `sourceImage`, `sourceAudio`), преобразования (`textCompose`, `textConcat`, `drawImage`, `imageOutline`, `imageShadow`, `imageGradient`, `imageColor`, `audioProcess`), AI (`aiText`, `textEdit`, `promptGen`, `nanoBananaGen`, `imageEdit`, `imageExtract`, `removeBackground`, `videoGen`, `sfxGen`, `ttsSpeech`, `musicGen`), выходы (`composer`, `finishImage`, `finishText`, `finishVideo`, `finishAudio`). Ноды с `instant` выполняются локально и автоматически перезапускаются редактором.

Провайдеры (`o2/Pipeline/Providers`): `GeminiProvider` (текст, генерация и правка картинок, Imagen, Veo, Lyria, TTS), `ElevenLabsProvider` (речь, звуковые эффекты, музыка), `VideoProviders` (Kling). Ключи берутся из `Editor::PipelineSettings` (`Work/Pipelines/PipelineSettings.json`). Запросы идут через `o2Network.RequestAsync`, повторы сообщаются событиями `Retry`.

Помощники для картинок: `Editor::PipelineImageOps` (масштаб, обрезка, обрезка по содержимому, хромакей, двухпроходная маска, обводка, тень, градиент, цветокоррекция, отражение, поворот, nine-slice, `ComposeLayers`, `Blank`, доступ `Pixel` — битмапы RGBA, `Pixel(bmp, x, y)` в координатах картинки), `Editor::PipelineTransparency` (пост-шаги белый/чёрный и хромакей), `Editor::PipelineAudio`. `ResolveComposerLayers(node)` перечисляет слои композера от нижнего к верхнему (входы-картинки, дубликаты из `dupLayers`, порядок из `layerOrder`).

## Выполнение

`Editor::PipelineExecutor::Execute(pipelineId, graph, targetNodeId, dirtyNodeIds, cachedOnly)` выполняет цепочку выше целевой ноды в порядке зависимостей. Результаты кешируются в `Work/Pipelines/cache/<pipelineId>/{content,previews,ran}` по сигнатуре ноды: нода с совпавшей сигнатурой берётся с диска, `cachedOnly` отказывает нодам провайдеров без кеша (`Not cached`). `ExecuteSingle` выполняет одну ноду, `Cancel()` останавливает после текущей, `assetsPathOverride` перенаправляет финишные ноды (тесты). События (`Editor::PipelineExecEvent`): `NodeState` (`queued`, `running`, `done`, `error`), `NodeOutput` (`Editor::PipelineValue` с `previewPath`/`srcPreviewPath`), `Retry`, `Log`, `Done`, `Fatal`. `LoadPreview` и `ClearNodeCache` управляют кешем из редактора.

Финишные ноды пишут результат в `<assets>/<assetPath>.<ext>` (при необходимости обрезанный и масштабированный) и ставят `assetsChanged`; в редакторе исполнитель затем вызывает `o2Assets.RebuildAssets()`.

`Editor::PipelineValue` — `type`, `data` (текст или закодированные байты), `mimeType`; конструкторы `Text`, `Image(bitmap)`, `ImageBytes`, `Bytes`, `GetBitmap()`; `DecodeImageBytes` декодирует PNG/JPEG. `Editor::PipelineUtils` — рабочие пути (`SetWorkPathOverride`), загрузки, data URL, base64, FNV-хеши, hex-цвета, чтение и запись файлов.

<details>
<summary>Пример</summary>

```cpp
PipelineGraph graph;
auto text = PipelineNodeRegistry::CreateNode("sourceText", Vec2F());
text->SetConfigString("text", "pixel art coin");
auto gen = PipelineNodeRegistry::CreateNode("nanoBananaGen", Vec2F(300, 0));
auto finish = PipelineNodeRegistry::CreateNode("finishImage", Vec2F(600, 0));
finish->SetConfigString("assetPath", "Generated/coin");
graph.nodes = { text, gen, finish };
// связи: text.out -> gen.prompt, gen.out -> finish.in

auto executor = mmake<PipelineExecutor>();
executor->onEvent = [](const PipelineExecEvent& e) { if (e.type == PipelineExecEvent::Type::Log) o2Debug.Log(e.message); };
executor->Execute("coin", graph, finish->id, {}, false);
```
</details>

## Импорт
`PipelineImport::ParseBytes` читает экспорт AssetsLine по содержимому: JSON пайплайна, JSON-бандл с результатами в data URL (`Parse`) или ZIP-бандл (`ParseZip`: `pipeline.json`, необязательный `manifest.json` со списком `results/<файл>` по нодам с типом и mime, файлы вне манифеста берутся по имени `results/<nodeId>.<ext>`) в `PipelineGraph` со свежим `id` и карту результатов `PipelineValue`; `StoreResults` записывает их как превью, записи кеша содержимого и отметки актуальности для id пайплайна; `ImportFile` создаёт ассет в папке, сохраняет результаты под id графа и пересобирает ассеты. `PipelineZip` - небольшой читатель и писатель архивов под этим (записи stored и deflate, zlib).

`id` графа - ключ кеша, которым пользуется редактор (`PipelineEditor::GetPipelineId`); ассет без id при открытии берёт свой UID, поэтому результаты, закешированные по UID до появления поля, остаются доступны, а граф, сохранённый в другой ассет, сохраняет свой кеш.
