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

### Layers
The scene window allows configuring layers and their order. Click Layers at the top (1).

In the dropdown menu they can be reordered via drag'n'drop, created, deleted, renamed by double-click, and enabled/disabled with the checkbox.

### Gizmos and selection
The Gizmos button on the top opens the view display panel. The Gizmos switch turns off gizmos
entirely, below it is the list of actor and component types which draw gizmos: each one is switched
separately. The list contains the types which override `OnDrawGizmos` and are present on the current
scene (colliders, joints, cameras) — they appear right away, before anything was drawn. Gizmos are
drawn for all scene objects and work in both 2D and 3D view mode.

The last row, under a separator, is the Selection switch — it turns off drawing of the objects
selection: the transform frames and the silhouette outline of the selected content. The outline is
drawn in both modes: in 3D over the 3D components content, in 2D over the 2D graphics. The silhouette
covers the whole enabled hierarchy of the selected actor — its drawing components and the components
of all its children; disabled objects and components are skipped, widget layers are not included.

### Camera mode
By default the edit view renders the scene through the render pipeline of the first scene camera, so the picture matches the game. A custom pipeline (offscreen passes, screen shaders) can distort the editing view; the camera mode button at the top switches to a stable camera that draws through the default forward 3D/2D pipeline. The caption shows the mode the click switches to: `Stable cam` / `Scene cam`.
