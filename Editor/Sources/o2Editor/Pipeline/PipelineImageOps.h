#pragma once

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // -------------------------------------------------------------------------
    // Pixel operations on RGBA8 bitmaps used by pipeline nodes. Coordinates are
    // image space: (0,0) is the top-left pixel, y grows downwards
    // -------------------------------------------------------------------------
    namespace PipelineImageOps
    {
        // Crop rectangle as fractions of the image size
        struct CropRect { float x = 0, y = 0, w = 1, h = 1; };

        // Reads a fractional crop from config-like fields; false when it is the whole frame
        bool ParseCrop(const DataValue* value, CropRect& crop);

        // Cuts the fractional rectangle out of the image, at least one pixel in each dimension
        Ref<Bitmap> Crop(const Bitmap& src, const CropRect& crop);

        // Cuts a pixel rectangle out of the image, clamped to its bounds
        Ref<Bitmap> CropPixels(const Bitmap& src, int left, int top, int width, int height);

        // What counts as empty when trimming: transparent pixels only, or near-white ones too
        enum class ContentMode { Alpha, White };

        // Trims the empty border around the content leaving pad pixels; unchanged when there is no content
        Ref<Bitmap> CropToContent(const Bitmap& src, ContentMode mode, int pad = 0);

        // Scales to the size, box-filtering when shrinking two times or more so thin lines survive
        Ref<Bitmap> Resize(const Bitmap& src, const Vec2I& size);

        // True when any pixel has non-zero alpha; bitmaps without an alpha channel always do
        bool HasContent(const Bitmap& src);

        // Draws overlay scaled to the base size on top of the base
        Ref<Bitmap> CompositeOverlay(const Bitmap& base, const Bitmap& overlay);

        // Recovers alpha from the same subject rendered on white and on black
        Ref<Bitmap> TwoPassMatte(const Bitmap& white, const Bitmap& black);

        // Chroma key settings, tolerances in percent
        struct ChromaOptions
        {
            Color4 color = Color4(0, 177, 64, 255); // Key color to remove, green screen by default
            float  tolerance = 30.0f;               // Chroma distance that is fully keyed out
            float  softness = 15.0f;                // Width of the partial alpha band beyond tolerance
            float  spill = 60.0f;                   // How much of the key color bleed is pulled back
        };

        // Makes pixels close to the key color transparent and suppresses its spill on the rest
        Ref<Bitmap> ChromaKey(const Bitmap& src, const ChromaOptions& options);

        // Stroke settings
        struct OutlineOptions
        {
            Color4 color = Color4::Black(); // Stroke color
            float  width = 4.0f;            // Stroke width in pixels
            String position = "outside";    // outside / inside / center
            float  opacity = 1.0f;          // Stroke opacity 0..1
            float  softness = 0.0f;         // Feather of the outer stroke edge in pixels
        };

        // Strokes the alpha silhouette; the canvas is padded to fit the stroke
        Ref<Bitmap> Outline(const Bitmap& src, const OutlineOptions& options);

        // Drop and inner shadow settings
        struct ShadowOptions
        {
            Color4 color = Color4::Black(); // Shadow color
            float  opacity = 0.6f;          // Shadow opacity 0..1
            float  angle = 45.0f;           // Offset direction in degrees, 0 is right and 90 is down
            float  distance = 8.0f;         // Offset length in pixels
            float  blur = 8.0f;             // Blur amount in pixels, half of it is the gaussian sigma
            float  spread = 0.0f;           // Grows the silhouette by this many pixels before blurring
            bool   inner = false;           // Shade the inside edge instead of casting a drop shadow
        };

        // Casts a drop shadow (the canvas grows to fit it) or shades the inside edge when inner is set
        Ref<Bitmap> Shadow(const Bitmap& src, const ShadowOptions& options);

        // Gradient overlay settings
        struct GradientOptions
        {
            String kind = "linear";          // linear / radial
            String blend = "normal";         // normal / multiply / screen / overlay / map
            Color4 color1 = Color4::White(); // Start color, the color of black for map
            Color4 color2 = Color4::Black(); // End color, the color of white for map
            float  alpha1 = 1.0f;            // Start alpha 0..1
            float  alpha2 = 1.0f;            // End alpha 0..1
            float  angle = 90.0f;            // Linear direction in degrees, 0 is left to right and 90 top to bottom
            float  opacity = 1.0f;           // Overall strength 0..1
            bool   clipToAlpha = true;       // Keep the source alpha instead of filling transparent areas
        };

        // Overlays a gradient with the blend mode; "map" recolors the image by its luminance instead
        Ref<Bitmap> Gradient(const Bitmap& src, const GradientOptions& options);

        // Color adjustment settings
        struct ColorOptions
        {
            float  brightness = 0.0f;               // -100..100
            float  contrast = 0.0f;                 // -100..100
            float  saturation = 0.0f;               // -100..100
            float  hue = 0.0f;                      // Hue shift in degrees, -360..360
            Color4 tint = Color4(255, 136, 0, 255); // Tint color, also the hue and saturation used by colorize
            float  tintStrength = 0.0f;             // Mix toward tint 0..1, ignored when colorize is set
            bool   colorize = false;                // Replace hue and saturation with those of tint
            bool   grayscale = false;               // Reduce to luminance
            bool   invert = false;                  // Invert RGB before the other adjustments
        };

        // Applies invert, brightness, contrast, hue and saturation (or colorize), tint and grayscale in that order
        Ref<Bitmap> AdjustColor(const Bitmap& src, const ColorOptions& options);

        // Nine-slice borders in source pixels
        struct NineSlice { int l = 0, t = 0, r = 0, b = 0; };

        // One layer of ComposeLayers: the image and its placement on the canvas
        struct PlacedLayer
        {
            Ref<Bitmap> image;                      // Layer image, layers without one are skipped
            float       x = 0, y = 0, w = 1, h = 1; // Placement rectangle in canvas pixels
            float       rotation = 0.0f;            // Degrees, clockwise
            bool        flipH = false;              // Mirror horizontally
            bool        flipV = false;              // Mirror vertically
            float       opacity = 1.0f;             // Layer opacity 0..1
            bool        nine = false;               // Scale with nine-slice instead of stretching
            NineSlice   slice;                      // Nine-slice borders, used when nine is set
            float       sliceScale = 1.0f;          // Scale of the nine-slice borders on the canvas
        };

        // Scales with nine-slice: the borders keep their size times scale, the middle parts stretch
        Ref<Bitmap> NineSliceResize(const Bitmap& src, int targetW, int targetH, const NineSlice& slice, float scale);

        // Flattens the layers onto a canvas; last layer on top. Background null = transparent
        Ref<Bitmap> ComposeLayers(int canvasW, int canvasH, const Vector<PlacedLayer>& layers, const Color4* background);

        // Mirrors the image along the chosen axes
        Ref<Bitmap> Flip(const Bitmap& src, bool horizontal, bool vertical);

        // Rotates clockwise by degrees with bilinear sampling; the canvas grows to fit
        Ref<Bitmap> Rotate(const Bitmap& src, float degrees);

        // Multiplies the alpha channel by opacity
        Ref<Bitmap> WithOpacity(const Bitmap& src, float opacity);

        // Makes a solid RGBA8 bitmap of the color, at least 1x1
        Ref<Bitmap> Blank(int width, int height, const Color4& color = Color4(0, 0, 0, 0));

        // Returns the RGBA8 pixel at image space coordinates; bitmap rows are stored bottom-up, hence the flip
        inline UInt8* Pixel(Bitmap& bmp, int x, int y)
        {
            Vec2I size = bmp.GetSize();
            return bmp.GetData() + ((size.y - 1 - y) * size.x + x) * 4;
        }

        // Returns the RGBA8 pixel at image space coordinates, read-only
        inline const UInt8* Pixel(const Bitmap& bmp, int x, int y)
        {
            Vec2I size = bmp.GetSize();
            return bmp.GetData() + ((size.y - 1 - y) * size.x + x) * 4;
        }
    }
}
// --- META ---

PRE_ENUM_META(Editor::PipelineImageOps::ContentMode);
// --- END META ---
