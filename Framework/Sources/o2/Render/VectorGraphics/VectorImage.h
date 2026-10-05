#pragma once

#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    // Rule of the filled area of overlapping sub paths
    enum class VectorFillRule { NonZero, EvenOdd };

    // Shape of the stroke ends
    enum class VectorLineCap { Butt, Round, Square };

    // Shape of the stroke corners
    enum class VectorLineJoin { Miter, Round, Bevel };

    // Kind of fill or stroke paint
    enum class VectorPaintType { None, Solid, LinearGradient, RadialGradient };

    // -------------
    // Gradient stop
    // -------------
    struct VectorGradientStop
    {
        float  offset = 0.0f; // Position on the gradient ramp, 0..1
        Color4 color;         // Straight alpha color, alpha includes stop-opacity

    public:
        // Default constructor
        VectorGradientStop() = default;

        // Constructor
        VectorGradientStop(float offset, const Color4& color);
    };

    // -------------------------------------------------------------------------------
    // Fill or stroke paint: none, solid color or gradient with pad spread. Gradient
    // geometry is in gradient space, transform maps it into image space
    // -------------------------------------------------------------------------------
    struct VectorPaint
    {
        VectorPaintType type = VectorPaintType::None; // Paint type

        Color4 color = Color4::Black(); // Solid color

        Vector<VectorGradientStop> stops; // Gradient stops, sorted by offset

        Vec2F begin;             // Linear gradient begin point (offset 0)
        Vec2F end = Vec2F(1, 0); // Linear gradient end point (offset 1)

        Vec2F center;        // Radial gradient center (offset 1 circle center)
        Vec2F focal;         // Radial gradient focal point (offset 0)
        float radius = 1.0f; // Radial gradient radius

        Basis transform; // Gradient space to image space transform

    public:
        // Returns solid color paint
        static VectorPaint Solid(const Color4& color);

        // Returns linear gradient paint with points in image space
        static VectorPaint Linear(const Vec2F& begin, const Vec2F& end, const Vector<VectorGradientStop>& stops);

        // Returns radial gradient paint with center and radius in image space
        static VectorPaint Radial(const Vec2F& center, float radius, const Vector<VectorGradientStop>& stops);

        // Returns true when paint draws nothing
        bool IsNone() const;

        // Returns color of the gradient ramp at offset
        Color4 GetRampColor(float offset) const;

        // Returns ramp offset of image space point for gradients, 0 otherwise
        float GetRampOffset(const Vec2F& point) const;
    };

    // ---------------------------------------------------
    // Path segment: line or cubic bezier to the end point
    // ---------------------------------------------------
    struct VectorSegment
    {
        bool  cubic = false; // Is segment a cubic bezier, otherwise a line
        Vec2F control1;      // First control point, cubic only
        Vec2F control2;      // Second control point, cubic only
        Vec2F end;           // End point

    public:
        // Default constructor
        VectorSegment() = default;

        // Line constructor
        VectorSegment(const Vec2F& end);

        // Cubic constructor
        VectorSegment(const Vec2F& control1, const Vec2F& control2, const Vec2F& end);
    };

    // ------------------------------------------------
    // Continuous chain of segments from the start point
    // ------------------------------------------------
    struct VectorSubPath
    {
        Vec2F                 start;          // Start point
        Vector<VectorSegment> segments;       // Segments chain
        bool                  closed = false; // Is chain closed to the start point

    public:
        // Appends line segment
        void LineTo(const Vec2F& point);

        // Appends cubic bezier segment
        void CubicTo(const Vec2F& control1, const Vec2F& control2, const Vec2F& point);

        // Appends sub path flattened into polyline; tolerance is maximum deviation in path units
        void Flatten(float tolerance, Vector<Vec2F>& points) const;
    };

    // ---------------------------------------------------------------------------------------
    // Shape: sub paths in image space with fill and stroke. Alpha of the fill is fill color
    // alpha * fillOpacity * opacity, of the stroke - stroke color alpha * strokeOpacity * opacity
    // ---------------------------------------------------------------------------------------
    struct VectorShape
    {
        Vector<VectorSubPath> subPaths; // Geometry, image space

        VectorPaint    fill = VectorPaint::Solid(Color4::Black()); // Fill paint
        VectorFillRule fillRule = VectorFillRule::NonZero;         // Fill rule
        float          fillOpacity = 1.0f;                         // Fill opacity

        VectorPaint    stroke;                             // Stroke paint
        float          strokeWidth = 1.0f;                 // Stroke width in image units
        VectorLineCap  strokeCap = VectorLineCap::Butt;    // Stroke ends shape
        VectorLineJoin strokeJoin = VectorLineJoin::Miter; // Stroke corners shape
        float          strokeMiterLimit = 4.0f;            // Miter length to stroke width ratio limit
        float          strokeOpacity = 1.0f;               // Stroke opacity

        float opacity = 1.0f; // Common opacity, product of the element and its groups opacities

    public:
        // Appends rectangle sub path, optionally with rounded corners
        void AddRect(const Vec2F& position, const Vec2F& size, const Vec2F& cornerRadius = Vec2F());

        // Appends ellipse sub path
        void AddEllipse(const Vec2F& center, const Vec2F& radius);

        // Appends closed or open polyline sub path
        void AddPolyline(const Vector<Vec2F>& points, bool closed);

        // Applies transformation to geometry; stroke width is scaled by the mean scale of transformation
        void Transform(const Basis& transform);

        // Returns bounds of geometry without stroke: false when shape is empty
        bool GetBounds(Vec2F& min, Vec2F& max) const;
    };

    // ----------------------------------------------------------------------------------------
    // Vector image document: ordered list of shapes in image space. Image space origin is the
    // left top corner, Y axis is down, units are pixels at scale 1
    // ----------------------------------------------------------------------------------------
    struct VectorImage
    {
        Vec2F size;          // Image size
        Vec2F viewBoxOrigin; // Source view box left top corner, already mapped by shapes geometry
        Vec2F viewBoxSize;   // Source view box size

        Vector<VectorShape> shapes; // Shapes in drawing order

    public:
        // Removes all shapes and resets size
        void Clear();
    };
}
// --- META ---

PRE_ENUM_META(o2::VectorFillRule);

PRE_ENUM_META(o2::VectorLineCap);

PRE_ENUM_META(o2::VectorLineJoin);

PRE_ENUM_META(o2::VectorPaintType);
// --- END META ---
