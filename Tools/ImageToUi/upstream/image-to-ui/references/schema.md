# UI Structure Schema

Use this reference when creating or validating fields beyond the core
workflow in `SKILL.md`.

## Full Example

```json
{
  "canvas": { "width": 1080, "height": 1920, "name": "MainUI" },
  "root": {
    "type": "container",
    "name": "root",
    "position": { "x": 0, "y": 0 },
    "size": { "width": 1080, "height": 1920 },
    "anchor": { "horizontal": "left", "vertical": "top" },
    "children": [
      {
        "type": "image",
        "name": "background",
        "position": { "x": 0, "y": 0 },
        "size": { "width": 1080, "height": 1920 },
        "anchor": { "horizontal": "left", "vertical": "top" },
        "asset": "bg_main.png",
        "nineSlice": true
      },
      {
        "type": "container",
        "name": "rewards_row",
        "position": { "x": 100, "y": 600 },
        "size": { "width": 880, "height": 200 },
        "anchor": { "horizontal": "center", "vertical": "middle" },
        "layout": {
          "type": "row",
          "spacing": "even",
          "padding": { "x": 30, "y": 25 },
          "align": "space-evenly",
          "vAlign": "middle"
        },
        "children": [
          { "type": "image", "name": "gem", "size": { "width": 120, "height": 120 }, "anchor": { "horizontal": "left", "vertical": "middle" }, "asset": "icon_gem.png" },
          { "type": "image", "name": "coin", "size": { "width": 120, "height": 120 }, "anchor": { "horizontal": "center", "vertical": "middle" }, "asset": "icon_coin.png" },
          { "type": "image", "name": "key", "size": { "width": 120, "height": 120 }, "anchor": { "horizontal": "right", "vertical": "middle" }, "asset": "icon_key.png" }
        ]
      },
      {
        "type": "container",
        "name": "play_button",
        "position": { "x": 0, "y": 1620 },
        "size": { "width": 300, "height": 100 },
        "anchor": { "horizontal": "center", "vertical": "bottom" },
        "align": "center",
        "children": [
          { "type": "image", "name": "base", "position": { "x": 0, "y": 0 }, "size": { "width": 300, "height": 100 }, "anchor": { "horizontal": "left", "vertical": "top" }, "asset": "btn_play.png", "nineSlice": true },
          { "type": "text", "name": "label", "size": { "width": 300, "height": 100 }, "anchor": { "horizontal": "center", "vertical": "middle" }, "align": "center", "vAlign": "middle", "text": "PLAY", "fontSize": 32, "color": "#FFFFFF", "alignment": "center", "textVAlign": "middle" }
        ]
      }
    ]
  },
  "metadata": { "notes": "", "approximations": [] }
}
```

## Element Types

- `container`: logical group, no visual on its own.
- `image`: visual element with `asset`.
- `rect`: engine-generated solid rectangle with `color` and optional `opacity`.
- `text`: text label.
- `button`: interactive composite. A `container` with image and text works equally well.
- `overlay`: visual parent container for modal scrims, dim masks, and blockers.
  It draws its own `color` or `asset` first, then renders children on top.

## Required Fields

`type`, `name`, `size { width, height }`, and
`anchor { horizontal, vertical }` are required on every element, including
`root`. `position` is required unless the element's position is fully derived
from `align`/`vAlign` or from the parent's `layout`. Text must be non-empty. A
non-root leaf `container` or `button` must have children or its own
`asset`/`color` visual. Each `name` is one stable path segment: it cannot
contain `/` or `\\`, and it cannot be `.` or `..`.

## Anchor Alignment

`anchor` records which parent reference the node should remain attached to in
an engine adapter or responsive layout:

```json
"anchor": { "horizontal": "center", "vertical": "bottom" }
```

- `horizontal`: `left`, `center`, or `right`.
- `vertical`: `top`, `middle`, or `bottom`.

This creates nine possible anchor combinations. It applies to every element
type, including containers and text boxes. It does not align glyphs inside a
text box; text uses `alignment` and `textVAlign` for that purpose.

The structure's `position` remains the node bbox's top-left coordinate relative
to its parent. `anchor` is explicit alignment metadata and does not reposition
the bbox in the verifier. For manual authoring, choose the edge or center whose
spacing should remain stable if the parent is resized. Use `left` / `top` for
ordinary top-left placement, `center` / `middle` for centered surfaces, and
`right` / `bottom` for controls attached to those edges.

To upgrade an existing structure, run:

```bash
py -B <skill>/scripts/backfill_anchors.py --structure <old.json> --output <new.json>
```

Passing the same file as input and output performs an atomic in-place upgrade.
The tool preserves complete valid anchors. For missing axes, it prefers current
`align` / `vAlign` or effective layout cross-axis intent, then selects the
nearest resolved left/center/right and top/middle/bottom parent reference.
Geometric ties prefer the center except at the root; explicit alignment still
wins. This fallback cannot infer semantic stretching or state groups. Review
each region's space usage in the resized composition using
[components.md](components.md), including non-scrolling lists and scroll viewports.
Use `--overwrite` to deliberately recompute existing values.

## Component Behavior

Optional `responsive`, `state`, `scroll`, and `progress` blocks add size
constraints, appearance variants, rectangular scrolling, and linear progress
fills to the existing node types. `visible` is an optional boolean (default
true). Read [components.md](components.md) for validated fields, complete
examples, inference boundaries, and scenario preview commands. These blocks
are interpreted by the verifier; the nine-position `anchor` remains metadata.

## List Semantics

`layout` describes geometry; `role` describes meaning. Roles are opt-in and
must not be inferred from a row/column/grid layout. The `rewards_row` in the full
example is therefore an ordinary layout: its three evenly spaced children are
fixed slots.

Use overall visual and product meaning rather than requiring one decisive cue.
A group likely represents a list when several of these cues agree:

- many peers use one repeated item template and vary mainly in record content;
- the items form one continuous reading or interaction order;
- adding, removing, or reordering an item would preserve the group's meaning;
- the group reads as a dataset or result set, not a set of named positions;
- a specification, data binding, scroll, pagination, or clipped continuation
  reinforces the collection interpretation.

Pagination and scrolling are not required. A dense run of clearly repeated
peer records can be enough based on visual judgment. Conversely, item count is
not a mechanical threshold: even a large group remains fixed slots when each
position has a distinct function or the exact slot count is part of the UI.
Equal spacing, matching sizes, or names containing `list` / `item` never decide
the role alone. When the cues conflict, choose the interpretation that best
matches the whole component.

For example, these four same-template inventory records read as a collection
even though no pagination control is visible:

```json
{
  "type": "container",
  "name": "inventory_results",
  "role": "list",
  "size": { "width": 450, "height": 264 },
  "anchor": { "horizontal": "center", "vertical": "bottom" },
  "layout": { "type": "column", "spacing": 8 },
  "children": [
    {
      "type": "container",
      "name": "iron_sword",
      "role": "listItem",
      "size": { "width": 450, "height": 60 },
      "anchor": { "horizontal": "left", "vertical": "top" },
      "color": "#243044"
    },
    {
      "type": "container",
      "name": "oak_bow",
      "role": "listItem",
      "size": { "width": 450, "height": 60 },
      "anchor": { "horizontal": "left", "vertical": "top" },
      "color": "#243044"
    },
    {
      "type": "container",
      "name": "healing_potion",
      "role": "listItem",
      "size": { "width": 450, "height": 60 },
      "anchor": { "horizontal": "left", "vertical": "top" },
      "color": "#243044"
    },
    {
      "type": "container",
      "name": "tower_shield",
      "role": "listItem",
      "size": { "width": 450, "height": 60 },
      "anchor": { "horizontal": "left", "vertical": "bottom" },
      "color": "#243044"
    }
  ]
}
```

- `role: "list"` is valid only on a `container` with a row, column, or grid `layout`
  and at least one child.
- Every direct child of a list must use `role: "listItem"`.
- A `listItem` must be a direct child of a `list`; nested visual children do
  not inherit the role.
- `role` carries semantics only. `layout` still controls geometry and spacing.
- Fixed slot groups, toolbars, resource rows, navigation, layered controls,
  and decorative groups remain ordinary containers.
- The validator checks role values and hierarchy, but cannot prove runtime
  semantics. The author is responsible for the holistic judgment above.

When present, `role` is copied to the corresponding entry in
`all_elements_legend.json` and targeted `*_legend.json` files.

## Optional Fields

- `responsive`, `state`, `scroll`, `progress`: component descriptions documented
  in [components.md](components.md).
- `visible`: hide the node and descendants when false, while keeping its layout slot.

- `asset`: filename in the assets directory, matched case-insensitively. Nested asset directories are indexed. If duplicate basenames exist, use a relative path such as `icons/coin.png`.
- `color`: `"#RRGGBB"`; fill color on `rect` / `overlay`, multiplicative tint
  on images, text color on text.
- `hueShift`: optional image hue rotation in degrees from `-360` to `360`.
  Use it only when the source shape and shading already match; hue rotation is
  applied before multiplicative `color` tint and never changes alpha.
- `opacity`: `0.0` to `1.0`.
- `text`, `fontSize`, `alignment`: text fields. `alignment` is for text within the text box; `align` positions the text box inside its parent.
- `textScaleX`: optional positive horizontal scale for visible text ink. Keep
  it at `1` unless the exact supplied font still has the wrong width after
  font, size, stroke, and alignment are correct.
- `nineSlice`: stretchable asset handling.
- `layout`: declare this container as a row/column/grid group. The object must
  explicitly contain `"type": "row"`, `"type": "column"`, or `"type": "grid"`.
- `role`: opt-in semantic collection marker; either `list` or `listItem`,
  subject to the evidence and hierarchy rules above.
- `align`, `vAlign`, `offset`: derived positioning relative to parent.

Inside a parent `layout`, child-level alignment only overrides the cross axis:
`vAlign` in a row layout and `align` in a column layout. The layout keeps
ownership of the main axis. Use `offset` for small per-child nudges.

## Grid Layout

Use equal-size row-major cells when the design shows a grid. The grid owns cell
positions and sizes; item artwork can be a child inside each cell.

```json
"layout": {
  "type": "grid",
  "columns": 3,
  "cellSize": {"width": 96, "height": 96},
  "spacing": {"x": 12, "y": 16},
  "padding": {"x": 20, "y": 20},
  "align": "center",
  "vAlign": "top"
}
```

`columns` is a required positive integer; `cellSize` requires positive width
and height. Every direct child declares that same `size`. `spacing` is an
optional finite `{x,y}` object, defaulting to zero on both axes; `padding` keeps
the number or `{x,y}` syntax used by linear layouts. `align` positions the
whole grid horizontally (start/left, center, end/right); `vAlign` positions it
vertically (start/top, center/middle, end/bottom). Distributed alignment and
`spacing:"even"` are linear-layout features, not grid fields. Rows fill left
to right, then top to bottom; the last row may be incomplete. No child `align`
or `vAlign` is allowed because grid cells have one owner. Use `offset` only for
intentional visual nudges, or align artwork inside a cell.

Unity creates `GridLayoutGroup` with FixedColumnCount. Changing the column
count, cell size, or spacing there triggers native reflow. Resolution changes
do not automatically select a different column count. `responsive` belongs on
the grid container, not its layout-driven direct children. List/listItem roles
are available when the grid is a data collection, independent of scrolling.

## Generated Rectangles

Use `rect` for flat rectangular UI that has no sliced asset because the engine
can generate it directly: fills, rules, progress-bar segments, simple panels,
or semi-transparent blocks that are not parents. Do not skip these just because
they are absent from the asset inventory.

```json
{
  "type": "rect",
  "name": "progress_fill",
  "position": { "x": 24, "y": 8 },
  "size": { "width": 180, "height": 18 },
  "anchor": { "horizontal": "left", "vertical": "top" },
  "color": "#45D86A",
  "opacity": 1
}
```

## Overlay Containers

Use `overlay` when a semi-transparent UI layer is the parent of a popup or
foreground surface. Keep scene content behind the overlay out of the structure;
model only the UI-owned scrim and its children.

```json
{
  "type": "overlay",
  "name": "modal_overlay",
  "position": { "x": 0, "y": 0 },
  "size": { "width": 1080, "height": 1920 },
  "anchor": { "horizontal": "left", "vertical": "top" },
  "color": "#000000",
  "opacity": 0.55,
  "children": [
    {
      "type": "container",
      "name": "level_start_popup",
      "position": { "x": 87, "y": 490 },
      "size": { "width": 906, "height": 980 },
      "anchor": { "horizontal": "center", "vertical": "middle" },
      "children": []
    }
  ]
}
```

## Stretchable Assets

Use `nineSlice` for panels, frames, buttons, bars, and similar surfaces when
the design stretches their centers while preserving corner/border detail.
An asset's name or category alone does not establish that behavior. Uniformly
scaled artwork and atomic icons normally keep their source aspect.

An image node's `size` includes the source texture's transparent padding.
Inventory alpha bounds describe only visible artwork; see
[measurement.md](measurement.md) before converting visible bounds into node
position and size.

```json
"nineSlice": true
"nineSlice": 30
"nineSlice": { "left": 20, "right": 20 }
"nineSlice": { "left": 30, "top": 24, "right": 30, "bottom": 40 }
```

When sliced assets come from Unity and include sidecar `.png.meta` files,
`render_comparison.py` reads `spriteBorder` automatically for
`"nineSlice": true`, `"nineSlice": "auto"`, and `"nineSlice": "meta"`. Prefer
that over hand-guessing margins; use an explicit object only when overriding
importer metadata.

## Text Rendering Hints

The verifier supports fields that make comparisons closer to game UI exports:

```json
{
  "type": "text",
  "name": "play_label",
  "size": { "width": 300, "height": 100 },
  "anchor": { "horizontal": "center", "vertical": "middle" },
  "text": "PLAY",
  "fontFamily": "Cairo-Black 1.ttf",
  "fontSize": 72,
  "color": "#FFFFFF",
  "strokeColor": "#111111",
  "strokeWidth": 5,
  "lineHeight": 56,
  "textScaleX": 1.08,
  "alignment": "center",
  "textVAlign": "middle"
}
```

`fontFamily` is searched relative to the prepared task, asset, and design
directories and their parents; it never depends on the process working
directory. `alignment` and `textVAlign` align the visible glyph pixels,
including stroke, within the text box. Leading and trailing spaces remain in
the content but do not move that visible ink. `align` and `vAlign` still
position the text box within its parent. `fontSize` selects the font size but
is not the visible glyph height. `lineHeight` is the vertical step between
successive line draw origins; the renderer measures the real multi-line ink
bounds before applying vertical alignment.
`textScaleX` changes visible glyph width around the authored text alignment;
it does not resize or reposition the text box.

## Accepted Approximations

Use structured metadata for an intentional visual substitution or a limitation
caused by supplied assets. `finalize` accepts only entries whose path appears
in `all_elements_legend.json` and whose reason is explicit.

```json
"metadata": {
  "notes": "Underlying scene intentionally omitted from the active UI scope.",
  "approximations": [
    {
      "path": "root/popup/selected_badge/check",
      "kind": "missing_asset",
      "reason": "No check-mark sprite was supplied; a text glyph is used.",
      "accepted": true
    }
  ],
  "auditWaivers": [
    {
      "path": "root/popup/count_badge",
      "code": "parent_overflow",
      "reason": "The badge intentionally overlaps the slot edge by 8px.",
      "accepted": true
    }
  ]
}
```

`path`, `reason`, and boolean `accepted` are required. `kind` is a short stable
category such as `missing_asset`, `asset_color`, or `font_substitution`.

`auditWaivers` is separate from visual substitutions. Each entry must exactly
match a current warning's `path` and `code`; missing, rejected, duplicate, and
stale entries block `finalize`.
