# Asset Inventory Reference

Use this reference when selecting sliced sprites for `ui_structure.json`.

## Outputs

`inventory_assets.py` writes:

- `assets_inventory.json`: exact relative paths, texture dimensions,
  `alpha_bbox` (nonzero alpha), `alpha_size`, `alpha_padding`,
  `alpha_core_bbox` (alpha >= 128), `fully_transparent`, duplicate basename
  data, usage hints, and Unity `spriteBorder` metadata. Bounds are
  left/top/right/bottom with exclusive right/bottom edges; a missing core
  does not mean a translucent asset is empty.
- `assets_contact_sheet.png`: all assets.
- `assets_contact_sheet_usage_*.png`: grouped sheets. Prefer these first to
  reduce visual context.

## Selection Rules

1. Use grouped sheets first:
   - `assets_contact_sheet_usage_button_or_panel.png` for panels, frames,
     buttons, bubbles, bars, and stretchable backgrounds.
   - `assets_contact_sheet_usage_icon.png` for icons and item art.
   - Fall back to the full sheet only when grouped sheets do not contain the
     needed sprite.
2. If `duplicate_basenames` is non-empty, write `asset` as the relative path
   from `assets_inventory.json`, not as the basename.
3. When the design stretches a panel while preserving its borders, prefer
   `"nineSlice": "meta"` or `true` if `has_meta_border` is true. Usage hints
   and filenames help discovery; they do not determine rendering mode.
4. Do not use nine-slice for `icon`, `portrait`, or item art unless the design
   clearly stretches that asset.
5. Search by filename stem, not one exact filename only. Treat suffixes such as
   `Bg`, `BgShadow`, `BgLight`, `Glow`, `Border`, `FocusLine`, `Arrow`, and
   `Icon` as a related layer family. Compare the family against the design and
   build the visible layers back-to-front.
6. For a composite icon button, use the base/frame alpha bbox for the outer
   control and the glyph's source alpha aspect for the centered inner layer.
   A shared outer bbox does not imply that every child should share its size.
   The node still covers the full texture: use the alpha-padding conversion in
   [measurement.md](measurement.md) to place its visible artwork.
7. Compare plausible variants at readable resolution using silhouette,
   ornament placement, inner cutouts, border thickness, and state highlights.
   Open the source sprite when contact-sheet thumbnails are ambiguous. Choose
   shape/layers before tint; do not distort a similar-looking candidate to
   compensate for an incorrect asset.
