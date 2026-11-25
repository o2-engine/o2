#include "o2/stdafx.h"
#include "SdfTextStyle.h"

#include <cmath>

#include "o2/Render/FontStyle.h"
#include "o2/Render/VectorFont.h"

namespace o2
{
    namespace
    {
        // Half width of the smoothstep that follows the edge of a coverage blurred by the cone kernel of a radius
        const float blurEdgeWidth = 0.85f;

        // Hardest edge a sharpen leaves, of the antialiased one
        const float minEdgeScale = 0.25f;

        // Alpha of a stroke of Bitmap::Outline that is seen as its edge
        const float visibleAlpha = 0.4f;

        Color4 MultiplyColors(const Color4& a, const Color4& b)
        {
            return a*b;
        }
    }

    bool SdfTextStyle::Tap::operator==(const Tap& other) const
    {
        return offset == other.offset && Math::Equals(dilation, other.dilation) && Math::Equals(blur, other.blur) &&
            Math::Equals(alpha, other.alpha);
    }

    bool SdfTextStyle::Gradient::operator==(const Gradient& other) const
    {
        return Math::Equals(angle, other.angle) && Math::Equals(length, other.length) && origin == other.origin;
    }

    bool SdfTextStyle::Layer::operator==(const Layer& other) const
    {
        return color1 == other.color1 && color2 == other.color2 && gradient == other.gradient && taps == other.taps &&
            Math::Equals(edgeScale, other.edgeScale) && sharpCorners == other.sharpCorners && shadow == other.shadow;
    }

    SdfTextStyle::SdfTextStyle()
    {
        Layer glyph;
        glyph.taps.Add(Tap());
        glyph.sharpCorners = true;
        mLayers.Add(glyph);
    }

    SdfTextStyle SdfTextStyle::Build(const Ref<FontStyle>& style)
    {
        SdfTextStyle result;
        if (!style)
            return result;

        for (auto& effect : style->GetEffects())
        {
            if (!effect)
                continue;

            result.ExtendRasterBorder(effect->GetSizeExtend());
            if (!effect->AddToDistanceFieldStyle(result))
                result.SetUnsupported();
        }

        if (result.mLayers.Count() > maxLayers)
            result.SetUnsupported();

        return result;
    }

    bool SdfTextStyle::IsSupported() const
    {
        return mSupported;
    }

    void SdfTextStyle::SetUnsupported()
    {
        mSupported = false;
    }

    void SdfTextStyle::MultiplyColor(const Color4& color)
    {
        for (auto& layer : mLayers)
        {
            layer.color1 = MultiplyColors(layer.color1, color);
            layer.color2 = MultiplyColors(layer.color2, color);
        }
    }

    void SdfTextStyle::MultiplyGradient(const Color4& color1, const Color4& color2, const Gradient& gradient)
    {
        int index = mGradients.IndexOf(gradient);
        if (index < 0)
        {
            if (mGradients.Count() >= maxGradients)
            {
                SetUnsupported();
                return;
            }

            index = mGradients.Count();
            mGradients.Add(gradient);
        }

        for (auto& layer : mLayers)
        {
            // A product of two gradients is not linear; one of another direction is taken by its middle colour
            if (layer.gradient >= 0 && layer.gradient != index)
            {
                Color4 middle = Math::Lerp(color1, color2, 0.5f);
                layer.color1 = MultiplyColors(layer.color1, middle);
                layer.color2 = MultiplyColors(layer.color2, middle);
                continue;
            }

            layer.color1 = MultiplyColors(layer.color1, color1);
            layer.color2 = MultiplyColors(layer.color2, color2);
            layer.gradient = index;
        }
    }

    void SdfTextStyle::AddStroke(float radius, const Color4& color, int alphaThreshold)
    {
        float width = GetStrokeWidth(radius, alphaThreshold);
        if (width <= 0.0f)
            return;

        mShapeDilation += width;

        Layer stroke;
        stroke.color1 = stroke.color2 = color;
        Tap tap;
        tap.dilation = mShapeDilation;
        stroke.taps.Add(tap);
        mLayers.Add(stroke);
    }

    void SdfTextStyle::AddShadow(float blurRadius, const Vec2F& offset, const Color4& color)
    {
        Layer shadow;
        shadow.shadow = true;
        shadow.color1 = shadow.color2 = color;

        Tap main;
        main.offset = offset;
        main.dilation = mShapeDilation;
        main.blur = Math::Max(blurRadius, 0.0f);
        shadow.taps.Add(main);

        // The shadow is cast by the whole bitmap, the shadows under the glyph included
        Vector<Tap> casted;
        for (auto& layer : mLayers)
        {
            if (!layer.shadow)
                continue;

            for (auto& tap : layer.taps)
            {
                Tap copy = tap;
                copy.offset += offset;
                copy.blur = Math::Sqrt(tap.blur*tap.blur + main.blur*main.blur);
                copy.alpha = tap.alpha*layer.color1.AF();
                casted.Add(copy);
            }
        }

        casted.SortBy<float>([](const Tap& tap) { return -tap.alpha; });
        for (auto& tap : casted)
        {
            if (shadow.taps.Count() >= maxTaps)
                break;

            shadow.taps.Add(tap);
        }

        mLayers.Add(shadow);
    }

    void SdfTextStyle::Sharpen(float contrast, int threshold)
    {
        if (contrast <= 0.0f || Math::Equals(contrast, 1.0f))
            return;

        // An edge of 1 pixel, alpha a = 0.5 + d: the new half level is where threshold + (a - threshold)*contrast = 0.5
        float level = (float)threshold/255.0f;
        float shift = (level - 0.5f)*(1.0f - 1.0f/contrast);

        for (auto& layer : mLayers)
        {
            layer.edgeScale = Math::Max(layer.edgeScale/contrast, minEdgeScale);
            for (auto& tap : layer.taps)
            {
                tap.dilation -= shift;
                tap.blur /= contrast;
            }
        }

        mShapeDilation -= shift;
    }

    void SdfTextStyle::ExtendRasterBorder(const Vec2I& extend)
    {
        mRasterBorder.x = Math::Max(mRasterBorder.x, extend.x);
        mRasterBorder.y = Math::Max(mRasterBorder.y, extend.y);
    }

    const Vector<SdfTextStyle::Layer>& SdfTextStyle::GetLayers() const
    {
        return mLayers;
    }

    const Vector<SdfTextStyle::Gradient>& SdfTextStyle::GetGradients() const
    {
        return mGradients;
    }

    Vec2F SdfTextStyle::GetExtent() const
    {
        Vec2F extent;
        for (auto& layer : mLayers)
        {
            for (auto& tap : layer.taps)
            {
                float reach = Math::Max(tap.dilation + tap.blur*blurEdgeWidth, 0.0f) + 1.0f;
                extent.x = Math::Max(extent.x, reach + Math::Abs(tap.offset.x));
                extent.y = Math::Max(extent.y, reach + Math::Abs(tap.offset.y));
            }
        }

        return extent;
    }

    float SdfTextStyle::GetReach() const
    {
        float reach = 0.0f;
        for (auto& layer : mLayers)
        {
            for (auto& tap : layer.taps)
                reach = Math::Max(reach, tap.dilation + tap.blur*blurEdgeWidth);
        }

        return reach;
    }

    Vec2I SdfTextStyle::GetRasterBorder() const
    {
        return mRasterBorder + Vec2I(2, 2);
    }

    float SdfTextStyle::GetGradientCoordinate(int gradient, const Vec2F& point, const Vec2F& bitmapSize) const
    {
        if (gradient < 0 || gradient >= mGradients.Count())
            return 0.0f;

        const Gradient& g = mGradients[gradient];
        Vec2F direction = Vec2F::Rotated(Math::Deg2rad(g.angle + 90.0f));
        float size = Math::Equals(g.length, 0.0f) ? bitmapSize.Dot(direction) : g.length;
        if (Math::Abs(size) < 1e-6f)
            return 0.0f;

        return (point - g.origin*bitmapSize).Dot(direction)/size;
    }

    void SdfTextStyle::Pack(Vector<float>& data) const
    {
        data.Clear();
        data.resize(maxLayers*vectorsPerLayer*4, 0.0f);

        auto put = [&](int vector, float x, float y, float z, float w) {
            float* dst = &data[vector*4];
            dst[0] = x; dst[1] = y; dst[2] = z; dst[3] = w;
        };

        for (int i = 0; i < mLayers.Count() && i < maxLayers; i++)
        {
            const Layer& layer = mLayers[i];
            int base = i*vectorsPerLayer;
            put(base, layer.color1.RF(), layer.color1.GF(), layer.color1.BF(), layer.color1.AF());
            put(base + 1, layer.color2.RF(), layer.color2.GF(), layer.color2.BF(), layer.color2.AF());
            put(base + 2, (float)layer.gradient, (float)Math::Min(layer.taps.Count(), maxTaps), layer.edgeScale,
                layer.sharpCorners ? 1.0f : 0.0f);

            float alphas[maxTaps] = { 0.0f, 0.0f };
            for (int j = 0; j < layer.taps.Count() && j < maxTaps; j++)
            {
                const Tap& tap = layer.taps[j];
                put(base + 3 + j, tap.offset.x, tap.offset.y, tap.dilation, tap.blur*blurEdgeWidth);
                alphas[j] = tap.alpha;
            }

            put(base + 5, alphas[0], alphas[1], 0.0f, 0.0f);
        }
    }

    float SdfTextStyle::GetStrokeWidth(float radius, int alphaThreshold)
    {
        if (radius <= 0.0f)
            return 0.0f;

        // Bitmap::Outline at a straight edge of a glyph: a pixel gets the stroke by the mean distance to the solid
        // pixels in the square around it. The width is where the stroked alpha crosses 0.4 (its soft tail is seen as the
        // stroke too), from the edge, averaged over the edge's places in a pixel
        int m = Math::CeilToInt(radius);
        const float edges[] = { 0.125f, 0.375f, 0.625f, 0.875f };
        float widthSum = 0.0f;
        for (float edge : edges)
        {
            auto glyphCoverage = [&](int x) { return Math::Clamp01(edge - (float)x); };
            auto solid = [&](int x) { return (int)(glyphCoverage(x)*255.0f) > alphaThreshold; };
            auto alpha = [&](int x) {
                float squares = 0.0f;
                int count = 0;
                for (int dx = -m; dx <= m; dx++)
                {
                    if (!solid(x + dx))
                        continue;

                    squares += (float)((2*m + 1)*dx*dx);
                    count += 2*m + 1;
                    for (int dy = -m; dy <= m; dy++)
                        squares += (float)(dy*dy);
                }

                float stroke = 0.0f;
                if (count > 0)
                {
                    float distance = Math::Sqrt(squares/(float)count);
                    if (distance < radius + 1.0f)
                        stroke = Math::Clamp01(1.0f - (distance - radius));
                }

                float glyph = glyphCoverage(x);
                return glyph + (1.0f - glyph)*stroke;
            };

            // Pixel x has its centre at x + 0.5
            float crossing = edge;
            float previous = alpha(-1);
            for (int x = 0; x <= m + 2; x++)
            {
                float current = alpha(x);
                if (current < visibleAlpha)
                {
                    float from = (float)x - 0.5f;
                    crossing = previous > current ? from + (previous - visibleAlpha)/(previous - current) : from;
                    break;
                }

                previous = current;
            }

            widthSum += crossing - edge;
        }

        return Math::Max(widthSum/(float)(sizeof(edges)/sizeof(edges[0])), 0.0f);
    }

    bool SdfTextStyle::operator==(const SdfTextStyle& other) const
    {
        return mLayers == other.mLayers && mGradients == other.mGradients && mSupported == other.mSupported;
    }
}
