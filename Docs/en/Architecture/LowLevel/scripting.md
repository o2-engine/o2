## Scripting
o2 has a built-in JS scripting engine with several backends: QuickJS (default), JerryScript and BrowserJS (the browser's JS engine in Emscripten builds). The backend is selected with the `O2_SCRIPTING_BACKEND` CMake option.

The scripting engine is managed by the `o2::ScriptEngine` subsystem, with quick access via the `o2Scripts` macro. It stores the global state, which can be obtained through `GetGlobal`. It can parse and run scripts (`Parse`/`Run`/`Eval`)

It also manages connecting the debugger.

### Script libraries and data
Scripts attached to components (`o2::ScriptableComponent`) define one class each. Code shared between them lives in plain script assets and is pulled in with `include(path)` (a path inside the assets, like `"Scripts/Game/Core/Board.js"`): the file runs in the global scope once and again only after its built file changes; scripts including each other do not loop; `include` returns `false` on a missing or broken file and logs why.

`o2.FileSystem.ReadFile(path)` (`undefined` for a missing file), `WriteFile(path, text)`, `IsFileExist(path)` and `FileDelete(path)` give scripts plain text files, e.g. a save file. A data asset reaches scripts as JSON text: `JSON.parse(new o2.AssetRefDataAsset(path).Get().GetJson())`.

<details>
<summary>Example</summary>

```js
include("Scripts/Game/Core/Board.js");

var campaign = JSON.parse(new o2.AssetRefDataAsset("Game/campaign.json").Get().GetJson());
var saved = o2.FileSystem.ReadFile("progress.json");
var progress = saved !== undefined ? JSON.parse(saved) : { level: 0 };
o2.FileSystem.WriteFile("progress.json", JSON.stringify(progress));
```
</details>

### Script value wrapper, o2::ScriptValue
This class is a universal wrapper of any script value. It can hold a simple type (number, string, bool ...) as well as arrays, objects and functions.

To determine the object type, the getter functions `IsArray`/`IsObject`, or `GetValueType`, are used.

The class also has conversion operators to/from the needed type

#### Objects
For object types there is functionality for getting an object property: `GetProperty`, as well as iterating all properties: `ForEachProperties`. Properties can also be added to and removed from the object.

There is functionality for object prototypes; a prototype can be obtained or set: `Set/GetPrototype`

There is also a `Construct` function for constructing an object from a constructor function

#### Arrays
For working with arrays there are element access functions: `operator[int]` and `Set/GetElement`. As well as getting the array length, `GetLength()`. And adding/removing an element of the array: `Add/RemoveElement`.

Script values serialize to `DataValue` and back (fields of script components in scenes and prototypes): an array stays an array even when empty, and reading an array replaces the value instead of appending to it.

#### Functions
A variable can also hold a function that can be called. It can be called passing C++ parameters through templates, in which case they are converted internally into script values: `Invoke`. Or directly with already prepared script values: `InvokeRaw`.

When calling a function, the this parameter can also be passed as the first argument

#### Binding classes and functions into scripting
Every object, with scripting enabled, can store a cached script value for that object. This script value is an object with a native pointer to itself inside. It also has fields that are bound to scripting

For each type in reflection, a prototype is generated, which instances of that type then reference. That is, functions are effectively stored in the prototype. Base classes become the prototypes of their descendants. With multiple inheritance only the first base joins the prototype chain; the functions of the other bases (and of their own bases, at any depth) are copied into the derived prototype once all types are registered, so the result does not depend on the order types register in (it differs between platforms).
