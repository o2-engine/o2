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
- BinaryAsset: binary file
- DataAsset: serialized data, configs
- VectorFontAsset/BitmapFontAsset: vector/bitmap font (base FontAsset), FontStyleAsset: font style
- MaterialAsset, ShaderAsset (VertexShaderAsset/FragmentShaderAsset): materials and shaders
- Mesh3DAsset, SkinnedModelAsset: 3D meshes and skinned models
- SoundAsset: sound (wav, ogg, mp3, flac)
- SpineAsset, SpineAtlasAsset: Spine skeleton and atlas
- JavaScriptAsset: JS script

### Asset references
An asset reference is the template class `o2::AssetRef<AssetType>`, a descendant of the non-template base `o2::BaseAssetRef`. It functions like a typical smart pointer.

It is more correct to work with assets through references, to avoid duplicate asset loading.
