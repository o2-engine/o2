#include "o2Editor/stdafx.h"
#include "PipelineImageOps.h"

#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/PipelineValue.h"

#include <cmath>
#include <cstring>

namespace Editor::PipelineImageOps
{
    namespace
    {
        inline UInt8 Clamp255(float v) { return (UInt8)(v < 0.0f ? 0 : v > 255.0f ? 255 : (int)(v + 0.5f)); }
        inline float Clamp01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }

        struct RawImage
        {
            int width = 0, height = 0;
            Vector<float> data; // RGBA floats 0..255, image space rows top to bottom

            RawImage() = default;
            RawImage(int w, int h): width(w), height(h) { data.Resize(w * h * 4); std::memset(data.Data(), 0, sizeof(float) * w * h * 4); }

            float* At(int x, int y) { return data.Data() + (y * width + x) * 4; }
            const float* At(int x, int y) const { return data.Data() + (y * width + x) * 4; }
        };

        RawImage ToRaw(const Bitmap& bmp)
        {
            auto rgba = EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(bmp)));
            Vec2I size = rgba->GetSize();
            RawImage raw(size.x, size.y);
            for (int y = 0; y < size.y; y++)
            {
                for (int x = 0; x < size.x; x++)
                {
                    const UInt8* p = Pixel(*rgba, x, y);
                    float* d = raw.At(x, y);
                    d[0] = p[0]; d[1] = p[1]; d[2] = p[2]; d[3] = p[3];
                }
            }
            return raw;
        }

        Ref<Bitmap> FromRaw(const RawImage& raw)
        {
            auto bmp = mmake<Bitmap>(PixelFormat::R8G8B8A8, Vec2I(raw.width, raw.height));
            for (int y = 0; y < raw.height; y++)
            {
                for (int x = 0; x < raw.width; x++)
                {
                    const float* s = raw.At(x, y);
                    UInt8* p = Pixel(*bmp, x, y);
                    p[0] = Clamp255(s[0]); p[1] = Clamp255(s[1]); p[2] = Clamp255(s[2]); p[3] = Clamp255(s[3]);
                }
            }
            return bmp;
        }

        // Chamfer distance transform: distance to the nearest solid pixel (or transparent when inside)
        Vector<float> AlphaDistance(const RawImage& raw, float threshold, bool inside)
        {
            int w = raw.width, h = raw.height;
            Vector<float> d;
            d.Resize(w * h);
            const float FAR = 1e9f;
            for (int i = 0; i < w * h; i++)
            {
                float a = raw.data[i * 4 + 3];
                bool solid = inside ? a < threshold : a >= threshold;
                d[i] = solid ? 0.0f : FAR;
            }

            const float D1 = 1.0f, D2 = 1.41421356f;
            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    int i = y * w + x;
                    if (d[i] == 0.0f) continue;
                    float best = d[i];
                    if (y > 0)
                    {
                        if (x > 0) best = Math::Min(best, d[i - w - 1] + D2);
                        best = Math::Min(best, d[i - w] + D1);
                        if (x < w - 1) best = Math::Min(best, d[i - w + 1] + D2);
                    }
                    if (x > 0) best = Math::Min(best, d[i - 1] + D1);
                    d[i] = best;
                }
            }
            for (int y = h - 1; y >= 0; y--)
            {
                for (int x = w - 1; x >= 0; x--)
                {
                    int i = y * w + x;
                    if (d[i] == 0.0f) continue;
                    float best = d[i];
                    if (y < h - 1)
                    {
                        if (x < w - 1) best = Math::Min(best, d[i + w + 1] + D2);
                        best = Math::Min(best, d[i + w] + D1);
                        if (x > 0) best = Math::Min(best, d[i + w - 1] + D2);
                    }
                    if (x < w - 1) best = Math::Min(best, d[i + 1] + D1);
                    d[i] = best;
                }
            }
            return d;
        }

        // Separable gaussian blur of a single channel mask
        void BlurMask(Vector<float>& mask, int w, int h, float sigma)
        {
            if (sigma <= 0.01f)
                return;

            int radius = Math::Max(1, (int)std::ceil(sigma * 3.0f));
            Vector<float> kernel;
            kernel.Resize(radius * 2 + 1);
            float sum = 0.0f;
            for (int i = -radius; i <= radius; i++)
            {
                float v = std::exp(-(float)(i * i) / (2.0f * sigma * sigma));
                kernel[i + radius] = v;
                sum += v;
            }
            for (auto& k : kernel) k /= sum;

            Vector<float> tmp;
            tmp.Resize(w * h);
            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    float acc = 0.0f;
                    for (int i = -radius; i <= radius; i++)
                    {
                        int sx = Math::Clamp(x + i, 0, w - 1);
                        acc += mask[y * w + sx] * kernel[i + radius];
                    }
                    tmp[y * w + x] = acc;
                }
            }
            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    float acc = 0.0f;
                    for (int i = -radius; i <= radius; i++)
                    {
                        int sy = Math::Clamp(y + i, 0, h - 1);
                        acc += tmp[sy * w + x] * kernel[i + radius];
                    }
                    mask[y * w + x] = acc;
                }
            }
        }

        RawImage PadCanvas(const RawImage& src, int l, int t, int r, int b)
        {
            if (!l && !t && !r && !b)
                return src;

            RawImage out(src.width + l + r, src.height + t + b);
            for (int y = 0; y < src.height; y++)
                std::memcpy(out.At(l, y + t), src.At(0, y), sizeof(float) * 4 * src.width);
            return out;
        }

        void RgbToHsl(float r, float g, float b, float& h, float& s, float& l)
        {
            r /= 255.0f; g /= 255.0f; b /= 255.0f;
            float max = Math::Max(r, Math::Max(g, b));
            float min = Math::Min(r, Math::Min(g, b));
            l = (max + min) / 2.0f;
            if (max == min) { h = 0; s = 0; return; }
            float d = max - min;
            s = l > 0.5f ? d / (2.0f - max - min) : d / (max + min);
            if (max == r) h = ((g - b) / d + (g < b ? 6.0f : 0.0f)) / 6.0f;
            else if (max == g) h = ((b - r) / d + 2.0f) / 6.0f;
            else h = ((r - g) / d + 4.0f) / 6.0f;
        }

        float Hue2Rgb(float p, float q, float t)
        {
            if (t < 0) t += 1;
            if (t > 1) t -= 1;
            if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
            if (t < 0.5f) return q;
            if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
            return p;
        }

        void HslToRgb(float h, float s, float l, float& r, float& g, float& b)
        {
            if (s == 0) { r = g = b = l * 255.0f; return; }
            float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
            float p = 2 * l - q;
            r = Hue2Rgb(p, q, h + 1.0f / 3.0f) * 255.0f;
            g = Hue2Rgb(p, q, h) * 255.0f;
            b = Hue2Rgb(p, q, h - 1.0f / 3.0f) * 255.0f;
        }

        // Bilinear sample in image space, transparent outside
        void SampleBilinear(const Bitmap& src, float fx, float fy, float* out)
        {
            Vec2I size = src.GetSize();
            int x0 = (int)std::floor(fx), y0 = (int)std::floor(fy);
            float tx = fx - x0, ty = fy - y0;
            float acc[4] = { 0, 0, 0, 0 };
            float wsum = 0.0f;
            for (int j = 0; j < 2; j++)
            {
                for (int i = 0; i < 2; i++)
                {
                    int sx = x0 + i, sy = y0 + j;
                    float wgt = (i ? tx : 1 - tx) * (j ? ty : 1 - ty);
                    if (sx < 0 || sy < 0 || sx >= size.x || sy >= size.y)
                    {
                        wsum += wgt;
                        continue;
                    }
                    const UInt8* p = Pixel(src, sx, sy);
                    float a = p[3] * wgt;
                    acc[0] += p[0] * a; acc[1] += p[1] * a; acc[2] += p[2] * a; acc[3] += a;
                    wsum += wgt;
                }
            }
            if (acc[3] > 0.0001f)
            {
                out[0] = acc[0] / acc[3]; out[1] = acc[1] / acc[3]; out[2] = acc[2] / acc[3];
            }
            else
                out[0] = out[1] = out[2] = 0;
            out[3] = wsum > 0 ? acc[3] / wsum : 0;
        }
    }

    bool ParseCrop(const DataValue* value, CropRect& crop)
    {
        crop = CropRect();
        if (!value || !value->IsObject())
            return false;

        auto num = [&](const char* key, float def)
        {
            auto m = value->FindMember(key);
            return m ? Clamp01(PipelineUtils::ValueToNumber(*m, def)) : def;
        };
        crop.x = num("x", 0); crop.y = num("y", 0); crop.w = num("w", 1); crop.h = num("h", 1);
        bool partial = crop.x > 0.001f || crop.y > 0.001f || crop.w < 0.999f || crop.h < 0.999f;
        return partial && crop.w > 0 && crop.h > 0;
    }

    Ref<Bitmap> Crop(const Bitmap& src, const CropRect& crop)
    {
        Vec2I size = src.GetSize();
        int left = Math::Clamp((int)std::lround(crop.x * size.x), 0, size.x - 1);
        int top = Math::Clamp((int)std::lround(crop.y * size.y), 0, size.y - 1);
        int w = Math::Max(1, Math::Min(size.x - left, (int)std::lround(crop.w * size.x)));
        int h = Math::Max(1, Math::Min(size.y - top, (int)std::lround(crop.h * size.y)));
        return CropPixels(src, left, top, w, h);
    }

    Ref<Bitmap> CropPixels(const Bitmap& srcIn, int leftIn, int topIn, int widthIn, int heightIn)
    {
        auto src = EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(srcIn)));
        Vec2I size = src->GetSize();
        int left = Math::Clamp(leftIn, 0, size.x);
        int top = Math::Clamp(topIn, 0, size.y);
        int right = Math::Clamp(leftIn + widthIn, left, size.x);
        int bottom = Math::Clamp(topIn + heightIn, top, size.y);
        int w = Math::Max(1, right - left), h = Math::Max(1, bottom - top);
        auto out = mmake<Bitmap>(PixelFormat::R8G8B8A8, Vec2I(w, h));
        std::memset(out->GetData(), 0, w * h * 4);
        for (int y = 0; y < h; y++)
        {
            if (top + y >= size.y) break;
            int copyW = Math::Min(w, size.x - left);
            if (copyW > 0)
                std::memcpy(Pixel(*out, 0, y), Pixel(*src, left, top + y), copyW * 4);
        }
        return out;
    }

    Ref<Bitmap> CropToContent(const Bitmap& srcIn, ContentMode mode, int pad /*= 0*/)
    {
        auto src = EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(srcIn)));
        Vec2I size = src->GetSize();
        int minX = size.x, minY = size.y, maxX = -1, maxY = -1;
        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                const UInt8* p = Pixel(*src, x, y);
                if (p[3] <= 16) continue;
                if (mode == ContentMode::White && p[0] > 244 && p[1] > 244 && p[2] > 244) continue;
                minX = Math::Min(minX, x); maxX = Math::Max(maxX, x);
                minY = Math::Min(minY, y); maxY = Math::Max(maxY, y);
            }
        }
        if (maxX < minX || maxY < minY)
            return src;

        minX = Math::Max(0, minX - pad); minY = Math::Max(0, minY - pad);
        maxX = Math::Min(size.x - 1, maxX + pad); maxY = Math::Min(size.y - 1, maxY + pad);
        return CropPixels(*src, minX, minY, maxX - minX + 1, maxY - minY + 1);
    }

    Ref<Bitmap> Resize(const Bitmap& srcIn, const Vec2I& sizeIn)
    {
        auto src = EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(srcIn)));
        Vec2I size(Math::Max(1, sizeIn.x), Math::Max(1, sizeIn.y));
        if (src->GetSize() == size)
            return src;

        Vec2I srcSize = src->GetSize();
        // Box downsampling when shrinking a lot keeps thin lines from vanishing
        if (srcSize.x >= size.x * 2 && srcSize.y >= size.y * 2)
        {
            auto out = mmake<Bitmap>(PixelFormat::R8G8B8A8, size);
            float sx = (float)srcSize.x / size.x, sy = (float)srcSize.y / size.y;
            for (int y = 0; y < size.y; y++)
            {
                int y0 = (int)(y * sy), y1 = Math::Max(y0 + 1, (int)((y + 1) * sy));
                for (int x = 0; x < size.x; x++)
                {
                    int x0 = (int)(x * sx), x1 = Math::Max(x0 + 1, (int)((x + 1) * sx));
                    float acc[4] = { 0, 0, 0, 0 };
                    int n = 0;
                    for (int yy = y0; yy < y1 && yy < srcSize.y; yy++)
                    {
                        for (int xx = x0; xx < x1 && xx < srcSize.x; xx++)
                        {
                            const UInt8* p = Pixel(*src, xx, yy);
                            float a = p[3];
                            acc[0] += p[0] * a; acc[1] += p[1] * a; acc[2] += p[2] * a; acc[3] += a;
                            n++;
                        }
                    }
                    UInt8* d = Pixel(*out, x, y);
                    if (acc[3] > 0.0f)
                    {
                        d[0] = Clamp255(acc[0] / acc[3]); d[1] = Clamp255(acc[1] / acc[3]); d[2] = Clamp255(acc[2] / acc[3]);
                    }
                    else
                        d[0] = d[1] = d[2] = 0;
                    d[3] = n ? Clamp255(acc[3] / n) : 0;
                }
            }
            return out;
        }

        return src->Resized(size);
    }

    bool HasContent(const Bitmap& src)
    {
        if (src.GetFormat() != PixelFormat::R8G8B8A8)
            return true;

        Vec2I size = src.GetSize();
        const UInt8* data = src.GetData();
        for (int i = 0; i < size.x * size.y; i++)
        {
            if (data[i * 4 + 3] > 0)
                return true;
        }
        return false;
    }

    Ref<Bitmap> CompositeOverlay(const Bitmap& baseIn, const Bitmap& overlayIn)
    {
        auto base = EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(baseIn)));
        auto overlay = Resize(overlayIn, base->GetSize());
        Vec2I size = base->GetSize();
        auto out = mmake<Bitmap>(*base);
        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                const UInt8* o = Pixel(*overlay, x, y);
                UInt8* d = Pixel(*out, x, y);
                float oa = o[3] / 255.0f;
                if (oa <= 0.0f) continue;
                float da = d[3] / 255.0f;
                float a = oa + da * (1 - oa);
                for (int c = 0; c < 3; c++)
                    d[c] = Clamp255(a > 0 ? (o[c] * oa + d[c] * da * (1 - oa)) / a : 0);
                d[3] = Clamp255(a * 255.0f);
            }
        }
        return out;
    }

    Ref<Bitmap> TwoPassMatte(const Bitmap& whiteIn, const Bitmap& blackIn)
    {
        auto white = EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(whiteIn)));
        auto black = Resize(blackIn, white->GetSize());
        Vec2I size = white->GetSize();
        auto out = mmake<Bitmap>(PixelFormat::R8G8B8A8, size);
        const float bgDist = std::sqrt(3.0f * 255.0f * 255.0f);
        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                const UInt8* w = Pixel(*white, x, y);
                const UInt8* b = Pixel(*black, x, y);
                UInt8* d = Pixel(*out, x, y);
                float dr = (float)w[0] - b[0], dg = (float)w[1] - b[1], db = (float)w[2] - b[2];
                float dist = std::sqrt(dr * dr + dg * dg + db * db);
                float alpha = Clamp01(1.0f - dist / bgDist);
                if (alpha > 0.01f)
                {
                    float inv = 1.0f / alpha;
                    d[0] = Clamp255(b[0] * inv); d[1] = Clamp255(b[1] * inv); d[2] = Clamp255(b[2] * inv);
                }
                else
                    d[0] = d[1] = d[2] = 0;
                d[3] = Clamp255(alpha * 255.0f);
            }
        }
        return out;
    }

    Ref<Bitmap> ChromaKey(const Bitmap& srcIn, const ChromaOptions& opt)
    {
        auto src = EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(srcIn)));
        Vec2I size = src->GetSize();
        auto out = mmake<Bitmap>(PixelFormat::R8G8B8A8, size);

        auto chroma = [](float r, float g, float b, float& cb, float& cr)
        {
            cb = -0.168736f * r - 0.331264f * g + 0.5f * b;
            cr = 0.5f * r - 0.418688f * g - 0.081312f * b;
        };

        float keyCb, keyCr;
        chroma((float)opt.color.r, (float)opt.color.g, (float)opt.color.b, keyCb, keyCr);
        float t0 = (opt.tolerance / 100.0f) * 140.0f;
        float t1 = t0 + Math::Max(1.0f, (opt.softness / 100.0f) * 140.0f);
        float spill = opt.spill / 100.0f;
        int keyMax = Math::Max(opt.color.r, Math::Max(opt.color.g, opt.color.b));
        int dom = keyMax == opt.color.g ? 1 : keyMax == opt.color.b ? 2 : 0;

        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                const UInt8* p = Pixel(*src, x, y);
                UInt8* d = Pixel(*out, x, y);
                float ch[3] = { (float)p[0], (float)p[1], (float)p[2] };
                float cb, cr;
                chroma(ch[0], ch[1], ch[2], cb, cr);
                float dist = std::sqrt((cb - keyCb) * (cb - keyCb) + (cr - keyCr) * (cr - keyCr));
                float alpha = dist <= t0 ? 0.0f : dist >= t1 ? 1.0f : (dist - t0) / (t1 - t0);

                if (alpha > 0 && spill > 0)
                {
                    float others[2];
                    int k = 0;
                    for (int c = 0; c < 3; c++) if (c != dom) others[k++] = ch[c];
                    float avg = (others[0] + others[1]) / 2.0f;
                    if (ch[dom] > avg)
                    {
                        float amount = spill * (1.0f - alpha * 0.5f);
                        ch[dom] = ch[dom] + (avg - ch[dom]) * amount;
                    }
                }

                d[0] = Clamp255(ch[0]); d[1] = Clamp255(ch[1]); d[2] = Clamp255(ch[2]);
                d[3] = Clamp255(alpha * p[3]);
            }
        }
        return out;
    }

    Ref<Bitmap> Outline(const Bitmap& src, const OutlineOptions& opt)
    {
        float width = Math::Max(0.0f, opt.width);
        if (width <= 0.0f)
            return EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(src)));

        float softness = Math::Max(0.0f, opt.softness);
        float opacity = Clamp01(opt.opacity);
        float outward = opt.position == "outside" ? width : opt.position == "center" ? width / 2 : 0;
        int pad = (int)std::ceil(outward + softness);

        RawImage raw = PadCanvas(ToRaw(src), pad, pad, pad, pad);
        int w = raw.width, h = raw.height;
        auto outer = AlphaDistance(raw, 128.0f, false);
        Vector<float> inner;
        if (opt.position != "outside")
            inner = AlphaDistance(raw, 128.0f, true);

        float outerReach = opt.position == "outside" ? width : opt.position == "center" ? width / 2 : 0;
        float innerReach = opt.position == "inside" ? width : opt.position == "center" ? width / 2 : 0;
        auto cover = [&](float d, float reach, float soft)
        {
            if (reach <= 0) return 0.0f;
            if (d <= reach) return 1.0f;
            return Clamp01(1.0f - (d - reach) / (soft > 0 ? soft : 1.0f));
        };

        RawImage out(w, h);
        for (int i = 0; i < w * h; i++)
        {
            const float* s = raw.data.Data() + i * 4;
            float* d = out.data.Data() + i * 4;
            float srcA = s[3] / 255.0f;
            float band = outer[i] > 0 ? cover(outer[i], outerReach, softness) : (inner.Count() ? cover(inner[i], innerReach, softness) : 0.0f);
            float strokeA = Clamp01(band) * opacity;
            float r, g, b, a;
            if (opt.position == "inside")
            {
                float sa = strokeA * srcA;
                a = srcA;
                r = s[0] * (1 - sa) + opt.color.r * sa;
                g = s[1] * (1 - sa) + opt.color.g * sa;
                b = s[2] * (1 - sa) + opt.color.b * sa;
            }
            else
            {
                a = srcA + strokeA * (1 - srcA);
                if (a <= 0) { r = g = b = 0; }
                else
                {
                    r = (s[0] * srcA + opt.color.r * strokeA * (1 - srcA)) / a;
                    g = (s[1] * srcA + opt.color.g * strokeA * (1 - srcA)) / a;
                    b = (s[2] * srcA + opt.color.b * strokeA * (1 - srcA)) / a;
                }
            }
            d[0] = r; d[1] = g; d[2] = b; d[3] = a * 255.0f;
        }
        return FromRaw(out);
    }

    static Ref<Bitmap> InnerShadow(const RawImage& raw, const ShadowOptions& opt, int dx, int dy)
    {
        int w = raw.width, h = raw.height;
        Vector<float> mask;
        mask.Resize(w * h);
        if (opt.spread > 0)
        {
            auto dist = AlphaDistance(raw, 128.0f, true);
            for (int i = 0; i < w * h; i++)
                mask[i] = dist[i] <= opt.spread ? 255.0f : Math::Max(255.0f - raw.data[i * 4 + 3], 0.0f);
        }
        else
        {
            for (int i = 0; i < w * h; i++)
                mask[i] = 255.0f - raw.data[i * 4 + 3];
        }
        if (opt.blur > 0)
            BlurMask(mask, w, h, Math::Max(0.3f, opt.blur / 2.0f));

        RawImage out(w, h);
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                const float* s = raw.At(x, y);
                float* d = out.At(x, y);
                int sx = x - dx, sy = y - dy;
                float m = (sx >= 0 && sx < w && sy >= 0 && sy < h) ? mask[sy * w + sx] / 255.0f : 1.0f;
                float shadowA = m * opt.opacity * (s[3] / 255.0f);
                d[0] = s[0] * (1 - shadowA) + opt.color.r * shadowA;
                d[1] = s[1] * (1 - shadowA) + opt.color.g * shadowA;
                d[2] = s[2] * (1 - shadowA) + opt.color.b * shadowA;
                d[3] = s[3];
            }
        }
        return FromRaw(out);
    }

    Ref<Bitmap> Shadow(const Bitmap& src, const ShadowOptions& optIn)
    {
        ShadowOptions opt = optIn;
        opt.opacity = Clamp01(opt.opacity);
        if (opt.opacity <= 0)
            return EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(src)));

        float rad = opt.angle * Math::PI() / 180.0f;
        int dx = (int)std::lround(std::cos(rad) * opt.distance);
        int dy = (int)std::lround(std::sin(rad) * opt.distance);
        float blur = Math::Max(0.0f, opt.blur);
        float spread = Math::Max(0.0f, opt.spread);

        if (opt.inner)
            return InnerShadow(ToRaw(src), opt, dx, dy);

        int reach = (int)std::ceil(spread + blur * 2 + 2);
        RawImage raw = PadCanvas(ToRaw(src), reach + Math::Max(0, -dx), reach + Math::Max(0, -dy), reach + Math::Max(0, dx), reach + Math::Max(0, dy));
        int w = raw.width, h = raw.height;

        Vector<float> mask;
        mask.Resize(w * h);
        if (spread > 0)
        {
            auto dist = AlphaDistance(raw, 128.0f, false);
            for (int i = 0; i < w * h; i++)
                mask[i] = dist[i] <= spread ? 255.0f : Math::Clamp((1.0f - (dist[i] - spread)) * 255.0f, 0.0f, 255.0f);
        }
        else
        {
            for (int i = 0; i < w * h; i++)
                mask[i] = raw.data[i * 4 + 3];
        }
        if (blur > 0)
            BlurMask(mask, w, h, Math::Max(0.3f, blur / 2.0f));

        RawImage out(w, h);
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                const float* s = raw.At(x, y);
                float* d = out.At(x, y);
                int sx = x - dx, sy = y - dy;
                float shadowA = (sx >= 0 && sx < w && sy >= 0 && sy < h) ? (mask[sy * w + sx] / 255.0f) * opt.opacity : 0.0f;
                float srcA = s[3] / 255.0f;
                float a = srcA + shadowA * (1 - srcA);
                if (a <= 0) { d[0] = d[1] = d[2] = d[3] = 0; continue; }
                d[0] = (s[0] * srcA + opt.color.r * shadowA * (1 - srcA)) / a;
                d[1] = (s[1] * srcA + opt.color.g * shadowA * (1 - srcA)) / a;
                d[2] = (s[2] * srcA + opt.color.b * shadowA * (1 - srcA)) / a;
                d[3] = a * 255.0f;
            }
        }
        return FromRaw(out);
    }

    static float BlendChannel(const String& mode, float base, float top)
    {
        float b = base / 255.0f, t = top / 255.0f, v;
        if (mode == "multiply") v = b * t;
        else if (mode == "screen") v = 1 - (1 - b) * (1 - t);
        else if (mode == "overlay") v = b < 0.5f ? 2 * b * t : 1 - 2 * (1 - b) * (1 - t);
        else v = t;
        return v * 255.0f;
    }

    Ref<Bitmap> Gradient(const Bitmap& src, const GradientOptions& opt)
    {
        RawImage raw = ToRaw(src);
        int w = raw.width, h = raw.height;
        RawImage out(w, h);
        float rad = opt.angle * Math::PI() / 180.0f;
        float ux = std::cos(rad), uy = std::sin(rad);
        float span = std::abs(ux) * w + std::abs(uy) * h;
        if (span <= 0) span = 1;
        float originX = ux < 0 ? (float)w : 0, originY = uy < 0 ? (float)h : 0;
        float cx = w / 2.0f, cy = h / 2.0f;
        float maxR = std::sqrt((float)(w * w + h * h)) / 2.0f;
        if (maxR <= 0) maxR = 1;
        float strength = Clamp01(opt.opacity);
        bool isMap = opt.blend == "map";
        String mode = isMap ? "normal" : opt.blend;

        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                const float* s = raw.At(x, y);
                float* d = out.At(x, y);
                float srcA = s[3] / 255.0f;
                float t;
                if (isMap) t = (0.2126f * s[0] + 0.7152f * s[1] + 0.0722f * s[2]) / 255.0f;
                else if (opt.kind == "radial") t = Clamp01(std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy)) / maxR);
                else t = Clamp01(((x - originX) * ux + (y - originY) * uy) / span);

                float gr = opt.color1.r + (opt.color2.r - opt.color1.r) * t;
                float gg = opt.color1.g + (opt.color2.g - opt.color1.g) * t;
                float gb = opt.color1.b + (opt.color2.b - opt.color1.b) * t;
                float ga = (opt.alpha1 + (opt.alpha2 - opt.alpha1) * t) * strength;
                float k = Clamp01(ga);

                float mixed[3] = { BlendChannel(mode, s[0], gr), BlendChannel(mode, s[1], gg), BlendChannel(mode, s[2], gb) };
                float over[3] = { s[0] + (mixed[0] - s[0]) * k, s[1] + (mixed[1] - s[1]) * k, s[2] + (mixed[2] - s[2]) * k };

                if (opt.clipToAlpha)
                {
                    d[0] = over[0]; d[1] = over[1]; d[2] = over[2]; d[3] = s[3];
                    continue;
                }

                float outA = srcA + k * (1 - srcA);
                float grad[3] = { gr, gg, gb };
                for (int c = 0; c < 3; c++)
                    d[c] = outA <= 0 ? 0 : (over[c] * srcA + grad[c] * k * (1 - srcA)) / outA;
                d[3] = outA * 255.0f;
            }
        }
        return FromRaw(out);
    }

    Ref<Bitmap> AdjustColor(const Bitmap& src, const ColorOptions& opt)
    {
        RawImage raw = ToRaw(src);
        int w = raw.width, h = raw.height;
        RawImage out(w, h);
        float tintH, tintS, tintL;
        RgbToHsl((float)opt.tint.r, (float)opt.tint.g, (float)opt.tint.b, tintH, tintS, tintL);
        float tintK = Clamp01(opt.tintStrength);
        float brightness = Math::Clamp(opt.brightness, -100.0f, 100.0f) / 100.0f;
        float contrast = Math::Clamp(opt.contrast, -100.0f, 100.0f) / 100.0f;
        float cFactor = (259.0f * (contrast * 255.0f + 255.0f)) / (255.0f * (259.0f - contrast * 255.0f));
        float satK = 1.0f + Math::Clamp(opt.saturation, -100.0f, 100.0f) / 100.0f;
        float hueShift = Math::Clamp(opt.hue, -360.0f, 360.0f) / 360.0f;

        for (int i = 0; i < w * h; i++)
        {
            const float* s = raw.data.Data() + i * 4;
            float* d = out.data.Data() + i * 4;
            float r = s[0], g = s[1], b = s[2];
            if (opt.invert) { r = 255 - r; g = 255 - g; b = 255 - b; }
            if (brightness != 0) { float add = brightness * 255.0f; r += add; g += add; b += add; }
            if (contrast != 0)
            {
                r = cFactor * (r - 128) + 128; g = cFactor * (g - 128) + 128; b = cFactor * (b - 128) + 128;
            }
            r = Math::Clamp(r, 0.0f, 255.0f); g = Math::Clamp(g, 0.0f, 255.0f); b = Math::Clamp(b, 0.0f, 255.0f);

            if (hueShift != 0 || satK != 1 || opt.colorize)
            {
                float hh, ss, ll;
                RgbToHsl(r, g, b, hh, ss, ll);
                if (opt.colorize) { hh = tintH; ss = tintS; }
                else { hh = std::fmod(hh + hueShift + 1.0f, 1.0f); ss = Clamp01(ss * satK); }
                HslToRgb(hh, ss, ll, r, g, b);
            }

            if (!opt.colorize && tintK > 0)
            {
                r = r + (opt.tint.r - r) * tintK; g = g + (opt.tint.g - g) * tintK; b = b + (opt.tint.b - b) * tintK;
            }

            if (opt.grayscale)
            {
                float yv = 0.2126f * r + 0.7152f * g + 0.0722f * b;
                r = g = b = yv;
            }

            d[0] = r; d[1] = g; d[2] = b; d[3] = s[3];
        }
        return FromRaw(out);
    }

    Ref<Bitmap> Blank(int width, int height, const Color4& color /*= Color4(0, 0, 0, 0)*/)
    {
        auto bmp = mmake<Bitmap>(PixelFormat::R8G8B8A8, Vec2I(Math::Max(1, width), Math::Max(1, height)));
        UInt8* data = bmp->GetData();
        int count = bmp->GetSize().x * bmp->GetSize().y;
        for (int i = 0; i < count; i++)
        {
            data[i * 4] = Clamp255((float)color.r);
            data[i * 4 + 1] = Clamp255((float)color.g);
            data[i * 4 + 2] = Clamp255((float)color.b);
            data[i * 4 + 3] = Clamp255((float)color.a);
        }
        return bmp;
    }

    Ref<Bitmap> Flip(const Bitmap& srcIn, bool horizontal, bool vertical)
    {
        auto src = EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(srcIn)));
        if (!horizontal && !vertical)
            return src;

        Vec2I size = src->GetSize();
        auto out = mmake<Bitmap>(PixelFormat::R8G8B8A8, size);
        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                int sx = horizontal ? size.x - 1 - x : x;
                int sy = vertical ? size.y - 1 - y : y;
                std::memcpy(Pixel(*out, x, y), Pixel(*src, sx, sy), 4);
            }
        }
        return out;
    }

    Ref<Bitmap> Rotate(const Bitmap& srcIn, float degrees)
    {
        auto src = EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(srcIn)));
        float rot = std::fmod(std::fmod(degrees, 360.0f) + 360.0f, 360.0f);
        if (rot < 0.01f)
            return src;

        Vec2I size = src->GetSize();
        float rad = rot * Math::PI() / 180.0f;
        float cs = std::cos(rad), sn = std::sin(rad);
        int outW = (int)std::ceil(std::abs(size.x * cs) + std::abs(size.y * sn));
        int outH = (int)std::ceil(std::abs(size.x * sn) + std::abs(size.y * cs));
        auto out = mmake<Bitmap>(PixelFormat::R8G8B8A8, Vec2I(Math::Max(1, outW), Math::Max(1, outH)));
        float ocx = outW / 2.0f, ocy = outH / 2.0f, scx = size.x / 2.0f, scy = size.y / 2.0f;
        for (int y = 0; y < outH; y++)
        {
            for (int x = 0; x < outW; x++)
            {
                // Inverse rotation of the output pixel center into source space (clockwise on screen)
                float px = x + 0.5f - ocx, py = y + 0.5f - ocy;
                float sx = px * cs + py * sn + scx - 0.5f;
                float sy = -px * sn + py * cs + scy - 0.5f;
                float c[4];
                SampleBilinear(*src, sx, sy, c);
                UInt8* d = Pixel(*out, x, y);
                d[0] = Clamp255(c[0]); d[1] = Clamp255(c[1]); d[2] = Clamp255(c[2]); d[3] = Clamp255(c[3]);
            }
        }
        return out;
    }

    Ref<Bitmap> WithOpacity(const Bitmap& srcIn, float opacity)
    {
        auto out = EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(srcIn)));
        if (opacity >= 0.999f)
            return out;

        Vec2I size = out->GetSize();
        UInt8* data = out->GetData();
        for (int i = 0; i < size.x * size.y; i++)
            data[i * 4 + 3] = Clamp255(data[i * 4 + 3] * Clamp01(opacity));
        return out;
    }

    Ref<Bitmap> NineSliceResize(const Bitmap& srcIn, int targetW, int targetH, const NineSlice& slice, float scale)
    {
        auto src = EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(srcIn)));
        Vec2I size = src->GetSize();
        int tw = Math::Max(1, targetW), th = Math::Max(1, targetH);

        auto fit = [](int a, int b, int limit, int& x, int& y)
        {
            x = Math::Max(0, a); y = Math::Max(0, b);
            if (x + y > limit - 1)
            {
                float k = (float)(limit - 1) / (float)Math::Max(1, x + y);
                x = (int)(x * k); y = (int)(y * k);
            }
        };

        int l, r, t, b;
        fit(slice.l, slice.r, size.x, l, r);
        fit(slice.t, slice.b, size.y, t, b);
        float k = Math::Clamp(scale, 0.05f, 8.0f);
        int dl, dr, dt, db;
        fit((int)std::lround(l * k), (int)std::lround(r * k), tw, dl, dr);
        fit((int)std::lround(t * k), (int)std::lround(b * k), th, dt, db);

        int cols[3][4] = { { 0, l, 0, dl }, { l, size.x - l - r, dl, tw - dl - dr }, { size.x - r, r, tw - dr, dr } };
        int rows[3][4] = { { 0, t, 0, dt }, { t, size.y - t - b, dt, th - dt - db }, { size.y - b, b, th - db, db } };

        auto out = Blank(tw, th);
        for (auto& col : cols)
        {
            if (col[1] <= 0 || col[3] <= 0) continue;
            for (auto& row : rows)
            {
                if (row[1] <= 0 || row[3] <= 0) continue;
                auto piece = CropPixels(*src, col[0], row[0], col[1], row[1]);
                auto scaled = Resize(*piece, Vec2I(col[3], row[3]));
                for (int y = 0; y < row[3]; y++)
                {
                    int dy = row[2] + y;
                    if (dy < 0 || dy >= th) continue;
                    int copyW = Math::Min(col[3], tw - col[2]);
                    if (copyW > 0)
                        std::memcpy(Pixel(*out, col[2], dy), Pixel(*scaled, 0, y), copyW * 4);
                }
            }
        }
        return out;
    }

    Ref<Bitmap> ComposeLayers(int canvasW, int canvasH, const Vector<PlacedLayer>& layers, const Color4* background)
    {
        int cw = Math::Max(1, canvasW), ch = Math::Max(1, canvasH);
        auto out = Blank(cw, ch, background ? Color4(background->r, background->g, background->b, 255) : Color4(0, 0, 0, 0));

        for (auto& layer : layers)
        {
            if (!layer.image)
                continue;

            int w = Math::Max(1, (int)std::lround(layer.w)), h = Math::Max(1, (int)std::lround(layer.h));
            Ref<Bitmap> buf = layer.nine ? NineSliceResize(*layer.image, w, h, layer.slice, layer.sliceScale)
                : Resize(*layer.image, Vec2I(w, h));
            buf = Flip(*buf, layer.flipH, layer.flipV);
            buf = WithOpacity(*buf, layer.opacity);
            buf = Rotate(*buf, layer.rotation);

            Vec2I bs = buf->GetSize();
            float cx = layer.x + layer.w / 2.0f, cy = layer.y + layer.h / 2.0f;
            int left = (int)std::lround(cx - bs.x / 2.0f), top = (int)std::lround(cy - bs.y / 2.0f);

            for (int y = 0; y < bs.y; y++)
            {
                int dy = top + y;
                if (dy < 0 || dy >= ch) continue;
                for (int x = 0; x < bs.x; x++)
                {
                    int dx = left + x;
                    if (dx < 0 || dx >= cw) continue;
                    const UInt8* s = Pixel(*buf, x, y);
                    UInt8* d = Pixel(*out, dx, dy);
                    float sa = s[3] / 255.0f;
                    if (sa <= 0) continue;
                    float da = d[3] / 255.0f;
                    float a = sa + da * (1 - sa);
                    for (int c = 0; c < 3; c++)
                        d[c] = Clamp255(a > 0 ? (s[c] * sa + d[c] * da * (1 - sa)) / a : 0);
                    d[3] = Clamp255(a * 255.0f);
                }
            }
        }
        return out;
    }
}
// --- META ---

ENUM_META(Editor::PipelineImageOps::ContentMode, Editor__PipelineImageOps__ContentMode)
{
    ENUM_ENTRY(Alpha);
    ENUM_ENTRY(White);
}
END_ENUM_META;
// --- END META ---
