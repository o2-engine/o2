#pragma once

#include <optional>

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Types/Ref.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRaster.h"
#include "o2Editor/Pipeline/UiDoc/UiDocSkin.h"

// ------------------------------------------------------------------------------------------------
// The layout drawn into pixels (port of AssetsLine shared/uiDoc/render.ts, which draws SVG; this draws
// the same pictures natively). Two looks:
//   lines - line art: neutral blocks, each type with its own glyph (a button is a rounded box with its
//           label, an image a crossed box, a progress bar a track with its fill...);
//   kit   - the UI kit laid over it: each block draws the kit picture its `kit=` names or its type picks
//           (nine-slice, tiled or fitted, at the graphic's density), labels take the kit's text styles.
//           A block the kit has nothing for stays line art, outlined dashed orange.
// The viewBox is the screen (design pixels); the canvas is the screen times a scale, top-left origin.
// Kit pictures (decoded once per element), fonts and glyphs are cached across calls; thread-safe
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // --------------------------------
    // What and how to draw
    // --------------------------------
    struct UiRenderOptions
    {
        String         mode = "lines";     // lines | kit
        const UiKit*   kit = nullptr;      // The UI kit (kit mode); null draws text with the default styles
        double         width = 0;          // Screen size to lay out at (another device's); 0 = the design size
        double         height = 0;
        const UiRects* rects = nullptr;    // Boxes already solved at that size (else solved here)
        bool           showIds = false;    // Write each block's id on it (and outline groups in line art)
        double         scale = 1;          // Canvas pixels per design pixel
        int            pixelLong = 0;      // When > 0: the canvas's long side in pixels (the scale follows it)
    };

    // The layout drawn: straight 8-bit RGBA, rows top-down (UiImage::ToBitmap / Png give an o2 bitmap or PNG bytes)
    UiImage RenderLayout(const UiDoc& doc, const UiRenderOptions& options);

    // The layout drawn as an o2 bitmap (RGBA8)
    Ref<Bitmap> RenderLayoutBitmap(const UiDoc& doc, const UiRenderOptions& options);

    // The canvas size a layout is drawn at with the options (pixels)
    void RenderSize(const UiDoc& doc, const UiRenderOptions& options, int& width, int& height);

    // The Text style card's sample: the text on one line in the style on a screen that widens with it (the card scales it
    // down), dark or light paper
    UiDoc TextStyleSampleDoc(const TextStyle& style, const String& text, bool dark);

    // The Text style card's picture: TextStyleSampleDoc drawn with the style
    UiImage RenderTextStyleSample(const TextStyle& style, const String& text, bool dark, double scale = 1);

    // The UI kit as one picture for the layout agent (port of backend/src/uiKit/sheet.ts): every graphic (up to max) on a
    // checkerboard cell with its name and "type/variant . kind . WxH" under it, five across; sizes in the design pixels
    // of a layout at layoutDpi when given. An empty image for a kit without graphics
    UiImage RenderKitSheet(const UiKit& kit, int max = 40, std::optional<double> layoutDpi = std::nullopt);

    // Drops the cached kit pictures (fonts and glyphs too)
    void ClearUiRenderCache();
}
