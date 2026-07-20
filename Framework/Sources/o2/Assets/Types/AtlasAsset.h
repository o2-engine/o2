#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/Types/FolderAsset.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/TextureSource.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    // -----------
    // Atlas asset
    // -----------
    class AtlasAsset: public Asset
    {
    public: 
        class Meta;
        class Page;

    public:
        PROPERTIES(AtlasAsset);
        GETTER(Ref<Meta>, meta, GetMeta);                 // Meta information getter
        GETTER(Vector<AssetRef<ImageAsset>>, images, GetImages); // Images assets getter
        GETTER(Vector<Page>, pages, GetPages);            // Pages getter

    public:
        // Default constructor
        AtlasAsset();

        // Copy-constructor
        AtlasAsset(const AtlasAsset& asset);

        // Destructor
        ~AtlasAsset() override;

        // Check equals operator
        AtlasAsset& operator=(const AtlasAsset& asset);

        // Returns atlas sprite source
        TextureSource GetSpriteSource(const AssetRef<ImageAsset>& image);

        // Returns containing images assets
        const Vector<AssetRef<ImageAsset>>& GetImages() const;

        // Returns pages array
        const Vector<Page>& GetPages() const;

        // Is contains image
        bool ContainsImage(const AssetRef<ImageAsset>& image);

        // Adds image to atlas
        void AddImage(const AssetRef<ImageAsset>& image);

        // Removes image from atlas
        void RemoveImage(const AssetRef<ImageAsset>& image);

        // Removes all images from atlas
        void RemoveAllImages();

        // Reloads pages textures
        void ReloadPages();

        // Returns meta information
        Ref<Meta> GetMeta() const;

        // Returns extensions string
        static Vector<String> GetFileExtensions();

        // Returns atlas page's texture file name
        static String GetPageTextureFileName(const AssetInfo& atlasInfo, UInt pageIdx);

        // Returns atlas page's texture reference
        static TextureRef GetPageTextureRef(const AssetInfo& atlasInfo, UInt pageIdx);

        // Returns editor sorting weight
        static int GetEditorSorting() { return 97; }

        // Is this asset type is available to create from editor's assets window
        static bool IsAvailableToCreateFromEditor() { return true; }

        ASSET_TYPE(AtlasAsset, Meta);

    public:
        // -----------------------------------
        // Platform specified meta information
        // -----------------------------------
        struct PlatformMeta: public ISerializable, public RefCounterable, public ICloneableRef
        {
            Vec2I maxSize = Vec2I(2048, 2048); // Maximal atlas size @SERIALIZABLE

            TextureCompression compression = TextureCompression::None; // Pages block compression @SERIALIZABLE
            int                quality = 100; // Compression quality/effort, 0..100 @SERIALIZABLE

            int border = 0; // Images pack border @SERIALIZABLE

            float imagesScale = 1.0f; // Source images scale applied when packing @SERIALIZABLE

            bool operator==(const PlatformMeta& other) const;

            SERIALIZABLE(PlatformMeta);
            CLONEABLE_REF(PlatformMeta);
        };

        // ----------------
        // Meta information
        // ----------------
        class Meta: public DefaultAssetMeta<AtlasAsset>
        {
        public:
            PlatformMeta common; // Common meta @SERIALIZABLE

            Ref<PlatformMeta> ios = nullptr;     // IOS specified meta @SERIALIZABLE
            Ref<PlatformMeta> android = nullptr; // Android specified meta @SERIALIZABLE
            Ref<PlatformMeta> macOS = nullptr;   // MacOS specified meta @SERIALIZABLE
            Ref<PlatformMeta> windows = nullptr; // Windows specified meta @SERIALIZABLE
            Ref<PlatformMeta> linuxOS = nullptr; // Linux specified meta @SERIALIZABLE
            Ref<PlatformMeta> webAssembly = nullptr; // WebAssembly specified meta @SERIALIZABLE

        public:
            // Returns platform meta for specified platform
            PlatformMeta GetResultPlatformMeta(Platform platform) const;

            // Returns true if other meta is equal to this
            bool IsEqual(AssetMeta* other) const override;

            SERIALIZABLE(Meta);
            CLONEABLE_REF(Meta);
        };

        // ----------
        // Atlas page
        // ----------
        class Page: public ISerializable
        {
        public:
            // Returns number
            UInt ID() const;

            // Returns size
            Vec2I Size() const;

            // Returns texture reference
            TextureRef GetTexture() const;

            // Returns texture file name
            String GetTextureFileName() const;

            // Returns images rectangles
            const Map<UID, RectI>& ImagesRects() const;

            // Check equal operator
            bool operator==(const Page& other) const;

            SERIALIZABLE(Page);

        private:
            UInt  mId;   // Page number @SERIALIZABLE
            Vec2I mSize; // Size of page @SERIALIZABLE

            Map<UID, RectI> mImagesRects; // Images source rectangles @SERIALIZABLE
            TextureRef      mTexture;     // Page texture

            WeakRef<AtlasAsset> mOwner; // Owner atlas

            friend class AtlasAssetConverter;
            friend class AtlasAsset;
        };

    protected:
        Vector<AssetRef<ImageAsset>>  mImages;  // Loaded image infos @SERIALIZABLE @EDITOR_PROPERTY
        Vector<AssetRef<FolderAsset>> mFolders; // Folders, included in atlas @SERIALIZABLE @EDITOR_PROPERTY

        Vector<Page> mPages; // Pages @SERIALIZABLE

    protected:
        // It is called after reference initialization at object construction, registers atlas in render
        void PostRefConstruct();

        // Completion deserialization callback
        void OnDeserialized(const DataValue& node) override;

        friend class Assets;
        friend class ImageAsset;

        FRIEND_REF_MAKE();
    };
}
// --- META ---

CLASS_BASES_META(o2::AtlasAsset)
{
    BASE_CLASS(o2::Asset);
}
END_META;
CLASS_FIELDS_META(o2::AtlasAsset)
{
    FIELD().PUBLIC().NAME(meta);
    FIELD().PUBLIC().NAME(images);
    FIELD().PUBLIC().NAME(pages);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mImages);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mFolders);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mPages);
}
END_META;
CLASS_METHODS_META(o2::AtlasAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const AtlasAsset&);
    FUNCTION().PUBLIC().SIGNATURE(TextureSource, GetSpriteSource, const AssetRef<ImageAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<AssetRef<ImageAsset>>&, GetImages);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Page>&, GetPages);
    FUNCTION().PUBLIC().SIGNATURE(bool, ContainsImage, const AssetRef<ImageAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(void, AddImage, const AssetRef<ImageAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveImage, const AssetRef<ImageAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAllImages);
    FUNCTION().PUBLIC().SIGNATURE(void, ReloadPages);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Meta>, GetMeta);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetPageTextureFileName, const AssetInfo&, UInt);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(TextureRef, GetPageTextureRef, const AssetInfo&, UInt);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableToCreateFromEditor);
    FUNCTION().PROTECTED().SIGNATURE(void, PostRefConstruct);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;

CLASS_BASES_META(o2::AtlasAsset::PlatformMeta)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::AtlasAsset::PlatformMeta)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Vec2I(2048, 2048)).NAME(maxSize);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(TextureCompression::None).NAME(compression);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(100).NAME(quality);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(border);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(imagesScale);
}
END_META;
CLASS_METHODS_META(o2::AtlasAsset::PlatformMeta)
{
}
END_META;

CLASS_BASES_META(o2::AtlasAsset::Meta)
{
    BASE_CLASS(o2::DefaultAssetMeta<AtlasAsset>);
}
END_META;
CLASS_FIELDS_META(o2::AtlasAsset::Meta)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(common);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(ios);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(android);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(macOS);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(windows);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(linuxOS);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(webAssembly);
}
END_META;
CLASS_METHODS_META(o2::AtlasAsset::Meta)
{

    FUNCTION().PUBLIC().SIGNATURE(PlatformMeta, GetResultPlatformMeta, Platform);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEqual, AssetMeta*);
}
END_META;

CLASS_BASES_META(o2::AtlasAsset::Page)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::AtlasAsset::Page)
{
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().NAME(mId);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().NAME(mSize);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().NAME(mImagesRects);
    FIELD().PRIVATE().NAME(mTexture);
    FIELD().PRIVATE().NAME(mOwner);
}
END_META;
CLASS_METHODS_META(o2::AtlasAsset::Page)
{

    typedef const Map<UID, RectI>& _tmp1;

    FUNCTION().PUBLIC().SIGNATURE(UInt, ID);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, Size);
    FUNCTION().PUBLIC().SIGNATURE(TextureRef, GetTexture);
    FUNCTION().PUBLIC().SIGNATURE(String, GetTextureFileName);
    FUNCTION().PUBLIC().SIGNATURE(_tmp1, ImagesRects);
}
END_META;
// --- END META ---
