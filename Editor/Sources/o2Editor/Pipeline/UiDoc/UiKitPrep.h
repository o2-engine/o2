#pragma once

#include <optional>

#include "o2Editor/Pipeline/UiDoc/UiKitModel.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

// ------------------------------------------------------------------------------------------------
// Getting a picture ready for a UI kit, locally and deterministically (port of AssetsLine backend/src/
// uiKit/prep.ts; the UI kit node runs on every change, so nothing here calls a model):
//   1. key - cut a flat backdrop: flood-fill from the border through pixels close to the border colour,
//      with a soft edge;
//   2. trim - drop fully transparent margins;
//   3. nine-slice - the borders the user (or the AI) set, in input pixels, mapped into the trimmed picture;
//      or found: the run of equal neighbouring columns / rows through the middle (a pixel or two of noise
//      allowed), else the longest such run, else from the corner rounding;
//   4. collapse - a middle whose lines all equal its centre line shrinks to 4 px, so the sprite is small
//      and stretches the same.
// Pictures are UiLayout::UiImage (RGBA8, rows top-down, straight alpha), the web's Rgba
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    using UiLayout::UiImage;

    // The hash a prepared picture remembers its input by: SHA-1 of the bytes, the first 16 hex digits
    String InputHash(const String& bytes);

    // SHA-1 of the bytes as 40 lower-case hex digits
    String Sha1Hex(const String& bytes);

    // Flood-fills the backdrop from the border (pixels closer than 2 tol to the border's median colour; closer than tol
    // go clear, the rest of the reached ones fade: a soft edge that does not spread); returns how many went clear
    int KeyBackground(UiImage& img, double tol = 30);

    // -------------------------------------
    // An integer box in pixels of a picture
    // -------------------------------------
    struct UiPixelBox
    {
        int x = 0, y = 0, w = 0, h = 0;

        bool operator==(const UiPixelBox& other) const { return x == other.x && y == other.y && w == other.w && h == other.h; }
    };

    // The box of the pixels with alpha above min (the whole picture when it is all clear)
    UiPixelBox AlphaBox(const UiImage& img, int min = 6);

    // -------------------------------------------------------------------------------------------------
    // Borders found in a picture, with the stretchable runs [first, last] they came from (none = guessed from
    // the corner rounding)
    // -------------------------------------------------------------------------------------------------
    struct UiNineFound
    {
        UiNine                             nine;
        std::optional<std::pair<int, int>> uniformX; // Columns
        std::optional<std::pair<int, int>> uniformY; // Rows
    };

    // Borders from the picture: the uniform middle when there is one, else from the corner rounding
    UiNineFound DetectNine(const UiImage& img);

    // -----------------------------------------
    // What to do with an element's picture
    // -----------------------------------------
    struct UiPrepInput
    {
        String                data;               // PNG / JPEG bytes (used when image is empty)
        UiImage               image;              // Or the decoded picture
        String                assetType = "simple"; // simple | sliced | tiled
        bool                  key = false;        // Cut a flat backdrop (bg: key)
        bool                  trim = true;
        bool                  optimize = true;    // Collapse a uniform middle (sliced only)
        std::optional<UiNine> nineSlice;          // Borders in input pixels; none = find them
    };

    // ---------------------------------------------------------------------------------------------
    // The prepared element: its PNG, size, borders (sliced), where it sat in the input ({w, h, trim,
    // collapsed?}) and the borders found (input pixels) for the card to suggest
    // ---------------------------------------------------------------------------------------------
    struct UiPrepOutput
    {
        String                png;
        UiImage               image;
        double                width = 0, height = 0;
        std::optional<UiNine> nineSlice;
        Json                  source;
        std::optional<UiNine> detected;
    };

    // Prepares an element's picture; false when the bytes are not a picture
    bool PrepareElement(const UiPrepInput& input, UiPrepOutput& out);

    // -------------------------------------------------------------------------------------------
    // The borders the node would find, in INPUT pixels, and whether each axis has a uniform stretch
    // -------------------------------------------------------------------------------------------
    struct UiInputNine
    {
        UiNine nine;
        bool   uniformX = false, uniformY = false;
        int    w = 0, h = 0;
    };

    // The borders a picture's stretch shows (where an axis has none they are only a guess from the corner rounding - the
    // AI's measurement is better there); false when the bytes are not a picture
    bool DetectInputNine(const String& data, bool key, UiInputNine& out);
}
