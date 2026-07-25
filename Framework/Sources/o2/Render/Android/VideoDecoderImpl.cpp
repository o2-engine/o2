#include "o2/stdafx.h"

#ifdef PLATFORM_ANDROID

#include <android/asset_manager.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaExtractor.h>
#include <media/NdkMediaFormat.h>
#include <unistd.h>

#include <cstring>

#include "o2/Application/Android/AndroidPlatform.h"
#include "o2/Render/VideoDecoder.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    // -----------------------------------------------------------------------------
    // Hardware video decoder over AMediaExtractor + AMediaCodec (NDK). The codec
    // decodes into YUV420 byte buffers (planar 19 or semi-planar 21); frames are
    // converted to RGBA bottom-up on the CPU.
    // -----------------------------------------------------------------------------
    class AndroidVideoDecoder: public VideoDecoder
    {
    public:
        ~AndroidVideoDecoder();

        bool Open(const AssetRef<VideoAsset>& asset, bool streaming) override;

        Vec2I GetSize() const override;
        float GetFrameRate() const override;
        float GetDuration() const override;

        bool DecodeNextFrame(float& outTime) override;
        bool SeekFrame(float time, float& outTime) override;
        bool ReadLastFrame(Bitmap& into) override;

        // Opens the decoder over a file path; used by Open and the format info parse
        bool OpenFile(const String& path);

    private:
        AMediaExtractor* mExtractor = nullptr;
        AMediaCodec*     mCodec = nullptr;

        Vec2I mSize;
        float mFrameRate = 0.0f;
        float mDuration = 0.0f;

        int mColorFormat = 0; // 19 - planar I420, 21 - semi-planar NV12
        int mStride = 0;      // Y plane row stride in bytes
        int mSliceHeight = 0; // Y plane rows before the chroma planes

        ssize_t               mLastOutputIndex = -1;
        AMediaCodecBufferInfo mLastOutputInfo;

        bool mInputEnded = false;

        void ReleaseLastOutput();
        void ReadOutputFormat();
    };

    AndroidVideoDecoder::~AndroidVideoDecoder()
    {
        ReleaseLastOutput();

        if (mCodec)
        {
            AMediaCodec_stop(mCodec);
            AMediaCodec_delete(mCodec);
        }

        if (mExtractor)
            AMediaExtractor_delete(mExtractor);
    }

    bool AndroidVideoDecoder::Open(const AssetRef<VideoAsset>& asset, bool streaming)
    {
        return OpenFile(asset->GetBuiltFullPath());
    }

    bool AndroidVideoDecoder::OpenFile(const String& path)
    {
        if (path.IsEmpty())
            return false;

        mExtractor = AMediaExtractor_new();

        // APK assets live inside the zip: open through the asset manager and hand the
        // extractor a file descriptor with the asset's offset/length. Media extensions
        // are stored uncompressed by the APK packer, so the descriptor is seekable
        media_status_t status = AMEDIA_ERROR_BASE;
        if (AAssetManager* assetManager = AndroidPlatform::GetAssetManager())
        {
            if (AAsset* asset = AAssetManager_open(assetManager, path.Data(), AASSET_MODE_RANDOM))
            {
                off_t offset = 0, length = 0;
                int fd = AAsset_openFileDescriptor(asset, &offset, &length);
                if (fd >= 0)
                {
                    status = AMediaExtractor_setDataSourceFd(mExtractor, fd, offset, length);
                    close(fd); // the extractor dups the descriptor
                }

                AAsset_close(asset);
            }
        }

        if (status != AMEDIA_OK)
            status = AMediaExtractor_setDataSource(mExtractor, path.Data()); // plain file fallback

        if (status != AMEDIA_OK)
            return false;

        size_t trackCount = AMediaExtractor_getTrackCount(mExtractor);
        for (size_t i = 0; i < trackCount; i++)
        {
            AMediaFormat* format = AMediaExtractor_getTrackFormat(mExtractor, i);

            const char* mime = nullptr;
            if (AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime) && mime &&
                strncmp(mime, "video/", 6) == 0)
            {
                int32_t width = 0, height = 0, frameRate = 0;
                int64_t durationUs = 0;
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &width);
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &height);
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_FRAME_RATE, &frameRate);
                AMediaFormat_getInt64(format, AMEDIAFORMAT_KEY_DURATION, &durationUs);

                mSize = Vec2I(width, height);
                mFrameRate = (float)frameRate;
                mDuration = (float)((double)durationUs/1000000.0);
                mStride = width;
                mSliceHeight = height;

                AMediaExtractor_selectTrack(mExtractor, i);

                mCodec = AMediaCodec_createDecoderByType(mime);
                if (!mCodec ||
                    AMediaCodec_configure(mCodec, format, nullptr, nullptr, 0) != AMEDIA_OK ||
                    AMediaCodec_start(mCodec) != AMEDIA_OK)
                {
                    AMediaFormat_delete(format);
                    return false;
                }

                AMediaFormat_delete(format);
                return mSize.x > 0 && mSize.y > 0;
            }

            AMediaFormat_delete(format);
        }

        return false;
    }

    Vec2I AndroidVideoDecoder::GetSize() const
    {
        return mSize;
    }

    float AndroidVideoDecoder::GetFrameRate() const
    {
        return mFrameRate;
    }

    float AndroidVideoDecoder::GetDuration() const
    {
        return mDuration;
    }

    void AndroidVideoDecoder::ReleaseLastOutput()
    {
        if (mLastOutputIndex >= 0)
        {
            AMediaCodec_releaseOutputBuffer(mCodec, (size_t)mLastOutputIndex, false);
            mLastOutputIndex = -1;
        }
    }

    void AndroidVideoDecoder::ReadOutputFormat()
    {
        AMediaFormat* format = AMediaCodec_getOutputFormat(mCodec);
        if (!format)
            return;

        int32_t value = 0;
        if (AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, &value))
            mColorFormat = value;
        if (AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_STRIDE, &value))
            mStride = value;
        if (AMediaFormat_getInt32(format, "slice-height", &value))
            mSliceHeight = value;

        AMediaFormat_delete(format);
    }

    bool AndroidVideoDecoder::DecodeNextFrame(float& outTime)
    {
        if (!mCodec)
            return false;

        ReleaseLastOutput();

        int guard = 0;
        while (guard++ < 1024)
        {
            if (!mInputEnded)
            {
                ssize_t inputIndex = AMediaCodec_dequeueInputBuffer(mCodec, 2000);
                if (inputIndex >= 0)
                {
                    size_t capacity = 0;
                    uint8_t* inputBuffer = AMediaCodec_getInputBuffer(mCodec, (size_t)inputIndex, &capacity);
                    ssize_t sampleSize = AMediaExtractor_readSampleData(mExtractor, inputBuffer, capacity);

                    if (sampleSize < 0)
                    {
                        AMediaCodec_queueInputBuffer(mCodec, (size_t)inputIndex, 0, 0, 0,
                                                     AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
                        mInputEnded = true;
                    }
                    else
                    {
                        AMediaCodec_queueInputBuffer(mCodec, (size_t)inputIndex, 0, (size_t)sampleSize,
                                                     AMediaExtractor_getSampleTime(mExtractor), 0);
                        AMediaExtractor_advance(mExtractor);
                    }
                }
            }

            AMediaCodecBufferInfo info;
            ssize_t outputIndex = AMediaCodec_dequeueOutputBuffer(mCodec, &info, 10000);

            if (outputIndex >= 0)
            {
                if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM)
                {
                    AMediaCodec_releaseOutputBuffer(mCodec, (size_t)outputIndex, false);
                    return false;
                }

                mLastOutputIndex = outputIndex;
                mLastOutputInfo = info;
                outTime = (float)((double)info.presentationTimeUs/1000000.0);
                return true;
            }

            if (outputIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
                ReadOutputFormat();
        }

        return false;
    }

    bool AndroidVideoDecoder::SeekFrame(float time, float& outTime)
    {
        if (!mCodec || !mExtractor)
            return false;

        ReleaseLastOutput();
        AMediaExtractor_seekTo(mExtractor, (int64_t)((double)Math::Max(time, 0.0f)*1000000.0),
                               AMEDIAEXTRACTOR_SEEK_PREVIOUS_SYNC);
        AMediaCodec_flush(mCodec);
        mInputEnded = false;

        // Decode from the key frame up to the requested time
        float halfFrame = mFrameRate > 0.0f ? 0.5f/mFrameRate : 0.0f;
        int guard = 0;
        while (guard++ < 1024)
        {
            if (!DecodeNextFrame(outTime))
                return false;

            if (outTime + halfFrame >= time)
                return true;
        }

        return false;
    }

    bool AndroidVideoDecoder::ReadLastFrame(Bitmap& into)
    {
        if (mLastOutputIndex < 0)
            return false;

        size_t bufferSize = 0;
        uint8_t* buffer = AMediaCodec_getOutputBuffer(mCodec, (size_t)mLastOutputIndex, &bufferSize);
        if (!buffer)
            return false;

        // Planar I420 (19) and semi-planar NV12 (21) layouts; anything else needs the
        // Surface/ImageReader path and isn't supported by the byte-buffer decoder
        if (mColorFormat != 19 && mColorFormat != 21 && mColorFormat != 0)
        {
            o2Debug.LogError("Unsupported video decoder color format: " + (String)mColorFormat);
            return false;
        }

        int w = mSize.x, h = mSize.y;
        int stride = mStride > 0 ? mStride : w;
        int sliceHeight = mSliceHeight > 0 ? mSliceHeight : h;

        const uint8_t* yPlane = buffer + mLastOutputInfo.offset;
        const uint8_t* uPlane;
        const uint8_t* vPlane;
        int chromaStride, chromaStep;

        if (mColorFormat == 19) // planar I420
        {
            uPlane = yPlane + (size_t)stride*sliceHeight;
            vPlane = uPlane + (size_t)(stride/2)*(sliceHeight/2);
            chromaStride = stride/2;
            chromaStep = 1;
        }
        else // semi-planar NV12 (default assumption)
        {
            uPlane = yPlane + (size_t)stride*sliceHeight;
            vPlane = uPlane + 1;
            chromaStride = stride;
            chromaStep = 2;
        }

        UInt8* dstData = into.GetData();

        // BT.601 integer conversion; o2 bitmaps are bottom-up
        for (int y = 0; y < h; y++)
        {
            const uint8_t* yRow = yPlane + (size_t)y*stride;
            const uint8_t* uRow = uPlane + (size_t)(y/2)*chromaStride;
            const uint8_t* vRow = vPlane + (size_t)(y/2)*chromaStride;
            UInt8* dstRow = dstData + (size_t)(h - 1 - y)*w*4;

            for (int x = 0; x < w; x++)
            {
                int yv = ((int)yRow[x] - 16)*76309;
                int cb = (int)uRow[(x/2)*chromaStep] - 128;
                int cr = (int)vRow[(x/2)*chromaStep] - 128;

                int r = (yv + cr*104597) >> 16;
                int g = (yv - cb*25674 - cr*53278) >> 16;
                int b = (yv + cb*132201) >> 16;

                dstRow[x*4 + 0] = (UInt8)Math::Clamp(r, 0, 255);
                dstRow[x*4 + 1] = (UInt8)Math::Clamp(g, 0, 255);
                dstRow[x*4 + 2] = (UInt8)Math::Clamp(b, 0, 255);
                dstRow[x*4 + 3] = 255;
            }
        }

        return true;
    }

    Ref<VideoDecoder> CreatePlatformVideoDecoder()
    {
        return mmake<AndroidVideoDecoder>();
    }

    bool PlatformParseVideoFormatInfo(const String& path, Vec2I& size, float& frameRate, float& duration)
    {
        AndroidVideoDecoder decoder;
        if (!decoder.OpenFile(path))
            return false;

        size = decoder.GetSize();
        frameRate = decoder.GetFrameRate();
        duration = decoder.GetDuration();
        return true;
    }
}

#endif // PLATFORM_ANDROID
