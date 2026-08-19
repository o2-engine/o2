#include "o2/stdafx.h"

#if defined(PLATFORM_MAC) || defined(PLATFORM_IOS)

#import <AVFoundation/AVFoundation.h>
#import <Accelerate/Accelerate.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>

#include <vector>

#include "o2/Render/VideoDecoder.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    // -----------------------------------------------------------------------------
    // Hardware video decoder over AVAssetReader (VideoToolbox underneath). Decodes
    // H.264/HEVC containers from the built asset file. The reader outputs BGRA;
    // frames are permuted to RGBA with vImage and flipped to bottom-up rows.
    // Built without ARC: AVFoundation objects are retained/released manually.
    // -----------------------------------------------------------------------------
    class AppleVideoDecoder: public VideoDecoder
    {
    public:
        ~AppleVideoDecoder();

        bool Open(const AssetRef<VideoAsset>& asset, bool streaming) override;

        Vec2I GetSize() const override;
        float GetFrameRate() const override;
        float GetDuration() const override;

        bool DecodeNextFrame(float& outTime) override;
        bool SeekFrame(float time, float& outTime) override;
        bool ReadLastFrame(Bitmap& into) override;

    private:
        AVURLAsset*   mAsset = nil; // retained
        AVAssetTrack* mTrack = nil; // retained

        AVAssetReader*            mReader = nil; // retained
        AVAssetReaderTrackOutput* mOutput = nil; // retained

        CMSampleBufferRef mLastSample = nullptr; // owned, released on replace

        Vec2I mSize;
        float mFrameRate = 0.0f;
        float mDuration = 0.0f;

        std::vector<UInt8> mRowTemp; // Row buffer for the vertical flip

        bool RecreateReader(float startTime);
        void ReleaseReader();
    };

    AppleVideoDecoder::~AppleVideoDecoder()
    {
        ReleaseReader();

        [mTrack release];
        [mAsset release];
    }

    bool AppleVideoDecoder::Open(const AssetRef<VideoAsset>& asset, bool streaming)
    {
        @autoreleasepool
        {
            String path = asset->GetBuiltFullPath();
            if (path.IsEmpty())
                return false;

            NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.Data()]];
            mAsset = [[AVURLAsset URLAssetWithURL:url options:nil] retain];

            mTrack = [[[mAsset tracksWithMediaType:AVMediaTypeVideo] firstObject] retain];
            if (!mTrack)
                return false;

            CGSize naturalSize = mTrack.naturalSize;
            mSize = Vec2I((int)naturalSize.width, (int)naturalSize.height);
            mFrameRate = mTrack.nominalFrameRate;
            mDuration = (float)CMTimeGetSeconds(mAsset.duration);

            return RecreateReader(0.0f);
        }
    }

    Vec2I AppleVideoDecoder::GetSize() const
    {
        return mSize;
    }

    float AppleVideoDecoder::GetFrameRate() const
    {
        return mFrameRate;
    }

    float AppleVideoDecoder::GetDuration() const
    {
        return mDuration;
    }

    bool AppleVideoDecoder::RecreateReader(float startTime)
    {
        @autoreleasepool
        {
            ReleaseReader();

            NSError* error = nil;
            mReader = [[AVAssetReader alloc] initWithAsset:mAsset error:&error];
            if (!mReader)
            {
                o2Debug.LogError(String("Failed to create AVAssetReader: ") +
                                 (error ? error.localizedDescription.UTF8String : "unknown"));
                return false;
            }

            NSDictionary* settings = @{ (id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA) };
            mOutput = [[AVAssetReaderTrackOutput alloc] initWithTrack:mTrack outputSettings:settings];
            mOutput.alwaysCopiesSampleData = NO;

            if (![mReader canAddOutput:mOutput])
            {
                ReleaseReader();
                return false;
            }

            [mReader addOutput:mOutput];

            if (startTime > 0.0f)
            {
                // Bias half a frame back so the frame covering the requested time is included
                float bias = mFrameRate > 0.0f ? 0.5f/mFrameRate : 0.0f;
                CMTime start = CMTimeMakeWithSeconds(Math::Max(startTime - bias, 0.0f), 600);
                mReader.timeRange = CMTimeRangeMake(start, kCMTimePositiveInfinity);
            }

            if (![mReader startReading])
            {
                o2Debug.LogError(String("AVAssetReader failed to start: ") +
                                 (mReader.error ? mReader.error.localizedDescription.UTF8String : "unknown"));
                ReleaseReader();
                return false;
            }

            return true;
        }
    }

    void AppleVideoDecoder::ReleaseReader()
    {
        if (mLastSample)
        {
            CFRelease(mLastSample);
            mLastSample = nullptr;
        }

        if (mReader)
            [mReader cancelReading];

        [mOutput release];
        mOutput = nil;

        [mReader release];
        mReader = nil;
    }

    bool AppleVideoDecoder::DecodeNextFrame(float& outTime)
    {
        if (!mOutput)
            return false;

        @autoreleasepool
        {
            CMSampleBufferRef sample = [mOutput copyNextSampleBuffer];
            if (!sample)
                return false;

            if (mLastSample)
                CFRelease(mLastSample);

            mLastSample = sample;
            outTime = (float)CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(sample));
            return true;
        }
    }

    bool AppleVideoDecoder::SeekFrame(float time, float& outTime)
    {
        if (!RecreateReader(Math::Max(time, 0.0f)))
            return false;

        return DecodeNextFrame(outTime);
    }

    bool AppleVideoDecoder::ReadLastFrame(Bitmap& into)
    {
        if (!mLastSample)
            return false;

        CVImageBufferRef image = CMSampleBufferGetImageBuffer(mLastSample);
        if (!image)
            return false;

        int w = mSize.x, h = mSize.y;

        CVPixelBufferLockBaseAddress(image, kCVPixelBufferLock_ReadOnly);

        vImage_Buffer src;
        src.data = CVPixelBufferGetBaseAddress(image);
        src.rowBytes = CVPixelBufferGetBytesPerRow(image);
        src.width = (vImagePixelCount)w;
        src.height = (vImagePixelCount)h;

        vImage_Buffer dst;
        dst.data = into.GetData();
        dst.rowBytes = (size_t)w*4;
        dst.width = (vImagePixelCount)w;
        dst.height = (vImagePixelCount)h;

        const uint8_t bgraToRgba[4] = { 2, 1, 0, 3 };
        vImagePermuteChannels_ARGB8888(&src, &dst, bgraToRgba, kvImageNoFlags);

        CVPixelBufferUnlockBaseAddress(image, kCVPixelBufferLock_ReadOnly);

        // o2 bitmaps are bottom-up, decoded frames are top-down: flip rows in place
        mRowTemp.resize((size_t)w*4);
        UInt8* data = into.GetData();
        for (int y = 0; y < h/2; y++)
        {
            UInt8* a = data + (size_t)y*w*4;
            UInt8* b = data + (size_t)(h - 1 - y)*w*4;
            memcpy(mRowTemp.data(), a, (size_t)w*4);
            memcpy(a, b, (size_t)w*4);
            memcpy(b, mRowTemp.data(), (size_t)w*4);
        }

        return true;
    }

    Ref<VideoDecoder> CreatePlatformVideoDecoder()
    {
        return mmake<AppleVideoDecoder>();
    }

    bool PlatformParseVideoFormatInfo(const String& path, Vec2I& size, float& frameRate, float& duration)
    {
        @autoreleasepool
        {
            NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.Data()]];
            AVURLAsset* asset = [AVURLAsset URLAssetWithURL:url options:nil];

            AVAssetTrack* track = [[asset tracksWithMediaType:AVMediaTypeVideo] firstObject];
            if (!track)
                return false;

            CGSize naturalSize = track.naturalSize;
            size = Vec2I((int)naturalSize.width, (int)naturalSize.height);
            frameRate = track.nominalFrameRate;
            duration = (float)CMTimeGetSeconds(asset.duration);
            return true;
        }
    }
}

#endif // PLATFORM_MAC || PLATFORM_IOS
