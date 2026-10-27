#include "o2Editor/stdafx.h"
#include "UiLayoutPixels.h"

#include <algorithm>
#include <cmath>

#include "o2Editor/Pipeline/PipelineValue.h"

namespace Editor::UiLayout
{
    UInt8 Clamp8(double v)
    {
        if (!(v > 0.0))
            return 0;
        if (v >= 255.0)
            return 255;

        double f = std::floor(v);
        double frac = v - f;
        if (frac < 0.5)
            return (UInt8)f;
        if (frac > 0.5)
            return (UInt8)(f + 1.0);
        return ((int)f % 2 == 0) ? (UInt8)f : (UInt8)(f + 1.0);
    }

    UiImage::UiImage(int width, int height):
        w(Math::Max(0, width)), h(Math::Max(0, height)), data((size_t)Math::Max(0, width)*Math::Max(0, height)*4, 0)
    {}

    UiImage UiImage::Blank(int width, int height, UInt8 r, UInt8 g, UInt8 b, UInt8 a)
    {
        UiImage img(width, height);
        if (r || g || b || a)
        {
            for (size_t i = 0; i < img.data.size(); i += 4)
            {
                img.data[i] = r; img.data[i + 1] = g; img.data[i + 2] = b; img.data[i + 3] = a;
            }
        }
        return img;
    }

    UiImage UiImage::FromBitmap(const Bitmap& bitmap)
    {
        auto rgba = EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(bitmap)));
        Vec2I size = rgba->GetSize();
        UiImage img(size.x, size.y);
        const UInt8* src = rgba->GetData();
        for (int y = 0; y < size.y; y++)
            std::copy(src + (size_t)(size.y - 1 - y)*size.x*4, src + (size_t)(size.y - y)*size.x*4, img.data.data() + (size_t)y*size.x*4);
        return img;
    }

    bool UiImage::Decode(const String& bytes, UiImage& out)
    {
        auto bitmap = DecodeImageBytes(bytes);
        if (!bitmap)
            return false;

        out = FromBitmap(*bitmap);
        return !out.IsEmpty();
    }

    Ref<Bitmap> UiImage::ToBitmap() const
    {
        auto bitmap = mmake<Bitmap>(PixelFormat::R8G8B8A8, Vec2I(Math::Max(1, w), Math::Max(1, h)));
        UInt8* dst = bitmap->GetData();
        for (int y = 0; y < h; y++)
            std::copy(data.data() + (size_t)y*w*4, data.data() + (size_t)(y + 1)*w*4, dst + (size_t)(h - 1 - y)*w*4);
        return bitmap;
    }

    String UiImage::Png() const
    {
        return EncodeBitmapPng(*ToBitmap());
    }

    UiImage UiImage::Crop(int x, int y, int width, int height) const
    {
        UiImage out(Math::Max(1, width), Math::Max(1, height));
        for (int yy = 0; yy < height; yy++)
        {
            int sy = y + yy;
            if (sy < 0 || sy >= h)
                continue;

            for (int xx = 0; xx < width; xx++)
            {
                int sx = x + xx;
                if (sx < 0 || sx >= w)
                    continue;

                const UInt8* s = At(sx, sy);
                UInt8* d = out.At(xx, yy);
                d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
            }
        }
        return out;
    }

    static double Lanczos3(double x)
    {
        x = std::fabs(x);
        if (x < 1e-9)
            return 1.0;
        if (x >= 3.0)
            return 0.0;

        double px = Math::PI()*x;
        return 3.0*std::sin(px)*std::sin(px/3.0)/(px*px);
    }

    // One axis of a separable lanczos3 pass over premultiplied float pixels
    static void ResampleLine(const float* src, int srcLen, int srcStride, float* dst, int dstLen, int dstStride)
    {
        double scale = (double)srcLen/dstLen;
        double filterScale = Math::Max(1.0, scale);
        double support = 3.0*filterScale;
        std::vector<double> weights;
        for (int i = 0; i < dstLen; i++)
        {
            double center = (i + 0.5)*scale;
            int left = Math::Max(0, (int)std::floor(center - support));
            int right = Math::Min(srcLen - 1, (int)std::ceil(center + support));
            weights.assign(right - left + 1, 0.0);
            double sum = 0.0;
            for (int j = left; j <= right; j++)
            {
                double wgt = Lanczos3((j + 0.5 - center)/filterScale);
                weights[j - left] = wgt;
                sum += wgt;
            }

            double acc[4] = { 0, 0, 0, 0 };
            if (std::fabs(sum) > 1e-12)
            {
                for (int j = left; j <= right; j++)
                {
                    double wgt = weights[j - left]/sum;
                    const float* s = src + (size_t)j*srcStride;
                    acc[0] += s[0]*wgt; acc[1] += s[1]*wgt; acc[2] += s[2]*wgt; acc[3] += s[3]*wgt;
                }
            }

            float* d = dst + (size_t)i*dstStride;
            d[0] = (float)acc[0]; d[1] = (float)acc[1]; d[2] = (float)acc[2]; d[3] = (float)acc[3];
        }
    }

    UiImage UiImage::Resize(int width, int height, bool nearest /*= false*/) const
    {
        width = Math::Max(1, width);
        height = Math::Max(1, height);
        if (width == w && height == h)
            return *this;

        UiImage out(width, height);
        if (IsEmpty())
            return out;

        if (nearest)
        {
            for (int y = 0; y < height; y++)
            {
                int sy = Math::Min(h - 1, (int)std::floor((y + 0.5)*h/height));
                for (int x = 0; x < width; x++)
                {
                    int sx = Math::Min(w - 1, (int)std::floor((x + 0.5)*w/width));
                    const UInt8* s = At(sx, sy);
                    UInt8* d = out.At(x, y);
                    d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
                }
            }
            return out;
        }

        std::vector<float> premul((size_t)w*h*4);
        for (size_t i = 0; i < (size_t)w*h; i++)
        {
            float a = data[i*4 + 3]/255.0f;
            premul[i*4] = data[i*4]*a; premul[i*4 + 1] = data[i*4 + 1]*a; premul[i*4 + 2] = data[i*4 + 2]*a;
            premul[i*4 + 3] = data[i*4 + 3];
        }

        std::vector<float> horizontal((size_t)width*h*4);
        for (int y = 0; y < h; y++)
            ResampleLine(premul.data() + (size_t)y*w*4, w, 4, horizontal.data() + (size_t)y*width*4, width, 4);

        std::vector<float> vertical((size_t)width*height*4);
        for (int x = 0; x < width; x++)
            ResampleLine(horizontal.data() + (size_t)x*4, h, width*4, vertical.data() + (size_t)x*4, height, width*4);

        for (size_t i = 0; i < (size_t)width*height; i++)
        {
            float a = vertical[i*4 + 3];
            UInt8* d = out.data.data() + i*4;
            if (a <= 0.0f)
            {
                d[0] = d[1] = d[2] = d[3] = 0;
                continue;
            }

            float k = 255.0f/a;
            d[0] = Clamp8(vertical[i*4]*k); d[1] = Clamp8(vertical[i*4 + 1]*k); d[2] = Clamp8(vertical[i*4 + 2]*k);
            d[3] = Clamp8(a);
        }
        return out;
    }

    void UiImage::Blit(const UiImage& src, int x, int y, double opacity /*= 1.0*/)
    {
        for (int yy = 0; yy < src.h; yy++)
        {
            int dy = y + yy;
            if (dy < 0 || dy >= h)
                continue;

            for (int xx = 0; xx < src.w; xx++)
            {
                int dx = x + xx;
                if (dx < 0 || dx >= w)
                    continue;

                const UInt8* s = src.At(xx, yy);
                UInt8* d = At(dx, dy);
                double a = (s[3]/255.0)*opacity;
                if (a <= 0)
                    continue;

                double da = d[3]/255.0;
                double oa = a + da*(1 - a);
                for (int c = 0; c < 3; c++)
                    d[c] = Clamp8((s[c]*a + d[c]*da*(1 - a))/Math::Max(oa, 1e-6));
                d[3] = Clamp8(oa*255);
            }
        }
    }

    UiImage UiImage::Flatten(UInt8 r, UInt8 g, UInt8 b) const
    {
        UiImage out = *this;
        UInt8 bg[3] = { r, g, b };
        for (size_t i = 0; i < (size_t)w*h; i++)
        {
            UInt8* d = out.data.data() + i*4;
            double a = data[i*4 + 3]/255.0;
            for (int c = 0; c < 3; c++)
                d[c] = Clamp8(data[i*4 + c]*a + bg[c]*(1 - a));
            d[3] = 255;
        }
        return out;
    }

    Box IntBox(const Box& b)
    {
        double x0 = std::floor(b.x), y0 = std::floor(b.y);
        return Box(x0, y0, Math::Max(1.0, std::ceil(b.x + b.w) - x0), Math::Max(1.0, std::ceil(b.y + b.h) - y0));
    }

    Box Grow(const Box& b, double dx, double dy)
    {
        return Box(b.x - dx, b.y - dy, b.w + 2*dx, b.h + 2*dy);
    }

    Box ClampBox(const Box& b, double w, double h)
    {
        double x0 = Math::Max(0.0, Math::Min(w, b.x)), y0 = Math::Max(0.0, Math::Min(h, b.y));
        double x1 = Math::Max(x0, Math::Min(w, b.x + b.w)), y1 = Math::Max(y0, Math::Min(h, b.y + b.h));
        return Box(x0, y0, x1 - x0, y1 - y0);
    }

    double Inter(const Box& a, const Box& b)
    {
        double w = Math::Min(a.x + a.w, b.x + b.w) - Math::Max(a.x, b.x);
        double h = Math::Min(a.y + a.h, b.y + b.h) - Math::Max(a.y, b.y);
        return w > 0 && h > 0 ? w*h : 0.0;
    }

    double Iou(const Box& a, const Box& b)
    {
        double i = Inter(a, b);
        return i/Math::Max(1e-6, a.w*a.h + b.w*b.h - i);
    }

    double Coverage(const Box& inner, const Box& outer)
    {
        return Inter(inner, outer)/Math::Max(1e-6, inner.w*inner.h);
    }

    bool Contains(const Box& outer, const Box& inner, double slack /*= 0.0*/)
    {
        return inner.x >= outer.x - slack && inner.y >= outer.y - slack &&
            inner.x + inner.w <= outer.x + outer.w + slack && inner.y + inner.h <= outer.y + outer.h + slack;
    }

    String HexOf(const Rgb& rgb)
    {
        static const char* hex = "0123456789ABCDEF";
        String out = "#";
        for (int i = 0; i < 3; i++)
        {
            int v = (int)Math::Max(0.0, Math::Min(255.0, JsRound(rgb[i])));
            out += hex[v >> 4];
            out += hex[v & 15];
        }
        return out;
    }

    bool ParseHex(const String& text, Rgb& out)
    {
        String s = text.Trimed(" \t\n\r\f\v");
        if (s.StartsWith("#"))
            s = s.SubStr(1);

        if (s.Length() != 6)
            return false;

        int v = 0;
        for (int i = 0; i < 6; i++)
        {
            char c = s[i];
            int d = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
            if (d < 0)
                return false;
            v = v*16 + d;
        }

        out = Rgb((v >> 16) & 255, (v >> 8) & 255, v & 255);
        return true;
    }

    double Median(std::vector<double> values)
    {
        if (values.empty())
            return 0.0;

        std::sort(values.begin(), values.end());
        size_t m = values.size() >> 1;
        return values.size() % 2 ? values[m] : (values[m - 1] + values[m])/2;
    }

    Rgb RingMedian(const UiImage& img, const Box& b)
    {
        Box bb = IntBox(ClampBox(b, img.w, img.h));
        std::vector<double> r, g, bl;
        auto push = [&](int x, int y)
        {
            const UInt8* p = img.At(x, y);
            r.push_back(p[0]); g.push_back(p[1]); bl.push_back(p[2]);
        };
        int x0 = (int)bb.x, y0 = (int)bb.y, x1 = (int)(bb.x + bb.w), y1 = (int)(bb.y + bb.h);
        for (int x = x0; x < x1; x++) { push(x, y0); push(x, y1 - 1); }
        for (int y = y0; y < y1; y++) { push(x0, y); push(x1 - 1, y); }
        return Rgb(Median(r), Median(g), Median(bl));
    }

    double ColorDist(const Rgb& a, const Rgb& b)
    {
        double dr = a.r - b.r, dg = a.g - b.g, db = a.b - b.b;
        return std::sqrt(dr*dr + dg*dg + db*db);
    }

    std::vector<float> Luma(const UiImage& img)
    {
        std::vector<float> out((size_t)img.w*img.h);
        const UInt8* d = img.data.data();
        for (size_t i = 0; i < out.size(); i++)
            out[i] = (float)(0.299*d[i*4] + 0.587*d[i*4 + 1] + 0.114*d[i*4 + 2]);
        return out;
    }

    std::vector<float> Gradient(const std::vector<float>& l, int w, int h)
    {
        std::vector<float> out((size_t)w*h, 0.0f);
        for (int y = 1; y < h - 1; y++)
        {
            for (int x = 1; x < w - 1; x++)
            {
                int i = y*w + x;
                float gx = l[i + 1] - l[i - 1], gy = l[i + w] - l[i - w];
                out[i] = std::sqrt(gx*gx + gy*gy);
            }
        }
        return out;
    }

    void Components(const std::vector<UInt8>& mask, int w, int h, std::vector<int>& labels, std::vector<Component>& comps)
    {
        labels.assign((size_t)w*h, 0);
        comps.clear();
        std::vector<int> stack;
        int next = 0;
        for (int i = 0; i < w*h; i++)
        {
            if (!mask[i] || labels[i])
                continue;

            next++;
            int area = 0, x0 = w, y0 = h, x1 = -1, y1 = -1;
            labels[i] = next;
            stack.push_back(i);
            while (!stack.empty())
            {
                int p = stack.back();
                stack.pop_back();
                int x = p % w, y = p / w;
                area++;
                if (x < x0) x0 = x;
                if (x > x1) x1 = x;
                if (y < y0) y0 = y;
                if (y > y1) y1 = y;

                int nb[4] = { x > 0 ? p - 1 : -1, x < w - 1 ? p + 1 : -1, y > 0 ? p - w : -1, y < h - 1 ? p + w : -1 };
                for (int q : nb)
                {
                    if (q >= 0 && mask[q] && !labels[q])
                    {
                        labels[q] = next;
                        stack.push_back(q);
                    }
                }
            }

            Component comp;
            comp.area = area;
            comp.box = Box(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
            comp.label = next;
            comps.push_back(comp);
        }
    }

    bool AlphaBounds(const UiImage& img, int min, Box& out)
    {
        int x0 = img.w, y0 = img.h, x1 = -1, y1 = -1;
        for (int y = 0; y < img.h; y++)
        {
            const UInt8* row = img.data.data() + (size_t)y*img.w*4;
            for (int x = 0; x < img.w; x++)
            {
                if (row[x*4 + 3] >= min)
                {
                    if (x < x0) x0 = x;
                    if (x > x1) x1 = x;
                    if (y < y0) y0 = y;
                    if (y > y1) y1 = y;
                }
            }
        }

        if (x1 < 0)
            return false;

        out = Box(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
        return true;
    }
}
