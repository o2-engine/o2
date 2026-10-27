# Components and Size Adaptation

Read this when a design contains progress/health/experience bars, selected or
disabled controls, tabs/toggles, scrolling content, or geometry that must follow
screen size. The JSON describes those behaviors and the verifier renders
snapshots of them. Application code owns event handlers, animation, and data
bindings. The optional [Unity exporter](unity.md) assembles native UGUI objects
and visibility branches without adding custom runtime controllers.

## Identify behavior from evidence

Start with visible structure, then use supplied alternate screenshots, product
requirements, and asset families to describe behavior. Each block below accepts
an optional `evidence` string; use it to distinguish an observed value from an
inferred rule. A screenshot can show a selected tab, partially filled bar, or
clipped list without proving all states, the numeric range, or total item count.

- Keep the screenshot's observed state as `state.current`.
- Estimate an unknown progress ratio on a 0–1 range; do not invent a health
  maximum or a percentage more precise than the measured fill boundary.
- Include supplied/visible scroll records. Do not fabricate offscreen records
  to make the region appear scrollable.
- Infer edge attachment from spacing and grouping; distinguish fixed margins,
  centered fixed-size controls, and stretched regions.
- Record unavailable state artwork, unreadable values, and unsupported
  behavior as an approximation or uncertainty on the owning component.

## Decide stretching from the overall composition

Use one criterion for every region: after the interface resolution or aspect
ratio changes, does filling the available space improve the overall visual
balance and interaction layout, or does keeping the current extent do so?
Apply this to ordinary containers, non-scrolling lists, scroll viewports,
navigation groups, panels, and backgrounds. Scrolling describes how content
moves; list roles describe repetition. Neither decides whether a region stretches.

Evaluate horizontal and vertical axes separately in relation to neighboring
regions. Look for awkward empty strips, broken edge alignment, crowded or
excessive gaps, imbalance, clipped labels, and cramped or displaced interaction
areas. Choose fixed size, one-axis stretch, or two-axis stretch from those
observations. Intentional whitespace and fixed-size artwork can be essential
to the composition; filling every free pixel is not the goal.

A non-scrolling list may need its container to fill the space between a header
and footer and distribute its items evenly. A scrolling list in a compact
fixed-size popup may keep a fixed viewport. These are layout decisions, not
rules attached to the presence or absence of `scroll`. Resizing a container
does not imply deforming its icons, text, or item artwork: use suitable anchors,
spacing, and stretchable backgrounds within it.

Compare the full interface at smaller/larger intended resolutions and at a
changed aspect ratio. Record each significant region's chosen axes and the
visual/interaction reason in `component_review.md`, including regions kept
fixed. Express the decision with `responsive` and layout fields below; do not
add scrolling merely to obtain stretching. If the desired reflow cannot be
represented by these fields, record that engine behavior explicitly.

## Responsive rectangles

Keep the required nine-position `anchor` metadata for attachment intent.
Use `responsive` when that intent must actually drive geometry. It is
optional, so existing structures retain their original coordinates.

All coordinates here have a **top-left origin, positive x right, positive y
down**. Each `min`/`max` axis is normalized in [0, 1], with min <= max.
`offsetMin` and `offsetMax` are signed pixel offsets to the two edges:

```text
left   = parent_width  * min.x + offsetMin.x
top    = parent_height * min.y + offsetMin.y
right  = parent_width  * max.x + offsetMax.x
bottom = parent_height * max.y + offsetMax.y
width  = right - left
height = bottom - top
```

A fixed-size 120×40 button, attached 16px from the right and bottom:

```json
{
  "anchor": { "horizontal": "right", "vertical": "bottom" },
  "responsive": {
    "min": { "x": 1, "y": 1 },
    "max": { "x": 1, "y": 1 },
    "offsetMin": { "x": -136, "y": -56 },
    "offsetMax": { "x": -16, "y": -16 }
  }
}
```

For a stretched panel with 16px margins on all sides, use min=(0,0), max=(1,1),
offsetMin=(16,16), offsetMax=(-16,-16). For a centered 120×40 control use
min=max=(0.5,0.5), offsetMin=(-60,-20), offsetMax=(60,20). The node's required
`size` and optional `position` retain its reference-design geometry;
`responsive` controls the resolved preview rectangle.

A responsive node cannot also have `align`, `vAlign`, or `offset`, or be a
direct child of a row/column/grid layout: two systems would own its position. A
responsive container **can contain** a row/column/grid layout; the children then
use its resolved size. A child that should stretch with that container also
needs responsive constraints; attachment is not inherited automatically.

The root follows the requested preview canvas size. To respect device insets,
set `canvas.safeArea` to pixel values such as
`{"left":0,"top":44,"right":0,"bottom":24}`, and add
`"safeArea": true` to a direct root child's responsive block. Its anchors are
then measured in the inset rectangle. Use a safe-area container for deeper
descendants. Preview insets are supplied values, not inferred device geometry.

These fields are not a direct copy of Unity RectTransform: adapters must
convert the y-axis convention and choose the engine pivot. The verifier uses
edge constraints and does not expose an implicit pivot.

### Examples after choosing the size-adaptation behavior

When the composition calls for a centered, fixed-size illustration, keep its
native rectangle as the parent changes. For the 1672 x 941 background in this
example, use
`anchor={"horizontal":"center","vertical":"middle"}` and keep that `size`.
For explicit preview constraints use `responsive.min=max=(0.5,0.5)`,
`offsetMin=(-836,-470.5)`, `offsetMax=(836,470.5)`. This centers without stretching.
Do not substitute min=(0,0), max=(1,1): that distorts the artwork as the parent
aspect changes. Cropping/cover scaling is a separate requirement; do not infer
it just from a full-screen image. The structural root still follows the canvas.

When a region should fill the available rectangle with left/top/right/bottom
margins L/T/R/B, use `responsive.min=(0,0)`, `max=(1,1)`,
`offsetMin=(L,T)`, `offsetMax=(-R,-B)`. This applies equally to a non-scrolling
list container or a scroll viewport. Stretch only the chosen axes; for example,
a fixed-width side panel can attach to the right while stretching vertically.
Nested regions need their own constraints to realize the intended composition.

For a vertical scroll region whose content should fill the viewport width,
use content `min=(0,0)`, `max=(1,0)`, `offsetMin=(0,0)`,
`offsetMax=(0,contentHeight)`, retaining the authored content height. Swap axes
for horizontal scrolling when the same decision applies. Preserve the content
extent needed for scrolling instead of shrinking it to the viewport and erasing
the overflow. Verify the overall composition, chosen margins and spacing after
resizing, and scroll start/end where scrolling is present.

## State variants

Before authoring a `state` block, compare equivalent controls and inspect all
supplied family artwork. Selected/unselected tabs, enabled/disabled buttons,
and checked/unchecked toggles need every evidenced variant on each equivalent
control even when only one instance currently displays it. Share the behavior,
not instance content: preserve each control's own label and icon. Different
quest records, currencies, or item illustrations alone are data variations,
not evidence of switchable states. Missing alternate artwork or uncertain
semantics must be recorded explicitly; do not invent unseen pressed/hover art.
For state-only layers, include their nodes in the base tree with `visible:false`
and reveal them in the variant. Inspect inactive variants as well as current.

A `state` block belongs to a component node. Variant names are explicit and
each variant maps relative paths to property overrides:

```json
{
  "state": {
    "current": "normal",
    "variants": {
      "normal": {},
      "selected": {
        "background": { "asset": "buttons/tab_selected.png" },
        "checkmark": { "visible": true },
        "label": { "color": "#FFE8A0" }
      },
      "disabled": {
        "background": { "color": "#777777" },
        "label": { "color": "#999999" }
      }
    },
    "evidence": "Normal and selected designs supplied; disabled tint specified."
  }
}
```

For example, if a three-tab group shows only its middle tab selected, all
three tab nodes declare both variants. Each keeps its own icon and label; only
`current` differs (`normal`, `selected`, `normal`). A shared frame/checkmark
appearance can be written on **each** tab as:

```json
"state": {
  "current": "normal",
  "variants": {
    "normal": {
      "background": {"asset": "tabs/normal.png"},
      "selection": {"visible": false}
    },
    "selected": {
      "background": {"asset": "tabs/selected.png"},
      "selection": {"visible": true}
    }
  },
  "evidence": "These sibling tabs show both appearances of the same control."
}
```

Use actual supplied assets and include the named nodes in each tab. Set the
middle tab's `current` to `selected`. Export must return six state branches,
with exactly one selected branch per tab; inactive branches must still contain
their complete visuals. A group is not a reason to give only the selected tab
a state block. If the group's states need different layers, author the union
of those layers in each tab and control visibility in the variants.

The base tree includes those named children. Use `".": {...}` to override the
component itself. Unspecified properties come from the authored base tree,
not the previously rendered state. Parent state overrides apply before nested
component overrides; a generated progress label is applied last.

Supported overrides are appearance/text fields:
`asset`, `color`, `opacity`, `hueShift`, `nineSlice`, `text`, `fontFamily`,
`fontSize`, `lineHeight`, `textScaleX`, `strokeColor`, `strokeWidth`,
`alignment`, `textVAlign`, and `visible`. They cannot replace children or
geometry. Inactive variants are still checked for invalid fields and missing
assets; render each variant to check its font and text ink.

`visible: false` hides the node and descendants but retains its layout slot.
Opacity remains local to the node's own visual; it is not inherited by a
container's children. Change child opacities individually for a faded control.
Visibility can represent tab panels, toggle checkmarks, or exclusive labels.
Click events, mutual exclusion between multiple controls, and gameplay state
transitions are not inferred or executed by the verifier.

## Scroll viewport and content

Use a container for the viewport, with one direct child container for content.
The viewport may have stationary visual children such as a frame or scrollbar.
Only the named content subtree is translated and rectangularly clipped:

```json
{
  "scroll": {
    "content": "content",
    "direction": "vertical",
    "offset": { "x": 0, "y": 48 },
    "evidence": "The screenshot clips another record at the viewport bottom."
  }
}
```

- Direction is `horizontal`, `vertical`, or `both`. Offsets are non-negative
  pixels; a positive offset moves content left/up. Disabled axes must be zero.
- The content's unscrolled top-left must resolve to viewport (0,0). Its size
  describes the authored content extent; use padding on its inner list.
- Put row/column/grid layout and optional `role: "list"` on the content container,
  not on the viewport. Existing `list` roles alone do not imply scrolling.
- Offsets clamp to [0, content size - viewport size], also after resizing.
- Nested viewport and progress clips intersect. The render trace records
  effective scroll offsets and the pixels that remain visible after clipping.

Scrollbars and their thumbs can be described as visual children, but their
automatic positioning, inertia, paging, elastic overscroll, virtualized lists,
and arbitrary-shape masks are not implemented in this verifier.

## Linear progress bars

Model the track, full-capacity fill, frame, and label as separate children in
back-to-front order. Add `progress` to their owning component:

```json
{
  "type": "container",
  "name": "health",
  "position": { "x": 24, "y": 24 },
  "size": { "width": 240, "height": 32 },
  "anchor": { "horizontal": "left", "vertical": "top" },
  "progress": {
    "min": 0, "max": 100, "value": 65,
    "fill": "fill", "direction": "left-to-right",
    "label": "label", "format": "{value:.0f}/{max:.0f}",
    "evidence": "Numeric label is 65/100; fill boundary is approximately 65%."
  },
  "children": [
    {
      "type": "rect", "name": "track", "color": "#202630",
      "position": { "x": 0, "y": 0 }, "size": { "width": 240, "height": 32 },
      "anchor": { "horizontal": "left", "vertical": "top" }
    },
    {
      "type": "rect", "name": "fill", "color": "#35C779",
      "position": { "x": 4, "y": 4 }, "size": { "width": 232, "height": 24 },
      "anchor": { "horizontal": "left", "vertical": "top" }
    },
    {
      "type": "text", "name": "label", "text": "65/100", "fontSize": 18,
      "color": "#FFFFFF", "alignment": "center", "textVAlign": "middle",
      "position": { "x": 0, "y": 0 }, "size": { "width": 240, "height": 32 },
      "anchor": { "horizontal": "center", "vertical": "middle" }
    }
  ]
}
```

`value`, `fill`, and `direction` are required. Min/max default to 0/1;
values must be finite, min < max, and min <= value <= max. The ratio is
(value - min) / (max - min). Directions are `left-to-right`,
`right-to-left`, `top-to-bottom`, and `bottom-to-top`.

`fill` names a direct child with the **full 100% fill rectangle**, not the
currently visible width. The renderer clips that subtree at the ratio; it
does not squeeze the texture. A zero value produces no fill pixels, while the
track, frame, and sibling label remain visible. Account for the source
texture's transparent padding when choosing the fill rectangle.

The optional `label` names a separate direct text child. `format` defaults
to `"{percent:.0f}%"` and permits `value`, `min`, `max`, `percent` with
numeric format specifications. Omit label binding when the screenshot's
text should remain fixed.

Circular/radial fills, segmented bars, indeterminate animation, and draggable
slider handles need additional engine behavior. Preserve their observed
layers and record the limitation; do not describe a radial fill as a supported
linear progress bar.

## Preview scenarios

First run a full native `check`. Then render alternate scenarios without
changing the authored JSON or native comparison:

```bash
py -B <skill>/scripts/workflow.py preview --output <task-dir> --name health-empty --progress root/health=0
py -B <skill>/scripts/workflow.py preview --output <task-dir> --name health-full --progress root/health=100
py -B <skill>/scripts/workflow.py preview --output <task-dir> --name wide-selected --size 1440 900 --state root/tabs/inventory=selected
py -B <skill>/scripts/workflow.py preview --output <task-dir> --name list-end --scroll root/viewport=0,99999
```

Use actual paths and ranges from the structure. State/progress/scroll arguments
can be repeated for different components and combined with size changes.
Artifacts go under `previews/<name>/`: `reconstruction.png`,
`render_trace.json`, and `preview_report.json`, including the source
structure hash, scenario parameters, validation and visual audit. A failed
rerun removes the old same-name preview before reporting the failure.

For the implemented components, inspect progress at min/current/max, each
declared state, scroll at start/end, and at least one smaller and one larger
intended viewport plus a changed aspect ratio. Review the full composition,
including non-scrolling lists and fixed-size regions: check whether filled or
unfilled space preserves visual balance, edge alignment, repeated spacing, and
usable interaction areas. Also check text clipping, fill direction, and visible
content boundaries. Record findings and any limitations
in `component_review.md` with paths to the relevant preview reports/images.

Native `finalize` still certifies the screenshot state and its bound evidence;
it does not seal alternate previews or prove behavior on every device. Check
preview hashes against the current structure, rerun affected scenarios after
edits, and deliver the component review alongside the native review.
