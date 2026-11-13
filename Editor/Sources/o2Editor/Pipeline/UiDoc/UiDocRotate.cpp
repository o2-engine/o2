#include "o2Editor/stdafx.h"
#include "UiDocRotate.h"

#include <cmath>
#include <functional>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    namespace
    {
        constexpr double kPi = 3.141592653589793;
    }

    UiMat Mul(const UiMat& m, const UiMat& n)
    {
        return {
            m[0]*n[0] + m[2]*n[1], m[1]*n[0] + m[3]*n[1],
            m[0]*n[2] + m[2]*n[3], m[1]*n[2] + m[3]*n[3],
            m[0]*n[4] + m[2]*n[5] + m[4], m[1]*n[4] + m[3]*n[5] + m[5],
        };
    }

    UiMat RotationAbout(double deg, double cx, double cy)
    {
        if (!deg)
            return UI_IDENTITY;

        double a = (deg*kPi)/180, c = std::cos(a), s = std::sin(a);
        return { c, s, -s, c, cx - c*cx + s*cy, cy - s*cx - c*cy };
    }

    UiPoint Apply(const UiMat& m, const UiPoint& p)
    {
        return { m[0]*p.x + m[2]*p.y + m[4], m[1]*p.x + m[3]*p.y + m[5] };
    }

    UiPoint ApplyLinear(const UiMat& m, const UiPoint& v)
    {
        return { m[0]*v.x + m[2]*v.y, m[1]*v.x + m[3]*v.y };
    }

    UiMat Invert(const UiMat& m)
    {
        double det = m[0]*m[3] - m[1]*m[2];
        if (!det)
            det = 1;
        double a = m[3]/det, b = -m[1]/det, c = -m[2]/det, d = m[0]/det;
        return { a, b, c, d, -(a*m[4] + c*m[5]), -(b*m[4] + d*m[5]) };
    }

    UiPoint Centre(const UiRect& r)
    {
        return { r.x + r.w/2, r.y + r.h/2 };
    }

    double TurnOf(const UiNode& n)
    {
        return n.rotation && *n.rotation ? *n.rotation : 0.0;
    }

    Vector<const UiNode*> PathTo(const UiDoc& doc, const String& id)
    {
        Vector<const UiNode*> path;
        std::function<bool(const UiNode&)> find = [&](const UiNode& n) {
            path.Add(&n);
            if (n.id == id)
                return true;
            for (auto& c : n.children)
            {
                if (find(c))
                    return true;
            }
            path.PopBack();
            return false;
        };
        if (!find(doc.root))
            path.Clear();
        return path;
    }

    UiMat WorldMatrix(const UiDoc& doc, const UiRects& rects, const String& id, bool self /*= true*/)
    {
        auto path = PathTo(doc, id);
        UiMat m = UI_IDENTITY;
        int count = self ? path.Count() : path.Count() - 1;
        for (int i = 0; i < count; i++)
        {
            double deg = TurnOf(*path[i]);
            const UiRect* r = rects.Find(path[i]->id);
            if (deg && r)
            {
                UiPoint c = Centre(*r);
                m = Mul(m, RotationAbout(deg, c.x, c.y));
            }
        }
        return m;
    }

    double WorldTurn(const UiDoc& doc, const String& id, bool self /*= true*/)
    {
        auto path = PathTo(doc, id);
        double sum = 0;
        int count = self ? path.Count() : path.Count() - 1;
        for (int i = 0; i < count; i++)
            sum = sum + TurnOf(*path[i]);
        return sum;
    }

    std::array<UiPoint, 4> Corners(const UiRect& r, const UiMat& m)
    {
        return { Apply(m, { r.x, r.y }), Apply(m, { r.x + r.w, r.y }), Apply(m, { r.x + r.w, r.y + r.h }), Apply(m, { r.x, r.y + r.h }) };
    }

    bool InBlock(const UiDoc& doc, const UiRects& rects, const String& id, const UiPoint& p)
    {
        const UiRect* r = rects.Find(id);
        if (!r)
            return false;

        UiPoint q = Apply(Invert(WorldMatrix(doc, rects, id)), p);
        return q.x >= r->x && q.y >= r->y && q.x <= r->x + r->w && q.y <= r->y + r->h;
    }

    std::optional<double> CleanTurn(double n)
    {
        if (!std::isfinite(n))
            return std::nullopt;

        double d = std::fmod(n, 360.0);
        if (d > 180)
            d -= 360;
        if (d <= -180)
            d += 360;
        d = JsRound(d*10)/10;
        if (d == 0)
            return std::nullopt;
        return d;
    }

    std::optional<double> CleanTurn(const Json* v)
    {
        return CleanTurn(JsNumber(v));
    }

    UiSize TurnedBounds(double w, double h, double deg)
    {
        double a = (deg*kPi)/180, c = std::abs(std::cos(a)), s = std::abs(std::sin(a));
        return { w*c + h*s, w*s + h*c };
    }

    UiSize UnturnedSize(double W, double H, double deg, std::optional<double> aspect /*= std::nullopt*/)
    {
        double a = (deg*kPi)/180, c = std::abs(std::cos(a)), s = std::abs(std::sin(a));
        if (!s)
            return { W, H };

        if (aspect && *aspect > 0)
        {
            double h = std::min(W/(*aspect*c + s), H/(*aspect*s + c));
            return { *aspect*h, h };
        }

        // cos 2θ of the folded angle: away from 45° (either side, so 170° reads like 10° and 100° like a quarter turn) the
        // box's sides solve for the block's own; near 45° they don't, and the block is taken as a square
        double det = c*c - s*s;
        if (std::abs(det) > 0.3)
        {
            double w = (W*c - H*s)/det, h = (H*c - W*s)/det;
            if (w > 0 && h > 0)
                return { w, h };
        }

        double side = std::min(W, H)/(c + s);
        return { side, side };
    }
}
