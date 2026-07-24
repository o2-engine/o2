## Jobs
`o2::JobSystem` is a singleton pool of worker threads that run units of work off priority queues. The global access point is the `o2Jobs` macro. Jobs are held via [`o2::SharedRef<Job>`](/Docs/en/Architecture/Utils/threading.md), so a handle is safe to keep and drop on any thread.

## Scheduling
- **Schedule(body, priority, thread)** — creates and immediately submits a job, returns its handle.
- **CreateJob(body, priority, thread)** + **Submit(job)** — create first (to set up dependencies), then submit.
- **AddDependency(job, dependency)** — `job` starts only after `dependency` completes; call before submitting `job`.
- **ExecuteMainThreadJobs(quotaSeconds)** — on the main thread, runs queued `JobThread::Main` jobs until the queue is empty or the time quota is exceeded (`< 0` = no limit). Best-effort: a running job is never interrupted, so the quota may be overrun by at most one job.
- **WaitForIdle()** — blocks until there are no unfinished parallel (`JobThread::Any`) jobs.
- **Initialize(workersCount)** / **Shutdown()** — start / stop the worker pool (`workersCount < 0` uses hardware threads − 1, minimum 1). Driven by the application, not by game code.

## o2::Job
- **onCompleted** — [`o2::Function<>`](/Docs/en/Architecture/Utils/function.md) callback invoked right after the body finishes, on the executing thread.
- **DependsOn(dependency)** — declares a prerequisite; set dependencies before submitting.
- **Then(body, priority, thread)** — schedules a continuation that runs after this job completes, returns its handle.
- **Wait()** — blocks the caller until the job is done. Never call it on a `JobThread::Main` job from the main thread — deadlock.
- **IsDone()** / **IsRunning()** / **GetPriority()** / **GetThread()**.

## Priorities and threads
- **JobPriority** — `Low`, `Normal`, `High`, `Critical`. Higher-priority jobs are picked by workers first.
- **JobThread** — `Any` runs on the parallel worker pool; `Main` runs only on the main thread, drained by `ExecuteMainThreadJobs`.

**Isolation contract:** a `JobThread::Any` body must not touch shared `o2::Ref` scene/asset state that the main thread concurrently mutates (`o2::Ref`'s count is non-atomic). Work that needs the scene or assets must run on `JobThread::Main`. See [threading](/Docs/en/Architecture/Utils/threading.md).

<details>
<summary>Example</summary>

```C++
// Parallel work on the workers, then a continuation on the main thread
auto job = o2Jobs.Schedule([]() { /* heavy CPU work, no scene/asset touches */ },
                           o2::JobPriority::Normal, o2::JobThread::Any);

job->Then([]() { /* apply the results to the scene */ }, o2::JobPriority::Normal, o2::JobThread::Main);

// Dependencies: b runs only after a
auto a = o2Jobs.CreateJob([]() { /* ... */ });
auto b = o2Jobs.CreateJob([]() { /* ... */ });
b->DependsOn(a);
o2Jobs.Submit(a);
o2Jobs.Submit(b);

o2Jobs.WaitForIdle();
```
</details>
