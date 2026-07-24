#include "o2/stdafx.h"

#ifdef PLATFORM_WINDOWS

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <propvarutil.h>

#include "o2/Render/VideoDecoder.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Debug/Debug.h"

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")

namespace o2
{
    // -----------------------------------------------------------------------------
    // Hardware video decoder over Media Foundation IMFSourceReader (DXVA underneath
    // when available). Output is converted by the reader to RGB32 (BGRX); frames are
    // swizzled to RGBA and flipped to bottom-up rows.
    // -----------------------------------------------------------------------------
    class WindowsVideoDecoder: public VideoDecoder
    {
    public:
        ~WindowsVideoDecoder();

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
        IMFSourceReader* mReader = nullptr;
        IMFSample*       mLastSample = nullptr; // owned, released on replace

        Vec2I mSize;
        float mFrameRate = 0.0f;
        float mDuration = 0.0f;
        LONG  mStride = 0; // Positive - top-down rows, negative - bottom-up

        static bool EnsureMediaFoundation();
    };

    bool WindowsVideoDecoder::EnsureMediaFoundation()
    {
        static bool initialized = false;
        static bool succeeded = false;
        if (initialized)
            return succeeded;

        initialized = true;

        HRESULT co = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(co) && co != RPC_E_CHANGED_MODE)
            return false;

        succeeded = SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_LITE));
        return succeeded;
    }

    WindowsVideoDecoder::~WindowsVideoDecoder()
    {
        if (mLastSample)
            mLastSample->Release();

        if (mReader)
            mReader->Release();
    }

    bool WindowsVideoDecoder::Open(const AssetRef<VideoAsset>& asset, bool streaming)
    {
        return OpenFile(asset->GetBuiltFullPath());
    }

    bool WindowsVideoDecoder::OpenFile(const String& path)
    {
        if (!EnsureMediaFoundation() || path.IsEmpty())
            return false;

        int wideLength = MultiByteToWideChar(CP_UTF8, 0, path.Data(), -1, nullptr, 0);
        Vector<wchar_t> widePath;
        widePath.Resize(wideLength);
        MultiByteToWideChar(CP_UTF8, 0, path.Data(), -1, widePath.Data(), wideLength);

        IMFAttributes* attributes = nullptr;
        MFCreateAttributes(&attributes, 2);
        if (attributes)
        {
            attributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
            attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
        }

        HRESULT hr = MFCreateSourceReaderFromURL(widePath.Data(), attributes, &mReader);

        if (attributes)
            attributes->Release();

        if (FAILED(hr) || !mReader)
            return false;

        mReader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
        mReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);

        IMFMediaType* outputType = nullptr;
        MFCreateMediaType(&outputType);
        outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        hr = mReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputType);
        outputType->Release();

        if (FAILED(hr))
            return false;

        IMFMediaType* currentType = nullptr;
        if (FAILED(mReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &currentType)))
            return false;

        UINT32 width = 0, height = 0;
        MFGetAttributeSize(currentType, MF_MT_FRAME_SIZE, &width, &height);
        mSize = Vec2I((int)width, (int)height);

        UINT32 fpsNum = 0, fpsDen = 0;
        if (SUCCEEDED(MFGetAttributeRatio(currentType, MF_MT_FRAME_RATE, &fpsNum, &fpsDen)) && fpsDen != 0)
            mFrameRate = (float)fpsNum/(float)fpsDen;

        UINT32 strideValue = MFGetAttributeUINT32(currentType, MF_MT_DEFAULT_STRIDE, 0);
        mStride = (LONG)strideValue;
        if (mStride == 0)
        {
            LONG stride = 0;
            if (SUCCEEDED(MFGetStrideForBitmapInfoHeader(MFVideoFormat_RGB32.Data1, width, &stride)))
                mStride = stride;
            else
                mStride = (LONG)width*4;
        }

        currentType->Release();

        PROPVARIANT durationVar;
        PropVariantInit(&durationVar);
        if (SUCCEEDED(mReader->GetPresentationAttribute((DWORD)MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &durationVar)))
            mDuration = (float)((double)durationVar.uhVal.QuadPart/10000000.0);
        PropVariantClear(&durationVar);

        return mSize.x > 0 && mSize.y > 0;
    }

    Vec2I WindowsVideoDecoder::GetSize() const
    {
        return mSize;
    }

    float WindowsVideoDecoder::GetFrameRate() const
    {
        return mFrameRate;
    }

    float WindowsVideoDecoder::GetDuration() const
    {
        return mDuration;
    }

    bool WindowsVideoDecoder::DecodeNextFrame(float& outTime)
    {
        if (!mReader)
            return false;

        DWORD flags = 0;
        LONGLONG timestamp = 0;
        IMFSample* sample = nullptr;
        HRESULT hr = mReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr,
                                         &flags, &timestamp, &sample);

        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM) || !sample)
        {
            if (sample)
                sample->Release();

            return false;
        }

        if (mLastSample)
            mLastSample->Release();

        mLastSample = sample;
        outTime = (float)((double)timestamp/10000000.0);
        return true;
    }

    bool WindowsVideoDecoder::SeekFrame(float time, float& outTime)
    {
        if (!mReader)
            return false;

        PROPVARIANT positionVar;
        InitPropVariantFromInt64((LONGLONG)((double)Math::Max(time, 0.0f)*10000000.0), &positionVar);
        HRESULT hr = mReader->SetCurrentPosition(GUID_NULL, positionVar);
        PropVariantClear(&positionVar);

        if (FAILED(hr))
            return false;

        // The reader resumes from the previous key frame: decode up to the requested time
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

    bool WindowsVideoDecoder::ReadLastFrame(Bitmap& into)
    {
        if (!mLastSample)
            return false;

        IMFMediaBuffer* buffer = nullptr;
        if (FAILED(mLastSample->ConvertToContiguousBuffer(&buffer)) || !buffer)
            return false;

        BYTE* data = nullptr;
        DWORD maxLength = 0, currentLength = 0;
        if (FAILED(buffer->Lock(&data, &maxLength, &currentLength)))
        {
            buffer->Release();
            return false;
        }

        int w = mSize.x, h = mSize.y;
        LONG absStride = mStride > 0 ? mStride : -mStride;
        UInt8* dstData = into.GetData();

        // RGB32 is B,G,R,X; o2 bitmaps are bottom-up. Negative stride means the buffer
        // already stores rows bottom-up
        for (int y = 0; y < h; y++)
        {
            const BYTE* srcRow = mStride > 0 ? data + (size_t)y*absStride : data + (size_t)(h - 1 - y)*absStride;
            UInt8* dstRow = dstData + (size_t)(h - 1 - y)*w*4;

            for (int x = 0; x < w; x++)
            {
                dstRow[x*4 + 0] = srcRow[x*4 + 2];
                dstRow[x*4 + 1] = srcRow[x*4 + 1];
                dstRow[x*4 + 2] = srcRow[x*4 + 0];
                dstRow[x*4 + 3] = 255;
            }
        }

        buffer->Unlock();
        buffer->Release();
        return true;
    }

    Ref<VideoDecoder> CreatePlatformVideoDecoder()
    {
        return mmake<WindowsVideoDecoder>();
    }

    bool PlatformParseVideoFormatInfo(const String& path, Vec2I& size, float& frameRate, float& duration)
    {
        WindowsVideoDecoder decoder;
        if (!decoder.OpenFile(path))
            return false;

        size = decoder.GetSize();
        frameRate = decoder.GetFrameRate();
        duration = decoder.GetDuration();
        return true;
    }
}

#endif // PLATFORM_WINDOWS
