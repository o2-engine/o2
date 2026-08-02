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

### Materials and shaders
Drawing goes through materials `o2::Material`: a shader `o2::Shader` plus a set of uniform parameters (`o2::IShaderParam`). The default material can be overridden on any `IDrawable` via `SetMaterial`. Materials and shaders are stored in the `o2::MaterialAsset` and `o2::ShaderAsset` assets.

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

It is enabled by default where `IsMultithreadedRenderSupported()` is true (currently macOS / Metal); other platforms use the single-threaded path, where the main thread submits draws directly. Because a command carries a full state snapshot, the render thread never reads the live, concurrently mutated `Render` members, and the command buffer is reset on the main thread so texture/material references are never ref-counted from the render thread. This mirrors the isolation rules of the [job system](/Docs/en/Architecture/Utils/jobs.md).

### Gizmos, o2::Gizmos
The `o2Gizmos` singleton draws editor helper wireframe primitives with lines: `DrawLine`,
`DrawPolyLine`, `DrawCircle`, `DrawRect`, `DrawBox`, `DrawSphere`, `DrawCapsule`, `DrawPoint`. Points
are given in world `Vec3F` coordinates, and the projection into the drawing space is set from outside
by `SetProjection` — in 2D it drops z, in 3D view it projects with the perspective camera. The
`GetDrawnPrimitives` counter tells whether an object has drawn anything. Used by the scene gizmos
system, see [scene](/Docs/en/Architecture/HighLevel/scene.md).

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

Graphic effects can be applied to a vector font — stroke, gradient, shadow and custom ones. Effects are defined by the font style `o2::FontStyle` (`o2::FontStyleAsset`) and applied per character, during glyph rendering into the atlas, on the CPU. Glyphs of different styles and sizes are packed into the font's shared atlas, cached by the style+size key; styles with identical content share the same glyphs.

### Particle effects, o2::ParticlesEmitter
Inherits from `o2::IRectDrawable` and `o2::IAnimation`. Emits specific particles, handles their dynamics and effects.

The emission shape is set by an `o2::ParticlesEmitterShape` object (circle, rectangle, sphere). The particle limit, the number of particles emitted per second, emission duration, lifetime and initial particle parameters (speed, angle, size) are configured. There is a 3D mode: emission in 3D space with billboard rendering.

During the update, effects — descendants of `o2::ParticlesEffect` — are applied to the particles: gravity, color, size, velocity, movement along a spline and custom ones
