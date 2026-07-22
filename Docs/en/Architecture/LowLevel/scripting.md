## Scripting
o2 has a built-in JS scripting engine with several backends: QuickJS (default), JerryScript and BrowserJS (the browser's JS engine in Emscripten builds). The backend is selected with the `O2_SCRIPTING_BACKEND` CMake option.

The scripting engine is managed by the `o2::ScriptEngine` subsystem, with quick access via the `o2Scripts` macro. It stores the global state, which can be obtained through `GetGlobal`. It can parse and run scripts (`Parse`/`Run`/`Eval`)

It also manages connecting the debugger.

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

#### Functions
A variable can also hold a function that can be called. It can be called passing C++ parameters through templates, in which case they are converted internally into script values: `Invoke`. Or directly with already prepared script values: `InvokeRaw`.

When calling a function, the this parameter can also be passed as the first argument

#### Binding classes and functions into scripting
Every object, with scripting enabled, can store a cached script value for that object. This script value is an object with a native pointer to itself inside. It also has fields that are bound to scripting

For each type in reflection, a prototype is generated, which instances of that type then reference. That is, functions are effectively stored in the prototype. Base classes become the prototypes of their descendants.
