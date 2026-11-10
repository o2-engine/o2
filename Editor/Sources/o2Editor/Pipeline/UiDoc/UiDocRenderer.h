#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"

// ------------------------------------------------------------------------------------------------
// The layout renderer's insides, shared by UiDocRender.cpp (the tree, line art, kit skins),
// UiDocRenderText.cpp (labels) and UiDocKitSheet.cpp; not for other users. Values that render.ts writes
// into SVG attributes are rounded the way it rounds them (F), so shapes land where the web's land
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc::Render
{
    // render.ts's f(): a number as the SVG writes it, to 0.01
    inline double F(double n) { return JsRound(n*100.0)/100.0; }

    // Line art colours
    extern const char* const INK;
    extern const char* const INK_SOFT;
    extern const char* const PAPER;
    extern const char* const FILL;
    extern const char* const FILL2;
    extern const char* const WHITE;

    // -----------------------------------------------------------
    // An SVG stroke: colour, width and dashes, in design units
    // -----------------------------------------------------------
    struct Stroke
    {
        UiRgba              color;
        double              width = 0;
        std::vector<double> dashes;
    };

    // --------------------------------------------------------------------------------------------
    // A label to draw (render.ts's TextBox): plain = no gradient, outline or shadow; size 0 = the fitted
    // style size; color overrides the style's fill (and drops its gradient)
    // --------------------------------------------------------------------------------------------
    struct TextBox
    {
        String                text;
        UiRect                rect;
        TextStyle             style;
        String                align = "center";
        std::optional<String> color;
        double                size = 0;
        bool                  plain = false;
        double                padX = 0;
    };

    // ---------------------------------------------------------------------------------------
    // One drawing of a layout: the canvas (with opacity layers), the scale from design units to
    // pixels and the line art's stroke and id sizes
    // ---------------------------------------------------------------------------------------
    class Renderer
    {
    public:
        const UiDoc&           doc;
        const UiRenderOptions& o;
        UiRects                rects;
        bool                   kitMode = false;
        double                 W = 0, H = 0;        // Screen, design units
        double                 dpi = UI_BASE_DPI;   // The layout's density
        double                 sw = 1.5;            // Line art stroke width
        double                 idSize = 12;
        double                 s = 1, ox = 0, oy = 0; // Pixels = offset + design * s
        int                    cw = 1, ch = 1;      // Canvas size

    public:
        Renderer(const UiDoc& doc, const UiRenderOptions& options);

        // Draws the layout and returns the picture
        UiImage Run();

        // Starts a drawing on a transparent canvas (Run does it); Finish returns the picture
        void Begin();
        UiImage Finish();

        // A shape in pixels painted with a colour per pixel (a pattern)
        void FillPattern(const UiShape& shape, const std::function<UiRgba(int x, int y)>& colorAt);

        double X(double x) const { return ox + x*s; }
        double Y(double y) const { return oy + y*s; }

        // The canvas drawn on now (the top opacity layer)
        UiCanvas& Top() { return *layers[depth]; }

        // A layer for a group with opacity; PopLayer composites it faded
        void PushLayer();
        void PopLayer(float opacity);

        // A shape in pixels filled with a colour
        void FillShape(const UiShape& shape, const UiRgba& color, float opacity = 1);

        // <rect> (rx clamped to half the sides; rx 0 = square corners)
        void RectEl(const UiRect& r, double rx, const UiRgba* fill, const Stroke* stroke);

        // <circle>
        void CircleEl(double cx, double cy, double r, const UiRgba* fill, const Stroke* stroke);

        // <line>
        void LineEl(double x1, double y1, double x2, double y2, const Stroke& stroke);

        // <path>: a contour in design units, filled and / or stroked
        void PathEl(const UiPath& path, bool closed, const UiRgba* fill, const Stroke* stroke);

        // <text> of render.ts's textSvg
        void Text(const TextBox& t);

        // A plain one-line text with its start at (x, baseline), design units (ids, the kit sheet)
        void PlainText(const String& text, const String& fontFile, double size, double x, double baseline, const UiRgba& color,
                       float opacity = 1);

        // A kit picture into a box: nine-slice, tiled, or fitted (contain / cover / stretch), tinted; clip limits it
        void Sprite(const UiKitElement& el, const UiRect& r, const String& fit, const std::optional<String>& tint,
                    const UiRect* clip = nullptr);

        // A block and its children
        void Node(const UiNode& n);

        // A block's line art (missing: the kit has nothing for it - dashed orange)
        void LineArt(const UiNode& n, const UiRect& r, bool missing);

        // A block drawn with its kit skin and its label
        void Skinned(const UiNode& n, const UiRect& r, const UiSkin& skin);

    private:
        // Canvases and the mask, kept per thread between drawings (sizing and clearing large buffers costs more than
        // drawing a layout)
        struct Buffers
        {
            std::vector<std::unique_ptr<UiCanvas>> layers;
            UiMask                                 mask;
        };

        Buffers&                                 buffers;
        std::vector<std::unique_ptr<UiCanvas>>&  layers;
        UiMask&                                  mask;
        int                                      depth = 0;

        static Buffers& ThreadBuffers();
    };

    // The decoded picture of a kit element, cached by its name and picture (null when it does not decode)
    std::shared_ptr<const UiPicture> KitPicture(const UiKitElement& el);

    // Drops the cached kit pictures
    void ClearKitPictures();
}
