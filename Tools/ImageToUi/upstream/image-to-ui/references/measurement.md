# Native-Pixel Measurement

Use this when a whole-canvas preview cannot resolve small icons, text, padding,
or thin borders. Select the region from the design overview, with neighboring
landmarks and space around the feature; a crop centered only on a wrong draft
bbox may miss the real element.

## Local rulers before or during authoring

After `prepare`, even before `ui_structure.json` exists:

```bash
py -B <skill>/scripts/workflow.py measure --output <task-dir> --region 120 80 240 160 --zoom 3 --cell-size 10
```

`--region` is **x, y, width, height** in native design pixels. The region must
fit inside the canvas; the command rejects invalid bounds instead of silently
adding padding or shifting the crop. Choose a modest region that stays readable
at the selected zoom (integer 1–8).

Read `measurements/region_120_80_240_160/`:

- `design.png`: clean crop enlarged with nearest-neighbor sampling. Each
  original pixel becomes a uniform zoom-sized block, without invented edges.
- `design_grid.png`: the same crop with a 10-native-pixel grid and rulers
  labeled in **absolute design pixels**, not cell indices.
- `metrics.json`: native crop origin, zoom, ruler margins, tick coordinates,
  and source design hash.

Use the grid for measurement and the clean crop for artwork, text, and color
inspection; grid lines must not be mistaken for borders or sampled as UI colors.
Check the source hash against the current prepared design when reusing crops.

If measuring display-image coordinates rather than reading the ruler labels:

```text
design_x = region.x + (image_x - content_origin_px[0]) / zoom
design_y = region.y + (image_y - content_origin_px[1]) / zoom
design_width = image_width / zoom
design_height = image_height / zoom
```

Use the origin for the exact image in `metrics.images`: the clean crop has no
ruler margin, while the grid crop has a top/left margin. Do not count that
margin as design content. If a viewer resizes the crop again, read the printed
ruler; its display pixels no longer equal the saved image's pixels.

These are measurement aids, not render-review evidence. The command does not
edit the structure or promote workflow state. After editing the JSON, run
`check` and cite current check/target evidence in the final review.

## Texture bounds versus visible artwork

The renderer resizes the **entire source texture**, including transparent
padding, to an image node's `size`. The visible shape therefore need not touch
the node bbox. Inventory alpha boxes use **left, top, right, bottom** with
exclusive right/bottom edges; trace bboxes use **x, y, width, height**.

For a normal resized sprite (no nine-slice), let its texture size be `W × H`,
its chosen source alpha bounds be `[L, T, R, B]`, and the corresponding
visible bounds measured in the design be `[vx, vy, vw, vh]`:

```text
scale_x = vw / (R - L)
scale_y = vh / (B - T)
node_width  = W * scale_x
node_height = H * scale_y
node_abs_x  = vx - L * scale_x
node_abs_y  = vy - T * scale_y
position.x = node_abs_x - parent_abs_x
position.y = node_abs_y - parent_abs_y
```

For example, a 100×80 texture with alpha bounds `[10, 5, 90, 65]` and measured
visible bounds `[200, 120, 160, 120]` needs a **200×160** node at **(180, 110)**.
Assigning the visible 160×120 size directly to the node would shrink and
distort the artwork.

For an atomic icon, both scales should agree within measurement precision.
If they disagree, recheck the source candidate, visible edges, and aspect
before accepting non-uniform scaling. Center the visible artwork rather than
the full texture when padding is asymmetric. Retain fractional calculations
until authoring the final coordinates, then verify rendered ink/alpha edges.

`alpha_bbox` includes all nonzero alpha, including faint glow/shadow;
`alpha_core_bbox` includes pixels with alpha >= 128 and can help inspect the
more opaque body. This threshold does not identify a semantic border and can
be empty for a valid translucent sprite. Choose the same feature in source
and design; do not compare a solid core against a glow's outer boundary.
Fully transparent assets cannot serve as visible controls.

Do not use the linear formula for nine-sliced assets: edge/corner and center
segments transform differently. Confirm slicing from border thickness and
corner behavior, use supplied metadata where appropriate, then inspect the
render trace and actual pixels. Filename/usage hints alone do not prove that
a sprite should stretch or use nine-slice.
