#include "o2/stdafx.h"
#include "TextureRef.h"

#include "o2/Assets/Assets.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    TextureRef::TextureRef() :
        mTexture()
    {}

    TextureRef::TextureRef(std::nullptr_t) :
        mTexture(nullptr)
    {}

    TextureRef::TextureRef(Texture* ptr) :
        mTexture(ptr)
    {}

    TextureRef::TextureRef(const TextureRef& other) :
        mTexture(other.mTexture)
    {}

    TextureRef::TextureRef(const Ref<Texture>& other):
        mTexture(other)
    {}

    TextureRef::TextureRef(Ref<Texture>&& other):
        mTexture(std::move(other))
    {}

    TextureRef::TextureRef(TextureRef&& other) noexcept :
        mTexture(std::move(other.mTexture))
    {}

    TextureRef::TextureRef(const Vec2I& size,
                           TextureFormat format /*= TextureFormat::R8G8B8A8*/,
                           Texture::Usage usage /*= Texture::Usage::Default*/) :
        TextureRef(mmake<Texture>(size, format, usage))
    {}

    TextureRef::TextureRef(const String& fileName)
    {
        *this = o2Render.mTextures.FindOrDefault([&](const TextureRef& tex) { return tex->GetFileName() == fileName; });

        if (!mTexture)
            *this = mmake<Texture>(fileName);
    }

    TextureRef::TextureRef(const Bitmap& bitmap) :
        TextureRef(mmake<Texture>(bitmap))
    {}

    TextureRef::TextureRef(UID atlasAssetId, int page)
    {
        *this = (o2Render.mTextures.FindOrDefault(
            [&](const TextureRef& tex)
            {
                return tex->GetAtlasAssetId() == atlasAssetId && tex->GetAtlasPage() == page;
            }));

        if (!mTexture)
            *this = mmake<Texture>(atlasAssetId, page);
    }

    TextureRef::TextureRef(const String& atlasAssetName, int page)
    {
        UID atlasAssetId = o2Assets.GetAssetId(atlasAssetName);
        if (atlasAssetId == UID::empty)
        {
            o2Render.mLog->Error("Can't load texture for atlas " + atlasAssetName + " and page " + (String)page + ": atlas isn't exist");
            *this = nullptr;
            return;
        }

        *this = (o2Render.mTextures.FindOrDefault(
            [&](const TextureRef& tex)
            {
                return tex->GetAtlasAssetId() == atlasAssetId && tex->GetAtlasPage() == page;
            }));

        if (!mTexture)
            *this = mmake<Texture>(atlasAssetId, page);
    }

    bool TextureRef::operator!=(const Texture* other) const
    {
        return mTexture != other;
    }

    bool TextureRef::operator!=(const TextureRef& other) const
    {
        return mTexture != other.mTexture;
    }

    bool TextureRef::operator==(const Texture* other) const
    {
        return mTexture == other;
    }

    bool TextureRef::operator==(const TextureRef& other) const
    {
        return mTexture == other.mTexture;
    }

    TextureRef& TextureRef::operator=(Ref<Texture>&& other)
    {
        mTexture = other;
        return *this;
    }

    TextureRef& TextureRef::operator=(std::nullptr_t)
    {
        mTexture = nullptr;
        return *this;
    }

    TextureRef& TextureRef::operator=(const TextureRef& other)
    {
        mTexture = other.mTexture;
        return *this;
    }

    TextureRef& TextureRef::operator=(TextureRef&& other)
    {
        mTexture = std::move(other.mTexture);
        return *this;
    }

    bool TextureRef::operator<(const TextureRef& other) const
    {
        return mTexture < other.mTexture;
    }

    bool TextureRef::IsValid() const
    {
        return mTexture.IsValid();
    }

    TextureRef::operator bool() const
    {
        return mTexture.operator bool();
    }

    TextureRef::operator Ref<Texture>() const
    {
        return mTexture;
    }

    Texture& TextureRef::operator*() const
    {
        return mTexture.operator*();
    }

    Texture* TextureRef::operator->() const
    {
        return mTexture.operator->();
    }

    Texture* TextureRef::Get()
    {
        return mTexture.Get();
    }

    const Texture* TextureRef::Get() const
    {
        return mTexture.Get();
    }

    Ref<Texture>& TextureRef::GetRef()
    {
        return mTexture;
    }

    const Ref<Texture>& TextureRef::GetRef() const
    {
        return mTexture;
    }

    TextureRef TextureRef::Null()
    {
        return TextureRef();
    }
}
// --- META ---

DECLARE_CLASS(o2::TextureRef, o2__TextureRef);
// --- END META ---
