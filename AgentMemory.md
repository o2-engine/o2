# Agent memory: the o2 engine and its projects

Lessons the AI agents keep from people's reviews of their work - on the o2 portal and in local Claude Code sessions -
about the o2 engine, its editor and tools, and work on ANY project built on it. They are rules: follow them.

<!-- o2 agent memory v1 · general · synced with the o2 portal (Agent memory → Sync) -->
To add a lesson: one line at the end of the list, `- <the rule: imperative, specific, in English> — Why: <what goes wrong
without it>`. Edit or remove lessons freely; keep the `<!-- m:… -->` marks of the others as they are - the sync goes by them.

## Lessons

- Build visual effects and motion with o2's own tools, authored in the editor as assets: `.anim` clips on an AnimationComponent, particles (ParticlesEmitterComponent), the animation state graph (`.asg`, AnimationStateGraphComponent) for states and transitions, FlightTrajectoryComponent for flights along a spline (its `position` keyed in a clip), Spine (SpineComponent) for skeletal animation. Scripts only start them: Play, BlendTo, RewindAndPlay, a state of the graph. — Why: An effect built this way is visible, previewable and tunable in the editor by a person; an effect made in code is not. <!-- m:fb8789dd0670 2026-11-20T07:46Z -->
- Do not animate effects in code: no custom tweens or easing helpers, no per-frame lerp of position, scale, rotation, colour or alpha in Update, no particles simulated by a script. When a tool seems missing, use a clip track (any animatable property; sub-tracks play emitters inside a clip) or say what the engine lacks. — Why: Hand-coded motion buries the tuning in code and duplicates what the animation system already does. <!-- m:f29cbfa34933 2026-11-20T07:46Z -->
- In a script component keep every value a person may want to see or tune - speeds, durations, counts, delays, colours, thresholds, and links to actors and assets - as public instance fields set in the constructor (`this.speed = 2`): they are saved with the scene or prototype and edited in the inspector. Prefix internal state with `_` (`this._timer`) to keep it out of the inspector. — Why: Constants buried in code can only be changed by editing the script; public fields let a designer tune the game in the editor. <!-- m:579c0676a6c9 2026-11-20T07:46Z -->
- Link actors, components and assets through reference fields set in the inspector, not by names or paths: in scripts public fields like `this.target = new o2.LinkRefActor()`, `new o2.LinkRefWidget()`, `new o2.LinkRefComponent()`, `this.icon = new o2.AssetRefImageAsset()`, `this.prefab = new o2.AssetRefActorAsset()` (read them with `.Get()`); in C++ `LinkRef<T>` and `AssetRef<T>` fields. Avoid `GetChild("a/b")`, `FindChild(name)`, `SetImageName("UI/x.png")` and asset refs built from path strings. — Why: References point by unique id and survive renames and moves in the hierarchy and the assets; names and paths break silently when anything is renamed. <!-- m:45153d3f9396 2026-11-20T07:46Z -->
