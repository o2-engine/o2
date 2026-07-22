## Tasks
`o2::TaskManager` is a singleton for running delayed and per-frame updated tasks. The global access point is the `o2Tasks` macro. The manager updates all tasks every frame and removes completed ones.

## Methods
- **Run(update, isDone)** — runs a functional task: `update(dt)` is called every frame until `isDone()` returns `true`.
- **Run(update, time)** — runs a task for the given time in seconds.
- **Invoke(func, delay)** — calls a function once after a delay.
- **StopTask(id)** / **StopAllTasks()** — stops a task by id, or all tasks.
- **FindTask(id)** — returns a task by id.

## Task classes
The base class is `o2::Task` (registers itself in the manager; has `Update(dt)`, `IsDone()`, `ID()`). Descendants: `FunctionalTask`, `TimeTask`, `FunctionalTimeTask`, `DelayedTask`, `FunctionalDelayedTask`. Custom tasks can be inherited from `Task`.

Callbacks are specified with [`o2::Function<>`](/Docs/en/Architecture/Utils/function.md) delegates.

<details>
<summary>Example</summary>

```C++
o2Tasks.Invoke([]() { o2Debug.Log("After 1 second"); }, 1.0f);

o2Tasks.Run([](float dt) { /* every frame for 2 seconds */ }, 2.0f);

o2Tasks.Run([](float dt) { /* update */ },
            []() { return someCondition; }); // until the condition is met
```
</details>
