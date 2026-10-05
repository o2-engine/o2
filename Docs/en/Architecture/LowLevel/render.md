## Render
Rendering is handled by a separate Render subsystem, with the quick access macro o2Render.

It initializes the renderer and the needed APIs if required. Meshes are drawn through it, using batching. You can set the camera, clipping, and render-to-texture.

The render subsystem also includes the basic primitives: sprite, text and particle effects.

### Textures
The `o2::Texture` class and references to them, `o2::TextureRef`, are used for working with textures. Textures are usually atlases loaded from disk, or render targets.

It is important to know that textures and ImageAssets are separate entities. Textures are textures in video memory, while ImageAssets can be represented either as part of a texture (in an atlas) or as a standalone texture.

A texture can be created on its own, a bitmap can be loaded into it, and it can be loaded from disk in a specific format.

A texture can be a render target; for that it is created with the corresponding flag.

### Camera, o2::Camera
Defines the transformation through which the scene is rendered at the current moment. The position is set through the `o2::Transform` interface. In effect, the camera transformation can be thought of as a window through which we look at the scene

### Meshes, o2::Mesh
Meshes are used for drawing graphics; sprites, text, etc. are built from them.

A mesh consists of a vertex buffer and polygon indices. A texture is assigned to it.

When a mesh is submitted for drawing, its vertices and indices are copied into the current batch buffer if the draw state has not changed (texture, material, primitive type). Otherwise a new batch is created and the previous one is sent for drawing.

Geometry that its owner keeps between frames is drawn without that copying. `o2::RenderGeometry` holds `Vertex` vertices in the camera space and a pointer to triangle indexes; `Render::DrawGeometry(geometry, material)` puts it into a batch of such geometries. When the batch is closed, the render compares the versions of its geometries with what the same command of the previous frame was recorded from (`RenderDrawCommand::retainedGeometries`): the same list leaves the recorded bytes as they are, another one is built right in the command. The Metal backends (macOS, iOS) also keep the GPU buffers of a batch that has come unchanged for 4 frames in a row, so such a batch is uploaded once. The owner calls `RenderGeometry::OnChanged()` after every change and does not change a geometry while `IsQueued()` is true (it is drawn and its batch is not sent yet) — it makes another one. Without a render thread (the OpenGL backends: Windows, Linux, Android, WebAssembly) the render remembers what the batches of retained geometries of a frame were made of, by their order (`Render::RetainedBatchSlot`). A batch of the default material that has come the same for 4 frames gets its own vertex and index buffers on GPU (`o2::RetainedBatchesGL`) and is then drawn from them with no copying; until then, and with another material, its geometries are copied when the batch is closed. A copy the frame has not drawn is deleted at the next one. A copied buffer and a retained geometry do not share a batch.

`Render::GetBatchStatistics()` returns the counters of the frame: draws and vertices put into batches, `retainedVertices` not copied, and how many batches were closed by another texture, material, primitive type, vertex layout, full buffers or a switch between copied and retained geometry. `Render::IsClipped(rect)` tells that a rectangle of the camera space is out of the target and of the scissor clipping; with a 3D camera it is always false.

### Materials and shaders
Drawing goes through materials `o2::Material`: a shader `o2::Shader` plus a set of uniform parameters (`o2::IShaderParam`). The default material can be overridden on any `IDrawable` via `SetMaterial`. Materials and shaders are stored in the `o2::MaterialAsset` and `o2::ShaderAsset` assets. A material without its own shaders draws with the default ones: a material asset holding only a blend mode (e.g. `BlendMode::Add`) is an additive version of the default material.

### Render pipeline
A scene frame is assembled by the `o2::RenderPipeline` pipeline from `o2::RenderPass` passes. There is a forward pipeline (3D with depth test, then 2D layers) and a deferred pipeline (G-buffer, lighting from `o2::LightComponent` sources, then 2D; falls back to forward when MRT is not supported).

Besides 2D primitives, the renderer supports 3D meshes (`o2::Mesh3DFill`), skinned meshes (`o2::SkinningMesh`) and Spine skeletons (`o2::Spine`).

### Frame counters
`o2Render.GetDrawCallsCount()` and `GetDrawnPrimitives()` report what the whole frame cost.
`GetSceneDrawCallsCount()` and `GetSceneDrawnPrimitives()` report only the part drawn outside of an
[editor scope](/Docs/en/Editor/editor.md) — in the editor that is the scene rendered into the Game
window, without the editor UI on top; outside the editor the two pairs are equal. The
[profiler panel](/Docs/en/Architecture/Utils/profiling.md) shows the scene ones.

### Multithreaded rendering
Rendering can run across two threads. The main thread records a frame's draw batches into an `o2::RenderCommandBuffer` (each `o2::RenderDrawCommand` copies its geometry and snapshots the GPU state it needs), and the `o2::RenderThread` submits them to the GPU (encode / draw calls / present). The two threads rendezvous every frame: the main thread dispatches a frame and waits for the previous one to finish before starting the next, so neither runs more than a frame ahead.

Toggle it through the render subsystem:
- **o2Render.SetMultithreadedRenderEnabled(enabled)** — takes effect from the next frame.
- **o2Render.IsMultithreadedRenderEnabled()** — current state.
- **o2Render.IsMultithreadedRenderSupported()** — static, whether the platform supports it.

It is enabled by default where `IsMultithreadedRenderSupported()` is true; other platforms use the single-threaded path, where the main thread submits draws directly. The recording side (command buffer, render thread, per-frame rendezvous) is platform independent — each backend only answers `PlatformSupportsMultithreadedRender()` and implements the submit hooks (`PlatformAcquireFrameTarget`, `PlatformBeginThreaded`, `PlatformReplayDrawCommand`, `PlatformEndThreaded`).

A backend can support it once **nothing of its drawing touches the GPU API while the frame is being recorded**: the frame has to be fully described by the recorded commands. That holds for the Metal backends (macOS, iOS), where state lives in the per-command snapshot; there the drawable and the render pass descriptor are acquired on the main thread (they are main-thread affine) and the render thread only uses the captured objects. The OpenGL backends (Windows, Linux, Android, WebAssembly) still issue state changes, clears and material binds straight to GL while recording, and a GL context belongs to one thread at a time, so they report no support and use the single-threaded path. Making them support it means recording that state into the commands too and handing the context over to the render thread for the submit window (on Android the drawing already runs on the GLSurfaceView GL thread, which owns the context and the swap; on WebAssembly the WebGL context can only leave the browser main thread through an OffscreenCanvas in a worker). Because a command carries a full state snapshot, the render thread never reads the live, concurrently mutated `Render` members, and the command buffer is reset on the main thread so texture/material references are never ref-counted from the render thread. This mirrors the isolation rules of the [job system](/Docs/en/Architecture/Utils/jobs.md).

Reset drops the recorded commands but pools them: their geometry buffers stay allocated at the high-water mark, so a steady frame records into the same storage instead of re-allocating (and re-zeroing) the frame's vertex bytes. `o2::Mesh::Resize` follows the same rule — buffers grow but are never re-allocated to shrink, so a mesh refilled every frame at the same size never touches the allocator.

Consecutive batches that render into the same attachments share one GPU render pass; a new one starts only when the render target, the extra MRT targets, the depth attachment or a clear request changes (a clear is a load action, so it can only happen at the start of a pass). Everything else a batch needs — pipeline state, viewport, scissor, depth state, textures, buffers — is per-batch state set inside the shared pass. A pass per draw call would make a tiled GPU reload and store the whole render target for every batch, which a UI-heavy frame of a few hundred batches feels immediately. Both Metal backends do this, on the render thread replay and on their single-threaded path alike. The open pass is closed when the frame ends, before the command buffer is committed.

Because a clear is deferred until the next batch on Metal, a `Clear()` followed by no geometry would leak onto whatever target is bound next. A render target switch (`BindRenderTexture` / `UnbindRenderTexture` / `PopRenderTargets`) therefore materializes the pending clear on the still-current target first — as an immediately opened pass on the single-threaded path, or as a geometry-less clear-only command on the multithreaded one. GL backends clear immediately, so for them this is a no-op.

### Gizmos, o2::Gizmos
The `o2Gizmos` singleton draws editor helper wireframe primitives with lines: `DrawLine`,
`DrawPolyLine`, `DrawCircle`, `DrawRect`, `DrawBox`, `DrawSphere`, `DrawCapsule`, `DrawPoint`. Points
are given in world `Vec3F` coordinates, and the projection into the drawing space is set from outside
by `SetProjection` — in 2D it drops z, in 3D view it projects with the perspective camera. The
`GetDrawnPrimitives` counter tells whether an object has drawn anything. Used by the scene gizmos
system, see [scene](/Docs/en/Architecture/HighLevel/scene.md).

Wireframes are built from as few poly lines as the shape allows (a box is two face rings plus four
edges, not twelve separate segments) and the point buffers are reused between primitives: a scene of
wireframed colliders issues thousands of these per frame, where per-line allocations and draw batches
cost more than the drawing. Setting a projection that rebuilds the camera matrices per point costs
the same way — project with a matrix built once for the whole pass.

`SetProjection` optionally takes a world space clip plane (origin and normal, the normal pointing to
the visible side; a zero normal means no clipping). Lines are split by the plane before projection:
the parts behind it are dropped and the crossing points are drawn exactly on the plane, so one
primitive may turn into several poly lines. The 3D scene view passes the camera near plane here —
without it, perspective divide by a negative w mirrors geometry behind the camera in front of it.

### IDrawable
This is the base interface of a drawable entity; during drawing it remembers the current scissor rectangle

### IRectDrawable
The base primitive of a rectangular drawable entity. Inherits from `o2::Transform`, has color, transparency and can be disabled

### Sprites, o2::Sprite
Inherits from `o2::IRectDrawable`. A sprite is defined by a texture and a region of it. By default this is set through `o2::ImageAsset`. A sprite can also be created without a texture, in which case the default white texture is used.

The sprite has a drawing Mode:
- Default - default, stretches in all directions
- Sliced - stretches preserving border proportions, 9-slice
- Tiled - the texture is repeated when stretched
- FixedAspect - the sprite's aspect ratio is preserved, fitted into the sprite size
- FillLeftToRight, FillRightToLeft, FillUpToDown, FillDownToUp - filling the sprite horizontally/vertically
- Fill360CW, Fill360CCW - filling the sprite clockwise/counterclockwise

Transformations, color, transparency are set through the base class `IRectDrawable`.

### Video, o2::Video
Inherits from `o2::IRectDrawable` and `o2::IAnimation`. Plays a video asset `o2::VideoAsset` into a dynamic texture and draws it like a sprite. Playback is driven through the `IAnimation` interface (`Play`/`Stop`/`SetTime`/`loop`); the frame shown always follows the animation time, so a `Video` can be used as an animation sub-track — its `Evaluate` decodes and uploads the frame for the current time.

Decoding goes through a `o2::VideoDecoder` backend selected by file extension: `mp4`/`mov`/`m4v` use the platform hardware decoder, everything else (`mpg`/`mpeg`) uses the pl_mpeg MPEG-1 software decoder. Hardware backends: AVFoundation/VideoToolbox on Mac and iOS, Media Foundation (`IMFSourceReader`) on Windows, `AMediaCodec` on Android, an HTMLVideoElement with direct `texImage2D` upload on WebAssembly (asynchronous setup, frames never touch the CPU). Linux has no hardware backend — use `mpg` there. Hardware decoding is the recommended path: it is an order of magnitude cheaper on the CPU and does not depend on build optimization flags.

The encoded data source is selectable: by default the whole file is kept in memory, or with `streaming` enabled it is decoded straight from the asset file on disk, keeping only a small buffer resident (the hardware decoder always reads from the file).

Optionally keys out a solid background color with a soft edge: enabled via `SetChromaKeyEnabled`, configured by the key color (`keyColor`), the `similarity` threshold, the soft edge width `smoothness` and spill suppression `spill`. Keying is done by the `ChromaKey` shader material (it overrides the drawable material via `SetMaterial`).

The scene component is `o2::VideoComponent` (`Component` + `Video`), driving playback in `OnUpdate`; being an `IAnimation` it is picked up by the animation editor as a sub-track.

### Text, o2::Text
Inherits from `o2::IRectDrawable`. It defines the font (vector or bitmap), text size, text formatting and the text itself.

The font is set through the `o2::FontAsset` asset, which has 2 implementations: `o2::VectorFontAsset` and `o2::BitmapFontAsset`.

Additionally to the font, a font style can be set — the `o2::FontStyleAsset` asset (either a separate `.fntstyle` file or an instance inside a reference). One font can be used with different styles without duplicating the font file.

Text is formatted relative to the rectangular area defined by `o2::IRectDrawable`. The following formatting parameters are available:
- hor/verAlign horizontal/vertical alignment
- wrapping words to the next line on horizontal overflow
- ending the line with an ellipsis (...) on horizontal overflow
- letter and line spacing coefficients

### Bitmap and vector fonts
A bitmap font is defined by pre-made, pre-rendered characters in an atlas and their description.

A vector font is generated at runtime, rendering and packing the needed glyphs into a special atlas.

Glyphs are rendered for the pixel density of the target the text is drawn to: the screen graphics scale (2 on retina), 1 in a render texture. Sizes, origins and advances stay in resolution units and the advances are the ones of density 1, so text layout does not depend on the screen. `Text` rebuilds its mesh when it is drawn to a target of another density. Glyphs with style effects are always rendered at density 1: effects work in pixels of the glyph bitmap.

Graphic effects can be applied to a vector font — stroke, gradient, shadow and custom ones. Effects are defined by the font style `o2::FontStyle` (`o2::FontStyleAsset`) and applied per character, during glyph rendering into the atlas, on the CPU. Glyphs of different styles and sizes are packed into the font's shared atlas, cached by the style+size key; styles with identical content share the same glyphs.

### Particle effects, o2::ParticlesEmitter
Inherits from `o2::IRectDrawable` and `o2::IAnimation`. Emits specific particles, handles their dynamics and effects.

The emission shape is set by an `o2::ParticlesEmitterShape` object (circle, rectangle, sphere). The particle limit, the number of particles emitted per second, emission duration, lifetime and initial particle parameters (speed, angle, size) are configured. There is a 3D mode: emission in 3D space with billboard rendering. An emitter driven by an animation sub-track (`SetSubControlled`) bakes frames for scrubbing in the editor's edit mode (a scene marked `Scene::SetIsEditor` and not playing) and otherwise — a running game, the editor's play mode, a shipped build — is simulated forward in 1/60 steps up to the sub-track time (`SimulateTo`); a time going backwards restarts the emission. The emitter's material, including a material asset set with `SetMaterialAsset`, is saved and applies to the particles. Once a one-shot emitter (`Loop::None`) reaches its full duration every particle is killed (`KillAllParticles`) — the remainder of the last fixed step leaves no dying particles behind, in both modes.

During the update, effects — descendants of `o2::ParticlesEffect` — are applied to the particles: gravity, color, size, velocity, movement along a spline and custom ones; `ParticlesVelocityStretchEffect` turns a particle along its velocity and stretches its x side with speed — sparks become streaks (add it after a size effect, which rewrites both sides).


### Vector graphics
`Render/VectorGraphics` turns SVG into plain triangles with per-vertex color and alpha, drawn by the default material without a texture — no shaders. The code is pure CPU and does not need `Render` or `Application`.

- `o2::VectorImage` — the document: `size`, source view box and an ordered list of `o2::VectorShape`. A shape holds sub paths of lines and cubic beziers (`o2::VectorSubPath`, `o2::VectorSegment`), fill and stroke paints (`o2::VectorPaint`: none, solid, linear or radial gradient with pad spread), fill rule, stroke width, cap, join, miter limit and opacities. Geometry is in image space: origin at the left top corner, Y down, units are pixels at scale 1; transforms are already applied.
- `o2::SvgParser` — reads the simple SVG subset: `svg` (`width`, `height`, `viewBox`, `preserveAspectRatio`, nested `svg`), `g`, `a`, `switch`, `defs`, `symbol`, `use` (`href`, `x`, `y`, `transform`), `path` (all commands, absolute and relative, arcs), `rect` with `rx`/`ry`, `circle`, `ellipse`, `line`, `polyline`, `polygon`, `linearGradient`, `radialGradient` with `href` inheritance, `stop`; presentation attributes, the `style` attribute and class selectors of the `style` element; `transform` lists; colors `#rgb`, `#rgba`, `#rrggbb`, `#rrggbbaa`, `rgb()`, `rgba()`, `hsl()`, `hsla()`, CSS names, `currentColor`; lengths in `px`, `pt`, `pc`, `mm`, `cm`, `in` and percents of the view box. `SvgParseOptions::unitsAsPixels` reads the physical units as pixels — for files where 1 pt is a pixel, as `pdftocairo` writes. Group opacity is multiplied into children, not composited as a layer; a stroke under a non-uniform scale gets a uniform width. Unsupported elements and properties (`text`, `image`, `clip-path`, `mask`, `filter`, `pattern`, `stroke-dasharray`, `em` units) are skipped and listed in warnings; `Parse` returns false only for non-XML text or a missing `svg` root.
- `o2::VectorTessellator` — builds `o2::VectorMesh` (positions in image space, colors as `Color4::ABGR` with straight alpha, triangle indexes) for the given `pixelScale` — screen pixels per image unit along X and Y. Curves are flattened with `curveTolerance` (screen pixels). Fills support non-zero and even-odd rules, holes and self intersections; strokes are converted into outlines and filled as their union, so a translucent stroke does not overlap itself. Under a non-uniform `pixelScale` the stroke width is scaled by the mean scale. `pixelSnapped` (on by default) tells that the image is drawn from a whole screen pixel: edges lying on the pixel bounds of the scale then get no fringe; turn it off for a mesh that is drawn at fractional positions. `simplification` (0.5 color levels by default, 0 turns it off) removes the vertices that the triangles around them draw the same without, see `o2::VectorMeshSimplifier`.
- `o2::VectorMeshSimplifier` — removes a vertex when the colors of the vertices around it, of the vertex itself and of the vertices removed there before lie within the tolerance of one plane per channel, and fills its place with fewer triangles. A vertex inside is replaced by 2 triangles less, a vertex on a straight outline by 1 less; vertices of different colors at one point (a hard color step) stay. It works inside a layer — the triangles of one paint of one shape, which do not overlap — so the drawing order is kept. Any point changes by at most twice the tolerance: 1 level by default. Cells of the fringe hold the coverage of every pixel and stay, flat and linearly shaded areas collapse: the editor sprites lose 15% of triangles at scale 1 and 14-33% in the frames of the editor on a retina screen, for 13-20% more tessellation time.
- `o2::VectorMesh` — besides the triangles keeps `size` and the `pixelScale` it is built for. `GetBounds` returns the bounds with the fringe, which goes out of the shapes by half a screen pixel, except the edges on pixel bounds. `FillVertices` writes `o2::Vertex` array for the basis of the image rectangle (origin in the left bottom corner, as `o2::Sprite` has) with the color multiplied in.
- `o2::VectorRasterizer` — software rasterizer of the mesh into `o2::Bitmap`, repeating the render: sample at the pixel center, top-left rule, blending in drawing order. For tests, tooling and previews.
- `o2::BitmapCompare` — similarity of two bitmaps composited over a background: the part of pixels whose maximum channel difference is within tolerance.

Anti-aliasing is done by geometry and gives box filter coverage, as usual 2D rasterizers do. The mesh is built for pixels: the image origin is expected at a whole screen pixel, as UI layout places sprites. A straight edge that is not slanted gets a fringe 1 screen pixel wide, centered on the edge, with alpha going from 1 inside to 0 outside; an axis-parallel edge lying exactly on a pixel bound gets none — pixel centers are covered exactly without it, so a pixel-aligned rectangle is 2 triangles. A long slanted edge gets three ramps that follow the coverage curve. Everything else — corners, curves of radius below 32 pixels, short edges, features thinner than a pixel, tips, crossings — is covered with cells between pixel centers, whose vertices get the exact coverage of the pixel centered there: at 1:1 these pixels are exact and a symmetric shape renders symmetric, at a fractional position they are interpolated as a bilinear texture would be. Small curves are flattened ten times finer than `curveTolerance`, which costs no triangles in the cells. The mesh fits one scale: build it again when `pixelScale` changes.

A fill and an opaque stroke of one shape do not blend their fringes over each other: with the same solid color they are tessellated as one area; with different paints the fill gets a hard edge under a stroke of a pixel or wider, and under a thinner one or on an open path its cells get the coverage that gives the exact union after blending. A translucent stroke is blended over the fill fringe as is.

Linear gradients split triangles along the stop lines, so the ramp is exact. Radial gradients (with the focal point in the center) split triangles by rays from the center and by polygons at the stops — 8 sectors near the center, up to 64 farther, by the steepness of the ramp — and the color is linear inside every piece: the result is within about 2 levels of the ramp at pixel centers and keeps the symmetry of the sectors; a small area is split by the lines of pixel centers instead when that takes fewer triangles. A radial gradient with a moved focal point is subdivided until vertex colors are close enough. Nearly linear runs of stops are merged first (`SimplifyStops`).

`VectorMesh::SplitBySlices` cuts triangles along the 9-slice lines; `VectorMesh::MapSlicedPoint` maps a vertex into the stretched image and `FillSlicedVertices` writes the vertices of it. Borders larger than the target are cut by half of the excess each, as `o2::Sprite` does. The fringe is stretched with the geometry, as pixels of a raster 9-slice are: edges across the stretched zone get blurred.

Limits: a pixel-aligned rectangle costs 2 triangles, one at fractional coordinates about 60, a rounded one or a small circle 100-250, an icon — hundreds, a radial gradient over a curved shape up to 2-3 thousands; curves of radius above 32 pixels keep the mitered single ramp where they are near axis-parallel and get cells and three ramps elsewhere; overlapping shapes are anti-aliased each on its own, as in other rasterizers, and so are a fill and a translucent stroke of one shape; gradient color is taken at pixel centers and is not averaged over the pixel; where a curve is bent sharper than half of its stroke width, the folded inner side of the stroke is approximate; needles of near parallel long edges may be off by up to 10% in single pixels.

<details>
<summary>Example</summary>

```cpp
VectorImage image;
String error;
Vector<String> warnings;
if (SvgParser::Parse(svgText, image, error, warnings))
{
    VectorTessellationParams params;
    params.pixelScale = Vec2F(2, 2);

    VectorMesh mesh;
    VectorTessellator::Tessellate(image, mesh, params);

    Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(mesh, 2.0f, Color4(255, 255, 255, 255));
}
```
</details>

#### Vector sprite, o2::VectorSprite
Inherits from `o2::IRectDrawable`, a separate drawable next to `o2::Sprite`: it draws the triangles of an `o2::VectorImageAsset` (an `.svg` file) by the default material without a texture. It is created from an asset reference or an asset path, is serializable and cloneable, so it works as a `WidgetLayer` drawable and anywhere an `IRectDrawable` is taken.

- `LoadFromImage(asset | path | uid, setSizeByImage = true)` takes `sliceBorder` and `defaultMode` from the asset meta and the size from the image; `SetImageAsset` does the same and keeps the size. Properties: `image`, `imageName`, `mode`, `sliceBorder`; `NormalizeSize`, `NormalizeAspect*` work by the image size.
- Modes (`o2::SpriteMode`): `Default` stretches the image to the rectangle; `Sliced` is the 9-slice with the same rules as `o2::Sprite` — borders are in image units, corners keep their size, borders larger than the rectangle are cut by half of the excess and the corners are clipped to them, not squeezed; `FixedAspect` fits the image into the rectangle keeping its aspect. `Tiled` and the `Fill*` modes are drawn as `Default`.
- The sprite color (`color`, `overrideColor`, `transparency`) is multiplied into the vertex colors.
- Pixel scale. The mesh is taken from the asset for the number of target pixels one image unit covers by axes: the drawn size divided by the image size, times `Render::GetViewPixelScale()` — the camera view scale with the screen graphics scale (1 inside a render target). In `Sliced` an axis that has a border keeps the scale of the view alone, an axis with both borders zero is stretched as a whole and is scaled as in `Default`. The scale is quantized to powers of two, so the fringe is one pixel wide at the scales 1, 2, 4 and from 0.7 to 1.4 pixel between them.
- Zoom. The mesh scale is a power of two, so a continuous zoom asks a new mesh once per octave. When the mesh of the new scale is not built yet and the sprite has one of another scale, it keeps drawing that one: one new mesh a frame is built (tessellated or cut by slices) over all sprites, so a zoom step of a view with many images does not stall a frame. `VectorImageAsset::IsMeshBuilt` tells whether getting a mesh is cheap.
- Pixel alignment. A sprite whose not rotated basis starts and ends on whole target pixels takes the pixel snapped mesh (`VectorImageAsset::GetMesh(scale, true)`, hard edges on pixel bounds); between pixels or rotated it takes the mesh with the fringe along every edge, so edges do not alias (`IsMeshPixelSnapped`). The asset caches both kinds apart.
- Vertices are built lazily at `Draw()`, only when the basis, the image, the mode, the borders or the mesh scale step changed; a color change rewrites colors only (`GetMeshRebuildsCount`, `GetColorUpdatesCount`). The same basis set again, as a layout update does, changes nothing; a basis that has only moved shifts the vertices, and after 32 shifts they are built from the image again, so float rounding does not add up. Indexes are shared with the cached mesh of the asset. Meshes larger than a render batch are split by `Render::DrawBuffer`.
- An invalid or empty image and a rectangle of zero width or height draw nothing. A sprite whose vertices are all out of the target or of the scissor clipping (`Render::IsClipped`), or whose color has zero alpha with the default material, sends nothing to the render.

Performance. What is cached: the asset keeps meshes by scale step, slices and pixel alignment (up to 32, built at the first use: about 1 ms per mesh in a release build and 8 ms in a debug one); the sprite keeps its transformed vertices in an `o2::RenderGeometry`; the render keeps the batches of sprites that have not changed since the previous frame, and their GPU buffers on every backend. So a still vector sprite costs the same per frame as a raster one whatever its triangle count: nothing is copied. The editor frame of 105-162 vector sprites and 49-99 thousand triangles (1366x768, retina) takes 0.03-0.05 ms of the draw traversal in a release build and 0.16-0.25 ms in a debug one — 0.83-1.02 of the same frame drawn with raster sprites, `o2EditorUITests/EditorVectorUIPerf` holds it within 1.15. A sprite that moves or changes color rebuilds its batch: about 4 ns per vertex in a release build, so the frame where every sprite moves costs 0.12-0.3 ms against 0.03-0.06 ms of the raster one. A sprite drawn several times a frame with different transforms or colors takes a new geometry for every drawing and is copied every frame. The first frame builds the meshes of everything visible: 75 ms in a release build and about 600 ms in a debug one for the 86 meshes of the editor frame.

<details>
<summary>Example</summary>

```cpp
auto sprite = mmake<VectorSprite>("ui/panel.svg"); // mode and borders come from panel.svg.meta
sprite->SetRect(RectF(0, 0, 200, 80));
sprite->transparency = 0.5f;
sprite->Draw();

widget->AddLayer("back", mmake<VectorSprite>("ui/panel.svg"), Layout::BothStretch());
```
</details>

The command line tool `o2SvgRasterizer` (target of the same name, built with tests) works without a window:

<details>
<summary>Usage</summary>

```
o2SvgRasterizer <in.svg> <out.png> [--scale N] [--no-aa] [--bg RRGGBB] [--curve-tolerance PX] [--simplification LEVELS] [--units-as-px] [--quiet] [--stats]
o2SvgRasterizer --batch <listfile> [options]
o2SvgRasterizer --compare <listfile> [--tolerance T] [--bg RRGGBB] [--out-dir D] [options]
```
`listfile` lines are `svgPath<TAB>pngPath`. `--batch` writes every png. `--compare` renders every svg over `--bg` (white by default) into the size of its png and prints `similarity<TAB>meanAbsDiff<TAB>svgPath<TAB>pngPath`, `-1` for both numbers when a file can't be read; without `--scale` the svg is scaled to the width of the png; with `--out-dir` it also saves the render and the difference image. `--units-as-px` reads `pt` and other physical units as pixels, `--simplification 0` keeps all vertices of the mesh, `--stats` prints triangles and vertices count. The exit code is 1 when any file failed, 2 for wrong arguments.
</details>
