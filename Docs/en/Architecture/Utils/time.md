## Time
`o2::Time` is the timing system singleton, updated by the application every frame. The global access point is the `o2Time` macro.

## Methods
- **GetApplicationTime()** — application working time in seconds.
- **GetLocalTime()** / **SetLocalTime(time)** / **ResetLocalTime()** — user-controlled local time.
- **GetCurrentFrame()** — current frame index.
- **GetDeltaTime()** — frame delta time.
- **GetFPS()** — averaged FPS.
- **CurrentTime()** — current system time as a `TimeStamp`.

## Timer
`o2::Timer` is a timer for measuring intervals, with platform-specific implementations:
- **Reset()** — resets the timer.
- **GetTime()** — time in seconds since the last `Reset()`.
- **GetDeltaTime()** — time since the last `Reset()` or `GetDeltaTime()` call.

`o2::ScopeTimer` is a helper timer that prints its scope's lifetime to the console.

## TimeStamp
`o2::TimeStamp` is a serializable structure with date and time (year, month, day, hour, minute, second).

<details>
<summary>Example</summary>

```C++
float dt = o2Time.GetDeltaTime();
o2Debug.Log("FPS: %f", o2Time.GetFPS());

Timer timer;
DoSomethingLong();
o2Debug.Log("Elapsed: %f sec", timer.GetTime());
```
</details>
