#pragma once

#include "o2/Render/TextureRef.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/String.h"
#include "o2Editor/Pipeline/UiDoc/UiKitModel.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

using namespace o2;

// ------------------------------------------------------------------------------------------------
// Drawing of the AI UI layout workspace (UiWorkspace.h): its panels draw themselves the way the web's
// CSS lays them out (styles.css .ulw / .uls / .uld), in a box given from its top-left corner with y
// going down. Texts and icons are cached across frames (a text keeps its mesh while it does not change),
// kit graphics become thumbnails once per picture. Colours are the Pipeline theme's tokens
// ------------------------------------------------------------------------------------------------
namespace Editor::UiWsDraw
{
    // ----------------------------------------------------------------------------------------
    // A box measured from the top-left corner of a world rectangle, y down (the web's coordinates)
    // ----------------------------------------------------------------------------------------
    struct Box
    {
        RectF world; // The area, world space (y up)

        Box() = default;
        explicit Box(const RectF& world): world(world) {}

        // The world rectangle of x, y, w, h from the top-left corner
        RectF R(float x, float y, float w, float h) const { return RectF(world.left + x, world.top - y, world.left + x + w, world.top - y - h); }

        // The world point of x, y
        Vec2F P(float x, float y) const { return Vec2F(world.left + x, world.top - y); }

        // The local point of a world point
        Vec2F L(const Vec2F& p) const { return Vec2F(p.x - world.left, world.top - p.y); }

        float W() const { return world.Width(); }
        float H() const { return world.Height(); }
    };

    // True when the world point is inside the world rectangle (y up, top > bottom)
    inline bool Inside(const RectF& r, const Vec2F& p) { return p.x >= r.left && p.x <= r.right && p.y <= r.top && p.y >= r.bottom; }

    // --- colours -------------------------------------------------------------------------------

    // The current theme
    const PipelinePalette& T();

    // The colour with its alpha multiplied
    Color4 A(const Color4& color, float alpha);

    // Snap lines (#FF5FA2)
    Color4 SnapColor();

    // --- shapes -------------------------------------------------------------------------------

    // Fills a rounded rectangle
    void Fill(const RectF& rect, float radius, const Color4& color);

    // Strokes a rounded rectangle inside its edge
    void Border(const RectF& rect, float radius, const Color4& color, float width = 1.0f);

    // Strokes a rectangle with a line centred on its edge
    void Frame(const RectF& rect, float radius, const Color4& color, float width);

    // A dashed rectangle outline (dash and gap in pixels)
    void DashedFrame(const RectF& rect, const Color4& color, float dash, float gap, float width = 1.0f);

    // A dashed line
    void DashedLine(const Vec2F& a, const Vec2F& b, const Color4& color, float dash, float gap, float width = 1.0f);

    // A straight line
    void Line(const Vec2F& a, const Vec2F& b, const Color4& color, float width = 1.0f);

    // A soft shadow of a rounded rectangle
    void Shadow(const RectF& rect, float radius, const Color4& color, float blur);

    // A filled circle
    void Circle(const Vec2F& center, float radius, const Color4& color);

    // A filled convex polygon (counter-clockwise)
    void Polygon(const Vector<Vec2F>& points, const Color4& color);

    // A checker of square cells
    void Checker(const RectF& rect, float radius, float cell, const Color4& a, const Color4& b);

    // A floating panel (.editor-island / a popover): shadow, fill, border
    void Island(const RectF& rect, float radius, bool glass);

    // --- text ---------------------------------------------------------------------------------

    // ----------------------------------
    // How a text is drawn into its box
    // ----------------------------------
    struct TextStyle
    {
        float    size = 12.0f;
        int      weight = 400;
        bool     mono = false;
        Color4   color = Color4::White();
        HorAlign hor = HorAlign::Left;
        VerAlign ver = VerAlign::Middle;
        bool     wrap = false;
        bool     dots = true;
        float    lineHeight = 0.0f; // Pixels per line when wrapping; 0 = the font's

        TextStyle() = default;
        TextStyle(float size, int weight, const Color4& color, HorAlign hor = HorAlign::Left, bool mono = false):
            size(size), weight(weight), mono(mono), color(color), hor(hor) {}
    };

    // Draws the text into the world rectangle
    void Text(const String& text, const RectF& rect, const TextStyle& style);

    // The width of the text on one line
    float TextWidth(const String& text, float size, int weight = 400, bool mono = false);

    // The height the text takes wrapped into the width
    float TextHeight(const String& text, float width, const TextStyle& style);

    // --- icons ---------------------------------------------------------------------------------

    // Draws the icon (an SVG of ui/pipeline/, name without the folder and extension: uiws_plus, al_check...) centred
    void Icon(const String& name, const Vec2F& center, float size, const Color4& color);

    // The icon of a block type word (uiws_type_<word>, a group's for an unknown one)
    String TypeIcon(const String& typeWord);

    // The icon of a preview device kind (uiws_dev_<kind>)
    String DeviceIcon(const String& kind);

    // --- kit graphics ---------------------------------------------------------------------------

    // The texture of a kit graphic (decoded once per picture) and its size in pixels; empty when it does not decode
    TextureRef KitTexture(const UiDoc::UiKitElement& element, Vec2I& size);

    // Draws a kit graphic fitted into the box on a small checker (the hierarchy's and the pickers' thumbnails)
    void KitThumb(const UiDoc::UiKitElement& element, const RectF& rect, float checkerCell = 3.0f, bool checker = true);

    // Draws a texture into a rectangle (the whole texture, or its source rect when given)
    void Picture(const TextureRef& texture, const Vec2I& size, const RectF& rect, float alpha = 1.0f);

    // Drops cached texts and icons not drawn for a while; called once a frame by the workspace
    void Sweep();
}
