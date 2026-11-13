#include "o2Editor/stdafx.h"
#include "UiLayoutCompare.h"

#include <cmath>

#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Pipeline/UiDoc/UiDocSolve.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutDraw.h"

namespace Editor::UiLayout
{
    namespace D = ::Editor::UiDoc;

    // The editor's dark card behind pictures without a background of their own
    static const Rgb cardBg(0x20, 0x23, 0x2B);

    UiImage RenderDocShot(const D::UiDoc& doc, const D::UiKit* kit, int max /*= 1024*/)
    {
        bool skinned = kit && (!kit->elements.IsEmpty() || !kit->textStyles.IsEmpty());
        double k = std::min(1.0, max/std::max(doc.width, doc.height));
        D::UiRenderOptions o;
        o.mode = skinned ? "kit" : "lines";
        o.kit = skinned ? kit : nullptr;
        o.pixelLong = std::max(1, (int)JsRound(std::max(doc.width, doc.height)*k));
        return D::RenderLayout(doc, o);
    }

    UiImage FitInside(const UiImage& img, int max)
    {
        if (img.w <= max && img.h <= max)
            return img;
        double k = std::min((double)max/img.w, (double)max/img.h);
        return img.Resize(std::max(1, (int)JsRound(img.w*k)), std::max(1, (int)JsRound(img.h*k)));
    }

    double GridStep(double w, double h)
    {
        double want = std::max(w, h)/12;
        for (double s : { 25.0, 50.0, 100.0, 200.0, 250.0, 500.0, 1000.0 })
        {
            if (s >= want)
                return s;
        }
        return 1000;
    }

    // Blends a colour over a rectangle of the picture
    static void FillRect(UiImage& img, double x0, double y0, double x1, double y1, const Rgb& rgb, double alpha)
    {
        int ax = std::max(0, (int)std::floor(x0)), ay = std::max(0, (int)std::floor(y0));
        int bx = std::min(img.w, (int)std::ceil(x1)), by = std::min(img.h, (int)std::ceil(y1));
        for (int y = ay; y < by; y++)
        {
            for (int x = ax; x < bx; x++)
            {
                UInt8* p = img.At(x, y);
                for (int c = 0; c < 3; c++)
                    p[c] = Clamp8(p[c]*(1 - alpha) + rgb[c]*alpha);
                p[3] = 255;
            }
        }
    }

    void DrawLabel(UiImage& img, const String& text, double x, double y, const Rgb& color, int px)
    {
        UiImage glyphs = UiImage::Blank(text.Length()*6 + 4, 11);
        DrawCaption(glyphs, text, 1, 1, color, 11);
        int scale = std::max(1, (int)JsRound(px/9.0));
        UiImage big = scale > 1 ? glyphs.Resize(glyphs.w*scale, glyphs.h*scale, true) : glyphs;
        img.Blit(big, (int)JsRound(x), (int)JsRound(y));
    }

    UiImage WithGrid(const UiImage& img, double w, double h, double step /*= 0*/)
    {
        if (step <= 0)
            step = GridStep(w, h);
        UiImage out = img.Flatten((UInt8)cardBg.r, (UInt8)cardBg.g, (UInt8)cardBg.b);
        double kx = out.w/std::max(1.0, w), ky = out.h/std::max(1.0, h);
        int fs = std::max(10, (int)JsRound(std::max(out.w, out.h)/85.0));
        Rgb dark(0x10, 0x13, 0x1A), line(0x7C, 0xF3, 0xFF);
        for (double x = step; x < w; x += step)
        {
            double px = x*kx;
            FillRect(out, px - 1.2, 0, px + 1.2, out.h, dark, 0.35);
            FillRect(out, px - 0.5, 0, px + 0.5, out.h, line, 0.6);
        }
        for (double y = step; y < h; y += step)
        {
            double py = y*ky;
            FillRect(out, 0, py - 1.2, out.w, py + 1.2, dark, 0.35);
            FillRect(out, 0, py - 0.5, out.w, py + 0.5, line, 0.6);
        }

        auto tag = [&](double x, double y, const String& text, bool middle)
        {
            int scale = std::max(1, (int)JsRound(fs/9.0));
            double tw = text.Length()*6*scale + 6, th = 10*scale + 4;
            double bx = middle ? x - tw/2 : x;
            FillRect(out, bx, y - th/2, bx + tw, y + th/2, dark, 0.78);
            DrawLabel(out, text, bx + 3, y - th/2 + 2, line, fs);
        };
        for (double x = step; x < w; x += step)
            tag(x*kx, fs, Json::FormatNumber(x), true);
        for (double y = step; y < h; y += step)
            tag(2, y*ky, Json::FormatNumber(y), false);
        return out;
    }

    // The picture flattened on the card colour and box-averaged down to cw x ch, RGB
    static std::vector<double> SmallRgb(const UiImage& img, int cw, int ch)
    {
        std::vector<double> out(cw*ch*3, 0.0);
        for (int cy = 0; cy < ch; cy++)
        {
            double sy0 = (double)cy*img.h/ch, sy1 = (double)(cy + 1)*img.h/ch;
            for (int cx = 0; cx < cw; cx++)
            {
                double sx0 = (double)cx*img.w/cw, sx1 = (double)(cx + 1)*img.w/cw;
                double sum[3] = { 0, 0, 0 }, n = 0;
                for (int y = (int)sy0; y < std::max((int)sy0 + 1, (int)std::ceil(sy1)) && y < img.h; y++)
                {
                    for (int x = (int)sx0; x < std::max((int)sx0 + 1, (int)std::ceil(sx1)) && x < img.w; x++)
                    {
                        const UInt8* p = img.At(x, y);
                        double a = p[3]/255.0;
                        for (int c = 0; c < 3; c++)
                            sum[c] += p[c]*a + cardBg[c]*(1 - a);
                        n++;
                    }
                }
                for (int c = 0; c < 3; c++)
                    out[(cy*cw + cx)*3 + c] = n > 0 ? sum[c]/n : cardBg[c];
            }
        }

        // A light blur (sigma 0.6): what counts is placement, not edges
        std::vector<double> blurred(out.size());
        const double kw[3] = { 0.2, 0.6, 0.2 };
        for (int cy = 0; cy < ch; cy++)
        {
            for (int cx = 0; cx < cw; cx++)
            {
                for (int c = 0; c < 3; c++)
                {
                    double s = 0, ws = 0;
                    for (int dy = -1; dy <= 1; dy++)
                    {
                        for (int dx = -1; dx <= 1; dx++)
                        {
                            int x = cx + dx, y = cy + dy;
                            if (x < 0 || y < 0 || x >= cw || y >= ch)
                                continue;
                            double k = kw[dx + 1]*kw[dy + 1];
                            s += out[(y*cw + x)*3 + c]*k;
                            ws += k;
                        }
                    }
                    blurred[(cy*cw + cx)*3 + c] = s/ws;
                }
            }
        }
        return blurred;
    }

    ShotCompare CompareShots(const UiImage& reference, const UiImage& shot, double w, double h, int maxRegions /*= 5*/)
    {
        const double cellPx = 12;
        int cw = std::max(8, (int)JsRound(w/cellPx)), ch = std::max(8, (int)JsRound(h/cellPx));
        std::vector<double> a = SmallRgb(reference, cw, ch), b = SmallRgb(shot, cw, ch);
        std::vector<double> d(cw*ch);
        double total = 0;
        for (int i = 0; i < cw*ch; i++)
        {
            d[i] = (std::abs(a[i*3] - b[i*3]) + std::abs(a[i*3 + 1] - b[i*3 + 1]) + std::abs(a[i*3 + 2] - b[i*3 + 2]))/(3*255.0);
            total += d[i];
        }
        double score = std::max(0.0, 1 - (total/(cw*ch))*2.5);

        // Cells that differ clearly, grown into places (4-connected), ranked by how much difference they hold. Where the
        // whole screen differs a little one place would take in everything: the bar is raised until the places are parts
        // of the screen
        struct Comp { int x0, y0, x1, y1; double sum; int n; };
        auto placesAt = [&](double th)
        {
            std::vector<UInt8> seen(cw*ch, 0);
            std::vector<Comp> found;
            for (int s = 0; s < cw*ch; s++)
            {
                if (seen[s] || d[s] < th)
                    continue;
                Comp comp = { cw, ch, 0, 0, 0, 0 };
                std::vector<int> stack = { s };
                seen[s] = 1;
                while (!stack.empty())
                {
                    int i = stack.back();
                    stack.pop_back();
                    int x = i%cw, y = i/cw;
                    comp.x0 = std::min(comp.x0, x); comp.y0 = std::min(comp.y0, y);
                    comp.x1 = std::max(comp.x1, x); comp.y1 = std::max(comp.y1, y);
                    comp.sum += d[i];
                    comp.n++;
                    int next[4] = { x > 0 ? i - 1 : -1, x < cw - 1 ? i + 1 : -1, y > 0 ? i - cw : -1, y < ch - 1 ? i + cw : -1 };
                    for (int j : next)
                    {
                        if (j >= 0 && !seen[j] && d[j] >= th)
                        {
                            seen[j] = 1;
                            stack.push_back(j);
                        }
                    }
                }
                if (comp.n >= 4)
                    found.push_back(comp);
            }
            return found;
        };
        auto areaOf = [](const Comp& c) { return (double)(c.x1 - c.x0 + 1)*(c.y1 - c.y0 + 1); };
        double big = 0.22*cw*ch;
        std::vector<Comp> boxes;
        for (double th : { 0.14, 0.2, 0.27, 0.35, 0.45 })
        {
            boxes = placesAt(th);
            bool small = true;
            for (auto& c : boxes)
                small = small && areaOf(c) <= big;
            if (small)
                break;
        }

        // Places that touch or nearly touch are one place - while it stays a part of the screen
        for (bool merged = true; merged;)
        {
            merged = false;
            for (size_t i = 0; i < boxes.size() && !merged; i++)
            {
                for (size_t j = i + 1; j < boxes.size(); j++)
                {
                    const Comp& p = boxes[i];
                    const Comp& q = boxes[j];
                    if (p.x0 <= q.x1 + 2 && q.x0 <= p.x1 + 2 && p.y0 <= q.y1 + 2 && q.y0 <= p.y1 + 2)
                    {
                        Comp u = { std::min(p.x0, q.x0), std::min(p.y0, q.y0), std::max(p.x1, q.x1), std::max(p.y1, q.y1), p.sum + q.sum, p.n + q.n };
                        if (areaOf(u) > big)
                            continue;
                        boxes[i] = u;
                        boxes.erase(boxes.begin() + j);
                        merged = true;
                        break;
                    }
                }
            }
        }
        std::stable_sort(boxes.begin(), boxes.end(), [](const Comp& p, const Comp& q) { return p.sum > q.sum; });

        ShotCompare res;
        res.score = JsRound(score*1000)/1000;
        double sx = w/cw, sy = h/ch;
        for (int i = 0; i < (int)boxes.size() && i < maxRegions; i++)
        {
            const Comp& c = boxes[i];
            if (c.sum/std::max(1e-6, total) < 0.015)
                continue;
            DiffRegion r;
            double x = std::max(0.0, JsRound((c.x0 - 1)*sx)), y = std::max(0.0, JsRound((c.y0 - 1)*sy));
            r.box = Box(x, y, std::min(w - x, JsRound((c.x1 - c.x0 + 3)*sx)), std::min(h - y, JsRound((c.y1 - c.y0 + 3)*sy)));
            r.share = JsRound((c.sum/std::max(1e-6, total))*100)/100;
            res.regions.Add(r);
        }
        return res;
    }

    Vector<String> BlocksIn(const D::UiDoc& doc, const Box& box, int max /*= 6*/)
    {
        D::UiRects rects = D::SolveLayout(doc);
        Vector<Pair<String, double>> found;
        for (auto& id : rects.Ids())
        {
            if (id == "root")
                continue;
            D::UiRect r = rects.Get(id);
            double iw = std::min(r.x + r.w, box.x + box.w) - std::max(r.x, box.x);
            double ih = std::min(r.y + r.h, box.y + box.h) - std::max(r.y, box.y);
            if (iw <= 0 || ih <= 0)
                continue;
            double inter = iw*ih;
            if (inter >= 0.5*r.w*r.h || inter >= 0.35*box.w*box.h)
                found.Add({ id, r.w*r.h });
        }
        StableSortBy(found, [](const Pair<String, double>& a, const Pair<String, double>& b) { return a.second < b.second; });
        Vector<String> out;
        for (int i = 0; i < found.Count() && i < max; i++)
            out.Add(found[i].first);
        return out;
    }

    bool SideBySide(const UiImage& reference, const UiImage& shot, double w, double h, const Vector<DiffRegion>& regions,
                    UiImage& out)
    {
        if (regions.IsEmpty())
            return false;

        const int CW = 460, MAXH = 420, PAD = 12, HEAD = 26;
        struct Row { UiImage ref, now; int h; String label; };
        Vector<Row> rows;
        for (int i = 0; i < regions.Count() && i < 4; i++)
        {
            const Box& b = regions[i].box;
            // Some air around the place, the same crop of both
            double px = std::max(12.0, b.w*0.12), py = std::max(12.0, b.h*0.12);
            double x0 = std::max(0.0, b.x - px), y0 = std::max(0.0, b.y - py);
            double x1 = std::min(w, b.x + b.w + px), y1 = std::min(h, b.y + b.h + py);
            double k = std::min(CW/(x1 - x0), MAXH/(y1 - y0));
            int tw = std::max(8, (int)JsRound((x1 - x0)*k)), th = std::max(8, (int)JsRound((y1 - y0)*k));
            auto crop = [&](const UiImage& img)
            {
                double kx = img.w/w, ky = img.h/h;
                int left = std::min(img.w - 1, (int)std::floor(x0*kx)), top = std::min(img.h - 1, (int)std::floor(y0*ky));
                int cw = std::max(1, std::min(img.w - left, (int)std::ceil((x1 - x0)*kx)));
                int chh = std::max(1, std::min(img.h - top, (int)std::ceil((y1 - y0)*ky)));
                return img.Crop(left, top, cw, chh).Resize(tw, th).Flatten((UInt8)cardBg.r, (UInt8)cardBg.g, (UInt8)cardBg.b);
            };
            rows.Add({ crop(reference), crop(shot), th,
                       (String)(i + 1) + ".  " + Json::FormatNumber(JsRound(b.x)) + "," + Json::FormatNumber(JsRound(b.y)) + "  " +
                       Json::FormatNumber(JsRound(b.w)) + "x" + Json::FormatNumber(JsRound(b.h)) });
        }

        int W = PAD*3 + CW*2, H = PAD;
        for (auto& r : rows)
            H += HEAD + r.h + PAD;
        out = UiImage::Blank(W, H, 0x1B, 0x1D, 0x26, 255);
        int y = PAD;
        for (auto& r : rows)
        {
            DrawLabel(out, r.label + " - reference", PAD, y + 4, Rgb(0xF1, 0xF2, 0xF6), 14);
            DrawLabel(out, "the screen now", PAD*2 + CW, y + 4, Rgb(0x7C, 0xF3, 0xFF), 14);
            out.Blit(r.ref, PAD, y + HEAD);
            out.Blit(r.now, PAD*2 + CW, y + HEAD);
            y += HEAD + r.h + PAD;
        }
        return true;
    }

    UiImage WithPins(const UiImage& img, const Vector<D::UiStepPoint>& points, double w, double h)
    {
        UiImage out = img;
        int r = std::max(11, (int)JsRound(std::max(out.w, out.h)/60.0));
        double ring = std::max(2.0, r/5.0);
        for (int i = 0; i < points.Count(); i++)
        {
            double cx = JsRound((points[i].x/std::max(1.0, w))*out.w), cy = JsRound((points[i].y/std::max(1.0, h))*out.h);
            for (int y = (int)(cy - r - ring); y <= (int)(cy + r + ring); y++)
            {
                for (int x = (int)(cx - r - ring); x <= (int)(cx + r + ring); x++)
                {
                    if (x < 0 || y < 0 || x >= out.w || y >= out.h)
                        continue;
                    double dist = std::sqrt((x - cx)*(x - cx) + (y - cy)*(y - cy));
                    if (dist > r + ring/2)
                        continue;
                    UInt8* p = out.At(x, y);
                    bool rim = dist > r - ring/2;
                    p[0] = rim ? 255 : 0xFF; p[1] = rim ? 255 : 0x2D; p[2] = rim ? 255 : 0x6F; p[3] = 255;
                }
            }
            String n = (String)(i + 1);
            int scale = std::max(1, (int)JsRound(r*1.15/9.0));
            DrawLabel(out, n, cx - n.Length()*3*scale - scale, cy - 5*scale, Rgb(255, 255, 255), r);
        }
        return out;
    }

    UiImage DiffPicture(const UiImage& reference, const UiImage& shot, int max /*= 900*/)
    {
        double k = std::min(1.0, max/(double)std::max(reference.w, reference.h));
        int w = std::max(1, (int)JsRound(reference.w*k)), h = std::max(1, (int)JsRound(reference.h*k));
        UiImage a = reference.Resize(w, h).Flatten((UInt8)cardBg.r, (UInt8)cardBg.g, (UInt8)cardBg.b);
        UiImage b = shot.Resize(w, h).Flatten((UInt8)cardBg.r, (UInt8)cardBg.g, (UInt8)cardBg.b);
        UiImage out(w, h);
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                const UInt8* p = a.At(x, y);
                const UInt8* q = b.At(x, y);
                double d = std::max({ std::abs(p[0] - q[0]), std::abs(p[1] - q[1]), std::abs(p[2] - q[2]) });
                double grey = (0.299*p[0] + 0.587*p[1] + 0.114*p[2])*0.42 + 18;
                // Under 36 of 255 is the same pixel for the eye (resampling, a kit graphic's slightly other shade); from there
                // the heat grows
                double t = std::max(0.0, std::min(1.0, (d - 36)/140));
                double heat = t > 0 ? 0.35 + 0.65*t : 0;
                UInt8* o = out.At(x, y);
                o[0] = (UInt8)JsRound(grey*(1 - heat) + 255*heat);
                o[1] = (UInt8)JsRound(grey*(1 - heat) + (220 - 190*t)*heat);
                o[2] = (UInt8)JsRound(grey*(1 - heat) + 30*heat);
                o[3] = 255;
            }
        }
        return out;
    }

    UiImage ContactStrip(const Vector<UiImage>& pictures, int cell /*= 96*/, int cols /*= 8*/)
    {
        int n = std::min(pictures.Count(), 32);
        int c = std::max(1, std::min(cols, n)), rows = (n + c - 1)/c;
        UiImage out = UiImage::Blank(c*(cell + 8) + 8, std::max(1, rows)*(cell + 8) + 8, 43, 47, 58, 255);
        for (int i = 0; i < n; i++)
        {
            UiImage pic = pictures[i];
            double k = std::min((double)cell/std::max(1, pic.w), (double)cell/std::max(1, pic.h));
            pic = pic.Resize(std::max(1, (int)JsRound(pic.w*k)), std::max(1, (int)JsRound(pic.h*k)));
            out.Blit(pic, 8 + (i%c)*(cell + 8) + (cell - pic.w)/2, 8 + (i/c)*(cell + 8) + (cell - pic.h)/2);
        }
        return out;
    }
}
