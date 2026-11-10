#pragma once

#include <vector>

#include "o2Editor/Pipeline/UiDoc/UiDocRaster.h"

// ------------------------------------------------------------------------------------------------
// The UI fonts for the layout renderer: the same open fonts the web ships (AssetsLine
// backend/src/uiLayout/fonts, copied to o2/Tools/UiFonts with their licences), drawn with FreeType -
// outlines unhinted, positioned at quarter pixels, outlined with FreeType's stroker (round joins).
// Faces, advances and rendered glyphs are cached; every call is thread-safe
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // The folder of the UI fonts: O2_UI_FONTS_DIR, else o2/Tools/UiFonts found from the project root or upwards; "" when
    // missing
    String UiFontsHome();

    // ------------------------------------------------------------
    // A glyph placed on a line: its font file, glyph index and pen x
    // ------------------------------------------------------------
    struct UiPlacedGlyph
    {
        int      face = -1;  // Index of the loaded face
        unsigned glyph = 0;
        double   x = 0;      // Pen position from the line's start, pixels
    };

    // ------------------------------------------------------------------------------------------
    // One line of text shaped in a font at a pixel size: glyphs and the advance width (letter spacing
    // after every character, as browsers count it)
    // ------------------------------------------------------------------------------------------
    struct UiShapedLine
    {
        std::vector<UiPlacedGlyph> glyphs;
        double                     width = 0;
        double                     ascent = 0, descent = 0; // Of the font at that size, pixels (descent positive)
    };

    // Shapes a line in the font file at the pixel size (characters the font lacks come from Roboto); empty when the font
    // file is missing
    UiShapedLine ShapeUiText(const String& fontFile, const String& text, double sizePx, double letterSpacingPx);

    // Covers the line's glyphs in the mask with the line's start at (x, baseline), pixels; strokeWidthPx > 0 covers their
    // outline of that width (centred on the outline, round joins) instead
    void DrawUiText(const UiShapedLine& line, double x, double baseline, double sizePx, double strokeWidthPx, UiMask& mask);

    // True when the font file is in the UI fonts folder and loads
    bool HasUiFont(const String& fontFile);

    // Drops the cached glyph pictures (faces stay)
    void ClearUiGlyphCache();
}
