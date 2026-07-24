## Threading
CamelCase wrappers over the C++ standard threading primitives, so threaded code reads in the o2 style. Headers live in `Framework/Sources/o2/Utils/Threading/`.

## o2::Thread
Wrapper over `std::thread`; auto-joins on destruction to avoid `std::terminate` (jthread-like). Non-copyable, movable.
- **Thread(function, args...)** — constructs and immediately starts a thread.
- **Join()** / **Detach()** — waits for the thread to finish, or lets it run independently.
- **IsJoinable()** / **GetId()** — running state and thread identifier.
- **SleepFor(seconds)** / **SleepForMilliseconds(ms)** / **Yield()** — static, act on the current thread.
- **HardwareConcurrency()** — number of hardware threads (cores), or 0 if unknown.
- **GetCurrentThreadId()** / **SetCurrentThreadName(name)** — static; current thread id / debug name for profilers.

## Mutexes
- **o2::Mutex** — basic mutual-exclusion lock: `Lock()`, `TryLock()`, `Unlock()`.
- **o2::RecursiveMutex** — same API, can be re-locked by the owning thread (must be unlocked as many times).
- **o2::SharedMutex** — many concurrent readers or one exclusive writer: `LockShared()`/`TryLockShared()`/`UnlockShared()` plus the exclusive `Lock()`/`TryLock()`/`Unlock()`.

## Scoped locks
RAII helpers over the o2 mutexes:
- **ScopeLock&lt;T&gt;** — exclusive lock over any o2 mutex; locks on construction, unlocks on destruction.
- **SharedLock** — shared (read) lock over a `SharedMutex`.
- **UniqueLock** — movable, manually controllable exclusive lock over a `Mutex`; can be unlocked and re-locked (`Lock`/`TryLock`/`Unlock`/`OwnsLock`) and passed to a `ConditionVariable`. `UniqueLock(mutex, DeferLock{})` associates without locking.

## o2::ConditionVariable
Wrapper over `std::condition_variable`; atomically releases its `UniqueLock` while waiting.
- **Wait(lock)** / **Wait(lock, predicate)** — block until notified; the predicate guards against spurious wakeups.
- **WaitFor(lock, seconds, predicate)** — block until the predicate holds or the timeout expires; returns the final predicate value.
- **NotifyOne()** / **NotifyAll()** — wake one / all waiting threads.

## o2::Atomic&lt;T&gt;
Lock-free wrapper over `std::atomic`. Non-copyable.
- **Load(order)** / **Store(value, order)** / **Exchange(value, order)**.
- **CompareExchange(expected, desired, order)** / **CompareExchangeWeak(...)**.
- **FetchAdd(value, order)** / **FetchSub(value, order)**, plus `++` / `--` operators.
- **WaitWhileEquals(old, order)** — block while the value stays `old`, until another thread changes it and calls **NotifyOne()** / **NotifyAll()**. A cheap alternative to a condition variable.

## o2::ThreadSafeQueue&lt;T&gt;
Multi-producer multi-consumer FIFO. Consumers can block until an item is available; closing the queue wakes them so worker threads can exit.
- **Push(value)** — enqueues an item and wakes one waiting consumer.
- **TryPop(out)** — non-blocking pop, returns false if empty.
- **WaitAndPop(out)** — blocks until an item is available or the queue is closed; returns false only when closed and empty.
- **WaitAndPopFor(out, seconds)** — same, with a timeout.
- **Close()** / **Reopen()** / **IsClosed()** — close wakes all waiters; also **Clear()**, **Count()**, **IsEmpty()**.

## o2::SharedRef&lt;T&gt;
An atomically reference-counted handle for objects that cross threads. Unlike [`o2::Ref`](/Docs/en/Architecture/Utils/memory.md) — whose counter is non-atomic and single-thread only — a `SharedRef` may be copied and destroyed on any thread. The referenced type must derive from `o2::ThreadSafeRefCounterable`; objects are created with **MakeShared&lt;T&gt;(...)** and deleted when the last reference drops. Use it for the handles that legitimately cross the main/worker boundary: [jobs](/Docs/en/Architecture/Utils/jobs.md) and [coroutine](/Docs/en/Architecture/Utils/coroutines.md) state.

**Isolation rule:** parallel work must not mutate shared `o2::Ref` state (the scene, assets), because its reference count is non-atomic. Keep worker bodies off `o2::Ref` graphs — use `SharedRef` for cross-thread handles and defer scene/asset touches to the main thread.

<details>
<summary>Example</summary>

```C++
o2::ThreadSafeQueue<int> queue;

o2::Thread producer([&]()
{
    for (int i = 0; i < 10; i++)
        queue.Push(i);

    queue.Close(); // wakes the consumer so it can exit
});

int value;
while (queue.WaitAndPop(value))
    Process(value);

producer.Join();

o2::Atomic<int> counter{ 0 };
counter.FetchAdd(1);
```
</details>
