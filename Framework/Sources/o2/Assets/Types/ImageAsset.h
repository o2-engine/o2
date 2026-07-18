#pragma once

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Render/TextureSource.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Bitmap/Bitmap.h"

namespace o2
{
    // -----------
    // Image asset
    // -----------
    class ImageAsset: public Asset
    {
    public:
        class Meta;

    public:
        PROPERTIES(ImageAsset);
        PROPERTY(Ref<Bitmap>, bitmap, SetBitmap, GetBitmap); // Bitmap data property

        PROPERTY(BorderI, sliceBorder, SetSliceBorder, GetSliceBorder);    // Slice border property
        PROPERTY(SpriteMode, defaultMode, SetDefaultMode, GetDefaultMode); // Sprite default mode property

        PROPERTY(UID, atlas, SetAtlas, GetAtlasUID); // Atlas owner asset property
        GETTER(UInt, atlasPage, GetAtlasPage);    // Atlas page index getter
        GETTER(RectI, atlasRect, GetAtlasRect);   // Atlas source image rectangle getter

        GETTER(Vec2F, size, GetSize);     // Image size getter
        GETTER(float, width, GetWidth);   // Image width getter
        GETTER(float, height, GetHeight); // Image height getter

        GETTER(Ref<Meta>, meta, GetMeta); // Meta information getter

    public:
        // Default constructor
        ImageAsset();

        // Copy-constructor
        ImageAsset(const ImageAsset& asset);

        // Check equals operator
        ImageAsset& operator=(const ImageAsset& asset);

        // Returns bitmap data
        const Ref<Bitmap>& GetBitmap();

        // Sets bitmap data
        void SetBitmap(Ref<Bitmap> bitmap);

        // Returns atlas asset
        UID GetAtlasUID() const;

        // Sets atlas
        void SetAtlas(const UID& atlas);

        // Returns is in atlas @SCRIPTABLE
        bool IsInAtlas() const;

        // Sets slice border @SCRIPTABLE
        void SetSliceBorder(const BorderI& border);

        // Returns slice border @SCRIPTABLE
        BorderI GetSliceBorder() const;

        // Sets default sprite mode @SCRIPTABLE
        void SetDefaultMode(SpriteMode mode);

        // Returns default sprite mode @SCRIPTABLE
        SpriteMode GetDefaultMode() const;

        // Returns atlas page @SCRIPTABLE
        UInt GetAtlasPage() const;

        // Returns atlas source rectangle @SCRIPTABLE
        RectI GetAtlasRect() const;

        // Returns image size @SCRIPTABLE
        Vec2F GetSize() const;

        // Returns image width @SCRIPTABLE
        float GetWidth() const;

        // Returns image height @SCRIPTABLE
        float GetHeight() const;

        // Returns atlas sprite source @SCRIPTABLE
        TextureSource GetTextureSource() const;

        // Returns meta information @SCRIPTABLE
        Ref<Meta> GetMeta() const;

        // Returns extensions string
        static Vector<String> GetFileExtensions();

        ASSET_TYPE(ImageAsset, Meta);

    public:
        // -----------------------
        // Platform specified meta
        // -----------------------
        struct PlatformMeta: public ISerializable, public RefCounterable, public ICloneableRef
        {
            TextureFormat format = TextureFormat::R8G8B8A8; // Texture format @SERIALIZABLE

            bool operator==(const PlatformMeta& other) const;

            SERIALIZABLE(PlatformMeta);
            CLONEABLE_REF(PlatformMeta);
        };

        // ----------------
        // Meta information
        // ----------------
        class Meta: public DefaultAssetMeta<ImageAsset>
        {
        public:
            UID atlasId = UID::empty; // Atlas owner id @SERIALIZABLE

            PlatformMeta common; // Common platform meta @SERIALIZABLE

            Ref<PlatformMeta> ios = nullptr;     // IOS specified meta @SERIALIZABLE
            Ref<PlatformMeta> android = nullptr; // Android specified meta @SERIALIZABLE
            Ref<PlatformMeta> macOS = nullptr;   // MacOS specified meta @SERIALIZABLE
            Ref<PlatformMeta> windows = nullptr; // Windows specified meta @SERIALIZABLE
            Ref<PlatformMeta> linuxOS = nullptr; // Linux specified meta @SERIALIZABLE
            Ref<PlatformMeta> webAssembly = nullptr; // WebAssembly specified meta @SERIALIZABLE

            BorderI    sliceBorder;          // Default slice border @SERIALIZABLE @EDITOR_IGNORE
            SpriteMode defaultMode;          // Default sprite mode @SERIALIZABLE @EDITOR_IGNORE

        public:
            // Returns platform meta for specified platform
            PlatformMeta GetResultPlatformMeta(Platform platform) const;

            // Returns true if other meta is equal to this
            bool IsEqual(AssetMeta* other) const override;

            SERIALIZABLE(Meta);
            CLONEABLE_REF(Meta);
        };

    protected:
        Ref<Bitmap> mBitmap = nullptr; // Image bitmap. Loading only when needs

        TextureRef mTexture; // Texture reference, if image is not in atlas, it loads texture

        AssetRef<Asset> mAtlas; // Owner atlas

        UInt  mAtlasPage; // Owner atlas page index @SERIALIZABLE
        RectI mSourceRect; // Owner atlas rectangle @SERIALIZABLE

    protected:
        // Loads texture if image is not in atlas, otherwise loads serializable data
        void LoadData(const String& path) override;

        // Saves data
        void SaveData(const String& path) const override;

        // Load bitmap
        void LoadBitmap();

        friend class AtlasAsset;
        friend class Assets;
    };
}
// --- META ---

CLASS_BASES_META(o2::ImageAsset)
{
    BASE_CLASS(o2::Asset);
}
END_META;
CLASS_FIELDS_META(o2::ImageAsset)
{
    FIELD().PUBLIC().NAME(bitmap);
    FIELD().PUBLIC().NAME(sliceBorder);
    FIELD().PUBLIC().NAME(defaultMode);
    FIELD().PUBLIC().NAME(atlas);
    FIELD().PUBLIC().NAME(atlasPage);
    FIELD().PUBLIC().NAME(atlasRect);
    FIELD().PUBLIC().NAME(size);
    FIELD().PUBLIC().NAME(width);
    FIELD().PUBLIC().NAME(height);
    FIELD().PUBLIC().NAME(meta);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mBitmap);
    FIELD().PROTECTED().NAME(mTexture);
    FIELD().PROTECTED().NAME(mAtlas);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mAtlasPage);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSourceRect);
}
END_META;
CLASS_METHODS_META(o2::ImageAsset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const ImageAsset&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Bitmap>&, GetBitmap);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBitmap, Ref<Bitmap>);
    FUNCTION().PUBLIC().SIGNATURE(UID, GetAtlasUID);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAtlas, const UID&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsInAtlas);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetSliceBorder, const BorderI&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(BorderI, GetSliceBorder);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetDefaultMode, SpriteMode);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(SpriteMode, GetDefaultMode);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(UInt, GetAtlasPage);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(RectI, GetAtlasRect);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vec2F, GetSize);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetWidth);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetHeight);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(TextureSource, GetTextureSource);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<Meta>, GetMeta);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, SaveData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadBitmap);
}
END_META;

CLASS_BASES_META(o2::ImageAsset::PlatformMeta)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::ImageAsset::PlatformMeta)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(TextureFormat::R8G8B8A8).NAME(format);
}
END_META;
CLASS_METHODS_META(o2::ImageAsset::PlatformMeta)
{
}
END_META;

CLASS_BASES_META(o2::ImageAsset::Meta)
{
    BASE_CLASS(o2::DefaultAssetMeta<ImageAsset>);
}
END_META;
CLASS_FIELDS_META(o2::ImageAsset::Meta)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(UID::empty).NAME(atlasId);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(common);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(ios);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(android);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(macOS);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(windows);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(linuxOS);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(webAssembly);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(sliceBorder);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(defaultMode);
}
END_META;
CLASS_METHODS_META(o2::ImageAsset::Meta)
{

    FUNCTION().PUBLIC().SIGNATURE(PlatformMeta, GetResultPlatformMeta, Platform);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEqual, AssetMeta*);
}
END_META;
// --- END META ---
