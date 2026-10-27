#include "o2/stdafx.h"
#include "VectorFontEffects.h"

#include "o2/Utils/Bitmap/Bitmap.h"

namespace o2
{
    FontStrokeEffect::FontStrokeEffect(float radius /*= 1.0f*/, const Color4& color /*= Color4::Black()*/,
                                       int alphaThreshold /*= 100*/):
        radius(radius), color(color), alphaThreshold(alphaThreshold)
    {}

    void FontStrokeEffect::Process(Bitmap& bitmap)
    {
        bitmap.Outline(radius, color, alphaThreshold);
    }

    Vec2I FontStrokeEffect::GetSizeExtend() const
    {
        return Vec2F::One()*radius;
    }

    bool FontStrokeEffect::IsEqual(VectorFont::Effect* other) const
    {
        if (!VectorFont::Effect::IsEqual(other))
            return false;

        FontStrokeEffect* otherEff = (FontStrokeEffect*)other;
        return Math::Equals(radius, otherEff->radius) && alphaThreshold == otherEff->alphaThreshold &&
            color == otherEff->color;
    }

    FontGradientEffect::FontGradientEffect(const Color4& color1 /*= Color4(200, 200, 200, 255)*/, 
                                           const Color4& color2 /*= Color4(255, 255, 255, 255)*/, 
                                           float angle /*= 0*/, float length /*= 0*/, Vec2F origin /*= Vec2F()*/):
        color1(color1), color2(color2), angle(angle), length(length), origin(origin)
    {}

    void FontGradientEffect::Process(Bitmap& bitmap)
    {
        bitmap.GradientByAlpha(color1, color2, angle, length, origin);
    }

    Vec2I FontGradientEffect::GetSizeExtend() const
    {
        return Vec2I();
    }

    bool FontGradientEffect::IsEqual(VectorFont::Effect* other) const
    {
        if (!VectorFont::Effect::IsEqual(other))
            return false;

        FontGradientEffect* otherEff = (FontGradientEffect*)other;
        return Math::Equals(angle, otherEff->angle) && Math::Equals(length, otherEff->length) &&
            color1 == otherEff->color1 && color2 == otherEff->color2 && origin == otherEff->origin;
    }

    FontColorEffect::FontColorEffect(const Color4& color /*= Color4::White()*/):
        color(color)
    {}

    void FontColorEffect::Process(Bitmap& bitmap)
    {
        bitmap.Colorise(color);
    }

    Vec2I FontColorEffect::GetSizeExtend() const
    {
        return Vec2I();
    }

    bool FontColorEffect::IsEqual(VectorFont::Effect* other) const
    {
        if (!VectorFont::Effect::IsEqual(other))
            return false;

        FontColorEffect* otherEff = (FontColorEffect*)other;
        return color == otherEff->color;
    }

    FontShadowEffect::FontShadowEffect(float blurRadius /*= 2.0f*/, const Vec2I offset /*= Vec2I(2, 2)*/, 
                                       const Color4& color /*= Color4::Black()*/):
        blurRadius(blurRadius), offset(offset), color(color)
    {}

    void FontShadowEffect::Process(Bitmap& bitmap)
    {
        Bitmap shadow(bitmap);
        shadow.Colorise(color);
        shadow.Blur(blurRadius);
        bitmap.BlendImage(shadow, offset);
    }

    Vec2I FontShadowEffect::GetSizeExtend() const
    {
        return offset + (Vec2I)(Vec2F::One()*blurRadius);
    }

    bool FontShadowEffect::IsEqual(VectorFont::Effect* other) const
    {
        if (!VectorFont::Effect::IsEqual(other))
            return false;

        FontShadowEffect* otherEff = (FontShadowEffect*)other;
        return Math::Equals(blurRadius, otherEff->blurRadius) && offset == otherEff->offset && 
            color == otherEff->color;
    }

    FontSharpenEffect::FontSharpenEffect(float contrast /*= 3.0f*/, int threshold /*= 128*/):
        contrast(contrast), threshold(threshold)
    {}

    void FontSharpenEffect::Process(Bitmap& bitmap)
    {
        UInt8* data = bitmap.GetData();
        Vec2I size = bitmap.GetSize();
        int bpp = bitmap.GetFormat() == PixelFormat::R8G8B8A8 ? 4 : 3;
        if (bpp != 4)
            return;

        for (int i = 0; i < size.x*size.y; i++)
        {
            float alpha = (float)data[i*4 + 3];
            data[i*4 + 3] = (UInt8)Math::Clamp(threshold + (alpha - threshold)*contrast, 0.0f, 255.0f);
        }
    }

    Vec2I FontSharpenEffect::GetSizeExtend() const
    {
        return Vec2I();
    }

    bool FontSharpenEffect::IsEqual(VectorFont::Effect* other) const
    {
        if (!VectorFont::Effect::IsEqual(other))
            return false;

        FontSharpenEffect* otherEff = (FontSharpenEffect*)other;
        return Math::Equals(contrast, otherEff->contrast) && threshold == otherEff->threshold;
    }
}
// --- META ---

DECLARE_CLASS(o2::FontStrokeEffect, o2__FontStrokeEffect);

DECLARE_CLASS(o2::FontGradientEffect, o2__FontGradientEffect);

DECLARE_CLASS(o2::FontColorEffect, o2__FontColorEffect);

DECLARE_CLASS(o2::FontShadowEffect, o2__FontShadowEffect);

DECLARE_CLASS(o2::FontSharpenEffect, o2__FontSharpenEffect);
// --- END META ---
