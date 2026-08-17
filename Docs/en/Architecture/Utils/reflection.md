## Reflection

The engine uses its own type system. It provides runtime access to type parameters: size, name, list of fields and functions. It also allows changing object fields by name at runtime and calling functions by name as well.

Attributes for fields and functions are supported. In code they are denoted starting with `@`, usually in upper case, e.g. `@SERIALIZABLE`. There are built-in engine attributes, and custom ones can be added.

Changing a type or adding new fields or functions is not possible.

Reflection is used for serialization and deserialization, finding animated fields, and generating the editor interface.

Conceptually everything is split into 2 subsystems: the type system and code generation. A separate utility shipped with the engine parses header .h/.hpp files, finds classes that should be reflected, and generates type description meta-code appended to the end of the header file.

It looks approximately like this:
<details>
<summary>Meta-code example</summary>

```
// --- META ---

CLASS_BASES_META(Reel)
{
	BASE_CLASS(Component);
}
END_META;
CLASS_FIELDS_META(Reel)
{
	FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(blurredImages);
	FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(100.0f).NAME(imagesDistance);
	FIELD().PRIVATE().NAME(mImages);
	FIELD().PRIVATE().DEFAULT_VALUE(0.0f).NAME(mRotatingOffset);
}
END_META;
CLASS_METHODS_META(Reel)
{
	FUNCTION().PUBLIC().CONSTRUCTOR();
	FUNCTION().PRIVATE().SIGNATURE(void, OnDisabled);
	FUNCTION().PRIVATE().SIGNATURE(void, UpdateImagesLayout);
}
END_META;
// --- END META ---
```
</details>

Appending to the end of the file does not disturb development much, and it is the optimal choice for compilation compared to extracting all meta-code into one big file. Such a file would include the whole project, causing long compilation on any change.

Based on the generated meta-information, types are initialized at application start, scripted types are registered, and serialization happens when needed.

### Type system
Strange as it may sound, a type also has subtypes. The base type is `o2::Type`. It stores basic type information: name, size, unique id, list of base types, list of fields, list of member functions and list of static functions.

For functions and fields, a list of attributes is stored, which may contain metadata.

Besides providing type data, the `o2::Type` class gives extra functionality:
- creating an instance of an object of this type, `CreateSample()`
- finding a pointer to a field by path. The field can be nested inside sub-objects; the path is then specified like a directory path with slash separators: `path/to/some/field`.
- getting a proxy structure for a field of this type. These proxy structures (`IAbstractValueProxy`) unify different approaches of owning and changing a value. It can be a pointer to a value, a setter/getter function pair, a value from a script, etc.
- serialization/deserialization
- equality checking. `IsValueEquals` takes two `void*` values as input and the check happens automatically inside
- value copying, `CopyValue`

Specialized types inherit from the base `o2::Type`:
- `FunctionType` - a `Function<>` delegate
- `ObjectType` - an object. Has methods for casting up and down relative to `IObject`
- `FundamentalType` - a fundamental language type (int, float, string ...)
- `PointerType` - a pointer to a type; can return the non-pointer type
- `ReferenceType` - a type for `Ref<>` smart references; can return the referenced object type
- `PropertyType` - a type for properties
- `VectorType` - a type for `Vector<>`; can return the element count and a pointer to a specific element by index
- `MapType` - a type for `Map<>`; can return values by keys and the key list
- `StringPointerAccessorType` - a type for the string pointer accessor, a special wrapper overloading `operator[](String& key)`
- `EnumType` - a type for enums; provides the list of all possible enum values

### Attributes
Attributes are used to mark certain traits of fields or functions. They can indicate whether a field is serialized, excluded from or included into the editor, etc. Custom attributes can be created.

An attribute is defined by a special class inherited from `IAttribute` with internal macros defining its naming in code and meta-information.
```
ATTRIBUTE_COMMENT_DEFINITION("SERIALIZABLE");         // In code the tag is read as @SERIALIZABLE
ATTRIBUTE_SHORT_DEFINITION("SERIALIZABLE_ATTRIBUTE"); // Added to the meta-information as .SERIALIZABLE_ATTRIBUTE()
```

### Type description meta-code and its usage
This meta-code effectively consists of template functions injected into the class the type is built for. It splits into 3 blocks: the list of base classes, the list of fields and the list of functions. Each of these blocks is a template function.

The template parameter is a special type processor like `o2::BaseTypeProcessor`, which has handler functions for entries about base classes, fields and methods.

This way you can define some object type processor that performs the needed function while iterating type data.

That is how types are initialized at startup. Besides the meta-information added to the header file, type registration is automatically added to the .cpp files, e.g. `DECLARE_CLASS(o2::Actor, o2__Actor);` (the second parameter is a unique registrator identifier). It initiates type registration with a specific processor. This processor creates a type object of the right kind (descendants of `o2::Type`) and fills it with data.

The same processor mechanism also performs serialization and deserialization, covered in a separate section.

### The o2::Reflection subsystem
The engine has a singleton class responsible for the reflection system - `o2::Reflection`. For shorter access there is the `o2Reflection` macro.

Through it you can get the list of all types, create a type instance, get a type by name, and convert an enum to a string and back.

### Object reflection
For a class to be reflected, it must inherit `o2::IObject` and place the `IOBJECT(NAME_OF_CLASS)` macro inside the class body. This macro adds the needed fields and functions to the class. The code generation system recognizes these markers itself and generates the meta-code.

### Getting an object's type
Classes reflected in the type system get a static field `o2::Type* type` storing the class type. A **non-static** method `const o2::Type& GetType() const` is also added, returning the real type of the object at runtime.

There is also the `TypeOf(TYPE)` macro, returning the type for any C++ type. It can be a class as well as an elementary type: int, float, bool, etc.

### Code generation and the preprocessor
The generator writes metadata to the end of the `.h` and the `DECLARE_CLASS` registrator to the paired `.cpp`. Without that registrator the static field `type` stays null, and `GetType()` returns a null reference - so a class marked with `IOBJECT` is always reflected, even when its base chain runs through files the generator doesn't see (excluded ones, for example); such a case is reported as a warning.

A class inside a conditional compilation block gets its metadata wrapped into the same condition. An include guard (`#ifndef NAME` immediately followed by `#define NAME`) isn't a condition: the metadata sits after its `#endif`, wrapping would disable it, so such guards are recognized and ignored.
