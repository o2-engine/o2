#include "o2/stdafx.h"
#include "Color.h"

#include "o2/Utils/Reflection/Type.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Serialization/DataValue.h"

namespace o2
{
    Color4::Color4():
        r(255), g(255), b(255), a(255)
    {}

    Color4::Color4(int r, int g, int b, int a) :
        r(r), g(g), b(b), a(a)
    {}

    Color4::Color4(float r, float g, float b, float a)
    {
        this->r = (int)(r*255.0f);
        this->g = (int)(g*255.0f);
        this->b = (int)(b*255.0f);
        this->a = (int)(a*255.0f);
    }

    Color4::Color4(const b2Color& color):
        Color4(color.r, color.g, color.b, 1.0f)
    {}

    Color4::operator b2Color() const
    {
        return b2Color(RF(), GF(), BF());
    }

    bool Color4::operator==(const Color4& color) const
    {
        return a == color.a && r == color.r && g == color.g && b == color.b;
    }

    bool Color4::operator!=(const Color4& color) const
    {
        return a != color.a || r != color.r || g != color.g || b != color.b;
    }

    Color4 Color4::operator+(const Color4& v) const
    {
        return Color4(r + v.r, g + v.g, b + v.b, a + v.a);
    }

    Color4 Color4::operator+=(const Color4& v)
    {
        r += v.r; g += v.g; b += v.b; a += v.a;
        return *this;
    }

    Color4 Color4::operator-(const Color4& v) const
    {
        return Color4(r - v.r, g - v.g, b - v.b, a - v.a);
    }

    Color4 Color4::operator-=(const Color4& v)
    {
        r -= v.r; g -= v.g; b -= v.b; a -= v.a;
        return *this;
    }

    Color4 Color4::operator*(float v) const
    {
        return Color4((int)((float)r*v), (int)((float)g*v), (int)((float)b*v), (int)((float)a*v));
    }

    Color4 Color4::operator*=(float v)
    {
        *this = (*this)*v;
        return *this;
    }

    Color4 Color4::operator/(float v) const
    {
        return *this*(1.0f/v);
    }

    Color4 Color4::operator/=(float v)
    {
        *this = *this/v;
        return *this;
    }

    Color4 Color4::operator*(const Color4& v) const
    {
        return Color4(r*v.r/255, g*v.g/255, b*v.b/255, a*v.a/255);
    }

    Color4 Color4::operator/(const Color4& v) const
    {
        return Color4((int)(255.0f*(RF()/v.RF())), (int)(255.0f*(GF()/v.GF())),
                      (int)(255.0f*(BF()/v.BF())), (int)(255.0f*(AF()/v.AF())));
    }

    Color4 Color4::operator/=(const Color4& v)
    {
        *this = *this/v;
        return *this;
    }

    Color4 Color4::operator*=(const Color4& v)
    {
        *this = *this*v;
        return *this;
    }

    float Color4::RF() const
    {
        return (float)r/255.0f;
    }

    float Color4::GF() const
    {
        return (float)g/255.0f;
    }

    float Color4::BF() const
    {
        return (float)b/255.0f;
    }

    float Color4::AF() const
    {
        return (float)a/255.0f;
    }

    void Color4::SetRF(float value)
    {
        r = (int)(value*255.0f);
    }

    void Color4::SetGF(float value)
    {
        g = (int)(value*255.0f);
    }

    void Color4::SetBF(float value)
    {
        b = (int)(value*255.0f);
    }

    void Color4::SetAF(float value)
    {
        a = (int)(value*255.0f);
    }

    float Color4::GetF(int i) const
    {
        return (float)(*(int*)(this + sizeof(int)*i))/255.0f;
    }

    int& Color4::Get(int i) const
    {
        return (*(int*)(this + sizeof(int)*i));
    }

    void Color4::Normalize()
    {
        r = Math::Clamp(r, 0, 255);
        g = Math::Clamp(g, 0, 255);
        b = Math::Clamp(b, 0, 255);
        a = Math::Clamp(a, 0, 255);
    }

    Color4 Color4::Normalized() const
    {
        return Color4(Math::Clamp(r, 0, 255),
                      Math::Clamp(g, 0, 255),
                      Math::Clamp(b, 0, 255),
                      Math::Clamp(a, 0, 255));
    }

    Color32Bit Color4::ARGB() const
    {
        return (Color32Bit)((a << 24) | (r << 16) | (g << 8) | b);
    }

    Color32Bit Color4::ABGR() const
    {
        return (Color32Bit)((a << 24) | (b << 16) | (g << 8) | r);
    }

    Color32Bit Color4::RGBA() const
    {
        return (Color32Bit)((r << 24) | (g << 16) | (b << 8) | a);
    }

    void Color4::SetARGB(Color32Bit color)
    {
        a = (int)(color >> 24);
        r = (int)((color << 8) >> 24);
        g = (int)((color << 16) >> 24);
        b = (int)((color << 24) >> 24);
    }

    void Color4::SetABGR(Color32Bit color)
    {
        a = (int)(color >> 24);
        b = (int)((color << 8) >> 24);
        g = (int)((color << 16) >> 24);
        r = (int)((color << 24) >> 24);
    }

    void Color4::SetRGBA(Color32Bit color)
    {
        r = (int)(color >> 24);
        g = (int)((color << 8) >> 24);
        b = (int)((color << 16) >> 24);
        a = (int)((color << 24) >> 24);
    }

    void Color4::SetHSL(float hue, float saturation, float lightness)
    {
        if (saturation == 0)
        {
            r = (int)(lightness*255.0f);
            g = (int)(lightness*255.0f);
            b = (int)(lightness*255.0f);
        }
        else
        {
            auto hue2rgb = [](float p, float q, float t)
            {
                if (t < 0.0f) t += 1.0f;
                if (t > 1.0f) t -= 1.0f;
                if (t < 1.0f/6.0f) return p + (q - p)*6.0f * t;
                if (t < 1.0f/2.0f) return q;
                if (t < 2.0f/3.0f) return p + (q - p)*(2.0f/3.0f - t)*6.0f;
                return p;
            };

            float q = lightness < 0.5f ? lightness*(1.0f + saturation) : lightness + saturation - lightness*saturation;
            float p = 2.0f*lightness - q;

            r = (int)(hue2rgb(p, q, hue + 1.0f/3.0f)*255.0f);
            g = (int)(hue2rgb(p, q, hue)*255.0f);
            b = (int)(hue2rgb(p, q, hue - 1.0f/3.0f)*255.0f);
        }
    }

    void Color4::ToHSL(float& hue, float& saturation, float& lightness) const 
    {
        float r = this->r/255.0f;
        float g = this->g/255.0f;
        float b = this->b/255.0f;

        float max = Math::Max(r, Math::Max(g, b)), min = Math::Min(r, Math::Min(g, b));
        lightness = (max + min)*0.5f;

        if (Math::Equals(max, min))
            hue = saturation = 0;
        else
        {
            float d = max - min;
            saturation = lightness > 0.5f ? d / (2.0f - max - min) : d / (max + min);

            if (Math::Equals(max, r))
                hue = (g - b) / d + (g < b ? 6 : 0);
            else if (Math::Equals(max, g))
                hue = (b - r) / d + 2;
            else if (Math::Equals(max, b))
                hue = (r - g) / d + 4;

            hue /= 6.0f;
        }
    }

    Color4& Color4::ChangeHue(float offset)
    {
        float h, s, l;
        ToHSL(h, s, l);
        h = Math::Mod(h + offset, 1.0f);
        SetHSL(h, s, l);

        return *this;
    }

    Color4& Color4::ChangeSaturation(float offset)
    {
        float h, s, l;
        ToHSL(h, s, l);
        s = Math::Clamp(s + offset, 0.0f, 1.0f);
        SetHSL(h, s, l);

        return *this;
    }

    Color4& Color4::ChangeLightness(float offset)
    {
        float h, s, l;
        ToHSL(h, s, l);
        l = Math::Clamp(l + offset, 0.0f, 1.0f);
        SetHSL(h, s, l);

        return *this;
    }

    Color4 Color4::BlendByAlpha(const Color4& other)
    {
        float a1 = AF(), a2 = other.AF();
        float inva1 = 1.0f - a1;

        return Color4(int(a1*(float)r + a2*inva1*(float)other.r),
                      int(a1*(float)g + a2*inva1*(float)other.g),
                      int(a1*(float)b + a2*inva1*(float)other.b),
                      int(255.0f*(a1 + a2*inva1)));
    }

    Color4 Color4::White()
    {
        return Color4(255, 255, 255, 255);
    }

    Color4 Color4::Black()
    {
        return Color4(0, 0, 0, 255);
    }

    Color4 Color4::Red()
    {
        return Color4(255, 0, 0, 255);
    }

    Color4 Color4::Blue()
    {
        return Color4(0, 0, 255, 255);
    }

    Color4 Color4::Green()
    {
        return Color4(0, 255, 0, 255);
    }

    Color4 Color4::Gray()
    {
        return Color4(150, 150, 150, 255);
    }

    Color4 Color4::HSL(float hue, float saturation, float lightness, float alpha /*= 1.0f*/)
    {
        Color4 res;
        res.SetHSL(hue, saturation, lightness);
        res.SetAF(alpha);

        return res;
    }

    Color4 Color4::SomeColor(int idx, int alpha /*= 255*/)
    {
        Color4 res = Color4::White();

        float fi = (float)idx;
        float cornersOnCircle = 3;
        float cycleOffset = 0.11f;
        float startOffset = 0.25f;

        float currCorner = Math::Mod(fi, cornersOnCircle)/cornersOnCircle;
        float currCycle = Math::Floor(fi/cornersOnCircle);
        res.SetHSL(Math::Mod(startOffset + currCorner + currCycle*cycleOffset, 1.0f), 0.7f, 0.6f);
        res.a = alpha;
        return res;
    }

}
