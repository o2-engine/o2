#include "o2Editor/stdafx.h"
#include "PipelineValue.h"

#include "o2/Utils/Bitmap/PngFormat.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor
{
    PipelineValue PipelineValue::Text(const String& text)
    {
        PipelineValue v;
        v.type = PipelinePortType::Text;
        v.data = text;
        v.mimeType = "text/plain";
        v.valid = true;
        return v;
    }

    PipelineValue PipelineValue::Image(const Ref<Bitmap>& bitmap)
    {
        PipelineValue v;
        v.type = PipelinePortType::Image;
        v.mimeType = "image/png";
        if (bitmap)
        {
            v.bitmap = EnsureRgba(bitmap);
            v.data = EncodeBitmapPng(*v.bitmap);
            v.valid = !v.data.IsEmpty();
        }
        return v;
    }

    PipelineValue PipelineValue::ImageBytes(const String& pngBytes, const String& mime /*= "image/png"*/)
    {
        PipelineValue v;
        v.type = PipelinePortType::Image;
        v.data = pngBytes;
        v.mimeType = mime;
        v.valid = !pngBytes.IsEmpty();
        return v;
    }

    PipelineValue PipelineValue::Bytes(PipelinePortType type, const String& bytes, const String& mime)
    {
        PipelineValue v;
        v.type = type;
        v.data = bytes;
        v.mimeType = mime;
        v.valid = true;
        return v;
    }

    Ref<Bitmap> PipelineValue::GetBitmap() const
    {
        if (type != PipelinePortType::Image || !valid)
            return nullptr;

        if (!bitmap)
            bitmap = DecodeImageBytes(data);

        return bitmap;
    }

    String PipelineValue::GetPngBytes() const
    {
        if (mimeType == "image/png")
            return data;

        auto bmp = GetBitmap();
        if (!bmp)
            return data;

        return EncodeBitmapPng(*bmp);
    }

    String PipelineValue::GetExtension() const
    {
        switch (type)
        {
            case PipelinePortType::Image: return "png";
            case PipelinePortType::Video: return "mp4";
            case PipelinePortType::Audio:
            {
                String ext = PipelineUtils::ExtensionForMime(mimeType);
                if (ext == "mp3" || ext == "wav" || ext == "ogg" || ext == "m4a" || ext == "flac")
                    return ext;
                return "mp3";
            }
            default: return "txt";
        }
    }

    String EncodeBitmapPng(const Bitmap& bitmap)
    {
        String out;
        if (!SavePngImageToMemory(&bitmap, out))
            return "";

        return out;
    }

    Ref<Bitmap> DecodeImageBytes(const String& bytes)
    {
        if (bytes.IsEmpty())
            return nullptr;

        auto bitmap = mmake<Bitmap>();
        if (!bitmap->LoadFromMemory((const UInt8*)bytes.Data(), (UInt)bytes.Length(), Bitmap::ImageType::Auto))
            return nullptr;

        return EnsureRgba(bitmap);
    }

    Ref<Bitmap> EnsureRgba(const Ref<Bitmap>& bitmap)
    {
        if (!bitmap)
            return nullptr;

        if (bitmap->GetFormat() == PixelFormat::R8G8B8A8)
            return bitmap;

        Vec2I size = bitmap->GetSize();
        auto res = mmake<Bitmap>(PixelFormat::R8G8B8A8, size);
        const UInt8* src = bitmap->GetData();
        UInt8* dst = res->GetData();
        int count = size.x * size.y;
        for (int i = 0; i < count; i++)
        {
            dst[i * 4] = src[i * 3];
            dst[i * 4 + 1] = src[i * 3 + 1];
            dst[i * 4 + 2] = src[i * 3 + 2];
            dst[i * 4 + 3] = 255;
        }

        return res;
    }
}
