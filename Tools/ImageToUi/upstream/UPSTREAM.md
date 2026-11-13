# Upstream

`image-to-ui/` and `LICENSE` here are a copy of
[XuToWei/Image-To-UI](https://github.com/XuToWei/Image-To-UI) (MIT), commit
`3aa6527dcf09d8c28fe22936dc051d98b331d545` (2026-09-23), made with
`git archive 3aa6527 image-to-ui LICENSE`.

The AI image to UI node runs this skill: its scripts do the preparing,
rendering, checking and finalizing, and its `SKILL.md` + `references/` are
the instructions the model follows as the agent (`../agent/agent.py`).

## o2's edits

The upstream targets another engine; o2's copy speaks only about o2. The
edits change texts, never what a script does:

- `SKILL.md`: the description, the inputs (`.png.meta` sidecar files with
  sprite borders), step 4 (how o2 exports row / column / grid layouts), step 7
  and the Deliver section, which points to `references/o2.md`.
- `references/o2.md` (new) replaces the upstream's engine-specific export
  reference: how `ui_structure.json` becomes an o2 prototype.
- `references/schema.md` (grid export, `.png.meta` borders),
  `references/components.md` (export link, anchors note),
  `references/assets.md` (inventory's border metadata): engine-neutral words.
- `scripts/inventory_assets.py`, `scripts/render_comparison.py`,
  `scripts/validate_structure.py`: printed texts, warnings, docstrings and
  comments; the border reader was renamed to `read_sprite_border` (inventory,
  validate) and `AssetCache._read_sprite_border` (render). The sidecar format
  they parse (`spriteBorder: {x: L, y: B, z: R, w: T}`) is unchanged.
- `tests/test_layout_validation.py`: one test renamed to
  `test_nine_slice_validates_sidecar_metadata_margins`.

To update, replace the whole folder from a newer commit, record the commit
here, re-apply the edits above (`grep -rn -i` for engine names over this
folder must stay empty), and run both test suites:

```bash
python3 -m unittest discover -s upstream/image-to-ui/tests -t upstream/image-to-ui/tests
python3 -m unittest discover -s agent/tests -t agent/tests
```
