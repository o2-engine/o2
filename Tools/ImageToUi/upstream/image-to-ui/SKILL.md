---
name: image-to-ui
description: "Extract structured UI data from design images. Use when a UI screenshot or mockup plus sliced assets must become layout hierarchy, positions, text, asset references, and ui_structure.json for Unity/Cocos-style game UI. Trigger for UI analysis, screenshot/design to UI JSON, parse design, extract UI structure, Unity/Cocos UI configuration, or reconstruct UI from a design image and Sprite directory."
---

# Image to UI

Create `ui_structure.json` from one design image and one sliced-asset directory.
Represent visible component behavior as well as geometry when the evidence
supports progress bars, state variants, scroll regions, or size adaptation.
Use the workflow wrapper for normal runs; call individual scripts only when
debugging the skill itself.

## Inputs and task folder

Require:

- a PNG/JPG design at native resolution;
- a directory of sliced PNG assets, optionally nested and accompanied by Unity
  `.png.meta` files;
- a fresh output directory for this design.

Keep each design in its own task folder. The workflow records input snapshots
in `workflow_state.json` and refuses mismatched or stale task folders.

## 1. Prepare

```bash
py -B <skill>/scripts/workflow.py prepare --design <design.png> --assets <sprite-dir> --output <task-dir>
```

This checks Python dependencies, inventories assets, creates grouped contact
sheets, overlays a measurement grid using a complementary color selected from
the design's dominant tone plus a contrast halo, and writes grid metrics. If
dependencies are missing, install only those named by the error and rerun the
same command.

Inspect these files first:

- `<task-dir>/design_grid.png`
- `<task-dir>/design_grid_metrics.json`
- `<task-dir>/assets/assets_inventory.json`
- relevant `<task-dir>/assets/assets_contact_sheet_usage_*.png`

Read [references/assets.md](references/assets.md) when selecting sprites.

For small or ambiguous features, measure a design-selected region before
committing coordinates:

```bash
py -B <skill>/scripts/workflow.py measure --output <task-dir> --region 120 80 240 160 --zoom 3 --cell-size 10
```

This writes a clean pixel-preserving crop, an absolute-pixel ruler image, and
crop/zoom metrics under `measurements/`; it also works before a structure
exists. Read [references/measurement.md](references/measurement.md) for the
coordinate mapping and transparent sprite-padding conversion. Choose regions
from the design overview and include nearby landmarks.

## 2. Write the structure

Write `<task-dir>/ui_structure.json`. Use the design grid for every bbox and
the inventory/contact sheets for every asset choice. Read
[references/schema.md](references/schema.md) for fields and examples.
First scan the source for panels, controls, labels, badges, and decorative
layers; the asset inventory is not a complete list of visible UI.

Apply these rules:

1. Set `canvas` to the native design size from the metrics file.
2. Build parent-relative hierarchy before fine coordinates.
3. Give every element, including `root`, an `anchor` object with horizontal
   `left` / `center` / `right` and vertical `top` / `middle` / `bottom`.
   Choose the parent edge or center the element is intended to remain attached
   to. `anchor` records engine-facing alignment intent; it does not change the
   existing top-left `position` coordinates. Add `responsive` edge constraints
   for actual fixed-margin, centered, or stretched size adaptation; do not claim
   that the nine-position `anchor` alone implements it. Choose from intended
   resize behavior and the overall composition at changed resolutions. For
   every region, including non-scrolling lists and scroll viewports, decide
   separately per axis whether filling the available space preserves visual
   balance, spacing, alignment, and usable interaction areas. Stretch when
   leaving that space unfilled would harm the layout; keep a fixed extent when
   filling it would distort artwork or weaken the composition. List roles,
   scrolling, and a full-parent bbox do not determine this choice. Read the
   size-adaptation decision guide in `references/components.md`.
4. Use `layout`, `align`, and `vAlign` for derived positions; use explicit
   `position` only for free placement. Every `layout` object must declare
   `"type": "row"`, `"type": "column"`, or `"type": "grid"`. Use grid for
   repeated equal-size cells with explicit `cellSize`, `columns`, and x/y
   spacing; see `references/schema.md`. Unity exports these as native
   HorizontalLayoutGroup, VerticalLayoutGroup, and GridLayoutGroup without
   LayoutElement. Distributed gaps are baked to reference-size spacing on
   export; read `references/unity.md` before claiming runtime redistribution.
5. Model composite controls as layered children and flat generated shapes as
   `rect` or `overlay`. Give the frame/base the control's outer bbox; keep an
   icon glyph at its own visible aspect ratio and center it inside the frame.
   Never stretch a small glyph to the full button bbox. Image `size` covers
   the full texture, including transparent padding; convert measured visible
   bounds before assigning node geometry.
6. Include the active UI surface and its scrim; omit unrelated scene content
   behind it.
7. Use exact relative asset paths when basenames are duplicated. Prefer Unity
   sprite-border metadata for stretchable panels and buttons.
8. Treat a text element's bbox as its text box. Use `alignment` and
   `textVAlign` for visible-glyph alignment inside that box; use `align` and
   `vAlign` only to position the box inside its parent.
9. For repeated siblings, measure their centers and use one row/column/grid layout.
   Tune the parent position, spacing, and cross-axis alignment before adding
   child offsets. A layout describes geometry only; decide list roles from the
   overall UI meaning. Mark a group as `"role": "list"` with direct-child
   `"role": "listItem"` values when its siblings visually read as one
   repeatable collection: many peer records use the same item template,
   primarily vary by content, follow one continuous order, and could naturally
   be added, removed, or reordered. A large collection can be a list without
   visible scrolling or pagination. Explicit specs, data bindings, scrolling,
   pagination, and clipped continuation strengthen the judgment but are not
   required. Use no hard item-count threshold, and never use equal spacing or
   a `list`-like name alone. Fixed-purpose slots, layered composites, toolbars,
   resource bars, navigation, and button groups remain ordinary layouts.
   Assess size adaptation for all these groups using rule 3; a non-scrolling
   list can stretch, and a scrolling list can retain a fixed-size viewport.
10. When choosing a family asset, inspect its sibling layers. A `Bg` commonly
   needs the matching `Shadow`, `BgLight`, `Glow`, `Border`, or `FocusLine`;
   include only layers visible in the design, in back-to-front child order.
11. Do not add empty text or visual-less leaf containers as placeholders. They
    cannot count as foreground or review coverage.
12. Transcribe visible text exactly, including punctuation and line breaks.
    Inspect a clean local crop when uncertain. Check whether text is already
    baked into an asset before adding a text child; record unreadable content
    or unavailable glyphs as uncertainty rather than inventing a label.
13. Identify progress bars, stateful controls, and scroll viewports from the
    design and supplied context. Read [references/components.md](references/components.md)
    for `progress`, `state`, `scroll`, and `responsive`. Preserve their component
    children and observed values. Inspect the asset family and repeated controls
    together: the screenshot shows the current state, not the complete set of
    supported appearances. Write every evidenced alternate into `state.variants`,
    including inactive layers in the base tree; apply that state model to each
    equivalent control, keeping instance-specific icons/text. Do not turn
    different record content into states. Record missing artwork or uncertain
    state semantics on the owning component rather than silently dropping them.
    Do not reduce a progress bar to a permanently shortened fill image or treat
    every list as scrollable.

Convert grid readings with both axis scales:

```text
x = column * px_per_cell_x
y = row * px_per_cell_y
width = column_span * px_per_cell_x
height = row_span * px_per_cell_y
```

When adapting an older draft, scale it once before manual review:

```bash
py -B <skill>/scripts/scale_structure.py --structure <draft.json> --target-design <design.png> --output <task-dir>/ui_structure.json
```

Then add any missing anchors without changing geometry:

```bash
py -B <skill>/scripts/backfill_anchors.py --structure <task-dir>/ui_structure.json --output <task-dir>/ui_structure.json
```

The backfill preserves complete valid anchors. It first uses explicit
element/layout alignment intent, then chooses the nearest resolved parent edge
or center for missing axes (center wins geometric ties except at the root).
This is a fallback, not a semantic layout decision: review each region's
fixed-size or stretching behavior in the overall composition explicitly. It never creates stretch rules.
Use `--overwrite` only when every existing anchor
must be recomputed.

## 3. Check and iterate

```bash
py -B <skill>/scripts/workflow.py check --output <task-dir>
```

`check` is strict by default: structural warnings fail the run.
`--allow-warnings` is diagnostic only; `finalize` still requires a warning-free
structural report. Add `--transparent-bg` when scene content behind an active
popup was intentionally omitted.

The command stops immediately on invalid JSON or structure errors. On success
it writes:

- `validate_report.json`
- `all_elements.png` and `all_elements_legend.json`
- `comparison.png`
- `reconstruction.png` and `render_trace.json`
- `visual_audit.json`
- `review_risk.json`, `risk_review.png`, and `risk_review_legend.json`

The render trace is collected during the same render pass. The audit blocks
missing requested fonts and visible text ink outside its box. Atomic sprite
aspect changes and child boxes outside their parents are diagnostic warnings;
review them in context instead of moving elements automatically. Every current
visual-audit warning requires an exact accepted entry in
`metadata.auditWaivers`; stale waivers also block completion.

Use `review_risk.json` to prioritize inspection. It scores every rendered
visual leaf from visual salience (visible area, text size, local contrast, and
foreground order) plus the residual between the native design crop and the
same reconstruction crop. The score is name-agnostic. It selects evidence
depth only: never treat it as a similarity verdict or use it to change
coordinates automatically. Inspect every high-risk row in `risk_review.png`;
orange outlines mark design bboxes, cyan outlines mark reconstruction bboxes,
and green outlines mark rendered text-line ink.

Every successful `check` or `target` must rewrite `comparison.png` from the
current structure. Before validation, the workflow removes the old comparison
so a failed run cannot leave a stale image that looks current. Do not use
`validate_structure.py` directly during normal iteration because that
low-level validator does not render.

Inspect both images. Fix hierarchy or parent placement before adjusting child
coordinates; fix asset, nine-slice, tint, opacity, text, or layer order when
bboxes align but rendering differs. Rerun `check` after every JSON edit.
When an asset's shape and shading match but its hue does not, use a small
validated `hueShift` before replacing the asset or distorting its geometry.
Use `visible_bbox` and text `line_ink_bboxes` in `render_trace.json` for pixel
edges. Correct the font, size, line height, stroke, and alignment first; use a
small positive `textScaleX` only when the supplied font has the wrong width.

For crowded or ambiguous regions, validate once and generate one or more
focused views:

```bash
py -B <skill>/scripts/workflow.py target --output <task-dir> --element-path "root/popup/close" --element-path "root/popup/rewards"
```

`target` requires a successful full `check`, refreshes the render artifacts,
and reuses the transparent-background mode from that check. Pass
`--transparent-bg` only when changing the recorded mode intentionally.

Cap focused rechecks at three per element. Read
[references/alignment.md](references/alignment.md) for correction heuristics and
[references/validation.md](references/validation.md) when a check fails.

For components with behavior or responsive geometry, use `workflow.py preview`
after a successful native check to inspect alternate sizes, states, progress
values, and scroll positions. Follow [references/components.md](references/components.md)
and record scenario findings in `component_review.md`, including each stateful
control's current state, all evidenced states, and any unavailable states.
For size adaptation, also review non-scrolling regions and record the chosen
axes and why filling or leaving space best preserves composition and usability
at smaller/larger resolutions and changed aspect ratios.
Compare that inventory with `state.variants` before export; preview evidence does
not replace the native comparison or the alignment review below.

## 4. Review and finalize

Inspect `all_elements.png` and `comparison.png`. Also scan the source design
region by region and account for each visible UI feature in the reconstruction.
Reviewing only JSON paths cannot find missing nodes; risk scoring analyzes
rendered elements and cannot certify completeness, especially over transparent
scene regions. Add omitted UI or explicitly record a known limitation on its
owning component before completing the review.

After the latest `check` or `target`, write `<task-dir>/alignment_review.md` as a four-column Markdown table
with every path from `all_elements_legend.json`: element path, status
(`aligned`, `adjusted`, or `skipped`), observed issue/change, and recheck PNG.
Copy the exact `Review binding:` HTML comment printed by that final command
into the review. An aligned/adjusted row must cite current `all_elements.png`,
`comparison.png`, `risk_review.png`, or a current workflow-generated
`target_*.png`; focused evidence is valid only for paths covered by its
companion legend. A skipped row must explain why.

Every non-skipped high-risk row must cite `risk_review.png` or a current
covering `target_*.png`. Overview-only evidence cannot finalize a high-risk
row. A high residual remains review guidance rather than a blocking visual
error; after inspecting the focused evidence, correct the structure or record
an accepted approximation as appropriate. Read
[references/alignment.md](references/alignment.md) for score interpretation.

Record intentional substitutions or known visual limits in
`metadata.approximations`; each needs an element `path`, `kind`, `reason`, and
`"accepted": true`. Do not hide approximations in prose notes only.
Record every accepted visual-audit warning in `metadata.auditWaivers` with its
exact `path`, `code`, reason, and `"accepted": true`.

```bash
py -B <skill>/scripts/workflow.py finalize --output <task-dir>
```

`finalize` does not rerender elements. It verifies current artifact hashes,
review coverage, validation, and accepted approximations, then crops only
suspicious regions from the existing images into `detail_comparison.png`.
Finish only when it writes `completion_report.json` with `complete: true` and
sets workflow status to `completed`.

## Do not

- Do not derive coordinates from a resized preview without grid/crop metrics.
- Do not guess assets from filenames without checking the inventory.
- Do not reuse another design's output directory.
- Do not use pixel/template matching or automatic coordinate correction.
- Do not change coordinates to hide an asset, text, layering, or nine-slice
  problem.

## Deliver

When a Unity Prefab is requested, read [references/unity.md](references/unity.md)
for the optional UnityAgentBridge exporter and target-engine verification.

Keep `ui_structure.json`, `comparison.png`, `reconstruction.png`, render/audit
reports, review-risk report/evidence, `alignment_review.md`,
`completion_report.json`, `workflow_state.json`, inventories, grid files, bbox
images, legends, and any measurement crops in the task folder. Report
element/anchor/layout counts, structure revision and check counts, high-risk review coverage, accepted
warnings and approximations, and anything skipped or uncertain. When components
are present, also deliver `previews/` and `component_review.md`, reporting the
sizes/states/ranges inspected and any engine behavior not implemented.
