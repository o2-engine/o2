#include "o2Editor/stdafx.h"
#include "UiDocSolve.h"

#include <cmath>

#include "o2Editor/Pipeline/UiDoc/UiDocRotate.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    void UiRects::Set(const String& id, const UiRect& rect)
    {
        auto it = mIndex.find(id);
        if (it != mIndex.end())
        {
            mRects[it->second] = rect;
            return;
        }
        mIndex[id] = mIds.Count();
        mIds.Add(id);
        mRects.Add(rect);
    }

    const UiRect* UiRects::Find(const String& id) const
    {
        auto it = mIndex.find(id);
        return it == mIndex.end() ? nullptr : &mRects[it->second];
    }

    UiRect UiRects::Get(const String& id) const
    {
        const UiRect* r = Find(id);
        return r ? *r : UiRect();
    }

    static const Box4 Z4 = { 0, 0, 0, 0 };

    static double DefaultW(const UiNode& n, const UiRect& parent)
    {
        if (n.type == "group" || n.type == "background" || n.type == "divider")
            return parent.w;
        return DefaultSize(n.type).w;
    }

    static double DefaultH(const UiNode& n, const UiRect& parent)
    {
        if (n.type == "group" || n.type == "background")
            return parent.h;
        return DefaultSize(n.type).h;
    }

    UiRect AnchoredRect(const UiNode& n, const UiRect& p)
    {
        Anchor a = ParseAnchor(n.anchor);
        const Box4& m = n.margin ? *n.margin : Z4;
        double mt = m[0], mr = m[1], mb = m[2], ml = m[3];
        double ox = n.x ? *n.x : 0, oy = n.y ? *n.y : 0;
        double x, w, y, h;
        if (a.h == "stretch")
        {
            x = p.x + ml;
            w = std::max(0.0, p.w - ml - mr);
        }
        else
        {
            w = n.w ? *n.w : DefaultW(n, p);
            x = a.h == "left" ? p.x + ox : a.h == "right" ? p.x + p.w - ox - w : p.x + (p.w - w)/2 + ox;
        }
        if (a.v == "stretch")
        {
            y = p.y + mt;
            h = std::max(0.0, p.h - mt - mb);
        }
        else
        {
            h = n.h ? *n.h : DefaultH(n, p);
            y = a.v == "top" ? p.y + oy : a.v == "bottom" ? p.y + p.h - oy - h : p.y + (p.h - h)/2 + oy;
        }
        return UiRect(x, y, w, h);
    }

    static UiRect Inner(const UiNode& n, const UiRect& r)
    {
        const Box4& p = n.pad ? *n.pad : Z4;
        double t = p[0], rr = p[1], b = p[2], l = p[3];
        return UiRect(r.x + l, r.y + t, std::max(0.0, r.w - l - rr), std::max(0.0, r.h - t - b));
    }

    static void SolveChildren(const UiNode& n, const UiRect& r, UiRects& out);

    static void Linear(const UiNode& n, const UiRect& r, const std::vector<const UiNode*>& kids, UiRects& out)
    {
        bool row = n.layout && *n.layout == "row";
        UiRect box = Inner(n, r);
        double gap = n.gap ? *n.gap : 0;
        double mainLen = row ? box.w : box.h;
        double crossLen = row ? box.h : box.w;
        String align = n.align ? *n.align : String("stretch");
        auto own = [&](const UiNode* k) { return row ? k->w : k->h; };
        auto ownCross = [&](const UiNode* k) { return row ? k->h : k->w; };
        auto defMain = [&](const UiNode* k) { return row ? DefaultSize(k->type).w : DefaultSize(k->type).h; };
        auto defCross = [&](const UiNode* k) { return row ? DefaultSize(k->type).h : DefaultSize(k->type).w; };
        auto growOf = [&](const UiNode* k) { return k->grow ? *k->grow : 0.0; };

        std::vector<double> sizes;
        for (auto k : kids)
        {
            auto o = own(k);
            sizes.push_back(o ? *o : (growOf(k) != 0 ? 0.0 : defMain(k)));
        }

        double used = 0;
        for (double s : sizes)
            used = used + s;
        used = used + gap*std::max(0, (int)kids.size() - 1);
        double free = mainLen - used;

        double growSum = 0;
        for (auto k : kids)
            growSum = growSum + growOf(k);
        if (growSum > 0 && free > 0)
        {
            for (size_t i = 0; i < kids.size(); i++)
            {
                if (growOf(kids[i]) != 0)
                    sizes[i] += (free*growOf(kids[i]))/growSum;
            }
        }

        double lead = 0, extra = 0;
        double left = growSum > 0 ? 0 : std::max(0.0, free);
        double count = (double)kids.size();
        String justify = n.justify ? *n.justify : String("start");
        if (justify == "center")
            lead = left/2;
        else if (justify == "end")
            lead = left;
        else if (justify == "between")
        {
            extra = count > 1 ? left/(count - 1) : 0;
            lead = count > 1 ? 0 : left/2;
        }
        else if (justify == "around")
        {
            extra = count ? left/count : 0;
            lead = extra/2;
        }
        else if (justify == "evenly")
        {
            extra = left/(count + 1);
            lead = extra;
        }

        double at = lead;
        for (size_t i = 0; i < kids.size(); i++)
        {
            const UiNode* k = kids[i];
            double main = sizes[i];
            auto given = ownCross(k);
            double cross = given ? *given : (align == "stretch" ? crossLen : defCross(k));
            double c0 = align == "center" ? (crossLen - cross)/2 : align == "end" ? crossLen - cross : 0;
            double kx = k->x ? *k->x : 0, ky = k->y ? *k->y : 0;
            UiRect rect = row ? UiRect(box.x + at + kx, box.y + c0 + ky, main, cross)
                              : UiRect(box.x + c0 + kx, box.y + at + ky, cross, main);
            out.Set(k->id, rect);
            SolveChildren(*k, rect, out);
            at += main + gap + extra;
        }
    }

    UiGridGeometry GridGeometry(const UiNode& n, const UiRect& box, int count)
    {
        UiGridGeometry g;
        g.gx = n.gap ? *n.gap : 0;
        g.gy = n.gapY ? *n.gapY : g.gx;
        double cols = n.cols ? *n.cols : 0;
        if (!(cols != 0 && !std::isnan(cols)))
            cols = n.cellW && *n.cellW != 0 ? std::max(1.0, std::floor((box.w + g.gx)/(*n.cellW + g.gx))) : 3;
        g.cols = cols;
        g.cw = n.cellW ? *n.cellW : std::max(1.0, (box.w - g.gx*(cols - 1))/cols);
        g.ch = n.cellH ? *n.cellH : g.cw;
        double usedCols = std::min(cols, std::max(1.0, (double)count));
        double rows = std::ceil(count/cols);
        g.gridW = usedCols*g.cw + (usedCols - 1)*g.gx;
        g.gridH = rows*g.ch + std::max(0.0, rows - 1)*g.gy;
        return g;
    }

    static void Grid(const UiNode& n, const UiRect& r, const std::vector<const UiNode*>& kids, UiRects& out)
    {
        UiRect box = Inner(n, r);
        UiGridGeometry g = GridGeometry(n, box, (int)kids.size());
        String align = n.align ? *n.align : String(), justify = n.justify ? *n.justify : String();
        double x0 = box.x + (align == "center" ? (box.w - g.gridW)/2 : align == "end" ? box.w - g.gridW : 0);
        double y0 = box.y + (justify == "center" ? (box.h - g.gridH)/2 : justify == "end" ? box.h - g.gridH : 0);
        for (size_t i = 0; i < kids.size(); i++)
        {
            const UiNode* k = kids[i];
            double col = std::fmod((double)i, g.cols);
            double cx = x0 + col*(g.cw + g.gx);
            double cy = y0 + std::floor(i/g.cols)*(g.ch + g.gy);
            double w = k->w ? *k->w : g.cw, h = k->h ? *k->h : g.ch;
            double kx = k->x ? *k->x : 0, ky = k->y ? *k->y : 0;
            UiRect rect(cx + (g.cw - w)/2 + kx, cy + (g.ch - h)/2 + ky, w, h);
            out.Set(k->id, rect);
            SolveChildren(*k, rect, out);
        }
    }

    static void SolveChildren(const UiNode& n, const UiRect& r, UiRects& out)
    {
        std::vector<const UiNode*> kids, hidden;
        for (auto& k : n.children)
            (k.IsShown() ? kids : hidden).push_back(&k);

        String layout = n.layout ? *n.layout : String();
        if (layout == "row" || layout == "column")
            Linear(n, r, kids, out);
        else if (layout == "grid")
            Grid(n, r, kids, out);
        else
        {
            for (auto k : kids)
            {
                UiRect rect = AnchoredRect(*k, r);
                out.Set(k->id, rect);
                SolveChildren(*k, rect, out);
            }
        }

        // Hidden blocks still get a box (the inspector shows it), anchored, outside any flow
        for (auto k : hidden)
        {
            UiRect rect = AnchoredRect(*k, r);
            out.Set(k->id, rect);
            SolveChildren(*k, rect, out);
        }
    }

    UiRects SolveLayout(const UiDoc& doc, double w /*= 0*/, double h /*= 0*/)
    {
        UiRects out;
        UiRect root(0, 0, w > 0 ? w : doc.width, h > 0 ? h : doc.height);
        out.Set(doc.root.id, root);
        SolveChildren(doc.root, root, out);
        return out;
    }

    String HitTest(const UiDoc& doc, const UiRects& rects, double x, double y)
    {
        String hit;
        // back: from the screen to the unturned boxes of this block's frame (UiDocRotate.h) - a turned block takes the point
        // in its own frame, and so do the blocks in it
        std::function<void(const UiNode&, bool, const UiMat&, bool)> visit = [&](const UiNode& n, bool shown, const UiMat& back,
                                                                                  bool turned) {
            bool vis = shown && n.IsShown();
            const UiRect* r = rects.Find(n.id);
            double deg = TurnOf(n);
            UiMat mine = deg && r ? Mul(RotationAbout(-deg, r->x + r->w/2, r->y + r->h/2), back) : back;
            bool mineTurned = turned || (deg && r);
            UiPoint p = mineTurned ? Apply(mine, { x, y }) : UiPoint{ x, y };
            if (vis && r && &n != &doc.root && p.x >= r->x && p.y >= r->y && p.x <= r->x + r->w && p.y <= r->y + r->h)
                hit = n.id;
            for (auto& c : n.children)
                visit(c, vis, mine, mineTurned);
        };
        visit(doc.root, true, UI_IDENTITY, false);
        return hit;
    }
}
