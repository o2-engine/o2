## Application, the application wrapper
This class is the entry point of the engine. It is responsible for initializing all subsystems, for the game loop and for the application frame (a window on PC or a fullscreen application on other platforms).

A developer can subclass this class and override the functions responsible for the application's operation: `OnStarted`, `OnUpdate`, `OnFixedUpdate`, `OnDraw`, `OnActivated`/`OnDeactivated`, etc.

Frame parameters can also be changed: caption, size, etc., if supported on the current OS.

## Initialization
To start the application, an instance of the class is created and one or several methods are called depending on the platform: `Initialize`, `Launch`.

Internally, `BasicInitialize` is called one way or another, which initializes the subsystems.

The engine core is factored out into the base class `o2::Integration` (singleton `o2Integration`): it owns the subsystems, initializes them and processes the frame. `o2::Application` inherits from it, adding the platform window and input. When embedding o2 into another application, `Integration` can be used without the platform window and render part.

For unit tests there is a headless mode: `Integration::SetHeadless(true)` before initialization skips creating the window, render and UI styles.

## Update loop
It happens in the `ProcessFrame` method, which is called cyclically from the platform-specific part.

The lifecycle itself is a [coroutine](/Docs/en/Architecture/Utils/coroutines.md). `ProcessFrame` advances it one frame per call: on the first call `EnsureLifecycleStarted()` starts a lifecycle coroutine that runs `OnLifecycleLoad()` once (override it to load content), then loops `ProcessFrameBody()` — the frame update and draw — yielding via `co_await WaitNextFrame()` each frame. Every frame it also pumps queued main-thread [jobs](/Docs/en/Architecture/Utils/jobs.md) under a time budget set by `SetMainThreadJobsQuota(seconds)` (negative = unlimited).

It measures the frame time for updating the internal subsystems. After that, FPS limiting happens if necessary.

Then the engine subsystems are updated and rendering starts. After that, input processing runs.

Physics and the fixed scene update are processed with a fixed step (`fixedFPS`): the accumulated frame time is split into iterations with a constant dt.

To inject code into the update or drawing loop, the `OnUpdate()`, `OnFixedUpdate()` and `OnDraw()` functions can be overridden

## Background window
`Integration::SetBackgroundWindow(true)`, called before `Initialize()`, brings the window up without
making it key and keeps the application out of the Dock / task switcher (accessory activation policy
on macOS, `SW_SHOWNOACTIVATE` on Windows). The window keeps rendering, so screenshots and cursor
injection still work; the keyboard focus stays where the user left it. The rendered test runners use
it — a suite that grabs the focus on every launch makes the machine unusable while it runs.
