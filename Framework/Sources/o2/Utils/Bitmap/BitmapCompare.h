#pragma once

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Math/Color.h"

namespace o2
{
    // -------------------------
    // Result of bitmaps compare
    // -------------------------
    struct BitmapCompareResult
    {
        bool  comparable = false;    // Are bitmaps RGBA8 of the same size
        float similarity = 0.0f;     // Part of pixels with difference within tolerance, 0..1
        float meanDifference = 0.0f; // Mean pixel difference, 0..255
        int   maxDifference = 0;     // Maximum pixel difference, 0..255
    };

    // Pixel comparison of straight alpha RGBA8 bitmaps over a background, by the maximum channel difference
    namespace BitmapCompare
    {
        // Compares bitmaps; fills difference with matched pixels in gray and mismatched in red when given
        BitmapCompareResult Compare(const Bitmap& a, const Bitmap& b, int tolerance = 8,
                                    const Color4& background = Color4::White(), Bitmap* difference = nullptr);

        // Returns part of pixels with difference within tolerance
        float GetSimilarity(const Bitmap& a, const Bitmap& b, int tolerance = 8,
                            const Color4& background = Color4::White());

        // Composites bitmap over opaque background; premultiplied tells that RGB is already multiplied by alpha
        void CompositeOver(Bitmap& bitmap, const Color4& background, bool premultiplied = false);
    }
}
