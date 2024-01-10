#include "o2/stdafx.h"
#include "AtlasAsset.h"

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Assets/Assets.h"
#include "o2/Render/Render.h"

namespace o2
{
    AtlasAsset::PlatformMeta AtlasAsset::Meta::GetResultPlatformMeta(Platform platform) const
    {
        PlatformMeta res = common;

        switch (platform)
        {
            case Platform::iOS: if (ios) res = *ios; break;
            case Platform::Android: if (android) res = *android; break;
            case Platform::Mac: if (macOS) res = *macOS; break;
            case Platform::Windows: if (windows) res = *windows; break;
            case Platform::Linux: if (linuxOS) res = *linuxOS; break;
        }

        return res;
    }

    bool AtlasAsset::Meta::IsEqual(AssetMeta* other) const
    {
        if (!AssetMeta::IsEqual(other))
            return false;

        Meta* otherMeta = (Meta*)other;

        if (!(common == otherMeta->common))
            return false;

        auto comparePlatformMeta = [](const Ref<PlatformMeta>& a, const Ref<PlatformMeta>& b)
        {
            if (a && b)
            {
                if (!(*a == *b))
                    return false;
            }
            else if (a || b)
                return false;

            return true;
        };

        if (!comparePlatformMeta(ios, otherMeta->ios) ||
            !comparePlatformMeta(android, otherMeta->android) ||
            !comparePlatformMeta(macOS, otherMeta->macOS) ||
            !comparePlatformMeta(windows, otherMeta->windows) ||
            !comparePlatformMeta(linuxOS, otherMeta->linuxOS))
        {
            return false;
        }

        return true;
    }

    UInt AtlasAsset::Page::ID() const
    {
        return mId;
    }

    Vec2I AtlasAsset::Page::Size() const
    {
        return mSize;
    }

    Ref<Texture> AtlasAsset::Page::GetTexture() const
    {
        return mTexture;
    }

    String AtlasAsset::Page::GetTextureFileName() const
    {
        return AtlasAsset::GetPageTextureFileName(mOwner.Lock()->mInfo, mId);
    }

    const Map<UID, RectI>& AtlasAsset::Page::ImagesRects() const
    {
        return mImagesRects;
    }

    bool AtlasAsset::Page::operator==(const Page& other) const
    {
        return mId == other.mId;
    }

    AtlasAsset::AtlasAsset() :
        Asset(mmake<Meta>())
    {
        o2Render.OnAtlasCreated(this);
    }

    AtlasAsset::AtlasAsset(const AtlasAsset& other) :
        Asset(other), mImages(other.mImages), mPages(other.mPages), meta(this), images(this), pages(this)
    {
        for (auto& page : mPages)
            page.mOwner = WeakRef(this);

        o2Render.OnAtlasCreated(this);
    }

    AtlasAsset::~AtlasAsset()
    {
        o2Render.OnAtlasDestroyed(this);
    }

    void AtlasAsset::OnDeserialized(const DataValue& node)
    {
        for (auto& page : mPages)
        {
            page.mOwner = WeakRef(this);
            page.mTexture = GetPageTextureRef(mInfo, page.mId);
        }
    }

    AtlasAsset& AtlasAsset::operator=(const AtlasAsset& other)
    {
        Asset::operator=(other);

        mImages = other.mImages;
        mPages = other.mPages;

        return *this;
    }

    TextureSource AtlasAsset::GetSpriteSource(const ImageAssetRef& image)
    {
        for (auto& page : mPages)
        {
            auto fnd = page.mImagesRects.find(image->GetUID());
            if (fnd != page.mImagesRects.end())
            {
                return { page.mTexture, fnd->second };
            }
        }

        return {};
    }

    const Vector<ImageAssetRef>& AtlasAsset::GetImages() const
    {
        return mImages;
    }

    const Vector<AtlasAsset::Page>& AtlasAsset::GetPages() const
    {
        return mPages;
    }

    bool AtlasAsset::ContainsImage(const ImageAssetRef& image)
    {
        return mImages.Contains(image);
    }

    void AtlasAsset::AddImage(const ImageAssetRef& image)
    {
        if (!mImages.Contains(image))
            mImages.Add(image);
    }

    void AtlasAsset::RemoveImage(const ImageAssetRef& image)
    {
        mImages.Remove(image);
    }

    void AtlasAsset::RemoveAllImages()
    {
        mImages.Clear();
    }

    void AtlasAsset::ReloadPages()
    {
        Reload();

        for (auto& img : mImages)
            img->Reload();
    }

    Ref<AtlasAsset::Meta> AtlasAsset::GetMeta() const
    {
        return DynamicCast<Meta>(mInfo.meta);
    }

    Vector<String> AtlasAsset::GetFileExtensions()
    {
        return { "atlas" };
    }

    String AtlasAsset::GetPageTextureFileName(const AssetInfo& atlasInfo, UInt pageIdx)
    {
        auto meta = DynamicCast<AtlasAsset::Meta>(atlasInfo.meta);
        String extension = Texture::formatFileExtensions.Get(meta->GetResultPlatformMeta(::GetEnginePlatform()).format);
        return (atlasInfo.tree ? atlasInfo.tree.Lock()->builtAssetsPath : String()) + atlasInfo.path + (String)pageIdx + "." + extension;
    }

    Ref<Texture> AtlasAsset::GetPageTextureRef(const AssetInfo& atlasInfo, UInt pageIdx)
    {
        return Ref<Texture>(GetPageTextureFileName(atlasInfo, pageIdx));
    }

    bool AtlasAsset::PlatformMeta::operator==(const PlatformMeta& other) const
    {
        return maxSize == other.maxSize && format == other.format;
    }
}

DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::AtlasAsset>);
DECLARE_TEMPLATE_CLASS(o2::Ref<o2::AtlasAsset>);
// --- META ---

DECLARE_CLASS(o2::AtlasAsset, o2__AtlasAsset);

DECLARE_CLASS(o2::AtlasAsset::PlatformMeta, o2__AtlasAsset__PlatformMeta);

DECLARE_CLASS(o2::AtlasAsset::Meta, o2__AtlasAsset__Meta);

DECLARE_CLASS(o2::AtlasAsset::Page, o2__AtlasAsset__Page);
// --- END META ---
