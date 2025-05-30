#pragma once

#include "o2/Render/VectorFont.h"

namespace o2
{
    // ------------------
    // Font stroke effect
    // ------------------
    class FontStrokeEffect: public VectorFont::Effect
    {
    public:
        float  radius;         // Stroke radius @SERIALIZABLE
        int    alphaThreshold; // Stroke alpha threshold @SERIALIZABLE
        Color4 color;          // Color @SERIALIZABLE

    public:
        // Constructor
        FontStrokeEffect(float radius = 1.0f, const Color4& color = Color4(0, 0, 0, 150), int alphaThreshold = 100);

        // Process bitmap with glyph
        void Process(Bitmap& bitmap) override;

        // Returns bitmap extending size
        Vec2I GetSizeExtend() const override;

        // Check effects equals
        bool IsEqual(VectorFont::Effect* other) const override;

        SERIALIZABLE(FontStrokeEffect);
        CLONEABLE_REF(FontStrokeEffect);
    };

    // -----------------------
    // Gradient filling effect
    // -----------------------
    class FontGradientEffect: public VectorFont::Effect
    {
    public:
        Color4 color1; // First color @SERIALIZABLE
        Color4 color2; // Second color @SERIALIZABLE
        float  angle;  // Angle in degrees @SERIALIZABLE
        float  length; // Length. 0 means maximum size @SERIALIZABLE
        Vec2F  origin; // Relative origin @SERIALIZABLE
        
    public:
        // Constructor
        FontGradientEffect(const Color4& color1 = Color4(200, 200, 200, 255),
                           const Color4& color2 = Color4(255, 255, 255, 255),
                           float angle = 0, float length = 0, Vec2F origin = Vec2F());

        // Process bitmap with glyph
        void Process(Bitmap& bitmap) override;

        // Returns bitmap extending size
        Vec2I GetSizeExtend() const override;

        // Check effects equals
        bool IsEqual(VectorFont::Effect* other) const override;

		SERIALIZABLE(FontGradientEffect);
		CLONEABLE_REF(FontGradientEffect);
    };

    class FontColorEffect: public VectorFont::Effect
    {
    public:
        Color4 color; // @SERIALIZABLE

    public:
        // Constructor
        FontColorEffect(const Color4& color = Color4::White());

        // Process bitmap with glyph
        void Process(Bitmap& bitmap) override;

        // Returns bitmap extending size
        Vec2I GetSizeExtend() const override;

        // Check effects equals
        bool IsEqual(VectorFont::Effect* other) const override;

		SERIALIZABLE(FontColorEffect);
		CLONEABLE_REF(FontColorEffect);
    };

    class FontShadowEffect: public VectorFont::Effect
    {
    public:
        float  blurRadius; // Bluring radius @SERIALIZABLE
        Vec2I  offset;     // Offset @SERIALIZABLE
        Color4 color;      // Shadow color @SERIALIZABLE

    public:
        // Constructor
        FontShadowEffect(float blurRadius = 2.0f, const Vec2I offset = Vec2I(2, 2), 
                         const Color4& color = Color4(0, 0, 0, 100));

        // Process bitmap with glyph
        void Process(Bitmap& bitmap) override;

        // Returns bitmap extending size
        Vec2I GetSizeExtend() const override;

        // Check effects equals
        bool IsEqual(VectorFont::Effect* other) const override;

		SERIALIZABLE(FontShadowEffect);
		CLONEABLE_REF(FontShadowEffect);
    };
}
// --- META ---

CLASS_BASES_META(o2::FontStrokeEffect)
{
    BASE_CLASS(o2::VectorFont::Effect);
}
END_META;
CLASS_FIELDS_META(o2::FontStrokeEffect)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(radius);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(alphaThreshold);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(color);
}
END_META;
CLASS_METHODS_META(o2::FontStrokeEffect)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(float, const Color4&, int);
    FUNCTION().PUBLIC().SIGNATURE(void, Process, Bitmap&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetSizeExtend);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEqual, VectorFont::Effect*);
}
END_META;

CLASS_BASES_META(o2::FontGradientEffect)
{
    BASE_CLASS(o2::VectorFont::Effect);
}
END_META;
CLASS_FIELDS_META(o2::FontGradientEffect)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(color1);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(color2);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(angle);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(length);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(origin);
}
END_META;
CLASS_METHODS_META(o2::FontGradientEffect)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(const Color4&, const Color4&, float, float, Vec2F);
    FUNCTION().PUBLIC().SIGNATURE(void, Process, Bitmap&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetSizeExtend);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEqual, VectorFont::Effect*);
}
END_META;

CLASS_BASES_META(o2::FontColorEffect)
{
    BASE_CLASS(o2::VectorFont::Effect);
}
END_META;
CLASS_FIELDS_META(o2::FontColorEffect)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(color);
}
END_META;
CLASS_METHODS_META(o2::FontColorEffect)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(void, Process, Bitmap&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetSizeExtend);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEqual, VectorFont::Effect*);
}
END_META;

CLASS_BASES_META(o2::FontShadowEffect)
{
    BASE_CLASS(o2::VectorFont::Effect);
}
END_META;
CLASS_FIELDS_META(o2::FontShadowEffect)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(blurRadius);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(offset);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(color);
}
END_META;
CLASS_METHODS_META(o2::FontShadowEffect)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(float, const Vec2I, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(void, Process, Bitmap&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetSizeExtend);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEqual, VectorFont::Effect*);
}
END_META;
// --- END META ---
