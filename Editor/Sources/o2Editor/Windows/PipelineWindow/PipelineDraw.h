#pragma once

#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Types/Containers/Vector.h"

using namespace o2;

namespace Editor
{
    // ----------------------------------------------------------------------------------------
    // Antialiased vector shapes of the Pipeline window: every edge gets a one-pixel fringe in the
    // current target, so cards, controls and shadows look the same at any camera zoom
    // ----------------------------------------------------------------------------------------
    namespace PipelineDraw
    {
        // Corner radii: left-bottom, right-bottom, right-top, left-top
        struct Corners
        {
            float lb = 0, rb = 0, rt = 0, lt = 0;

            Corners() = default;
            explicit Corners(float all): lb(all), rb(all), rt(all), lt(all) {}
            Corners(float top, float bottom): lb(bottom), rb(bottom), rt(top), lt(top) {}
        };

        // Vertical colour stops of a gradient: positions 0 (top) .. 1 (bottom)
        struct Gradient
        {
            Vector<Pair<float, Color4>> stops;

            Gradient() = default;
            explicit Gradient(const Color4& flat);
            Gradient(const Color4& top, const Color4& bottom);
            Gradient(const Color4& top, float midAt, const Color4& mid, const Color4& bottom);

            // Returns the colour at the position, 0 top .. 1 bottom
            Color4 At(float t) const;
        };

        // Returns the size of one target pixel in the current drawing units
        float Pixel();

        // Fills a rounded rectangle
        void Fill(const RectF& rect, const Corners& corners, const Color4& color);

        // Fills a rounded rectangle with a vertical gradient
        void Fill(const RectF& rect, const Corners& corners, const Gradient& gradient);

        // Fills a rounded rectangle with a checker of square cells from its top-left corner, the second colour on the
        // top-left cell of each 2 x 2 tile or on the top-right and bottom-left ones; one textured mesh whatever the cell count
        void Checker(const RectF& rect, const Corners& corners, float cell, const Color4& first, const Color4& second,
                     bool secondTopLeft);

        // Strokes a rounded rectangle between the insets from and to (units inside the edge, from < to); the gradient
        // runs over the rectangle height
        void Stroke(const RectF& rect, const Corners& corners, float from, float to, const Gradient& gradient);

        // Strokes a rounded rectangle with a line of the width centred on the edge
        void Frame(const RectF& rect, const Corners& corners, float width, const Color4& color);

        // Draws a soft shadow of a rounded rectangle, blur being the CSS blur radius
        void Shadow(const RectF& rect, const Corners& corners, const Color4& color, float blur);

        // Fills a circle
        void Circle(const Vec2F& center, float radius, const Color4& color);

        // Strokes a circle with a line of the width centred on the radius
        void Ring(const Vec2F& center, float radius, float width, const Color4& color);

        // Fills a convex polygon given counter-clockwise
        void Polygon(const Vector<Vec2F>& points, const Color4& color);

        // Draws a straight line of the width with round-free square ends
        void Line(const Vec2F& a, const Vec2F& b, float width, const Color4& color);
    }
}
