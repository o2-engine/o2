# Profiling documentation

## Description
The engine has a built-in frame profiler, `o2::NanoProfiler`, and an on-screen panel that shows it,
`o2::ProfilerWidget`. Both are compiled in by the `O2_PROFILER` CMake option (on by default), which
defines `O2_PROFILER_ENABLED`; with the option off the classes don't exist and every profiling macro
expands to nothing.

This is the lightweight always-available profiler. For a full timeline with threads, locks and GPU
zones the engine also ships a [Tracy](https://github.com/wolfpld/tracy) client, enabled by `O2_TRACY`
— the same `PROFILE_*` macros feed both.

## Macros
Put them in the code you want measured; they cost nothing when the profiler is compiled out or not
recording.

- **PROFILE_SAMPLE_FUNC()** — measures the enclosing function, named by `__PRETTY_FUNCTION__`.
- **PROFILE_SAMPLE(id)** — measures the enclosing scope under the given name. The name must be a
  string literal or otherwise have static lifetime: scopes are identified by the pointer.
- **PROFILE_SAMPLE_COLOR(id, color)** — same, with an explicit color in Tracy.
- **PROFILE_NEW_FRAME()** — closes the profiler frame and opens the next one. Called by
  `Integration::ProcessFrame`; put it only where no profiling scope is open.
- **PROFILE_BIND_THREAD()** / **PROFILE_UNBIND_THREAD()** — makes the calling thread the recorded
  one. Called by `Integration` for the main thread.
- **PROFILE_FRAME()**, **PROFILE_THREAD(name)**, **PROFILE_FIBER_ENTER/LEAVE**, **PROFILE_INFO** —
  Tracy only.

## `o2::NanoProfiler`
A hierarchical per-frame profiler. Every scope is recorded into a preallocated array as
`(name, parent, begin, end)`, and the finished frame is published by swapping two buffers, so a frame
costs no allocation and no copy. Times are nanoseconds from the frame start.

- **BeginSample(name)** / **EndSample()** — open and close a scope; `SampleScope` is the RAII form.
- **ExcludeScope** — suspends recording for its scope. The profiler panel wraps its own update and
  drawing in one, so it never charges the game for its own cost.
- **NextFrame()** — publishes the recorded scopes and starts a new frame. Scopes still open at the
  boundary are closed at it.
- **SetEnabled(enabled)** / **IsEnabled()** — recording is switched with the panel's visibility and
  takes effect from the next frame.
- **BindThread()** / **UnbindThread()** — only the bound thread records. On every other thread a
  profiling scope is a single null check, so worker threads can keep their macros without racing on
  the profiler state.
- **GetFrameSamples()**, **GetFrameSamplesCount()**, **GetFrameDuration()**,
  **GetFrameDroppedSamples()** — the last completed frame.
- **AggregateFrame(buffer, capacity)** — reduces the frame to per-name self times (a scope's time
  minus the time of its nested scopes). Allocation free; names that don't fit are summed into a
  trailing `Other` entry.
- **DumpLog()** — writes the aggregated frame into the log.

Limits: `maxFrameSamples` scopes per frame and `maxDepth` nesting; anything over is dropped and
counted by `GetFrameDroppedSamples()`.

### Editor scope
Scopes taken while inside an [editor scope](/Docs/en/Editor/editor.md) are dropped. In the editor that
leaves exactly the game work — `EditorApplication::UpdateScene` and the Game window rendering both
leave the editor scope around it — so the panel shows the game and ignores the editor's own UI.

## `o2::ProfilerWidget` and `o2::ProfilerOverlay`
`ProfilerOverlay` owns the panel and lives on `Integration`; get it with
`o2::ProfilerOverlay::InstancePtr()`. It shows and hides the panel on **F12** or on a **long tap in
the top left corner of the screen**, and switches profiler recording with it.

In the game the panel is drawn over the whole screen, after the frame's debug drawables. In the editor
the Game window shows it with the `Profiler: on/off` button on its top panel, and claims its drawing: `GameWindow::GameView::Draw` calls `DrawIn(area)` with its own rect, so the
panel sits in the top left corner of the window whose contents it measures, and never covers the rest
of the editor. Whoever calls `DrawIn` in a frame takes over that frame's drawing; without it the
overlay falls back to the full screen.

The panel shows:
- the profiler timeline: one column per frame, stacked by scope self time. **Hovering it freezes the
  timeline and details the hovered frame** — every band gets a caption with its time and scope name,
  stacked bottom up next to the graph bottom, the same way the bands are, so the leaders stay short
  and never cross. The frames past the cursor are not drawn while it is frozen. Without the cursor the
  last frame is detailed;
- a time series per metric with its current value and the range over the series;
- object counters, sampled a few times a second;
- a `Base` button that switches the values to a difference from a captured baseline. Both it and the
  resize grip light up under the cursor.

The panel hangs on the top left corner of its host, flush with it. **Dragging the grip in the bottom
right corner resizes it**: the corner follows the cursor in both axes, the top left corner stays put.
Only the timeline stretches — a wider panel means wider frame columns, a taller one a taller graph,
while the caption rows keep their height and the captions their font size. The size is clamped between
`GetMinContentSize()` (a pixel per frame, the caption lines) and `GetDesignSize()*maxSizeFactor`;
`SetContentSize()` does the same from code.

`GetOverallStatus()` grades all the registered metrics and counters together; it is not drawn, the
game can use it to react to a sustained frame drop.

Built-in metrics are FPS, frame milliseconds (the whole profiler frame, so the update and the draw
together), draw calls and drawn primitives. The last two come from `Render::GetSceneDrawCallsCount()`
and `GetSceneDrawnPrimitives()`, which leave out everything drawn inside an editor scope — in the
editor that means the numbers describe the Game window, not the editor's own UI on top of it.

The `Entities` counters describe what the scene is made of. They are collected in one pass over the
scene actors a few times a second, and only actors that are actually on the scene are counted, so the
editor's own widgets never show up:
- **Actors** — actors on the scene, UI widgets included. A number that only grows between two
  identical game states means something isn't being freed
- **UI** — how many of them are widgets
- **Sprites** — `ImageComponent`s plus widget layers drawing a sprite
- **Texts** — widget layers drawing text
- **Animations** — `AnimationComponent` and `AnimationStateGraphComponent`
- **Particles** — particle emitters
- **Models** — 2D, 3D, skinned and primitive meshes
- **Spines** — Spine skeletons
- **Lights** — light sources

A game adds its own metrics and counters:

<details>
<summary>Example</summary>

```C++
if (auto overlay = o2::ProfilerOverlay::InstancePtr())
{
    o2::PerfMetricSettings settings;
    settings.goodValue = 100.0;  // enemies below this are fine
    settings.badValue = 1000.0;  // above this the metric counts as bad

    overlay->AddMetric(o2::PerfMetric("Enemies", []() { return (double)CountEnemies(); }, settings,
                                      { 100.0, 500.0, 2000.0 })); // graph scale steps

    overlay->AddCounter(o2::PerfCounter("Bullets", []() { return CountBullets(); }, settings));
}
```
</details>

The whole panel is drawn as a single mesh plus pooled `o2::Text` captions that are only re-laid out
when their content, box or color actually changes.
