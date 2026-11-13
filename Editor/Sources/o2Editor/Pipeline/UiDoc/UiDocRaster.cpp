#include "o2Editor/stdafx.h"
#include "UiDocRaster.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    UiRgba UiColorOf(const String& hex, const UiRgba& def /*= UiRgba()*/)
    {
        String s = JsTrim(hex);
        auto digit = [](char c) {
            return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
        };
        if (s.Length() == 4 && s[0] == '#')
            s = String("#") + s[1] + s[1] + s[2] + s[2] + s[3] + s[3];
        if (s.Length() != 7 || s[0] != '#')
            return def;

        int v[6];
        for (int i = 0; i < 6; i++)
        {
            v[i] = digit(s[i + 1]);
            if (v[i] < 0)
                return def;
        }
        return UiRgba{ (v[0]*16 + v[1])/255.0f, (v[2]*16 + v[3])/255.0f, (v[4]*16 + v[5])/255.0f, 1.0f };
    }

    // --- paths -----------------------------------------------------------------------------------------------

    UiPath RectPath(double x, double y, double w, double h)
    {
        return { { x, y }, { x + w, y }, { x + w, y + h }, { x, y + h } };
    }

    // Points of a quarter arc around (cx, cy) from angle a0 to a1 (radians, y down), the first point included
    static void Arc(UiPath& path, double cx, double cy, double r, double a0, double a1)
    {
        int n = std::max(2, std::min(32, (int)std::ceil(std::sqrt(std::max(r, 0.0))*2.5)));
        for (int i = 0; i <= n; i++)
        {
            double a = a0 + (a1 - a0)*i/n;
            path.push_back({ cx + std::cos(a)*r, cy + std::sin(a)*r });
        }
    }

    UiPath RoundRectPath(double x, double y, double w, double h, double r)
    {
        r = std::max(0.0, std::min(r, std::min(w/2, h/2)));
        if (r <= 0.01)
            return RectPath(x, y, w, h);

        const double pi = 3.14159265358979323846;
        UiPath path;
        path.push_back({ x + r, y });
        Arc(path, x + w - r, y + r, r, -pi/2, 0);
        Arc(path, x + w - r, y + h - r, r, 0, pi/2);
        Arc(path, x + r, y + h - r, r, pi/2, pi);
        Arc(path, x + r, y + r, r, pi, pi*1.5);
        return path;
    }

    UiPath CirclePath(double cx, double cy, double r)
    {
        const double pi = 3.14159265358979323846;
        int n = std::max(12, std::min(360, (int)std::ceil(r*1.5)));
        UiPath path;
        for (int i = 0; i < n; i++)
        {
            double a = 2*pi*i/n;
            path.push_back({ cx + std::cos(a)*r, cy + std::sin(a)*r });
        }
        return path;
    }

    void QuadTo(UiPath& path, const UiPt& c, const UiPt& p)
    {
        UiPt p0 = path.empty() ? c : path.back();
        double len = std::hypot(c.x - p0.x, c.y - p0.y) + std::hypot(p.x - c.x, p.y - c.y);
        int n = std::max(2, std::min(32, (int)std::ceil(len/3)));
        for (int i = 1; i <= n; i++)
        {
            double t = (double)i/n, u = 1 - t;
            path.push_back({ u*u*p0.x + 2*u*t*c.x + t*t*p.x, u*u*p0.y + 2*u*t*c.y + t*t*p.y });
        }
    }

    UiPath Reversed(const UiPath& path)
    {
        return UiPath(path.rbegin(), path.rend());
    }

    // --- coverage --------------------------------------------------------------------------------------------

    void UiMask::Reset(int width, int height)
    {
        if (width == w && height == h && (int)lo.size() == h)
        {
            Clear();
            return;
        }
        w = width;
        h = height;
        a.assign((size_t)std::max(0, w)*std::max(0, h), 0.0f);
        lo.assign(std::max(0, h), std::max(0, w));
        hi.assign(std::max(0, h), 0);
        mAcc.assign((size_t)(std::max(0, w) + 2)*std::max(0, h), 0.0f);
        mAccLo.assign(std::max(0, h), INT32_MAX);
        mAccHi.assign(std::max(0, h), -1);
        x0 = y0 = 0;
        x1 = y1 = 0;
    }

    void UiMask::Clear()
    {
        for (int y = y0; y < y1; y++)
        {
            if (lo[y] < hi[y])
                std::fill(a.begin() + (size_t)y*w + lo[y], a.begin() + (size_t)y*w + hi[y], 0.0f);
            lo[y] = w;
            hi[y] = 0;
        }
        x0 = y0 = 0;
        x1 = y1 = 0;
    }

    void UiMask::Touch(int ax0, int ay0, int ax1, int ay1)
    {
        ax0 = std::max(0, ax0); ay0 = std::max(0, ay0);
        ax1 = std::min(w, ax1); ay1 = std::min(h, ay1);
        if (ax1 <= ax0 || ay1 <= ay0)
            return;
        if (x1 <= x0 || y1 <= y0)
        {
            x0 = ax0; y0 = ay0; x1 = ax1; y1 = ay1;
            return;
        }
        x0 = std::min(x0, ax0); y0 = std::min(y0, ay0);
        x1 = std::max(x1, ax1); y1 = std::max(y1, ay1);
    }

    void UiMask::TouchRow(int y, int ax0, int ax1)
    {
        if (ax1 <= ax0)
            return;
        lo[y] = std::min(lo[y], ax0);
        hi[y] = std::max(hi[y], ax1);
        Touch(ax0, y, ax1, y + 1);
    }

    namespace
    {
        // Where one shape accumulates: rows of (w + 2) cells and the span written in each row
        struct Accumulator
        {
            float* acc;
            int*   rowLo;
            int*   rowHi;
            int    stride, cols, rows;

            void Mark(int y, int a, int b)
            {
                rowLo[y] = std::min(rowLo[y], a);
                rowHi[y] = std::max(rowHi[y], b);
            }
        };
    }

    // One edge into the accumulation (font-rs's signed-area accumulation); x inside [0, cols]
    static void AccumulateLine(Accumulator& t, double x0, double y0, double x1, double y1)
    {
        if (y0 == y1)
            return;

        double dir = 1;
        if (y0 > y1)
        {
            std::swap(x0, x1);
            std::swap(y0, y1);
            dir = -1;
        }

        double dxdy = (x1 - x0)/(y1 - y0);
        double x = x0;
        int yStart = (int)std::floor(y0);
        if (y0 < 0)
        {
            x -= y0*dxdy;
            yStart = 0;
        }
        int yEnd = std::min(t.rows, (int)std::ceil(y1));
        for (int y = std::max(0, yStart); y < yEnd; y++)
        {
            double dy = std::min((double)(y + 1), y1) - std::max((double)y, y0);
            double xnext = x + dxdy*dy;
            double d = dy*dir;
            double xa = std::min(x, xnext), xb = std::max(x, xnext);
            double xaf = std::floor(xa), xbc = std::ceil(xb);
            int xai = (int)xaf, xbi = (int)xbc;
            float* row = t.acc + (size_t)y*t.stride;
            if (xbi <= xai + 1)
            {
                double xmf = 0.5*(x + xnext) - xaf;
                row[xai] += (float)(d - d*xmf);
                row[xai + 1] += (float)(d*xmf);
                t.Mark(y, xai, xai + 2);
            }
            else
            {
                double s = 1.0/(xb - xa);
                double x0f = xa - xaf;
                double a0 = 0.5*s*(1 - x0f)*(1 - x0f);
                double x1f = xb - xbc + 1;
                double am = 0.5*s*x1f*x1f;
                row[xai] += (float)(d*a0);
                if (xbi == xai + 2)
                    row[xai + 1] += (float)(d*(1 - a0 - am));
                else
                {
                    double a1 = s*(1.5 - x0f);
                    row[xai + 1] += (float)(d*(a1 - a0));
                    for (int xi = xai + 2; xi < xbi - 1; xi++)
                        row[xi] += (float)(d*s);
                    double a2 = a1 + (xbi - xai - 3)*s;
                    row[xbi - 1] += (float)(d*(1 - a2 - am));
                }
                row[xbi] += (float)(d*am);
                t.Mark(y, xai, xbi + 1);
            }
            x = xnext;
        }
    }

    // An edge clipped to the columns [0, cols]: the parts outside run along the border, so the winding stays right
    static void AddEdge(Accumulator& t, double ax, double ay, double bx, double by)
    {
        double ts[4] = { 0, 1, 0, 0 };
        int n = 2;
        double cols = t.cols;
        if (ax != bx)
        {
            for (double edge : { 0.0, cols })
            {
                double v = (edge - ax)/(bx - ax);
                if (v > 0 && v < 1)
                    ts[n++] = v;
            }
        }
        std::sort(ts, ts + n);
        for (int i = 0; i + 1 < n; i++)
        {
            double pxa = ax + (bx - ax)*ts[i], pya = ay + (by - ay)*ts[i];
            double pxb = ax + (bx - ax)*ts[i + 1], pyb = ay + (by - ay)*ts[i + 1];
            AccumulateLine(t, std::clamp(pxa, 0.0, cols), pya, std::clamp(pxb, 0.0, cols), pyb);
        }
    }

    void UiMask::Fill(const UiShape& shape)
    {
        double miny = 1e300, maxy = -1e300;
        bool any = false;
        for (auto& path : shape)
        {
            for (auto& p : path)
            {
                miny = std::min(miny, p.y);
                maxy = std::max(maxy, p.y);
                any = true;
            }
        }
        int iy0 = std::max(0, (int)std::floor(miny)), iy1 = std::min(h, (int)std::ceil(maxy));
        if (!any || iy1 <= iy0 || w <= 0)
            return;

        // The whole width (edges clipped to it), rows of the shape only; the spans written are all that is read and cleared
        Accumulator t{ mAcc.data(), mAccLo.data(), mAccHi.data(), w + 2, w, h };
        for (auto& path : shape)
        {
            size_t n = path.size();
            if (n < 2)
                continue;
            for (size_t i = 0; i < n; i++)
            {
                const UiPt& p = path[i];
                const UiPt& q = path[(i + 1) % n];
                AddEdge(t, p.x, p.y, q.x, q.y);
            }
        }

        for (int y = iy0; y < iy1; y++)
        {
            int ra = mAccLo[y], rb = std::min(w + 2, mAccHi[y]);
            if (ra >= rb)
                continue;
            float* row = mAcc.data() + (size_t)y*(w + 2);
            float* out = a.data() + (size_t)y*w;
            float sum = 0;
            int xEnd = std::min(w, rb);
            int first = -1, last = -1;
            for (int x = ra; x < xEnd; x++)
            {
                sum += row[x];
                float c = std::min(1.0f, std::fabs(sum));
                if (c > 0)
                {
                    if (first < 0)
                        first = x;
                    last = x;
                    if (c > out[x])
                        out[x] = c;
                }
            }
            std::fill(row + ra, row + rb, 0.0f);
            mAccLo[y] = INT32_MAX;
            mAccHi[y] = -1;
            if (first >= 0)
                TouchRow(y, first, last + 1);
        }
        // Rows the edges reached outside the shape's rows (none: they are clipped to them) need no clearing
    }

    // The quad of a segment of width 2h
    static UiPath SegmentQuad(const UiPt& p, const UiPt& q, double h)
    {
        double dx = q.x - p.x, dy = q.y - p.y, len = std::hypot(dx, dy);
        if (len <= 0)
            return {};
        double nx = -dy/len*h, ny = dx/len*h;
        return { { p.x + nx, p.y + ny }, { q.x + nx, q.y + ny }, { q.x - nx, q.y - ny }, { p.x - nx, p.y - ny } };
    }

    void UiMask::Stroke(const UiPath& path, bool closed, double width, const std::vector<double>& dashes /*= {}*/)
    {
        if (path.size() < 2 || width <= 0)
            return;

        double h = width/2;
        auto strokeRun = [&](const UiPath& run, bool runClosed) {
            size_t n = run.size();
            size_t segs = runClosed ? n : n - 1;
            for (size_t i = 0; i < segs; i++)
            {
                UiPath quad = SegmentQuad(run[i], run[(i + 1) % n], h);
                if (!quad.empty())
                    Fill({ quad });
            }
            // Round joins at the inner points (every point of a closed run)
            for (size_t i = runClosed ? 0 : 1; i < (runClosed ? n : n - 1); i++)
                Fill({ CirclePath(run[i].x, run[i].y, h) });
        };

        double total = 0;
        for (double d : dashes)
            total += d;
        if (dashes.size() < 2 || total <= 0)
        {
            strokeRun(path, closed);
            return;
        }

        // Dashes from the start of the path, along its segments
        UiPath pts = path;
        if (closed)
            pts.push_back(path[0]);
        size_t di = 0;
        double left = dashes[0];
        bool on = true;
        UiPath run = { pts[0] };
        for (size_t i = 0; i + 1 < pts.size(); i++)
        {
            UiPt p = pts[i], q = pts[i + 1];
            double len = std::hypot(q.x - p.x, q.y - p.y), at = 0;
            while (len - at > left)
            {
                at += left;
                UiPt cut{ p.x + (q.x - p.x)*at/len, p.y + (q.y - p.y)*at/len };
                if (on)
                {
                    run.push_back(cut);
                    strokeRun(run, false);
                }
                run = { cut };
                on = !on;
                di = (di + 1) % dashes.size();
                left = dashes[di];
            }
            left -= len - at;
            if (on)
                run.push_back(q);
            else
                run = { q };
        }
        if (on && run.size() > 1)
            strokeRun(run, false);
    }

    void UiMask::Blit(const UInt8* coverage, int pitch, int width, int height, int x, int y)
    {
        int bx0 = std::max(0, x), by0 = std::max(0, y), bx1 = std::min(w, x + width), by1 = std::min(h, y + height);
        if (bx1 <= bx0 || by1 <= by0)
            return;
        for (int yy = by0; yy < by1; yy++)
        {
            const UInt8* src = coverage + (size_t)(yy - y)*pitch;
            float* out = a.data() + (size_t)yy*w;
            for (int xx = bx0; xx < bx1; xx++)
            {
                float c = src[xx - x]/255.0f;
                if (c > out[xx])
                    out[xx] = c;
            }
            TouchRow(yy, bx0, bx1);
        }
    }

    // --- pictures --------------------------------------------------------------------------------------------

    UiPicture UiPicture::Of(const UiImage& img)
    {
        UiPicture pic;
        pic.w = img.w;
        pic.h = img.h;
        pic.px.resize((size_t)img.w*img.h*4);
        for (size_t i = 0; i < (size_t)img.w*img.h; i++)
        {
            float a = img.data[i*4 + 3]/255.0f;
            pic.px[i*4] = img.data[i*4]/255.0f*a;
            pic.px[i*4 + 1] = img.data[i*4 + 1]/255.0f*a;
            pic.px[i*4 + 2] = img.data[i*4 + 2]/255.0f*a;
            pic.px[i*4 + 3] = a;
        }

        // Halved copies (box filter) down to a pixel
        const UiPicture* prev = &pic;
        std::vector<UiPicture> mips;
        while (prev->w > 1 || prev->h > 1)
        {
            UiPicture m;
            m.w = std::max(1, (prev->w + 1)/2);
            m.h = std::max(1, (prev->h + 1)/2);
            m.px.resize((size_t)m.w*m.h*4);
            for (int y = 0; y < m.h; y++)
            {
                for (int x = 0; x < m.w; x++)
                {
                    int sx0 = std::min(prev->w - 1, x*2), sx1 = std::min(prev->w - 1, x*2 + 1);
                    int sy0 = std::min(prev->h - 1, y*2), sy1 = std::min(prev->h - 1, y*2 + 1);
                    for (int c = 0; c < 4; c++)
                    {
                        float s = prev->px[((size_t)sy0*prev->w + sx0)*4 + c] + prev->px[((size_t)sy0*prev->w + sx1)*4 + c] +
                            prev->px[((size_t)sy1*prev->w + sx0)*4 + c] + prev->px[((size_t)sy1*prev->w + sx1)*4 + c];
                        m.px[((size_t)y*m.w + x)*4 + c] = s*0.25f;
                    }
                }
            }
            mips.push_back(std::move(m));
            prev = &mips.back();
        }
        pic.mips = std::move(mips);
        return pic;
    }

    // --- canvas ------------------------------------------------------------------------------------------------

    void UiCanvas::Reset(int width, int height)
    {
        if (width == w && height == h && px.size() == (size_t)std::max(0, w)*std::max(0, h)*4)
        {
            Clear();
            return;
        }
        w = width;
        h = height;
        px.resize((size_t)std::max(0, w)*std::max(0, h)*4);
        if (!px.empty())
            std::memset(px.data(), 0, px.size()*sizeof(float));
        x0 = y0 = x1 = y1 = 0;
    }

    void UiCanvas::Clear()
    {
        for (int y = y0; y < y1; y++)
            std::memset(px.data() + ((size_t)y*w + x0)*4, 0, (size_t)(x1 - x0)*4*sizeof(float));
        x0 = y0 = x1 = y1 = 0;
    }

    void UiCanvas::Touch(int ax0, int ay0, int ax1, int ay1)
    {
        ax0 = std::max(0, ax0); ay0 = std::max(0, ay0);
        ax1 = std::min(w, ax1); ay1 = std::min(h, ay1);
        if (ax1 <= ax0 || ay1 <= ay0)
            return;
        if (x1 <= x0 || y1 <= y0)
        {
            x0 = ax0; y0 = ay0; x1 = ax1; y1 = ay1;
            return;
        }
        x0 = std::min(x0, ax0); y0 = std::min(y0, ay0);
        x1 = std::max(x1, ax1); y1 = std::max(y1, ay1);
    }

    // Source-over of a premultiplied colour scaled by k onto a pixel
    static inline void Over(float* d, float r, float g, float b, float a)
    {
        float k = 1 - a;
        d[0] = r + d[0]*k;
        d[1] = g + d[1]*k;
        d[2] = b + d[2]*k;
        d[3] = a + d[3]*k;
    }

    void UiCanvas::Paint(const UiMask& mask, const UiRgba& color, float opacity /*= 1*/)
    {
        float ca = color.a*opacity;
        if (ca <= 0)
            return;
        for (int y = mask.y0; y < mask.y1; y++)
        {
            const float* m = mask.a.data() + (size_t)y*mask.w;
            float* d = px.data() + (size_t)y*w*4;
            for (int x = mask.lo[y]; x < mask.hi[y]; x++)
            {
                float c = m[x]*ca;
                if (c > 0)
                    Over(d + x*4, color.r*c, color.g*c, color.b*c, c);
            }
        }
        Touch(mask.x0, mask.y0, mask.x1, mask.y1);
    }

    void UiCanvas::PaintGradient(const UiMask& mask, const UiRgba& top, const UiRgba& bottom, double ya, double yb,
                                 float opacity /*= 1*/)
    {
        for (int y = mask.y0; y < mask.y1; y++)
        {
            double t = yb > ya ? std::clamp((y + 0.5 - ya)/(yb - ya), 0.0, 1.0) : 0.0;
            UiRgba c{ (float)(top.r + (bottom.r - top.r)*t), (float)(top.g + (bottom.g - top.g)*t), (float)(top.b + (bottom.b - top.b)*t),
                      (float)(top.a + (bottom.a - top.a)*t) };
            const float* m = mask.a.data() + (size_t)y*mask.w;
            float* d = px.data() + (size_t)y*w*4;
            for (int x = mask.lo[y]; x < mask.hi[y]; x++)
            {
                float k = m[x]*c.a*opacity;
                if (k > 0)
                    Over(d + x*4, c.r*k, c.g*k, c.b*k, k);
            }
        }
        Touch(mask.x0, mask.y0, mask.x1, mask.y1);
    }

    void UiCanvas::FillRect(int ax0, int ay0, int ax1, int ay1, const UiRgba& color)
    {
        ax0 = std::max(0, ax0); ay0 = std::max(0, ay0);
        ax1 = std::min(w, ax1); ay1 = std::min(h, ay1);
        float a = color.a;
        for (int y = ay0; y < ay1; y++)
        {
            float* d = px.data() + (size_t)y*w*4;
            for (int x = ax0; x < ax1; x++)
                Over(d + x*4, color.r*a, color.g*a, color.b*a, a);
        }
        Touch(ax0, ay0, ax1, ay1);
    }

    // A texel of a level, clamped to its edges or wrapped
    static inline const float* Texel(const UiPicture& p, int x, int y, bool repeat)
    {
        if (repeat)
        {
            x %= p.w; if (x < 0) x += p.w;
            y %= p.h; if (y < 0) y += p.h;
        }
        else
        {
            x = x < 0 ? 0 : x >= p.w ? p.w - 1 : x;
            y = y < 0 ? 0 : y >= p.h ? p.h - 1 : y;
        }
        return p.px.data() + ((size_t)y*p.w + x)*4;
    }

    void UiCanvas::Draw(const UiPicture& picture, const UiPictureDraw& d)
    {
        if (picture.w <= 0 || picture.h <= 0 || d.sx == 0 || d.sy == 0)
            return;

        int ix0 = std::max(0, (int)std::floor(d.cx0)), iy0 = std::max(0, (int)std::floor(d.cy0));
        int ix1 = std::min(w, (int)std::ceil(d.cx1)), iy1 = std::min(h, (int)std::ceil(d.cy1));
        if (ix1 <= ix0 || iy1 <= iy0)
            return;

        // The halved copy nearest the drawn size
        double scale = std::min(std::fabs(d.sx), std::fabs(d.sy));
        int level = scale < 1 ? (int)std::floor(std::log2(1/scale)) : 0;
        level = std::min(level, (int)picture.mips.size());
        const UiPicture& p = level == 0 ? picture : picture.mips[level - 1];
        double kx = (double)p.w/picture.w, ky = (double)p.h/picture.h;

        for (int y = iy0; y < iy1; y++)
        {
            double cy = std::min(y + 1.0, d.cy1) - std::max((double)y, d.cy0);
            if (cy <= 0)
                continue;
            double v = ((y + 0.5 - d.ty)/d.sy)*ky - 0.5;
            double vf = std::floor(v);
            int vy = (int)vf;
            float fy = (float)(v - vf);
            float* row = px.data() + (size_t)y*w*4;
            for (int x = ix0; x < ix1; x++)
            {
                double cx = std::min(x + 1.0, d.cx1) - std::max((double)x, d.cx0);
                if (cx <= 0)
                    continue;
                double u = ((x + 0.5 - d.tx)/d.sx)*kx - 0.5;
                double uf = std::floor(u);
                int ux = (int)uf;
                float fx = (float)(u - uf);
                const float *t00, *t10, *t01, *t11;
                if (ux >= 0 && vy >= 0 && ux + 1 < p.w && vy + 1 < p.h)
                {
                    t00 = p.px.data() + ((size_t)vy*p.w + ux)*4;
                    t10 = t00 + 4;
                    t01 = t00 + (size_t)p.w*4;
                    t11 = t01 + 4;
                }
                else
                {
                    t00 = Texel(p, ux, vy, d.repeat);
                    t10 = Texel(p, ux + 1, vy, d.repeat);
                    t01 = Texel(p, ux, vy + 1, d.repeat);
                    t11 = Texel(p, ux + 1, vy + 1, d.repeat);
                }
                float w00 = (1 - fx)*(1 - fy), w10 = fx*(1 - fy), w01 = (1 - fx)*fy, w11 = fx*fy;
                float k = (float)(std::min(1.0, cx)*std::min(1.0, cy))*d.opacity;
                float c[4];
                for (int i = 0; i < 4; i++)
                    c[i] = (t00[i]*w00 + t10[i]*w10 + t01[i]*w01 + t11[i]*w11)*k;
                if (c[3] <= 0)
                    continue;
                if (d.tinted)
                {
                    c[0] *= d.tint[0];
                    c[1] *= d.tint[1];
                    c[2] *= d.tint[2];
                }
                Over(row + x*4, c[0], c[1], c[2], c[3]);
            }
        }
        Touch(ix0, iy0, ix1, iy1);
    }

    void UiCanvas::Composite(const UiCanvas& layer, float opacity /*= 1*/)
    {
        for (int y = layer.y0; y < layer.y1; y++)
        {
            const float* s = layer.px.data() + (size_t)y*w*4;
            float* d = px.data() + (size_t)y*w*4;
            for (int x = layer.x0; x < layer.x1; x++)
            {
                const float* c = s + x*4;
                if (c[3] > 0)
                    Over(d + x*4, c[0]*opacity, c[1]*opacity, c[2]*opacity, c[3]*opacity);
            }
        }
        Touch(layer.x0, layer.y0, layer.x1, layer.y1);
    }

    void UiCanvas::CompositeTurned(const UiCanvas& layer, double deg, const UiPt& centre, const UiPt& layerCentre, float opacity /*= 1*/)
    {
        if (layer.x1 <= layer.x0 || layer.y1 <= layer.y0)
            return;

        const double a = (deg*3.141592653589793)/180, c = std::cos(a), s = std::sin(a);
        // Where the layer's drawn box lands: its corners turned about the centre
        double minX = INFINITY, minY = INFINITY, maxX = -INFINITY, maxY = -INFINITY;
        for (int i = 0; i < 4; i++)
        {
            double dx = (i == 1 || i == 2 ? layer.x1 : layer.x0) - layerCentre.x, dy = (i >= 2 ? layer.y1 : layer.y0) - layerCentre.y;
            double px = centre.x + c*dx - s*dy, py = centre.y + s*dx + c*dy;
            minX = std::min(minX, px); maxX = std::max(maxX, px);
            minY = std::min(minY, py); maxY = std::max(maxY, py);
        }
        int bx0 = std::max(0, (int)std::floor(minX) - 1), by0 = std::max(0, (int)std::floor(minY) - 1);
        int bx1 = std::min(w, (int)std::ceil(maxX) + 1), by1 = std::min(h, (int)std::ceil(maxY) + 1);

        // Every pixel takes the layer's colour where its centre was before the turn: bilinear between the four nearest
        auto tap = [&](int x, int y, float k, float* acc) {
            if (k <= 0 || x < layer.x0 || y < layer.y0 || x >= layer.x1 || y >= layer.y1)
                return;
            const float* p = layer.px.data() + ((size_t)y*layer.w + x)*4;
            acc[0] += p[0]*k; acc[1] += p[1]*k; acc[2] += p[2]*k; acc[3] += p[3]*k;
        };
        for (int y = by0; y < by1; y++)
        {
            float* d = px.data() + (size_t)y*w*4;
            for (int x = bx0; x < bx1; x++)
            {
                double ux = x + 0.5 - centre.x, uy = y + 0.5 - centre.y;
                double qx = c*ux + s*uy + layerCentre.x - 0.5, qy = -s*ux + c*uy + layerCentre.y - 0.5;
                int ix = (int)std::floor(qx), iy = (int)std::floor(qy);
                if (ix < layer.x0 - 1 || iy < layer.y0 - 1 || ix >= layer.x1 || iy >= layer.y1)
                    continue;

                float fx = (float)(qx - ix), fy = (float)(qy - iy);
                float acc[4] = { 0, 0, 0, 0 };
                tap(ix, iy, (1 - fx)*(1 - fy), acc);
                tap(ix + 1, iy, fx*(1 - fy), acc);
                tap(ix, iy + 1, (1 - fx)*fy, acc);
                tap(ix + 1, iy + 1, fx*fy, acc);
                if (acc[3] > 0)
                    Over(d + x*4, acc[0]*opacity, acc[1]*opacity, acc[2]*opacity, acc[3]*opacity);
            }
        }
        Touch(bx0, by0, bx1, by1);
    }

    void UiCanvas::CompositeBlock(const std::vector<float>& block, int bw, int bh, int x, int y, float opacity /*= 1*/)
    {
        int bx0 = std::max(0, x), by0 = std::max(0, y), bx1 = std::min(w, x + bw), by1 = std::min(h, y + bh);
        for (int yy = by0; yy < by1; yy++)
        {
            const float* s = block.data() + (size_t)(yy - y)*bw*4;
            float* d = px.data() + (size_t)yy*w*4;
            for (int xx = bx0; xx < bx1; xx++)
            {
                const float* c = s + (xx - x)*4;
                if (c[3] > 0)
                    Over(d + xx*4, c[0]*opacity, c[1]*opacity, c[2]*opacity, c[3]*opacity);
            }
        }
        Touch(bx0, by0, bx1, by1);
    }

    UiImage UiCanvas::ToImage() const
    {
        UiImage img(w, h);
        const float* src = px.data();
        UInt8* dst = img.data.data();
        size_t n = (size_t)w*h;
        for (size_t i = 0; i < n; i++, src += 4, dst += 4)
        {
            float a = src[3];
            if (a <= 0)
            {
                dst[0] = dst[1] = dst[2] = dst[3] = 0;
                continue;
            }
            if (a > 1)
                a = 1;
            float k = 255.0f/a;
            for (int c = 0; c < 3; c++)
            {
                float v = src[c]*k + 0.5f;
                dst[c] = v >= 255 ? 255 : v <= 0 ? 0 : (UInt8)v;
            }
            dst[3] = (UInt8)(a*255.0f + 0.5f);
        }
        return img;
    }

    // --- blur -----------------------------------------------------------------------------------------------------

    // One box pass along rows (or columns): out[i] = mean of in[i - lo .. i + hi], transparent outside
    static void BoxPass(std::vector<float>& data, int w, int h, int lo, int hi, bool vertical)
    {
        int n = vertical ? h : w, lines = vertical ? w : h;
        int size = lo + hi + 1;
        std::vector<float> line((size_t)n*4), out((size_t)n*4);
        for (int l = 0; l < lines; l++)
        {
            for (int i = 0; i < n; i++)
            {
                size_t at = vertical ? ((size_t)i*w + l)*4 : ((size_t)l*w + i)*4;
                for (int c = 0; c < 4; c++)
                    line[(size_t)i*4 + c] = data[at + c];
            }
            float sum[4] = { 0, 0, 0, 0 };
            for (int i = -lo; i <= hi - 1 && i < n; i++)
            {
                if (i >= 0)
                    for (int c = 0; c < 4; c++)
                        sum[c] += line[(size_t)i*4 + c];
            }
            for (int i = 0; i < n; i++)
            {
                int add = i + hi, sub = i - lo - 1;
                if (add >= 0 && add < n)
                    for (int c = 0; c < 4; c++)
                        sum[c] += line[(size_t)add*4 + c];
                if (sub >= 0 && sub < n)
                    for (int c = 0; c < 4; c++)
                        sum[c] -= line[(size_t)sub*4 + c];
                for (int c = 0; c < 4; c++)
                    out[(size_t)i*4 + c] = sum[c]/size;
            }
            for (int i = 0; i < n; i++)
            {
                size_t at = vertical ? ((size_t)i*w + l)*4 : ((size_t)l*w + i)*4;
                for (int c = 0; c < 4; c++)
                    data[at + c] = out[(size_t)i*4 + c];
            }
        }
    }

    // A true Gaussian along rows or columns (small deviations)
    static void GaussPass(std::vector<float>& data, int w, int h, double sigma, bool vertical)
    {
        int r = std::max(1, (int)std::ceil(sigma*3));
        std::vector<float> k((size_t)r*2 + 1);
        float total = 0;
        for (int i = -r; i <= r; i++)
            total += k[(size_t)(i + r)] = (float)std::exp(-(i*i)/(2*sigma*sigma));
        for (auto& v : k)
            v /= total;

        int n = vertical ? h : w, lines = vertical ? w : h;
        std::vector<float> line((size_t)n*4);
        for (int l = 0; l < lines; l++)
        {
            for (int i = 0; i < n; i++)
            {
                size_t at = vertical ? ((size_t)i*w + l)*4 : ((size_t)l*w + i)*4;
                for (int c = 0; c < 4; c++)
                    line[(size_t)i*4 + c] = data[at + c];
            }
            for (int i = 0; i < n; i++)
            {
                float s[4] = { 0, 0, 0, 0 };
                for (int j = -r; j <= r; j++)
                {
                    int at = i + j;
                    if (at < 0 || at >= n)
                        continue;
                    for (int c = 0; c < 4; c++)
                        s[c] += line[(size_t)at*4 + c]*k[(size_t)(j + r)];
                }
                size_t to = vertical ? ((size_t)i*w + l)*4 : ((size_t)l*w + i)*4;
                for (int c = 0; c < 4; c++)
                    data[to + c] = s[c];
            }
        }
    }

    void GaussianBlur(std::vector<float>& block, int w, int h, double sigma)
    {
        if (sigma <= 0 || w <= 0 || h <= 0)
            return;

        for (bool vertical : { false, true })
        {
            if (sigma < 2)
            {
                GaussPass(block, w, h, sigma, vertical);
                continue;
            }
            // SVG's approximation: three box blurs of size d (an even d: two shifted boxes and one of d + 1)
            int d = (int)std::floor(sigma*3*std::sqrt(2*3.14159265358979323846)/4 + 0.5);
            if (d % 2 == 1)
            {
                for (int i = 0; i < 3; i++)
                    BoxPass(block, w, h, d/2, d/2, vertical);
            }
            else
            {
                BoxPass(block, w, h, d/2, d/2 - 1, vertical);
                BoxPass(block, w, h, d/2 - 1, d/2, vertical);
                BoxPass(block, w, h, d/2, d/2, vertical);
            }
        }
    }
}
