## Serialization
Using the generated class meta-information, objects are serialized and deserialized automatically. There is no need to write reading and writing code by hand — everything happens automatically and in an optimized way.

By default serialization uses the JSON format, but any other data format can be added, for example a binary one.

To make a type serializable, inherit it from the base class `o2::ISerializable` (which already inherits `o2::IObject`) and add the `SERIALIZABLE(NAME_OF_CLASS)` macro inside the class; it already includes the `IOBJECT()` macro.

Serializable fields must be marked with the `@SERIALIZABLE` tag. Simple types, nested objects and user-defined types are serialized.

<details>
<summary>Serializable class example</summary>

```C++
class Reel : public ISerializable
{
public:
	LinkRef<Actor> imagesContainer; // @SERIALIZABLE

	Vector<AssetRef<ImageAsset>> images;        // @SERIALIZABLE
	Vector<AssetRef<ImageAsset>> blurredImages; // @SERIALIZABLE

	float imagesDistance = 100.0f; // @SERIALIZABLE

	bool disableExtendedSymbols = false; // @SERIALIZABLE

	SERIALIZABLE(Reel);
};
```
</details>

## DataValue
For working with the data structure (JSON in particular), a tree structure of DataValue is used. This implementation is very similar to rapidjson.

The structure stores the value type and the value itself. The value can be simple (number, string, bool, null), an object or an array.

The DataDocument wrapper is used for reading from and writing to files.
