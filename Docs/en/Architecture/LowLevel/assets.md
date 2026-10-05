## Assets, the asset system
Every game resource is an asset. They go into the assets folder in raw form, and before launch they are built in a specific way into their final format.

For example, at the build stage textures are packed into atlases and compressed.

Each asset contains metadata defining a unique identifier and other parameters. This metadata lies next to the asset, has the same file name, but with the .meta extension.

Entities in the engine can reference an asset both by path and by identifier. The second way is the primary one. The identifier stays unchanged if the asset is moved or renamed. Thus assets can be moved around freely and references stay intact.

### The asset system
For basic work with assets, the engine has a separate subsystem `o2::Assets`, with quick access via the `o2Assets` macro.

It contains the resource cache, the tree of available assets, and has functionality for working with assets: creating an instance, moving, deleting, etc.

### The base asset class, Asset
It contains basic information about the asset: identifier, asset path. As well as basic functionality — loading, saving.

### Asset types
Asset subtypes inherit from the base `o2::Asset`:
- FolderAsset: a folder with assets that can be retrieved
- ActorAsset: actor prototype
- SceneAsset: scene
- AnimationAsset: animation clip
- AnimationStateGraphAsset: animation state graph
- AtlasAsset: atlas
- ImageAsset: image, references an atlas
- VectorImageAsset: vector image, an `.svg` file drawn as triangles by `o2::VectorSprite`
- BinaryAsset: binary file
- DataAsset: serialized data, configs
- VectorFontAsset/BitmapFontAsset: vector/bitmap font (base FontAsset), FontStyleAsset: font style
- MaterialAsset, ShaderAsset (VertexShaderAsset/FragmentShaderAsset): materials and shaders
- Mesh3DAsset, SkinnedModelAsset: 3D meshes and skinned models
- SoundAsset: sound (wav, ogg, mp3, flac)
- SpineAsset, SpineAtlasAsset: Spine skeleton and atlas
- JavaScriptAsset: JS script

### Vector image, VectorImageAsset
`o2::VectorImageAsset` is bound to the `svg` extension. The builder copies the file as is; on load it is parsed by `o2::SvgParser` into `o2::VectorImage` (see [Vector graphics](/Docs/en/Architecture/LowLevel/render.md)). Loading needs no render and no textures, the asset is not placed into an atlas.

- Meta (`VectorImageAsset::Meta`): `sliceBorder` (`BorderI`, in image units: left, bottom, right, top; top is the top side of the image) and `defaultMode` (`SpriteMode`) — the same meaning as in `ImageAsset::Meta`, `o2::VectorSprite` takes them when the image is assigned.
- `GetSize`, `GetImage`, `IsValid`, `GetError`, `GetWarnings`. A file that is not a valid SVG gives an asset with `IsValid() == false`, an empty image and no meshes; the error and the warnings about unsupported elements are written to the assets log once per load.
- `GetMesh(pixelScale)` and `GetSlicedMesh(pixelScale, borders)` return the tessellated triangles (`o2::VectorImageMesh`) from a cache shared by all drawables of the asset. The key is the pixel scale quantized by `QuantizePixelScale` (powers of two, range 1/16..64) and, for the sliced variant, the borders; a sliced mesh is split from the cached whole mesh of the same scale, without tessellating again. The cache holds up to 32 meshes and is dropped when the image changes; meshes in use stay alive.
- `SetSource(svg)` replaces the image from text, `GetSource` returns it, `Save` writes it back to the file.
- `GetVersion` changes on every load and is never equal for two loaded assets, so a drawable also notices another asset by it. After `Assets::RebuildAssets` the asset reloads itself when its id is in the changed list; drawables compare the version at drawing and rebuild their vertices.

### Asset references
An asset reference is the template class `o2::AssetRef<AssetType>`, a descendant of the non-template base `o2::BaseAssetRef`. It functions like a typical smart pointer.

It is more correct to work with assets through references, to avoid duplicate asset loading.
