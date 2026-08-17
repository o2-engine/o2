## Properties
For convenient work with engine entities and user code, the engine has special wrappers over setter/getter functions.

For example, there are functions for a transparency value:
```C++
void SetTransparency(float value);
float GetTransparency() const;
```

Using them in math expressions is inconvenient, especially in complex calculations.

`object.SetTransparency(object.GetTransparency()*0.5f);`

It is much easier to operate on variable values:

`object.transparency *= 0.5f;`

The code becomes shorter and more readable.

For this the engine has macros defining C#-like properties.

```C++
PROPERTIES(MyClass);
PROPERTY(float, transparency, SetTransparency, GetTransparency);
GETTER(int, index, GetIndex);
ACCESSOR(Ref<Actor>, child, String, GetChild, GetAllChilds);
```

They define a nested class that overloads the required math operators, which use the specified setter/getter. The pointer to the owning object is computed via `offsetof` from the property's own address, so a property stores no data and adds no per-pointer overhead.

The example above shows 3 kinds:
- `PROPERTY` - standard getter/setter
- `GETTER`/`SETTER` - read-only or write-only value
- `ACCESSOR` - defines a variable with an overloaded `operator[]` of the specified type. Convenient for accessing elements by name, as in this example accessing an actor's children: `actor->child["name of child"]`;

Properties are also utilized in animations. They are available for animating, including `ACCESSOR`, which lets code react to value changes through the setter.

The macros are usable in classes outside `namespace o2` as well - everything they reference is fully qualified. This is how the cocos2d nodes of the integration declare properties over their own `set`/`get` pairs, which gives both a shorter API (`node->rotation = 45`) and editor rows going through the setters.
