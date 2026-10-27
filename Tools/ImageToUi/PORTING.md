# Hosting the image-to-UI agent (web, o2, Unity)

Every editor that has the **AI image to UI** node runs the same Python: the
upstream skill (`upstream/image-to-ui`, never edited) and its agent
(`agent/agent.py`). The editor (the *host*) does three things: it makes the
skill's inputs, it answers the agent's model calls, and it turns the task
folder into the node's outputs. The reference host is the AssetsLine server,
`backend/src/uiLayout/run.ts` — copy its behaviour, not just this summary.

## 1. Inputs (the node's own part)

Unchanged from before: analysis (`analyzePrompt`), sprite cuts
(`cutPrompt`, key colour, registration, nine-slice), backdrop
(`backgroundPrompt`, `plateLeftovers`). The old layout algorithm is gone:
no template matching / instance placement, no colour-variant split, no grid
snapping, no own structure / render / compare / review / glyph fit.

Then, in the node's work folder `<work>`:

```
<work>/input/design.png      the mockup, flattened on white
<work>/sprites/<name>.png    one per cut look, FLAT (no subfolders)
<work>/sprites/<name>.png.meta   only for nine-sliced looks:
                             "fileFormatVersion: 2\nTextureImporter:\n  spriteBorder: {x: L, y: B, z: R, w: T}\n"
<work>/sprites/bg_backdrop.png   the backdrop plate, when there is one
<work>/fonts/Roboto-Regular.ttf, Roboto-Medium.ttf, Roboto-Bold.ttf, LICENSE-Roboto.txt
<work>/input/Roboto-*.ttf    the same three fonts again: the skill resolves fontFamily
                             next to the task / assets / design folders and their parents,
                             so `fonts/Roboto-Bold.ttf` resolves from <work> and a bare
                             `Roboto-Bold.ttf` (models write that) from input/
<work>/task/                 removed before every run (the skill wants a fresh folder,
                             outside the assets folder)
<work>/agent-view/           removed before every run (model-sized picture copies)
<work>/trace/                the debug trace; the host writes its steps first
```

Sprite file names (`assetFileName` in run.ts): the look key made safe, then a
kind prefix unless the key already says it — panel/input → `panel_` (unless
it has panel/frame/bg/popup/border/slot/bubble/bar), button → `button_`
(unless button/btn), progress → `bar_` (unless bar), anything else smaller
than 10 % of the screen → `icon_` (unless icon/character/avatar/portrait/
profile). Duplicates get `_2`, `_3`. The skill's inventory sorts its usage
contact sheets by these names.

## 2. The agent

```
<python> -B image-to-ui/agent/agent.py
  --design <work>/input/design.png --assets <work>/sprites --output <work>/task
  --scratch <work>/agent-view --skill image-to-ui/upstream/image-to-ui
  --trace-dir <work>/trace --max-turns <maxTurns> --name <screenName> --hint <hint>
  --sprites-note <note>  --font fonts/Roboto-Regular.ttf --font fonts/Roboto-Medium.ttf --font fonts/Roboto-Bold.ttf
```

cwd = `<work>`. `<python>`: Python 3.10+ with `pillow` and `numpy`; the
server uses `/opt/image-to-ui/bin/python`, editors find `python3` (a setting
overrides it). Exit 3 + a result line with `"error": "missing_dependencies"`
means install them — show that text to the user.

`--sprites-note` (`spritesNote` in run.ts), exactly:
"About the sliced assets: they were cut from this very design, one PNG per
distinct piece of artwork, each at the size it appears, with the labels,
numbers and other text drawn on it removed — the text belongs in text
elements." + "N stretchable sprite(s) carry Unity nine-slice borders in their
.png.meta (has_meta_border in the inventory)." (when N > 0) + one of
"`bg_backdrop.png` is the screen's backdrop with every UI element removed." /
"The backdrop is a flat colour #RRGGBB; there is no sprite for it." /
"There is no backdrop sprite: leave the area behind the UI empty."

**Bridge** — read the agent's stdout line by line; only lines starting with
`@@bridge ` matter:

- `{"type":"model","id":N,"tag":"turn-3","prompt":"…","images":["/abs/v001-….jpg", …]}`
  → call the node's vision model (`model` config) with that prompt and those
  images (JPEG or PNG — sniff `FF D8`), memoised by (model, prompt, image
  bytes) like every other call of the node (↻ skips the memo), then write ONE
  line to its stdin: `{"id":N,"text":"<answer>"}` or `{"id":N,"error":"…"}`.
- `{"type":"event","message":"turn 3/40"}` → log it.
- `{"type":"result","ok":bool,"summary":{complete,turns,check_count,structure_revision,target_count,status,blockers},…}`
  → the last line. `ok` = a checked structure exists.

Drain stderr (keep its tail for errors); ignore EPIPE if the agent exits first.

## 3. Outputs

- `preview` = `<work>/task/reconstruction.png`
- `layout`  = `<work>/task/ui_structure.json`, byte for byte
- one image output per sprite the layout names: walk the JSON for every
  `"asset": "<file>"` (states/variants included), output name = file stem,
  id `sp-<name>`, data = `<work>/sprites/<file>`.
- `ok` false → the node fails ("the agent ended without a checked layout").
  Not `complete` → outputs still delivered; log + trace say NOT completed with
  the blockers.

## 4. Trace / debug view

The host writes its steps (input, analysis, sprite-*, background, assets) to
`<work>/trace/trace.json` (TraceFile: `{version, startedAt, design:{w,h,file},
steps:[{id,title,kind,ms?,note?,status?,images:[{title,file,w,h}],text?,data?}],
summary?}`), flushes, runs the agent (which appends: kinds `prepare`, `view`,
`write`, `check`, `target`, `preview`, `finalize`, `agent`, then `output`),
reloads the file and appends its own `output` step with the summary.

Agent turn steps have ids `turn-<n>`: `note` = the model's note, `data.action`
= the action JSON, `text` = the skill's answer, `images` = the evidence
(check: comparison.png, all_elements.png, risk_review.png, reconstruction.png).
The debug view shows, for an agent turn: the note, the action, the result
text, and the images at full width (prepare / measure / check / target /
preview / finalize / view); after a check or target, design | reconstruction
under one divider (the image titled `reconstruction.png`). The header shows
"completed · N checks" or "not completed · N blockers" from `summary`.

## 5. Config

`model`, `imageModel`, `maxTurns` (20 / 40 / 60 / 90 / 120, default 60 —
replaces the old `passes`), `sprites` (auto / ai), `background`
(auto / image / color / none), `screenName`, `hint`, `seed`.
