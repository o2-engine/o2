# VectorUI

Converts the artboards of the Illustrator source of the editor UI (`o2/UI/UI4.ai`) into one flat SVG per
editor sprite, limited to the subset the engine SVG loader supports, and measures each against the shipped PNG.

## Usage

```
python3 ai_to_svg.py --ai ../../UI/UI4.ai --png-dir ../../Editor/Assets/ui --out <dir> [--only UI4_button_regular,...] [--cache <dir>]
python3 compare_svg.py --svg-dir <dir> --png-dir ../../Editor/Assets/ui [--used referenced_images.txt] [--sheets 40] [--sheet-key tol24|ai_tol24|ov_tol24]
python3 fit_check.py --svg-dir <dir> --png-dir ../../Editor/Assets/ui [--only a,b] [--out <dir>] [--sheet] [--used referenced_images.txt] [--cli <o2SvgRasterizer>]
python3 import_svgs.py --src <dir> [--dst ../../Editor/Assets/ui] [--assets-root <dir>] [--dry-run]
```

`ai_to_svg.py` writes `<png basename>.svg` and `manifest.json` (page, class, primitive count, notes).
`compare_svg.py` writes `report.csv`, `SUMMARY.md` and `sheet_worst_NN.png` into the same directory.
The output is deterministic.

`fit_check.py` is the acceptance measure: it renders every SVG with the engine rasterizer (`o2SvgRasterizer --compare`,
path from `--cli`, `O2_SVG_RASTERIZER` or `Bin/<platform>`) and writes `report.csv`: similarity at tolerance 8 and 24
(worst of the backgrounds #606060, #ffffff, #000000), mean and max difference, pixels over tolerance 24, primitive
count. A sprite passes with similarity >= 0.98 at tolerance 24 and mean difference <= 3. `--sheet` writes
`sheets/<name>.png`: PNG | engine render | difference x8 (red: the render is more opaque, blue: less, grey: colour).

`import_svgs.py` puts the fitted SVGs into the editor assets: every `<src>/X.svg` that has `X.png` and `X.png.meta`
in the ui folder (same relative path and base name) is copied next to the PNG, and `X.svg.meta` is written with the
type `o2::VectorImageAsset::Meta`, `sliceBorder` and `defaultMode` of the PNG meta, and the id
`md5("vector:" + relative path)`. The run is repeatable: unchanged files are not rewritten, PNG files and metas are
never touched, an id that collides with another asset stops the import. Build `BuildEditorAssets` afterwards.

After an import the editor is checked by `o2SystemTests/EditorVectorSprites` (every sprite against its PNG, headless)
and `o2EditorUITests/EditorVectorUIParity` (the editor frame drawn with raster and with vector sprites); see
`Docs/en/Editor/editor.md`, "Vector graphics of the editor UI".

## Requirements

- python3 with `numpy`, `Pillow`, `pypdf`, `cairosvg`
- poppler (`pdftocairo` on PATH) and cairo (`brew install poppler cairo`; homebrew's lib dir is picked up automatically)

## What the converter does

- artboard names come from the Illustrator private data of the file (page N = artboard N); artboard `X` maps to
  `UI4_X.png`, a repeated name to `UI4_X-<page>.png`, `function_icon` to `function_icon.png`; `Editor` is skipped
- each page goes through `pdftocairo -svg`; `<use>` is resolved, transforms are baked into coordinates,
  artboard-sized clips are dropped, a clip over a covering gradient rectangle becomes the clip shape itself,
  constant-alpha masks become `fill-opacity`/`stroke-opacity` (`<g opacity>` when the shapes overlap), gradients
  are baked to user space and their stops merged
- a gradient under an image soft mask becomes a gradient with `stop-opacity`
- rasterised drop shadows are replaced by 9 or fewer primitives: a core rectangle, four strips with a linear
  alpha ramp and four corner squares with a radial one (`effects.py`); the ramp is fitted to the embedded raster
- a translucent group of a fill and a stroke of the same outline is flattened: the fill is inset to the inner edge
  of the stroke (the engine multiplies group opacity into the children); two fills of one outline are blended
- fills without a stroke are grown by the touch rule of the Illustrator export (`touch.py`, `--touch 0.125`): every
  curved or diagonal piece of the outline moves out by 0.125 px along x and y, an edge or an extremum on the
  quarter-pixel grid stays; strokes are not changed
- flat colours are checked against the PNG; `fixes.py` holds the per-sprite corrections

## Emitted subset

`svg` (`width`, `height`, `viewBox`), `defs` with `linearGradient`/`radialGradient` (`userSpaceOnUse`, `stop-color`,
`stop-opacity`), `g opacity`, `path` (absolute `M L C Z`), `rect`; attributes `fill`, `fill-opacity`, `fill-rule`,
`stroke`, `stroke-width`, `stroke-opacity`, `stroke-linecap`, `stroke-linejoin`, `stroke-miterlimit`, `opacity`.

## Measurements

`compare_svg.py` reports three renderings of every SVG against the PNG (tolerances 8 and 24, worst of grey,
white and black backgrounds):

- plain: box-filter coverage (cairo)
- overscan: the same with every fill grown by 0.125 px
- AI rule: an emulation of the Illustrator "art optimized" export the PNGs were made with: 4x4 sub-pixels,
  a sub-pixel is on when a fill touches it, strokes are 0.25 px thinner. A sprite that matches under this rule
  differs from the PNG only by edge anti-aliasing.
