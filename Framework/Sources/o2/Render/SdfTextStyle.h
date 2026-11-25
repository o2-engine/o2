#pragma once

#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    class FontStyle;

    // ------------------------------------------------------------------------------------------------------------------
    // Font style for the distance field text: the effects of a FontStyle turned into layers the text shader composites
    // per pixel, front to back. A layer is a colour (solid or a gradient) over the union of up to two taps of the field:
    // the glyph grown by a dilation, blurred and offset. Units are the pixels of the glyph bitmap the vector font would
    // render for the text height (the radii of the effects), so a style looks the same as on the vector font at 1:1 and
    // stays sharp at any scale
    // ------------------------------------------------------------------------------------------------------------------
    class SdfTextStyle
    {
    public:
        static constexpr int maxLayers = 6;       // Layers the shader composites
        static constexpr int maxTaps = 2;         // Field samples a layer takes
        static constexpr int maxGradients = 2;    // Gradients a style has, their coordinates go with the vertices
        static constexpr int vectorsPerLayer = 6; // vec4 of a layer in the shader parameters

        // Field sample of a layer
        struct Tap
        {
            Vec2F offset;         // Offset in pixels, Y down as the vector font effects have it
            float dilation = 0.0f; // Outline grown by pixels
            float blur = 0.0f;     // Blur radius in pixels (the cone kernel of Bitmap::Blur)
            float alpha = 1.0f;    // Opacity of the tap in the union

            bool operator==(const Tap& other) const;
        };

        // Gradient of the colours of a layer, as FontGradientEffect has it over the glyph bitmap
        struct Gradient
        {
            float angle = 0.0f;  // Degrees
            float length = 0.0f; // Pixels, 0 - the size of the bitmap along the direction
            Vec2F origin;        // Relative to the bitmap

            bool operator==(const Gradient& other) const;
        };

        // Coloured shape
        struct Layer
        {
            Color4      color1 = Color4::White(); // Colour, or the start of the gradient
            Color4      color2 = Color4::White(); // End of the gradient
            int         gradient = -1;            // Gradient index, -1 - solid colour
            Vector<Tap> taps;                     // Field samples, united
            float       edgeScale = 1.0f;         // Width of the antialiased edge, less is harder
            bool        sharpCorners = false;     // Takes the multi-channel distance: the glyph itself
            bool        shadow = false;           // A shadow: later shadows cast its copy too

            bool operator==(const Layer& other) const;
        };

    public:
        // Default constructor: the glyph filled white
        SdfTextStyle();

        // Builds the style of the font style; IsSupported is false when some effect has no distance field version
        static SdfTextStyle Build(const Ref<FontStyle>& style);

        // Returns true when every effect has turned into layers
        bool IsSupported() const;

        // Marks the style as not supported
        void SetUnsupported();

        // Multiplies the colours of the layers made so far
        void MultiplyColor(const Color4& color);

        // Multiplies the colours of the layers made so far by the gradient
        void MultiplyGradient(const Color4& color1, const Color4& color2, const Gradient& gradient);

        // Adds the stroke under the layers made so far, around their shape; the radius and threshold are the ones of
        // Bitmap::Outline
        void AddStroke(float radius, const Color4& color, int alphaThreshold);

        // Adds the shadow under the layers made so far: their coverage blurred, offset and coloured
        void AddShadow(float blurRadius, const Vec2F& offset, const Color4& color);

        // Hardens the antialiased edges of the layers made so far, as alpha contrast around the threshold
        void Sharpen(float contrast, int threshold);

        // Grows the pixels the glyph bitmap of the vector font would have around the glyph (the effects' size extend)
        void ExtendRasterBorder(const Vec2I& extend);

        // Returns the layers, the front one first
        const Vector<Layer>& GetLayers() const;

        // Returns the gradients
        const Vector<Gradient>& GetGradients() const;

        // Returns pixels the layers reach out of the glyph outline, by axes
        Vec2F GetExtent() const;

        // Returns pixels the field must keep out of the outline for the layers: the dilations and the blurs, without the
        // offsets (an offset moves the sample, not the edge)
        float GetReach() const;

        // Returns the border the vector font bitmap would have around the glyph: the effects' extends and 2 pixels
        Vec2I GetRasterBorder() const;

        // Returns the coordinate of the gradient at the point of the glyph bitmap of that size, 0..1 inside it
        float GetGradientCoordinate(int gradient, const Vec2F& point, const Vec2F& bitmapSize) const;

        // Packs the layers into the shader parameters: vectorsPerLayer float4 per layer
        void Pack(Vector<float>& data) const;

        // Returns the width the stroke of Bitmap::Outline with that radius covers out of a straight edge, in pixels
        static float GetStrokeWidth(float radius, int alphaThreshold);

        bool operator==(const SdfTextStyle& other) const;

    protected:
        Vector<Layer>    mLayers;            // Layers, the front one first
        Vector<Gradient> mGradients;         // Gradients of the layers
        float            mShapeDilation = 0; // Dilation of the union of the glyph and its strokes
        Vec2I            mRasterBorder;      // Effects' size extend, as the vector font sums them
        bool             mSupported = true;  // Every effect has turned into layers
    };
}
