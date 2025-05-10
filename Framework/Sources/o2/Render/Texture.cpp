#include "o2/stdafx.h"
#include "o2/Render/Texture.h"

#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/AtlasAsset.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/FileSystem.h"

namespace o2
{
    const Map<TextureFormat, String> Texture::formatFileExtensions =
    {
        { TextureFormat::R8G8B8A8, "png" },
        { TextureFormat::DXT5, "dds" }
    };

    Texture::Texture() :
        mReady(false), mAtlasAssetId(0), mAtlasPage(-1)
    {}

    Texture::Texture(const Vec2I& size, TextureFormat format /*= TextureFormat::R8G8B8A8*/, Usage usage /*= Usage::Default*/) :
        mReady(false), mAtlasAssetId(0), mAtlasPage(-1)
    {
        Create(size, format, usage);
    }

    Texture::Texture(const String& fileName) :
        mReady(false), mAtlasAssetId(0), mAtlasPage(-1)
    {
        Create(fileName);
    }

    Texture::Texture(const Bitmap& bitmap) :
        mReady(false), mAtlasAssetId(0), mAtlasPage(-1)
    {
        Create(bitmap);
    }

    Texture::Texture(UID atlasAssetId, int page) :
        mReady(false), mAtlasAssetId(0), mAtlasPage(-1)
    {
        Create(atlasAssetId, page);
    }

    Texture::Texture(const String& atlasAssetName, int page) :
        mReady(false), mAtlasAssetId(0), mAtlasPage(-1)
    {
        Create(atlasAssetName, page);
    }

    void Texture::PostRefConstruct()
    {
        o2Render.OnTextureCreated(this);
    }

    Texture::~Texture()
    {
        if (!mReady)
            return;

        PlatformDestroy();
    }

    void Texture::Create(const Vec2I& size, TextureFormat format /*= TextureFormat::R8G8B8A8*/,
                         Usage usage /*= Usage::Default*/)
    {
        if (mReady)
        {
            PlatformDestroy();
            mReady = false;
        }

        mFormat = format;
        mUsage = usage;
        mSize = size;

        mReady = PlatformCreate();
    }

    void Texture::Create(const Vec2I& size, Byte* data, TextureFormat format /*= TextureFormat::R8G8B8A8*/)
    {
        if (mReady)
        {
            PlatformDestroy();
            mReady = false;
        }

        mFormat = format;
        mUsage = Usage::Default;
        mSize = size;
        mReady = PlatformCreate();

        if (mReady)
            PlatformUploadData(size, data, format);
    }

    void Texture::Create(const Bitmap& bitmap)
    {
        mFileName = bitmap.GetFilename();
        Create(bitmap.GetSize(), (Byte*)bitmap.GetData(), TextureFormat::R8G8B8A8);
    }

    void Texture::SetData(const Bitmap& bitmap)
    {
        if (mSize != bitmap.GetSize())
        {
            o2Render.mLog->Error("Cant set data to texture with different size");
            return;
        }

        PlatformUploadData(mSize, (Byte*)bitmap.GetData(), TextureFormat::R8G8B8A8);
    }

    void Texture::SetSubData(const Vec2I& offset, const Bitmap& bitmap)
    {
        PlatformUploadRegionData(offset, bitmap.GetSize(), (Byte*)bitmap.GetData(), TextureFormat::R8G8B8A8);
    }

    Ref<Bitmap> Texture::GetData()
    {
        auto bitmap = mmake<Bitmap>(PixelFormat::R8G8B8A8, mSize);
        PlatformGetData((Byte*)bitmap->GetData());
        return bitmap;
    }

    void Texture::SetFilter(Filter filter)
    {
        mFilter = filter;
        PlatformSetFilter();
    }

    Texture::Filter Texture::GetFilter() const
    {
        return mFilter;
    }

    void Texture::Create(const String& fileName)
    {
        String extension = o2FileSystem.GetFileExtension(fileName);

        if (extension == "png")
            LoadPNG(fileName);
        else if (extension == "dds")
            LoadDDS(fileName);
        else
            o2Render.mLog->Error("Failed to load texture from file " + fileName);
    }

    void Texture::Create(UID atlasAssetId, int page)
    {
        auto& info = o2Assets.GetAssetInfo(atlasAssetId);
        if (info.IsValid())
        {
            mAtlasAssetId = atlasAssetId;
            mAtlasPage = page;
            String textureFileName = AtlasAsset::GetPageTextureFileName(info, page);
            Create(textureFileName);

            mReady = true;
        }
        else 
            o2Render.mLog->Error("Failed to load atlas texture with id " + (String)atlasAssetId + " and page " + (String)page);
    }

    void Texture::Create(const String& atlasAssetName, int page)
    {
        auto& info = o2Assets.GetAssetInfo(atlasAssetName);
        if (info.IsValid())
        {
            mAtlasAssetId = o2Assets.GetAssetId(atlasAssetName);
            mAtlasPage = page;
            String textureFileName = AtlasAsset::GetPageTextureFileName(info, page);
            Create(textureFileName);

            mReady = true;
        }
        else 
            o2Render.mLog->Error("Failed to load atlas texture with " + atlasAssetName + " and page " + (String)page);
    }

    void Texture::LoadDDS(const String& fileName)
    {
        mFileName = fileName;

        InFile file(fileName);
        if (file.IsOpened())
        {
            UInt dataSize = file.GetDataSize();
            auto data = mnew Byte[dataSize];
            file.ReadData(data, dataSize);
            file.Close();

            UInt height = *(UInt*)&(data[12]);
            UInt width = *(UInt*)&(data[16]);
            UInt linearSize = *(UInt*)&(data[20]);
            UInt mipMapCount = *(UInt*)&(data[28]);

            Create(Vec2I(width, height), &data[128], TextureFormat::DXT5);

            delete[] data;
        }
    }

    void Texture::LoadPNG(const String& fileName)
    {
        Bitmap image;
        if (image.Load(fileName, Bitmap::ImageType::Auto))
        {
            mFileName = fileName;
            Create(image);
        }
    }

    void Texture::Reload()
    {
        if (!mFileName.IsEmpty())
            Create(mFileName);
    }

    Vec2I Texture::GetSize() const
    {
        return mSize;
    }

    TextureFormat Texture::GetFormat() const
    {
        return mFormat;
    }

    Texture::Usage Texture::GetUsage() const
    {
        return mUsage;
    }

    String Texture::GetFileName() const
    {
        return mFileName;
    }

    bool Texture::IsReady() const
    {
        return mReady;
    }

    bool Texture::IsAtlasPage() const
    {
        return mAtlasAssetId != UID::empty;
    }

    UID Texture::GetAtlasAssetId() const
    {
        return mAtlasAssetId;
    }

    int Texture::GetAtlasPage() const
    {
        return mAtlasPage;
    }
}
// --- META ---

ENUM_META(o2::Texture::Usage)
{
    ENUM_ENTRY(Default);
    ENUM_ENTRY(RenderTarget);
}
END_ENUM_META;

ENUM_META(o2::Texture::Filter)
{
    ENUM_ENTRY(Linear);
    ENUM_ENTRY(Nearest);
}
END_ENUM_META;
// --- END META ---
