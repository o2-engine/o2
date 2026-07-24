## Coroutines
`o2::Coroutine<T>` lets asynchronous code be written linearly with `co_await`, running through the [job system](/Docs/en/Architecture/Utils/jobs.md). Its resumptions are scheduled as jobs, so a coroutine can hop between the main thread and the parallel workers. The control block is atomically counted (a [`SharedRef`](/Docs/en/Architecture/Utils/threading.md)), so a coroutine handle is safe to keep and drop on any thread. The `CoroutineScheduler` singleton (`o2Coroutines`) drives the time-based and next-frame wake-ups.

Coroutines start suspended: return `Coroutine<T>` (or `Coroutine<void>`) from a function that uses `co_await` / `co_return`, then start it.

## o2::Coroutine&lt;T&gt;
- **Start(thread, priority)** — starts the coroutine on the chosen thread and resume priority; no effect if already started.
- **IsDone()** / **IsValid()** — whether it finished / holds a coroutine.
- **Wait()** — blocks the caller until it finishes. Not on a main-thread coroutine from the main thread — its resumptions need the main thread to run.
- **GetResult()** — the returned value, valid only after it finished.
- **co_await subCoroutine** — starts a sub-coroutine (if needed), suspends until it finishes, then resumes on the awaiter's thread and yields its result.

## Launching
- **Async(coroutine, thread)** — starts a coroutine on the given thread and returns its handle, for the "launch now, await later" pattern that runs several coroutines in parallel.

## Awaiters
`co_await` one of these inside a coroutine:
- **SwitchToWorker()** / **SwitchToMain()** — move the rest of the coroutine to the workers / back to the main thread.
- **WaitTime(seconds)** — suspend for a delay, resumed from the scheduler's timer thread.
- **WaitNextFrame()** — suspend until the next `OnNewFrame()`.
- **WaitAll(coroutines)** — suspend until every coroutine in the array finishes (any not yet started are started); a non-blocking wait — the awaiting thread keeps running other work.
- **WaitAny(coroutines)** — suspend until any finishes; returns the index of the first one.
- **o2::Signal** — thread-safe wait/synchronize primitive: `co_await signal` (or `signal.Wait()`) suspends; `signal.Synchronize()`, callable from any thread, releases all current waiters. Also **IsSignaled()** / **Reset()**. To await an external callback, hand out a callback that calls `Synchronize()`.

## o2::CoroutineScheduler (o2Coroutines)
- **ScheduleAfter(seconds, action)** — run the action after a delay (backs `WaitTime`).
- **ScheduleNextFrame(action)** — run the action on the next `OnNewFrame()` (backs `WaitNextFrame`).
- **OnNewFrame()** — fires all next-frame wake-ups; called once per frame on the main thread by the application.

Headline pattern: start on the main thread, launch N children on the workers with `Async`, `co_await WaitAll(children)` (a non-blocking wait — the main thread keeps running), then continue on the main thread with the results.

<details>
<summary>Example</summary>

```C++
o2::Coroutine<int> ComputeChunk(int index)
{
    co_await o2::SwitchToWorker();       // do the heavy part on a worker
    int result = HeavyWork(index);
    co_return result;
}

o2::Coroutine<void> ProcessAll()
{
    o2::Vector<o2::Coroutine<int>> children;
    for (int i = 0; i < 8; i++)
        children.Add(o2::Async(ComputeChunk(i))); // launch on the workers

    co_await o2::WaitAll(children);       // non-blocking wait on the main thread

    for (auto& child : children)
        Apply(child.GetResult());         // gather results on the main thread
}

o2::Async(ProcessAll(), o2::JobThread::Main);
```
</details>
