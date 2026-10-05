#pragma once

#include "o2/Render/VectorGraphics/VectorImage.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // -------------------
    // SVG reading options
    // -------------------
    struct SvgParseOptions
    {
        bool unitsAsPixels = false; // Reads pt, pc, mm, cm and in as user units, for the files where 1 pt is a pixel
    };

    // Reader of the simple SVG subset into vector image; all that is skipped gets into warnings
    namespace SvgParser
    {
        // Parses SVG text; returns false and fills error when text is not XML or has no svg root
        bool Parse(const char* text, UInt size, VectorImage& image, String& error, Vector<String>& warnings,
                   const SvgParseOptions& options = SvgParseOptions());

        bool Parse(const String& text, VectorImage& image, String& error, Vector<String>& warnings,
                   const SvgParseOptions& options = SvgParseOptions());

        bool Parse(const String& text, VectorImage& image);

        // Parses path data of the d attribute into sub paths; returns false when data is cut at a syntax error
        bool ParsePathData(const String& data, Vector<VectorSubPath>& subPaths);

        // Parses color: #rgb, #rgba, #rrggbb, #rrggbbaa, rgb(), rgba(), hsl(), hsla(), named
        bool ParseColor(const String& text, Color4& color);

        // Parses transform list: matrix, translate, scale, rotate, skewX, skewY
        bool ParseTransform(const String& text, Basis& transform);
    }
}
