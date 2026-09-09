#pragma once

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"
#include "o2Editor/Pipeline/PipelineGraph.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------------------------
    // Value travelling over a pipeline edge: text, or encoded media bytes. Images
    // are kept as PNG bytes and decoded to a bitmap on demand
    // ---------------------------------------------------------------------------
    struct PipelineValue
    {
        PipelinePortType type = PipelinePortType::Text; // Kind of data the value holds
        String           data;                          // Text, or encoded bytes (png / mp4 / mp3 / wav / ogg)
        String           mimeType;                      // Mime of data for media values
        bool             valid = false;                 // False for an empty or failed value

        mutable Ref<Bitmap> bitmap; // Decoded image cache

    public:
        // Default constructor, makes an invalid value
        PipelineValue() = default;

        // Returns a valid text value
        static PipelineValue Text(const String& text);

        // Returns an image value with the bitmap encoded into PNG bytes, invalid for null or failed encoding
        static PipelineValue Image(const Ref<Bitmap>& bitmap);

        // Returns an image value from already encoded bytes, invalid when they are empty
        static PipelineValue ImageBytes(const String& pngBytes, const String& mime = "image/png");

        // Returns a valid value of the type from encoded bytes
        static PipelineValue Bytes(PipelinePortType type, const String& bytes, const String& mime);

        // Returns true when the value holds data
        bool IsValid() const { return valid; }

        // Returns true for a valid image value
        bool IsImage() const { return valid && type == PipelinePortType::Image; }

        // Returns true for a valid text value
        bool IsText() const { return valid && type == PipelinePortType::Text; }

        // Returns true for a valid audio value
        bool IsAudio() const { return valid && type == PipelinePortType::Audio; }

        // Returns true for a valid video value
        bool IsVideo() const { return valid && type == PipelinePortType::Video; }

        // Decodes (once) and returns the RGBA bitmap; null for non-images or broken data
        Ref<Bitmap> GetBitmap() const;

        // Returns the encoded bytes as PNG, re-encoding other image formats
        String GetPngBytes() const;

        // Returns file extension for storing this value
        String GetExtension() const;
    };

    // Encodes a bitmap into PNG bytes, empty on failure
    String EncodeBitmapPng(const Bitmap& bitmap);

    // Decodes image bytes into an RGBA bitmap, null on failure
    Ref<Bitmap> DecodeImageBytes(const String& bytes);

    // Returns a copy converted to RGBA8 when needed
    Ref<Bitmap> EnsureRgba(const Ref<Bitmap>& bitmap);
}
