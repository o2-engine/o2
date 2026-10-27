# Unity Prefab Export

Use when the user asks for a Unity Prefab from an existing `ui_structure.json`
or includes Prefab assembly in the reconstruction task. The optional repository
`Unity/` package supplies the `build_ui_prefab` UnityAgentBridge command.

The exporter is **Editor-only**. Generate native UGUI objects and ordinary
project assets, with no custom runtime controller, embedded JSON interpreter,
or exporter-specific resource registry.

1. Locate the target Unity project and read its installed Bridge's `AGENT.md`.
   Discover the command and parameter schema with live `list_commands`.
2. If unavailable, check installation from
   `https://github.com/XuToWei/Image-To-UI.git?path=Unity` through Unity Package
   Manager. The suffix selects the directory containing `package.json`.
   Installation details are in `Unity/README.md`; the Unity directory is
   optional and is not part of a standalone copy of this analysis skill.
3. Supply the structure, sliced-asset root(s), and an `Assets/` Prefab destination.
   `assetsPath` accepts either the existing single directory string or a
   non-empty array of directory strings, for example
   `["Assets/UI/Common", "Assets/UI/Feature"]`. Overlapping roots deduplicate
   the same file. Basenames must be unique across all roots; use unique relative
   paths to disambiguate. If the same relative path exists under different roots,
   adjust the roots or use a common parent with longer paths; no root silently
   overrides another. Font lookup also searches each directory.
   Existing full-image Sprites and fonts under Assets or registered Packages
   are referenced directly.
   A project texture without a full-image Sprite gets a Sprite asset referencing
   the original texture, without copying pixels or changing its importer.
   External sources and necessary hue/slice variants use a stable generated
   folder with reusable content/parameter-keyed files. Resolve ambiguous
   basenames and unavailable fonts using relative paths and `fontMap`.
4. Read the result and warnings, including `stateObjects` with final hierarchy
   paths. Verify the saved Prefab and inspect it in Unity when visual checking
   is part of the task. Compare `stateObjects` against the source's complete
   per-control variant inventory, including inactive states. A zero state count
   is not complete if the analysis identified alternate appearances.

Export every declared state as a complete `States/<name>/Content` branch.
Only `state.current` starts active; each branch retains its own `visible`
settings. Nested state snapshots include ancestor overrides. Keep alternate
artwork and labels available for developers to activate themselves. Do not
add a state controller or infer gameplay events.

Bake positions, native anchors, initial progress appearance, scroll references,
and initial visibility. Hue shifts become texture variants. Application code
owns future visibility, progress updates, device safe-area changes, and other
business behavior. Generated Prefabs remain independent of the exporter.

Preserve analysis artifacts and approximations. Unity font metrics and Outline
rasterization may differ from Python previews; use target-engine evidence
before claiming pixel-for-pixel agreement.

Ordinary Image, Text, Outline, and Button components belong on their authored
GameObjects. Extra transforms are reserved for real composition constraints
(progress clipping, scroll viewports, independently scaled text, or simultaneous
image/text graphics), not an unconditional child for every component. Verify
the background has coincident center anchors and a fixed rectangle when that
is the intended behavior. Resize the full interface and verify each region's
chosen fixed/stretching axes, space usage, alignment, and interaction areas,
including both non-scrolling lists and scroll viewports. Scrolling alone is
not a reason to stretch; follow the composition decision in `components.md`.

## Native layout components

`layout.type=row`, `column`, and `grid` generate HorizontalLayoutGroup,
VerticalLayoutGroup, and GridLayoutGroup. No LayoutElement or LayoutGap is
created. Linear groups disable child size control, force expansion, and scale
control; item sizes stay on RectTransform. Native spacing/padding and alignment
reflow direct items when added or removed.

For `space-between/around/evenly` (including spacing="even"), the exporter
calculates numeric spacing and end padding at the reference size. These values
remain fixed after resizing or changing the item count; they do not implement
CSS-style redistributed gaps at runtime. Between stays start-aligned;
around/evenly keep the whole group centered. Python alternate-size previews
still evaluate the authored distribution rule, so inspect Unity for this
explicit export difference. Grid has fixed columns and equal cells.

Only local offsets, per-item cross-axis alignment, or reserved hidden slots
require `<name>_LayoutSlot`. The slot uses RectTransform size, not LayoutElement.
Per-item cross-axis overrides are captured using the reference cross extent;
the group continues to apply its common alignment on later size changes.
Remove the slot when removing the item; hiding only its authored child retains
space. Returned `stateObjects` contain the final paths. If a node also needs
independent visual helpers (such as scaled text), its authored items move to a
stretching `LayoutContent` child with the LayoutGroup, leaving those graphics
outside the layout. Ordinary containers receive LayoutGroup directly.

ContentSizeFitter and custom runtime scripts are not added. Content growth,
scroll extents, redistributed gaps, and adaptive column counts are application
decisions; do not promise them from the presence of a native LayoutGroup.

Read `resourceUsage` to confirm which sources were referenced, copied, derived,
or reused. `resourceFolder` is null when no generated asset is needed. Repeating
an unchanged export must not create a numbered resource folder or new copies.
Changing a source or variant creates a new generated entry; older assets are
retained because other prefabs may still reference them. Failed builds remove
only assets and empty folders created during that build. Historical numbered
folders from older exporter versions are not deleted automatically.
