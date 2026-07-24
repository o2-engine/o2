#include "o2/stdafx.h"
#include "VideoDecoder.h"

#include "o2/Render/TextureRef.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/FileSystem/FileSystem.h"

#include "pl_mpeg.h"

namespace o2
{
    bool VideoDecoder::UploadLastFrame(const TextureRef& texture)
    {
        return false;
    }

    // MPEG-1 software decoder over pl_mpeg; works on every platform
    class PlMpegVideoDecoder: public VideoDecoder
    {
    public:
        ~PlMpegVideoDecoder();

        bool Open(const AssetRef<VideoAsset>& asset, bool streaming) override;

        Vec2I GetSize() const override;
        float GetFrameRate() const override;
        float GetDuration() const override;

        bool DecodeNextFrame(float& outTime) override;
        bool SeekFrame(float time, float& outTime) override;
        bool ReadLastFrame(Bitmap& into) override;

    private:
        AssetRef<VideoAsset> mAsset; // Keeps encoded bytes alive for the memory mode

        plm_t*       mPlm = nullptr;
        plm_frame_t* mLastFrame = nullptr; // Valid until the next decode call
    };

    PlMpegVideoDecoder::~PlMpegVideoDecoder()
    {
        if (mPlm)
            plm_destroy(mPlm);
    }

    bool PlMpegVideoDecoder::Open(const AssetRef<VideoAsset>& asset, bool streaming)
    {
        mAsset = asset;

        if (streaming)
        {
            String path = asset->GetBuiltFullPath();
            if (!path.IsEmpty())
                mPlm = plm_create_with_filename(path.Data());
        }

        if (!mPlm && asset->GetData() && asset->GetDataSize() > 0)
            mPlm = plm_create_with_memory((uint8_t*)asset->GetData(), (size_t)asset->GetDataSize(), 0 /* asset owns the data */);

        if (!mPlm)
            return false;

        plm_set_audio_enabled(mPlm, 0);
        return true;
    }

    Vec2I PlMpegVideoDecoder::GetSize() const
    {
        return mPlm ? Vec2I(plm_get_width(mPlm), plm_get_height(mPlm)) : Vec2I();
    }

    float PlMpegVideoDecoder::GetFrameRate() const
    {
        return mPlm ? (float)plm_get_framerate(mPlm) : 0.0f;
    }

    float PlMpegVideoDecoder::GetDuration() const
    {
        return mPlm ? (float)plm_get_duration(mPlm) : 0.0f;
    }

    bool PlMpegVideoDecoder::DecodeNextFrame(float& outTime)
    {
        if (!mPlm)
            return false;

        plm_frame_t* frame = plm_decode_video(mPlm);
        if (!frame)
            return false;

        mLastFrame = frame;
        outTime = (float)frame->time;
        return true;
    }

    bool PlMpegVideoDecoder::SeekFrame(float time, float& outTime)
    {
        if (!mPlm)
            return false;

        plm_frame_t* frame = plm_seek_frame(mPlm, (double)Math::Max(time, 0.0f), 1);
        if (!frame)
            return false;

        mLastFrame = frame;
        outTime = (float)frame->time;
        return true;
    }

    bool PlMpegVideoDecoder::ReadLastFrame(Bitmap& into)
    {
        if (!mLastFrame)
            return false;

        Vec2I size = GetSize();

        // o2 bitmaps are bottom-up, pl_mpeg frames are top-down: flip via negative stride
        UInt8* dst = into.GetData() + (size_t)(size.y - 1)*size.x*4;
        plm_frame_to_rgba(mLastFrame, dst, -size.x*4);
        return true;
    }

    Ref<VideoDecoder> CreateVideoDecoder(const AssetRef<VideoAsset>& asset, bool streaming)
    {
        if (!asset)
            return nullptr;

        String ext = FileSystem::GetFileExtension(asset->GetPath());
        bool hardwareFormat = ext == "mp4" || ext == "mov" || ext == "m4v";

        Ref<VideoDecoder> decoder;
        if (hardwareFormat)
        {
            decoder = CreatePlatformVideoDecoder();
            if (!decoder)
            {
                o2Debug.LogError("No hardware video decoder backend on this platform for: " + asset->GetPath());
                return nullptr;
            }
        }
        else
            decoder = mmake<PlMpegVideoDecoder>();

        if (!decoder->Open(asset, streaming))
        {
            o2Debug.LogError("Failed to open video: " + asset->GetPath());
            return nullptr;
        }

        return decoder;
    }

    bool ParseVideoFileFormatInfo(const String& path, Vec2I& size, float& frameRate, float& duration)
    {
        String ext = FileSystem::GetFileExtension(path);
        if (ext == "mp4" || ext == "mov" || ext == "m4v")
            return PlatformParseVideoFormatInfo(path, size, frameRate, duration);

        plm_t* plm = plm_create_with_filename(path.Data());
        if (!plm)
            return false;

        plm_set_audio_enabled(plm, 0);
        size = Vec2I(plm_get_width(plm), plm_get_height(plm));
        frameRate = (float)plm_get_framerate(plm);
        duration = (float)plm_get_duration(plm);
        plm_destroy(plm);
        return true;
    }

// Platforms with a hardware backend implement these in their Render/<Platform>/VideoDecoderImpl;
// the rest (Linux) have no hardware video decoding - only .mpg through pl_mpeg
#if !defined(PLATFORM_MAC) && !defined(PLATFORM_IOS) && !defined(PLATFORM_WINDOWS) && \
    !defined(PLATFORM_ANDROID) && !defined(PLATFORM_WASM)
    Ref<VideoDecoder> CreatePlatformVideoDecoder()
    {
        return nullptr;
    }

    bool PlatformParseVideoFormatInfo(const String& path, Vec2I& size, float& frameRate, float& duration)
    {
        return false;
    }
#endif
}
