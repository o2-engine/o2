#include "o2Editor/stdafx.h"
#include "UiLayoutMeasure.h"

#include <algorithm>
#include <cmath>
#include <set>

#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"

namespace Editor::UiLayout
{
    double FontSizeForInk(double inkH)
    {
        // Roboto's cap height is about 0.71 of the em
        return Math::Max(4.0, JsRound((inkH/0.71)*4)/4);
    }

    Rgb DominantColor(const UiImage& img, const Box& b, int skipAlphaBelow /*= 128*/)
    {
        Box bb = IntBox(ClampBox(b, img.w, img.h));
        struct Bin { int n = 0; double r = 0, g = 0, b = 0; };
        std::vector<Bin> bins(4096);
        std::vector<int> order;
        int x0 = (int)bb.x, y0 = (int)bb.y, x1 = (int)(bb.x + bb.w), y1 = (int)(bb.y + bb.h);
        for (int y = y0; y < y1; y++)
        {
            for (int x = x0; x < x1; x++)
            {
                if (x < 0 || y < 0 || x >= img.w || y >= img.h)
                    continue;

                const UInt8* p = img.At(x, y);
                if (p[3] < skipAlphaBelow)
                    continue;

                int k = ((p[0] >> 4) << 8) | ((p[1] >> 4) << 4) | (p[2] >> 4);
                Bin& v = bins[k];
                if (v.n == 0)
                    order.push_back(k);
                v.n++; v.r += p[0]; v.g += p[1]; v.b += p[2];
            }
        }

        const Bin* best = nullptr;
        for (int k : order)
        {
            if (!best || bins[k].n > best->n)
                best = &bins[k];
        }
        return best ? Rgb(best->r/best->n, best->g/best->n, best->b/best->n) : Rgb(128, 128, 128);
    }

    namespace
    {
        struct InkComp
        {
            Box              box;
            int              area = 0;
            std::vector<int> pixels;
        };

        // Connected blobs of pixels that differ from the region's backdrop, leaning towards the hint colour when known
        std::vector<InkComp> InkComponents(const UiImage& img, const Box& region, const Rgb* hint)
        {
            Box bb = IntBox(ClampBox(region, img.w, img.h));
            Rgb bg = DominantColor(img, bb);
            int bx = (int)bb.x, by = (int)bb.y, w = (int)bb.w, h = (int)bb.h;
            std::vector<float> dist((size_t)w*h, 0.0f);
            double maxD = 0;
            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    int gx = bx + x, gy = by + y;
                    double d = gx < img.w && gy < img.h ? ColorDist(RgbAt(img, gx, gy), bg) : 0.0;
                    dist[(size_t)y*w + x] = (float)d;
                    if (d > maxD)
                        maxD = d;
                }
            }

            double thr = Math::Max(48.0, maxD*0.45);
            std::vector<UInt8> mask((size_t)w*h, 0);
            for (int i = 0; i < w*h; i++)
            {
                if (dist[i] < thr)
                    continue;

                if (hint)
                {
                    Rgb px = RgbAt(img, bx + (i % w), by + i/w);
                    if (ColorDist(px, *hint) > ColorDist(px, bg))
                        continue;
                }
                mask[i] = 1;
            }

            std::vector<int> labels;
            std::vector<Component> comps;
            Components(mask, w, h, labels, comps);

            std::vector<InkComp> out;
            std::vector<int> indexOfLabel(comps.size() + 1, -1);
            for (auto& c : comps)
            {
                if (c.area < 3)
                    continue;

                InkComp ink;
                ink.box = Box(c.box.x + bx, c.box.y + by, c.box.w, c.box.h);
                ink.area = c.area;
                indexOfLabel[c.label] = (int)out.size();
                out.push_back(ink);
            }

            double core = Math::Max(thr, maxD*0.75);
            for (int i = 0; i < w*h; i++)
            {
                int index = labels[i] ? indexOfLabel[labels[i]] : -1;
                if (index >= 0 && dist[i] >= core)
                    out[index].pixels.push_back((by + i/w)*img.w + bx + (i % w));
            }
            return out;
        }

        bool UnionBox(const std::vector<const InkComp*>& comps, Box& out)
        {
            if (comps.empty())
                return false;

            double x0 = INFINITY, y0 = INFINITY, x1 = -INFINITY, y1 = -INFINITY;
            for (auto c : comps)
            {
                x0 = Math::Min(x0, c->box.x); y0 = Math::Min(y0, c->box.y);
                x1 = Math::Max(x1, c->box.x + c->box.w); y1 = Math::Max(y1, c->box.y + c->box.h);
            }
            out = Box(x0, y0, x1 - x0, y1 - y0);
            return true;
        }

        String InkColor(const UiImage& img, const std::vector<const InkComp*>& comps)
        {
            std::vector<double> r, g, b;
            for (auto c : comps)
            {
                for (int p : c->pixels)
                {
                    const UInt8* px = img.data.data() + (size_t)p*4;
                    r.push_back(px[0]); g.push_back(px[1]); b.push_back(px[2]);
                }
            }
            return r.empty() ? String() : HexOf(Rgb(Median(r), Median(g), Median(b)));
        }

        // Median colour of the darkest third of the pixels next to (but not in) the ink cores: the outline
        String RingColor(const UiImage& img, const std::vector<const InkComp*>& comps)
        {
            std::set<int> core;
            std::vector<int> coreOrder;
            for (auto c : comps)
            {
                for (int p : c->pixels)
                {
                    if (core.insert(p).second)
                        coreOrder.push_back(p);
                }
            }

            std::vector<double> r, g, b;
            std::set<int> seen;
            static const int offsets[8][2] = { { -2, 0 }, { 2, 0 }, { 0, -2 }, { 0, 2 }, { -1, -1 }, { 1, 1 }, { -1, 1 }, { 1, -1 } };
            for (int p : coreOrder)
            {
                int x = p % img.w, y = p / img.w;
                for (auto& o : offsets)
                {
                    int xx = x + o[0], yy = y + o[1];
                    if (xx < 0 || yy < 0 || xx >= img.w || yy >= img.h)
                        continue;

                    int q = yy*img.w + xx;
                    if (core.count(q) || seen.count(q))
                        continue;

                    seen.insert(q);
                    const UInt8* px = img.data.data() + (size_t)q*4;
                    r.push_back(px[0]); g.push_back(px[1]); b.push_back(px[2]);
                }
            }

            if (r.size() < 8)
                return String();

            std::vector<double> lum(r.size());
            for (size_t i = 0; i < r.size(); i++)
                lum[i] = 0.299*r[i] + 0.587*g[i] + 0.114*b[i];

            std::vector<int> order(r.size());
            for (size_t i = 0; i < order.size(); i++)
                order[i] = (int)i;
            std::stable_sort(order.begin(), order.end(), [&](int a, int z) { return lum[a] < lum[z]; });
            order.resize(Math::Max((size_t)4, (size_t)std::floor(lum.size()/3.0)));

            std::vector<double> rr, gg, bb;
            for (int i : order)
            {
                rr.push_back(r[i]); gg.push_back(g[i]); bb.push_back(b[i]);
            }
            return HexOf(Rgb(Median(rr), Median(gg), Median(bb)));
        }

        void ApplyInk(const UiImage& img, UiEl& el, const std::vector<const InkComp*>& comps)
        {
            Box ink;
            if (!UnionBox(comps, ink) || ink.h < 3)
                return;

            el.ink = ink;
            String color = InkColor(img, comps);
            if (!color.IsEmpty())
                el.color = color;

            if (!el.stroke.IsEmpty())
            {
                String ring = RingColor(img, comps);
                if (!ring.IsEmpty())
                    el.stroke = ring;
                el.strokeWidth = 1.0;
            }

            el.fontSize = FontSizeForInk(ink.h);
            el.box = TextBoxFor(ink, *el.fontSize, el.align.IsEmpty() ? String("center") : el.align);
            el.measured = "ink";
        }
    }

    void MeasureText(const UiImage& img, UiEl& el)
    {
        Box region = Grow(el.box, Math::Max(4.0, el.box.w*0.12), Math::Max(4.0, el.box.h*0.35));
        Rgb hint;
        bool hasHint = ParseHex(el.color, hint);
        auto comps = InkComponents(img, region, hasHint ? &hint : nullptr);
        Box core = Grow(el.box, Math::Max(2.0, el.box.w*0.04), Math::Max(2.0, el.box.h*0.15));
        std::vector<const InkComp*> keep;
        for (auto& c : comps)
        {
            double cx = c.box.x + c.box.w/2, cy = c.box.y + c.box.h/2;
            if (cx >= core.x && cx <= core.x + core.w && cy >= core.y && cy <= core.y + core.h)
                keep.push_back(&c);
        }
        ApplyInk(img, el, keep);
    }

    void MeasureCellTexts(const UiImage& img, const UiEl& cell, const Vector<Ref<UiEl>>& texts)
    {
        if (texts.IsEmpty())
            return;

        Box inner = Grow(cell.box, -Math::Max(2.0, cell.box.w*0.1), -Math::Max(2.0, cell.box.h*0.1));
        auto found = InkComponents(img, inner, nullptr);
        Box ib = IntBox(ClampBox(inner, img.w, img.h));

        std::vector<const InkComp*> comps;
        for (auto& c : found)
        {
            bool touches = c.box.x <= ib.x || c.box.y <= ib.y || c.box.x + c.box.w >= ib.x + ib.w || c.box.y + c.box.h >= ib.y + ib.h;
            if (touches)
            {
                bool thin = Math::Min(c.box.w, c.box.h) <= 0.2*Math::Max(c.box.w, c.box.h);
                bool spans = c.box.w >= ib.w*0.92 || c.box.h >= ib.h*0.92;
                if (thin || spans)
                    continue;
            }
            comps.push_back(&c);
        }

        if (comps.empty())
            return;

        double maxH = 0;
        for (auto c : comps)
            maxH = Math::Max(maxH, c->box.h);

        std::vector<const InkComp*> main, rest;
        for (auto c : comps)
            (c->box.h >= maxH*0.55 ? main : rest).push_back(c);

        Box mainBox;
        UnionBox(main, mainBox);

        std::vector<const InkComp*> inMain, mainAll = main;
        for (auto c : rest)
        {
            if (c->box.x >= mainBox.x - 1 && c->box.x + c->box.w <= mainBox.x + mainBox.w + 1 &&
                c->box.y >= mainBox.y - 1 && c->box.y + c->box.h <= mainBox.y + mainBox.h + 1)
            {
                inMain.push_back(c);
                mainAll.push_back(c);
            }
        }

        Vector<Ref<UiEl>> sorted = texts;
        StableSortBy(sorted, [](const Ref<UiEl>& a, const Ref<UiEl>& b) { return a->slot.value_or(0) < b->slot.value_or(0); });
        ApplyInk(img, *sorted[0], mainAll);

        std::vector<const InkComp*> cand;
        for (auto c : rest)
        {
            if (std::find(inMain.begin(), inMain.end(), c) != inMain.end())
                continue;
            if (c->area >= 4 && c->box.h <= mainBox.h*0.6 && c->box.w <= mainBox.w*0.9 &&
                Math::Min(c->box.w, c->box.h) > 0.22*Math::Max(c->box.w, c->box.h))
            {
                cand.push_back(c);
            }
        }

        if (sorted.Count() > 1 && !cand.empty())
        {
            const InkComp* seed = cand[0];
            for (auto c : cand)
            {
                if (c->area > seed->area)
                    seed = c;
            }

            std::vector<const InkComp*> group = { seed };
            for (bool grew = true; grew;)
            {
                grew = false;
                for (auto c : cand)
                {
                    if (std::find(group.begin(), group.end(), c) != group.end())
                        continue;

                    Box u;
                    UnionBox(group, u);
                    double gap = Math::Max(c->box.x - (u.x + u.w), u.x - (c->box.x + c->box.w));
                    double overlapY = Math::Min(u.y + u.h, c->box.y + c->box.h) - Math::Max(u.y, c->box.y);
                    if (gap <= seed->box.h*0.6 && overlapY > Math::Min(u.h, c->box.h)*0.5)
                    {
                        group.push_back(c);
                        grew = true;
                    }
                }
            }
            ApplyInk(img, *sorted[1], group);
        }
    }

    Box TextBoxFor(const Box& ink, double fs, const String& align)
    {
        double pad = JsRound(fs*0.3), boxH = Math::Max(ink.h + 2, JsRound(fs*1.25));
        double y = JsRound(ink.y + ink.h/2 - boxH/2);
        if (align == "left")
            return Box(std::floor(ink.x), y, std::ceil(ink.w + pad), boxH);
        if (align == "right")
            return Box(std::floor(ink.x - pad), y, std::ceil(ink.w + pad), boxH);
        return Box(std::floor(ink.x - pad), y, std::ceil(ink.w + 2*pad), boxH);
    }
}
