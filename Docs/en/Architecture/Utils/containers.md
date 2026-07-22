# Data containers
For convenience, the engine provides special data containers that wrap the standard library containers and add useful functionality.

## `o2::Vector`
`Vector<_type>` is a template class inheriting `std::vector<_type>`. It provides an extended set of methods for convenient list handling: adding, removing, searching, filtering, sorting, etc.

### Basic operations
- **Constructors** (empty, initializer list, copy, move).
- **Operators** `=`, `+`, `-`, `+=`, `-=`, `==`, `!=`:
  - Addition/subtraction with another vector or a single element.
  - Equality/inequality comparison.
- **Size and memory**: `Count()`, `IsEmpty()`, `Capacity()`, `Resize()`, `Reserve()`, `ShrinkToFit()`.
- **Element access**: `Get(idx)`, `Set(idx, value)`, `Data()`.
- **Modification**: `Add(...)`, `Insert(...)`, `Remove(...)`, `RemoveAt(idx)`, `PopBack()`, `Clear()`.
- **Search**: `IndexOf(value)`, `Contains(value)`, `Find(...)`, `FindAll(...)`, `First()`, `Last()`.
- **Filtering**: `Where(...)`, `All(...)`, `Any(...)`.
- **Sorting**: `Sort(...)`, `SortBy(...)`, `Sorted(...)`.
- **Conversion**: `Convert(...)`, `Cast()`, `DynamicCast()`.
- **Iteration**: `ForEach(...)`, plus `Begin()`, `End()` for iterators.

### Additionally
- **Take(...)** — returns a subset of elements by index or count.
- **Clone()** — creates a copy of the current vector.
- **Min(...) / Max(...)** — finds the minimum/maximum element by a selector.
- **Sum(...)** — sums values computed by a selector.
- **Reverse()** — reverses the element order.

With `ENABLE_MEMORY_ANALYZE` enabled, memory analysis methods are available.

## `o2::Map`
`Map<_key_type, _value_type>` is a template wrapper over `std::map`. It stores key-value pairs and provides additional methods for convenient data management.

### Basic operations
- **Adding**: `Add(key, value)`, `Add(Map)`.
- **Removing**: `Remove(key)`, `RemoveAll(match)`, `Clear()`.
- **Presence checks**: `ContainsKey(key)`, `ContainsValue(value)`, `Contains(pair)`.
- **Search**: `FindKey(key)`, `FindValue(value)`, `Find(match)`, `FindAll(match)`, `First(match)`, `Last(match)`.
- **Access**: `Get(key)`, `Set(key, value)`, `TryGetValue(key, outValue)`.
- **Size**: `Count()`, `IsEmpty()`.
- **Iteration**: `ForEach(func)`, plus `Begin()`, `End()` for iterators.
- **Conditions**: `All(match)`, `Any(match)`.
- **Min/max/sum**: `Min(selector)`, `Max(selector)`, `Sum(selector)`.

### Additionally
- **Where(...)** — filters pairs by a condition.
- **Comparison**: `==`, `!=`.
- **Assignment operators**: copy and initializer-list assignment.
- With `ENABLE_MEMORY_ANALYZE`, memory analysis functionality is available.
