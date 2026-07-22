## Memory management
The main mechanism for managing object lifetime is the engine's own smart pointers `Ref<>` and `WeakRef<>` with a reference counter.

The reference counter (`RefCounter`) is external, but the object stores a pointer to it by inheriting `RefCounterable`. This allows converting a plain raw pointer back into a `Ref<>`.

Objects are created via the `mmake` macro (managed make): it allocates a single memory block for the counter and the object (the counter placed before the object, cache-friendly) and returns a `Ref<>`.

<details>
<summary>Example</summary>

```C++
class MyObject: public o2::RefCounterable
{};

Ref<MyObject> obj = mmake<MyObject>();
WeakRef<MyObject> weak = obj;

MyObject* raw = obj.Get();
Ref<MyObject> obj2 = Ref(raw);
```
</details>

- `Ref<>` — strong reference, owns the object. When no strong references remain, the object is destroyed.
- `WeakRef<>` — weak reference, does not affect lifetime. Has `IsExpired()`/`IsValid()` methods and `Lock()` to obtain a `Ref<>`.
- Casts between reference types: `DynamicCast<T>(ref)` and `StaticCast<T>(ref)`.
- For forward-declared types, use the `FORWARD_CLASS_REF`/`FORWARD_REF_IMPL` macros.

## Allocation tracking
For manual allocations, the new() functions are overloaded with source file and line parameters. For convenience they are hidden behind the mnew (managed new) macro, which registers the allocation (similarly `mmalloc`/`mfree` for raw memory).

`o2::MemoryManager::DumpInfo()` prints the list of allocation sites with total sizes. This helps to detect leaks and memory usage. Works when `ENABLE_MEMORY_MANAGE` is enabled.

## Debug memory analysis
With `ENABLE_MEMORY_ANALYZE` enabled, the debug analyzer `MemoryAnalyzer` works: it builds the object ownership tree from references (`BuildMemoryTree()`), shows sizes and highlights possible leaks and cyclic references. It does not destroy objects — it is used only for analysis. The editor has a window for viewing this memory tree.
