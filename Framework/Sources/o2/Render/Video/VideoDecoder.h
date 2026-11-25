#pragma once

#include "o2/Assets/AssetRef.h"
#include "o2/Assets/Types/VideoAsset.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    class Bitmap;
    class TextureRef;

    // ---------------------------------------------------------------------------
    // Video decoder backend interface. Advances through frames and converts the
    // last decoded frame into an RGBA bottom-up bitmap on demand. Implementations:
    // pl_mpeg software decoder (MPEG-1, all platforms) and platform hardware
    // decoders (AVFoundation/VideoToolbox on Mac and iOS)
    // ---------------------------------------------------------------------------
    class VideoDecoder: public RefCounterable
    {
    public:
        virtual ~VideoDecoder() {}

        // Opens the video from asset; returns false when the source can't be decoded
        virtual bool Open(const AssetRef<VideoAsset>& asset, bool streaming) = 0;

        // Returns frame size in pixels
        virtual Vec2I GetSize() const = 0;

        // Returns frames per second
        virtual float GetFrameRate() const = 0;

        // Returns duration in seconds
        virtual float GetDuration() const = 0;

        // Decodes the next frame, advancing the position; returns false at end of stream
        virtual bool DecodeNextFrame(float& outTime) = 0;

        // Seeks to time and decodes the frame there
        virtual bool SeekFrame(float time, float& outTime) = 0;

        // Converts the last decoded frame into the bitmap (frame size, RGBA, bottom-up rows)
        virtual bool ReadLastFrame(Bitmap& into) = 0;

        // Uploads the last decoded frame straight into the texture, bypassing the CPU bitmap;
        // returns false when the backend has no direct upload path (the caller falls back to
        // ReadLastFrame + Texture::SetData)
        virtual bool UploadLastFrame(const TextureRef& texture);
    };

    // Creates and opens a decoder for the asset by its extension: mp4/mov/m4v - platform hardware
    // decoder, everything else - pl_mpeg software decoder. Returns null when unsupported
    Ref<VideoDecoder> CreateVideoDecoder(const AssetRef<VideoAsset>& asset, bool streaming);

    // Parses video file info without playback; returns false when the format isn't supported
    bool ParseVideoFileFormatInfo(const String& path, Vec2I& size, float& frameRate, float& duration);

    // Platform hardware decoder factory; returns null when the platform has no hardware backend
    Ref<VideoDecoder> CreatePlatformVideoDecoder();

    // Platform container info parse for hardware-decoded formats
    bool PlatformParseVideoFormatInfo(const String& path, Vec2I& size, float& frameRate, float& duration);
}
