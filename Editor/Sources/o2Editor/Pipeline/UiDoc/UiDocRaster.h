#pragma once

#include <vector>

#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

// ------------------------------------------------------------------------------------------------
// A small CPU rasterizer for the UI layout renderer (UiDocRender.h): anti-aliased polygons (nonzero
// winding, signed-area accumulation), strokes of rectangles, rounded rectangles, circles and polylines
// with dashes, pictures drawn through a scale + offset with bilinear sampling on mipmaps (clamped or
// repeated, clipped with anti-aliased edges, tinted), opacity layers and a Gaussian blur. Colours are
// premultiplied floats in sRGB, as browsers and librsvg composite SVG
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    using UiLayout::UiImage;

    // -------------------------------------
    // A colour with straight alpha, 0..1
    // -------------------------------------
    struct UiRgba
    {
        float r = 0, g = 0, b = 0, a = 1;
    };

    // #RGB / #RRGGBB -> a colour; def for anything else
    UiRgba UiColorOf(const String& hex, const UiRgba& def = UiRgba());

    // -------------------------
    // A point in canvas pixels
    // -------------------------
    struct UiPt
    {
        double x = 0, y = 0;
    };

    // A closed contour (or an open polyline for strokes)
    using UiPath = std::vector<UiPt>;

    // Contours filled together (nonzero winding)
    using UiShape = std::vector<UiPath>;

    // A rectangle as a contour, clockwise from the top-left (y down), as SVG's <rect> path runs
    UiPath RectPath(double x, double y, double w, double h);

    // A rounded rectangle, clockwise from (x + r, y); r clamped to half the sides
    UiPath RoundRectPath(double x, double y, double w, double h, double r);

    // A circle, clockwise from its rightmost point (SVG's start)
    UiPath CirclePath(double cx, double cy, double r);

    // Appends a quadratic curve from the path's last point through c to p
    void QuadTo(UiPath& path, const UiPt& c, const UiPt& p);

    // The contour reversed (a hole when filled with its outer contour)
    UiPath Reversed(const UiPath& path);

    // --------------------------------------------------------------------------------------
    // Coverage over the canvas (0..1 per pixel) with the box that was touched; shapes are combined
    // with max(), so overlapping strokes and glyphs never darken
    // --------------------------------------------------------------------------------------
    class UiMask
    {
    public:
        int                w = 0, h = 0;
        std::vector<float> a;
        int                x0 = 0, y0 = 0, x1 = 0, y1 = 0; // Touched box, x1 / y1 exclusive
        std::vector<int>   lo, hi;                         // Touched span of each row, hi exclusive (lo >= hi: none)

    public:
        // Sizes the mask, cleared (a mask of that size already only clears what was covered)
        void Reset(int width, int height);

        // Clears the touched box
        void Clear();

        // True when nothing is covered
        bool IsEmpty() const { return x1 <= x0 || y1 <= y0; }

        // Covers the shape (nonzero winding, anti-aliased)
        void Fill(const UiShape& shape);

        // Covers the stroke of a polyline: butt ends, round joins; dashes (on, off lengths) from its start when given
        void Stroke(const UiPath& path, bool closed, double width, const std::vector<double>& dashes = {});

        // Covers a glyph's 8-bit coverage placed with its top-left at (x, y)
        void Blit(const UInt8* coverage, int pitch, int width, int height, int x, int y);

        // Widens the touched box to hold the rectangle
        void Touch(int ax0, int ay0, int ax1, int ay1);

        // Widens a row's touched span
        void TouchRow(int y, int ax0, int ax1);

    private:
        std::vector<float> mAcc;           // Accumulation of one shape, (w + 2) x h, zero between shapes
        std::vector<int>   mAccLo, mAccHi; // Its written span per row
    };

    // ---------------------------------------------------------------------------
    // A picture ready for drawing: premultiplied RGBA floats with its halved copies
    // ---------------------------------------------------------------------------
    struct UiPicture
    {
        int                    w = 0, h = 0;
        std::vector<float>     px;    // Premultiplied RGBA
        std::vector<UiPicture> mips;  // Level 1, 2, ...: halved, box-filtered

        // The picture of an image (straight 8-bit RGBA)
        static UiPicture Of(const UiImage& img);
    };

    // -----------------------------------------------------------------------------------------
    // How a picture is drawn: canvas = source * scale + offset; clipped to a box with anti-aliased edges;
    // repeated (a pattern) or clamped at its edges; tinted (RGB multiplied) and faded
    // -----------------------------------------------------------------------------------------
    struct UiPictureDraw
    {
        double sx = 1, sy = 1, tx = 0, ty = 0;
        double cx0 = 0, cy0 = 0, cx1 = 0, cy1 = 0; // Clip box in canvas pixels
        bool   repeat = false;
        bool   tinted = false;
        float  tint[3] = { 1, 1, 1 };
        float  opacity = 1;
    };

    // --------------------------------------------------------------------------------------
    // A canvas of premultiplied RGBA floats, top-left origin; tracks the box drawn on for layers
    // --------------------------------------------------------------------------------------
    class UiCanvas
    {
    public:
        int                w = 0, h = 0;
        std::vector<float> px;
        int                x0 = 0, y0 = 0, x1 = 0, y1 = 0; // Box drawn on

    public:
        // Sizes the canvas, transparent (a canvas of that size already only clears what was drawn on)
        void Reset(int width, int height);

        // Clears what was drawn on
        void Clear();

        // Paints the mask's coverage with a colour
        void Paint(const UiMask& mask, const UiRgba& color, float opacity = 1);

        // Paints the mask's coverage with a vertical gradient from top (at y = ya) to bottom (at y = yb)
        void PaintGradient(const UiMask& mask, const UiRgba& top, const UiRgba& bottom, double ya, double yb, float opacity = 1);

        // Fills a box with a colour (no anti-aliasing: whole pixels)
        void FillRect(int ax0, int ay0, int ax1, int ay1, const UiRgba& color);

        // Draws a picture
        void Draw(const UiPicture& picture, const UiPictureDraw& d);

        // Composites another canvas of the same size over this one, faded
        void Composite(const UiCanvas& layer, float opacity = 1);

        // Composites a premultiplied RGBA block (w x h) at (x, y)
        void CompositeBlock(const std::vector<float>& block, int bw, int bh, int x, int y, float opacity = 1);

        // The canvas as straight 8-bit RGBA
        UiImage ToImage() const;

        // Widens the box drawn on to hold the rectangle
        void Touch(int ax0, int ay0, int ax1, int ay1);
    };

    // Blurs a premultiplied RGBA block in place with a Gaussian of the standard deviation (three box blurs, as SVG
    // implementations do), channels independently
    void GaussianBlur(std::vector<float>& block, int w, int h, double sigma);
}
