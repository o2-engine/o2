## Scene. Scene window
<img src="scene.png" width="50%">

This window shows the actors of the scene in layer order, independently of the camera. The scene can be scrolled and zoomed. Scroll with the right mouse button held, zoom with the wheel.

A grid is drawn in the background. The grid step depends on the zoom (powers of ten), every tenth line is highlighted.

Actors are selected with the left mouse button. Clicking the same point repeatedly cycles the selection from top to bottom. This way the desired node in the hierarchy can be selected with several clicks.

The active tool selected in the toolbar is also shown (2). It is interactive and works like in other editors.

The active tool may have special modes activated by holding Shift/Ctrl/Alt. Among them — snapping, axis alignment and others.

### Spline editing
Components with a spline (spline colliders and others) activate the spline tool when selected in the inspector. Points are added with a double click, removed with Delete. Dragging a point's bezier supports keeps them symmetric; with Alt held they move independently, and an Alt-click resets a support to zero.

A selected point also shows a corner rounding handle on the corner bisector (as in Illustrator/Figma). Dragging it into the corner rounds it: the point slides along the bisector and the supports lay on the arc tangent. Dragging back to the base offset restores a sharp corner. The rounding is limited by halves of the adjacent edges.

### Picking from the objects under the cursor
A right click (without moving the view) opens a menu of the objects seen under the cursor (`ScenePickMenu`): actors
that draw there, widgets whose layers draw there and, when the tree shows them, those layers. A widget without layers
only holds others, the layer of another state (transparent now, as a button's pressed one) is not seen, a camera only
frames what it shows, a particles emitter draws only where its live particles are (an idle one of a pool draws
nothing), so none of them is listed. A scaled or turned widget is hit where its layers are drawn. The menu is a tree of these objects only: an object hangs on its nearest ancestor that is in the menu too,
the other ancestors are left out. The hovered item outlines its object in the scene in orange, and the menu of its
left-out ancestors opens on the left of it, going up from the item: the root on top, the nearest ancestor right above
the item's row; its items outline and pick the ancestors the same way. A click on an item selects its object, as a click in the scene does, with an undoable action.

### View mode and navigation
The rounded 2D | 3D switch at the right end of the top panel switches the view modes: its thumb slides under the chosen one. In 3D:
- the right button held looks around, WASD/QE fly;
- the middle button held orbits around the selected objects (the center of their bounds when the orbit began; the
  view target without a selection), the pivot keeps its place on the screen, as in Unity; Alt + the left button does
  the same, for a trackpad;
- Shift + the middle button pans;
- the wheel zooms.

### Layers
The scene window allows configuring layers and their order. Click Layers at the top (1).

In the dropdown menu they can be reordered via drag'n'drop, created, deleted, renamed by double-click, and enabled/disabled with the checkbox.

### Gizmos and selection
The Gizmos button on the top opens the view display panel. The Gizmos switch turns off gizmos
entirely, below it is the list of actor and component types which draw gizmos: each one is switched
separately, with the icon of the component (or of the actor) before its name. The list contains the types which override `OnDrawGizmos` and are present on the current
scene (colliders, joints, cameras) — it is collected from the whole scene, so a type can be switched
off before anything of it is selected. Gizmos are drawn for the selected objects only, down their
children — the same hierarchy the selection outline covers — and work in both 2D and 3D view mode.

The last row, under a separator, is the Selection switch — it turns off drawing of the objects
selection: the transform frames and the silhouette outline of the selected content. The outline is
drawn in both modes: in 3D over the 3D components content, in 2D over the 2D graphics. The silhouette
covers the whole enabled hierarchy of the selected actor — its drawing components and widget layers
and those of all its children, so a selected UI panel is outlined round the images and texts inside it
(only the layers of the widgets: a widget is not drawn whole for the mask). A widget layer selected by
itself is outlined alone, with its sublayers. Disabled objects, components and layers are skipped, and so
are the sublayers of a disabled layer.

### Render mode
The narrow render mode button at the top shows the icon of the mode and opens the menu of the modes, each with its icon (`SceneEditScreen::SetRenderMode`, `SceneRenderMode`):
- **Scene camera** (default) — through the render pipeline of the first scene camera, so the picture matches the game.
- **Stable camera** — through the default forward 3D/2D pipeline: a custom pipeline (offscreen passes, screen shaders) can distort the editing view.
- **Unlit** — the default pipeline without the baked shading of the 3D meshes (`ScenePassFilters::SetRawAlbedoMode`); 2D has no lighting.
- **Wireframe** — the edges of the triangles in lines, without the fill and the textures.
- **Shaded wireframe** — the edges over the filled triangles.
- **Overdraw** — the triangles add a constant color up over a dark background, without textures: the brighter, the more layers are drawn there.

Every mode but Scene camera draws through the default pipeline. The debug modes apply to the scene content only: the grid, the selection, the gizmos and the other windows are drawn as they are (`Render::SetDebugDrawMode`, see [render](/Docs/en/Architecture/LowLevel/render.md)). A raster of a vector sprite from the atlas is two triangles in Wireframe and Overdraw.

The Scene window is a look at the game: vector sprites are drawn with the detail of the Game window (the raster and the mesh it takes), and the zoom magnifies them as they are instead of rasterizing them anew; a sprite the Game window has not drawn yet is detailed as at 100% zoom (`VectorSprite::DetailView`, see [render](/Docs/en/Architecture/LowLevel/render.md)).
