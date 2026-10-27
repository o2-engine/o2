#include "o2Editor/stdafx.h"
#include "UiLayoutSlice.h"

#include <algorithm>
#include <cmath>

namespace Editor::UiLayout
{
    Registration Register(const UiImage& render, const UiImage& crop, const Box& rough, const Vector<Box>& erase)
    {
        int w = crop.w, h = crop.h;
        Registration identity;
        Box rb;
        if (!AlphaBounds(render, 128, rb))
            return identity;

        auto Lr = Luma(render), Lc = Luma(crop);
        auto alpha = [&](int x, int y) { return render.data[(size_t)(y*render.w + x)*4 + 3]; };
        auto solid = [&](int x, int y)
        {
            return x > 0 && y > 0 && x < render.w - 1 && y < render.h - 1 && alpha(x, y) > 230 && alpha(x - 1, y) > 230 &&
                alpha(x + 1, y) > 230 && alpha(x, y - 1) > 230 && alpha(x, y + 1) > 230;
        };

        int step = (int)Math::Max(1.0, JsRound(std::sqrt((rb.w*rb.h)/6000)));
        struct Point { double x, y; float l; };
        std::vector<Point> solidPts;
        for (int y = 0; y < render.h; y += step)
        {
            for (int x = 0; x < render.w; x += step)
            {
                if (solid(x, y))
                    solidPts.push_back({ (double)x, (double)y, Lr[(size_t)y*render.w + x] });
            }
        }

        if (solidPts.size() < 20)
            return identity;

        // The render's outline: at the right place the design has its own edge there, a render shrunk into the element's flat
        // middle fits its colours but not its edge
        auto Gm = Gradient(Lc, w, h);
        auto opaque = [&](int x, int y) { return x >= 0 && y >= 0 && x < render.w && y < render.h && alpha(x, y) >= 128; };
        std::vector<Point> contour;
        for (int y = 0; y < render.h; y++)
        {
            for (int x = 0; x < render.w; x++)
            {
                if (opaque(x, y) && (!opaque(x - 1, y) || !opaque(x + 1, y) || !opaque(x, y - 1) || !opaque(x, y + 1)))
                    contour.push_back({ (double)x, (double)y, 0.0f });
            }
        }

        size_t cstep = (size_t)Math::Max(1.0, JsRound(contour.size()/600.0));
        std::vector<Point> contourPts;
        for (size_t i = 0; i < contour.size(); i += cstep)
            contourPts.push_back(contour[i]);

        auto regOf = [&](const Box& t)
        {
            double sx = t.w/rb.w, sy = t.h/rb.h;
            return Registration{ sx, sy, t.x - rb.x*sx, t.y - rb.y*sy };
        };

        // Mean absolute luminance difference with each side's mean taken out, over the render's solid area (labels left out);
        // points off the crop count as a miss
        std::vector<float> pairs;
        pairs.reserve(solidPts.size()*2);
        auto score = [&](const Box& t) -> double
        {
            if (t.w < 4 || t.h < 4)
                return -1e9;

            Registration r = regOf(t);
            int n = 0, miss = 0;
            double sr = 0, sc = 0;
            pairs.clear();
            for (auto& p : solidPts)
            {
                int cx = (int)JsRound(p.x*r.sx + r.tx), cy = (int)JsRound(p.y*r.sy + r.ty);
                if (cx < 0 || cy < 0 || cx >= w || cy >= h)
                {
                    miss++;
                    continue;
                }

                bool erased = false;
                for (auto& b : erase)
                {
                    if (cx >= b.x - 1 && cx < b.x + b.w + 1 && cy >= b.y - 1 && cy < b.y + b.h + 1)
                    {
                        erased = true;
                        break;
                    }
                }
                if (erased)
                    continue;

                float b = Lc[(size_t)cy*w + cx];
                pairs.push_back(p.l);
                pairs.push_back(b);
                sr += p.l;
                sc += b;
                n++;
            }

            if (n < 20)
                return -1e9;

            double off = (sc - sr)/n, mad = 0;
            for (size_t i = 0; i < pairs.size(); i += 2)
                mad += std::fabs(pairs[i + 1] - pairs[i] - off);

            // The edge may sit a pixel either side of the contour: the strongest counts
            double edge = 0;
            for (auto& p : contourPts)
            {
                int cx = (int)JsRound(p.x*r.sx + r.tx), cy = (int)JsRound(p.y*r.sy + r.ty);
                float g = 0;
                for (int d = -1; d <= 1; d++)
                {
                    for (int e = -1; e <= 1; e++)
                    {
                        int xx = cx + d, yy = cy + e;
                        if (xx >= 0 && yy >= 0 && xx < w && yy < h && Gm[(size_t)yy*w + xx] > g)
                            g = Gm[(size_t)yy*w + xx];
                    }
                }
                edge += Math::Min((double)g, 80.0);
            }

            return -(mad/n) + 0.35*(edge/Math::Max((size_t)1, contourPts.size())) -
                40.0*((double)miss/Math::Max((size_t)1, solidPts.size()));
        };

        bool reframed = rb.w < 0.8*rough.w || rb.w > 1.25*rough.w || rb.h < 0.8*rough.h || rb.h > 1.25*rough.h;
        double s0 = score(rb);
        Box best = rb;
        double bs = s0;
        auto sweep = [&](const Box& c, const std::vector<double>& ks, double range, double stepSize)
        {
            for (double k : ks)
            {
                double tw = c.w*k, th = c.h*k;
                for (double dy = -range; dy <= range; dy += stepSize)
                {
                    for (double dx = -range; dx <= range; dx += stepSize)
                    {
                        Box t(c.x + (c.w - tw)/2 + dx, c.y + (c.h - th)/2 + dy, tw, th);
                        double v = score(t);
                        if (v > bs)
                        {
                            bs = v;
                            best = t;
                        }
                    }
                }
            }
        };

        std::vector<double> ks;
        for (int i = 0; i < 21; i++)
            ks.push_back(0.82 + i*0.02);

        double range = Math::Max(3.0, JsRound(Math::Max(rb.w, rb.h)*0.06));
        sweep(rb, ks, range, 2);
        if (reframed)
        {
            double k0 = std::sqrt((rough.w*rough.h)/(rb.w*rb.h));
            sweep(Box(rough.x + (rough.w - rb.w*k0)/2, rough.y + (rough.h - rb.h*k0)/2, rb.w*k0, rb.h*k0), ks, range, 2);
        }

        Box coarse = best;
        sweep(coarse, { 0.99, 0.995, 1.0, 1.005, 1.01 }, 2, 0.5);

        // Under half a level better than the model's own framing is noise
        if (bs < s0 + 0.5)
            best = rb;
        return regOf(best);
    }

    UiImage Warp(const UiImage& src, const Registration& r, int w, int h)
    {
        UiImage out(w, h);
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                double sx = (x + 0.5 - r.tx)/r.sx - 0.5, sy = (y + 0.5 - r.ty)/r.sy - 0.5;
                int x0 = (int)std::floor(sx), y0 = (int)std::floor(sy);
                double fx = sx - x0, fy = sy - y0;
                double acc[4] = { 0, 0, 0, 0 };
                const int ox[4] = { 0, 1, 0, 1 }, oy[4] = { 0, 0, 1, 1 };
                const double wt[4] = { (1 - fx)*(1 - fy), fx*(1 - fy), (1 - fx)*fy, fx*fy };
                for (int k = 0; k < 4; k++)
                {
                    int xx = x0 + ox[k], yy = y0 + oy[k];
                    if (xx < 0 || yy < 0 || xx >= src.w || yy >= src.h)
                        continue;

                    const UInt8* s = src.At(xx, yy);
                    double a = s[3]*wt[k];
                    acc[0] += s[0]*a; acc[1] += s[1]*a; acc[2] += s[2]*a; acc[3] += a;
                }

                if (acc[3] <= 0)
                    continue;

                UInt8* d = out.At(x, y);
                d[0] = Clamp8(acc[0]/acc[3]); d[1] = Clamp8(acc[1]/acc[3]); d[2] = Clamp8(acc[2]/acc[3]); d[3] = Clamp8(acc[3]);
            }
        }
        return out;
    }

    void Despill(UiImage& img, const Rgb& key)
    {
        Vector<int> hi, lo;
        for (int c = 0; c < 3; c++)
            (key[c] > 128 ? hi : lo).Add(c);

        if (hi.IsEmpty() || lo.IsEmpty())
            return;

        auto edge = [&](int x, int y)
        {
            for (int dy = -2; dy <= 2; dy++)
            {
                for (int dx = -2; dx <= 2; dx++)
                {
                    int xx = x + dx, yy = y + dy;
                    if (xx < 0 || yy < 0 || xx >= img.w || yy >= img.h || img.At(xx, yy)[3] < 128)
                        return true;
                }
            }
            return false;
        };

        for (int y = 0; y < img.h; y++)
        {
            for (int x = 0; x < img.w; x++)
            {
                UInt8* p = img.At(x, y);
                int a = p[3];
                if (a == 0 || (a >= 250 && !edge(x, y)))
                    continue;

                double cap = 0;
                for (int c : lo)
                    cap = Math::Max(cap, (double)p[c]);

                double keep = a >= 250 ? 0.15 : (a/255.0)*0.3;
                for (int c : hi)
                {
                    if (p[c] > cap)
                        p[c] = Clamp8(cap + (p[c] - cap)*keep);
                }
            }
        }
    }

    UiImage Composite(const UiImage& crop, const UiImage& render, const Vector<Box>& erase, bool renderColors)
    {
        int w = crop.w, h = crop.h;
        std::vector<float> eraseW((size_t)w*h, 0.0f);
        const int feather = 2;
        for (auto& b : erase)
        {
            int x0 = (int)std::floor(b.x - 2 - feather), y0 = (int)std::floor(b.y - 2 - feather);
            int x1 = (int)std::ceil(b.x + b.w + 2 + feather), y1 = (int)std::ceil(b.y + b.h + 2 + feather);
            for (int y = Math::Max(0, y0); y < Math::Min(h, y1); y++)
            {
                for (int x = Math::Max(0, x0); x < Math::Min(w, x1); x++)
                {
                    int d = Math::Min(Math::Min(x - x0, x1 - 1 - x), Math::Min(y - y0, y1 - 1 - y));
                    float v = (float)Math::Min(1.0, (d + 1.0)/(feather + 1.0));
                    size_t i = (size_t)y*w + x;
                    if (v > eraseW[i])
                        eraseW[i] = v;
                }
            }
        }

        double gain[3], offset[3];
        for (int c = 0; c < 3; c++)
        {
            double n = 0, sx = 0, sy = 0, sxx = 0, sxy = 0;
            for (size_t i = 0; i < (size_t)w*h; i++)
            {
                if (render.data[i*4 + 3] < 235 || eraseW[i] > 0)
                    continue;

                double x = render.data[i*4 + c], y = crop.data[i*4 + c];
                n++; sx += x; sy += y; sxx += x*x; sxy += x*y;
            }

            if (n < 30)
            {
                gain[c] = 1;
                offset[c] = 0;
                continue;
            }

            double g0 = (n*sxy - sx*sy)/Math::Max(1e-6, n*sxx - sx*sx);
            double g = Math::Max(0.7, Math::Min(1.3, std::isfinite(g0) ? g0 : 1.0));
            gain[c] = g;
            offset[c] = (sy - g*sx)/n;
        }

        UiImage out(w, h);
        for (size_t i = 0; i < (size_t)w*h; i++)
        {
            size_t o = i*4;
            double a = render.data[o + 3]/255.0;
            if (a < 0.02)
                continue;

            double rc[3], diff = 0;
            for (int c = 0; c < 3; c++)
            {
                rc[c] = gain[c]*render.data[o + c] + offset[c];
                diff = Math::Max(diff, std::fabs(crop.data[o + c] - rc[c]));
            }

            double e = renderColors ? 1.0 : Math::Max((double)eraseW[i], Math::Max(0.0, Math::Min(1.0, (diff - 45)/30)));
            for (int c = 0; c < 3; c++)
            {
                double base = a >= 0.92 ? crop.data[o + c] : rc[c];
                out.data[o + c] = Clamp8(base*(1 - e) + rc[c]*e);
            }
            out.data[o + 3] = Clamp8(a*255);
        }
        return out;
    }

    void HarmonicFill(UiImage& img, int x0, int y0, int x1, int y1, bool fromBorder)
    {
        int w = x1 - x0 + 1, h = y1 - y0 + 1;
        if (w < 3 || h < 3)
            return;

        std::vector<float> buf((size_t)w*h*3);
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                const UInt8* p = img.At(x0 + x, y0 + y);
                for (int c = 0; c < 3; c++)
                    buf[((size_t)y*w + x)*3 + c] = p[c];
            }
        }

        if (fromBorder)
        {
            double sum[3] = { 0, 0, 0 };
            int n = 0;
            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    if (y > 0 && y < h - 1 && x > 0 && x < w - 1)
                        continue;
                    for (int c = 0; c < 3; c++)
                        sum[c] += buf[((size_t)y*w + x)*3 + c];
                    n++;
                }
            }
            for (int y = 1; y < h - 1; y++)
            {
                for (int x = 1; x < w - 1; x++)
                {
                    for (int c = 0; c < 3; c++)
                        buf[((size_t)y*w + x)*3 + c] = (float)(sum[c]/n);
                }
            }
        }

        for (int it = 0; it < 400; it++)
        {
            for (int y = 1; y < h - 1; y++)
            {
                for (int x = 1; x < w - 1; x++)
                {
                    size_t i = ((size_t)y*w + x)*3;
                    for (int c = 0; c < 3; c++)
                        buf[i + c] = (float)(((double)buf[i - 3 + c] + buf[i + 3 + c] + buf[i - (size_t)w*3 + c] + buf[i + (size_t)w*3 + c])/4);
                }
            }
        }

        for (int y = 1; y < h - 1; y++)
        {
            for (int x = 1; x < w - 1; x++)
            {
                UInt8* p = img.At(x0 + x, y0 + y);
                if (p[3] < 128)
                    continue;
                for (int c = 0; c < 3; c++)
                    p[c] = Clamp8(buf[((size_t)y*w + x)*3 + c]);
            }
        }
    }

    Vector<String> CleanLeftovers(UiImage& img, const UiImage& crop, const Vector<EraseItem>& labels, const Vector<Box>& boxes)
    {
        Vector<String> done;
        for (int k = 0; k < labels.Count(); k++)
        {
            const Box& box = boxes[k];
            Rgb rgb;
            bool hasColor = ParseHex(labels[k].color, rgb);

            int x0 = Math::Max(1, (int)std::floor(box.x - 2)), y0 = Math::Max(1, (int)std::floor(box.y - 2));
            int x1 = Math::Min(img.w - 2, (int)std::ceil(box.x + box.w + 2)), y1 = Math::Min(img.h - 2, (int)std::ceil(box.y + box.h + 2));
            if (x1 <= x0 || y1 <= y0)
                continue;

            int ink = 0, n = 0, same = 0;
            for (int y = y0; y <= y1; y++)
            {
                for (int x = x0; x <= x1; x++)
                {
                    const UInt8* p = img.At(x, y);
                    if (p[3] < 128)
                        continue;
                    n++;
                    Rgb px(p[0], p[1], p[2]);
                    if (hasColor && ColorDist(px, rgb) < 70)
                        ink++;
                    if (ColorDist(px, RgbAt(crop, x, y)) < 40)
                        same++;
                }
            }

            if (!n)
                continue;
            if (hasColor ? (double)ink/n < 0.04 : (double)same/n < 0.5 || box.w*box.h > 0.5*img.w*img.h)
                continue;

            HarmonicFill(img, x0, y0, x1, y1);
            done.Add(!labels[k].color.IsEmpty() ? labels[k].color :
                     (String)(int)JsRound(box.w) + "\xC3\x97" + (String)(int)JsRound(box.h) + " overlay");
        }
        return done;
    }

}
