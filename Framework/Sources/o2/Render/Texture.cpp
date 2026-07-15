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
        { TextureFormat::DXT1, "dds" },
        { TextureFormat::DXT5, "dds" },
        { TextureFormat::BC7, "dds" },
        { TextureFormat::ASTC4x4, "astc" }
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

    bool Texture::IsFormatCompressed(TextureFormat format)
    {
        return format == TextureFormat::DXT1 || format == TextureFormat::DXT5 ||
               format == TextureFormat::BC7 || format == TextureFormat::ASTC4x4;
    }

    int Texture::FormatBlockSize(TextureFormat format)
    {
        return format == TextureFormat::DXT1 ? 8 : 16;
    }

    TextureFormat Texture::FormatOfCompression(TextureCompression compression)
    {
        switch (compression)
        {
            case TextureCompression::DXT1: return TextureFormat::DXT1;
            case TextureCompression::DXT5: return TextureFormat::DXT5;
            case TextureCompression::BC7: return TextureFormat::BC7;
            case TextureCompression::ASTC4x4: return TextureFormat::ASTC4x4;
            default: return TextureFormat::R8G8B8A8;
        }
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

    void Texture::SetWrap(Wrap wrap)
    {
        mWrap = wrap;
        PlatformSetWrap();
    }

    Texture::Wrap Texture::GetWrap() const
    {
        return mWrap;
    }

    void Texture::Create(const String& fileName)
    {
        String extension = o2FileSystem.GetFileExtension(fileName);

        if (extension == "png")
            LoadPNG(fileName);
        else if (extension == "dds")
            LoadDDS(fileName);
        else if (extension == "astc")
            LoadASTC(fileName);
        else
            o2Render.mLog->Error("Failed to load texture from file " + fileName);

#if IS_EDITOR
        if (mReady && !mFileName.IsEmpty())
            mFileEditDate = o2FileSystem.GetFileInfo(mFileName).editDate;
#endif
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
            UInt fourCC = *(UInt*)&(data[84]);

            TextureFormat format = TextureFormat::DXT5;
            UInt dataOffset = 128;

            if (fourCC == 0x31545844) // "DXT1"
                format = TextureFormat::DXT1;
            else if (fourCC == 0x35545844) // "DXT5"
                format = TextureFormat::DXT5;
            else if (fourCC == 0x30315844) // "DX10": the DXGI format lives in the extended header
            {
                UInt dxgiFormat = *(UInt*)&(data[128]);
                dataOffset = 148;

                if (dxgiFormat == 98) // DXGI_FORMAT_BC7_UNORM
                    format = TextureFormat::BC7;
                else
                {
                    o2Render.mLog->Error("Unsupported DXGI format " + (String)(int)dxgiFormat +
                                         " in DDS file " + fileName);
                    delete[] data;
                    return;
                }
            }
            else
                o2Render.mLog->Error("Unknown fourCC in DDS file " + fileName + ", assuming DXT5");

            Create(Vec2I(width, height), &data[dataOffset], format);

            delete[] data;
        }
    }

    void Texture::LoadASTC(const String& fileName)
    {
        mFileName = fileName;

        InFile file(fileName);
        if (!file.IsOpened())
            return;

        UInt dataSize = file.GetDataSize();
        auto data = mnew Byte[dataSize];
        file.ReadData(data, dataSize);
        file.Close();

        auto readUInt24 = [&](int offset) {
            return (UInt)(UInt8)data[offset] | ((UInt)(UInt8)data[offset + 1] << 8) |
                   ((UInt)(UInt8)data[offset + 2] << 16);
        };

        bool validMagic = (UInt8)data[0] == 0x13 && (UInt8)data[1] == 0xAB &&
                          (UInt8)data[2] == 0xA1 && (UInt8)data[3] == 0x5C;
        bool block4x4 = (UInt8)data[4] == 4 && (UInt8)data[5] == 4 && (UInt8)data[6] == 1;

        if (!validMagic || !block4x4)
        {
            o2Render.mLog->Error("Unsupported ASTC file (only 4x4 LDR blocks): " + fileName);
            delete[] data;
            return;
        }

        UInt width = readUInt24(7);
        UInt height = readUInt24(10);

        Create(Vec2I(width, height), &data[16], TextureFormat::ASTC4x4);

        delete[] data;
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
        // Atlas pages resolve their file name freshly: the extension follows the atlas
        // compression setting and may have changed since the last load (png <-> dds/astc)
        if (mAtlasPage >= 0 && mAtlasAssetId != UID::empty)
        {
            Create(mAtlasAssetId, mAtlasPage);
            o2Render.mLog->Out("Reloaded atlas page texture " + mFileName);
            return;
        }

        if (!mFileName.IsEmpty())
        {
            Create(mFileName);
            o2Render.mLog->Out("Reloaded texture " + mFileName);
        }
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

#if IS_EDITOR
    const TimeStamp& Texture::GetFileEditDate() const
    {
        return mFileEditDate;
    }

    void Texture::SetFileEditDate(const TimeStamp& date)
    {
        mFileEditDate = date;
    }
#endif
}
// --- META ---

ENUM_META(o2::Texture::Usage, o2__Texture__Usage)
{
    ENUM_ENTRY(Default);
    ENUM_ENTRY(RenderTarget);
}
END_ENUM_META;

ENUM_META(o2::Texture::Filter, o2__Texture__Filter)
{
    ENUM_ENTRY(Linear);
    ENUM_ENTRY(Nearest);
}
END_ENUM_META;

ENUM_META(o2::Texture::Wrap, o2__Texture__Wrap)
{
    ENUM_ENTRY(ClampToEdge);
    ENUM_ENTRY(Repeat);
}
END_ENUM_META;
// --- END META ---
