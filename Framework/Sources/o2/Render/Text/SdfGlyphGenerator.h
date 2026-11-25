#pragma once

#include "ft2build.h"
#include FT_FREETYPE_H

#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // -------------------------------------------------------------------------------------------------------------
    // Settings of the distance fields of a font: the same settings give the same fields, so baked atlases are kept
    // while their settings and the generator version match
    // -------------------------------------------------------------------------------------------------------------
    struct SdfFontSettings
    {
        int   glyphSize = 48;        // Field pixels per em
        float distanceRange = 16.0f; // Field pixels the distances are encoded within: from -range/2 outside to range/2 inside
        bool  multiChannel = true;   // MTSDF: sharp corners in RGB, true distance in alpha; false - true distance everywhere

    public:
        // Returns padding of a glyph field around its outline, in field pixels: the half of the range and a pixel for
        // the filtering
        int GetPadding() const;

        // Returns the settings as a short string for the baked atlas stamp
        String ToString() const;

        bool operator==(const SdfFontSettings& other) const;
        bool operator!=(const SdfFontSettings& other) const;
    };

    // -----------------------------------------------------------------------------------------------
    // Distance field of a glyph: RGBA pixels, 0.5 is the outline, more is inside; rows from the bottom
    // -----------------------------------------------------------------------------------------------
    struct SdfGlyphField
    {
        UInt32        character = 0; // Character code
        Vec2I         size;          // Size in pixels, zero for a glyph without an outline (a space)
        Vector<UInt8> pixels;        // RGBA pixels, rows from the bottom
        RectF         bounds;        // Box of the field in em, from the pen at the baseline, Y up; the padding included
        float         advance = 0;   // Advance in em, not hinted
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Distance field glyph generator: the outline of the font (not hinted, in font units) goes to msdfgen, which makes
    // the multi-channel field with the true distance in alpha (MTSDF), corrects its artifacts and resolves overlapping
    // contours. Pure CPU, works without render
    // ----------------------------------------------------------------------------------------------------------------
    class SdfGlyphGenerator
    {
    public:
        static constexpr int version = 1; // Changes when the fields change: baked atlases of another version are rebuilt

    public:
        // Generates the field of the character; false when the font has no such glyph
        static bool Generate(FT_Face face, UInt32 character, const SdfFontSettings& settings, SdfGlyphField& field);

        // Returns the cap height of the font (the top of 'H', the ascender without it) in em
        static float GetCapHeight(FT_Face face);
    };
}
