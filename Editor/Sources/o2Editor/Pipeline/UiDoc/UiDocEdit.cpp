#include "o2Editor/stdafx.h"
#include "UiDocEdit.h"

#include <cmath>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    UiPatch PlaceAt(const UiNode& n, const UiRect& p, const UiRect& r, const Anchor* anchor /*= nullptr*/, bool size /*= false*/)
    {
        Anchor before = ParseAnchor(n.anchor);
        Anchor a = anchor ? *anchor : before;
        UiPatch patch;
        Vector<String> unset;
        auto own = [&](const char* k) -> std::optional<double> {
            String key = k;
            return key == "x" ? n.x : key == "y" ? n.y : key == "w" ? n.w : n.h;
        };
        auto put = [&](const char* k, std::optional<double> v) {
            String key = k;
            // An offset of 0 is the default: it goes rather than stays as a 0
            if (!v || ((key == "x" || key == "y") && *v == 0))
            {
                if (own(k) && !unset.Contains(key))
                    unset.Add(key);
                return;
            }
            auto o = own(k);
            if (!o || *o != *v)
                patch.set[key] = *v;
        };

        Box4 m = { 0, 0, 0, 0 };
        if (a.h == "stretch")
        {
            m[3] = JsRound(r.x - p.x);
            m[1] = JsRound(p.x + p.w - r.x - r.w);
            put("x", std::nullopt);
            put("w", std::nullopt);
        }
        else
        {
            double x = a.h == "left" ? r.x - p.x : a.h == "right" ? p.x + p.w - r.x - r.w : r.x + r.w/2 - (p.x + p.w/2);
            put("x", JsRound(x));
            if (size || before.h == "stretch")
                put("w", std::max(1.0, JsRound(r.w)));
        }
        if (a.v == "stretch")
        {
            m[0] = JsRound(r.y - p.y);
            m[2] = JsRound(p.y + p.h - r.y - r.h);
            put("y", std::nullopt);
            put("h", std::nullopt);
        }
        else
        {
            double y = a.v == "top" ? r.y - p.y : a.v == "bottom" ? p.y + p.h - r.y - r.h : r.y + r.h/2 - (p.y + p.h/2);
            put("y", JsRound(y));
            if (size || before.v == "stretch")
                put("h", std::max(1.0, JsRound(r.h)));
        }

        bool allZero = m[0] == 0 && m[1] == 0 && m[2] == 0 && m[3] == 0;
        if (allZero)
        {
            if (n.margin && !unset.Contains("margin"))
                unset.Add("margin");
        }
        else if (!n.margin || (*n.margin)[0] != m[0] || (*n.margin)[1] != m[1] || (*n.margin)[2] != m[2] || (*n.margin)[3] != m[3])
        {
            Json margin = Json::Array();
            for (double v : m)
                margin.Add(Json(v));
            patch.set["margin"] = margin;
        }

        if (anchor && AnchorName(a) != AnchorName(before))
            patch.set["anchor"] = AnchorName(a);

        for (auto& k : unset)
        {
            if (!patch.set.Has(k))
                patch.unset.Add(k);
        }
        return patch;
    }

    UiPatch SizeInLayout(const UiNode& n, const UiNode& parent, const UiRect& from, const UiRect& to)
    {
        UiPatch patch;
        double w = std::max(1.0, JsRound(to.w)), h = std::max(1.0, JsRound(to.h));
        if (w != JsRound(from.w))
            patch.set["w"] = w;
        if (h != JsRound(from.h))
            patch.set["h"] = h;

        String layout = parent.layout ? *parent.layout : String();
        bool mainChanged = layout == "row" ? patch.set.Has("w") : layout == "column" ? patch.set.Has("h") : false;
        if (mainChanged && n.grow && *n.grow != 0)
            patch.unset.Add("grow");
        return patch;
    }

    bool PatchOp(const String& id, const UiPatch& patch, UiOp& out)
    {
        if (patch.IsEmpty())
            return false;
        out = MakeUpdateOp(id, patch.set, patch.unset);
        return true;
    }

    Anchor AnchorAt(const UiRect& p, double x, double y)
    {
        double fx = p.w > 0 ? (x - p.x)/p.w : 0.5, fy = p.h > 0 ? (y - p.y)/p.h : 0.5;
        Anchor a;
        a.h = fx < 0.25 ? "left" : fx > 0.75 ? "right" : "center";
        a.v = fy < 0.25 ? "top" : fy > 0.75 ? "bottom" : "middle";
        return a;
    }

    UiAnchorBox AnchorBox(const UiNode& n, const UiRect& p)
    {
        Anchor a = ParseAnchor(n.anchor);
        auto fx = [](const String& h, double& a0, double& a1) {
            if (h == "left") { a0 = 0; a1 = 0; }
            else if (h == "right") { a0 = 1; a1 = 1; }
            else if (h == "center") { a0 = 0.5; a1 = 0.5; }
            else { a0 = 0; a1 = 1; }
        };
        auto fy = [](const String& v, double& a0, double& a1) {
            if (v == "top") { a0 = 0; a1 = 0; }
            else if (v == "bottom") { a0 = 1; a1 = 1; }
            else if (v == "middle") { a0 = 0.5; a1 = 0.5; }
            else { a0 = 0; a1 = 1; }
        };
        double ax0, ax1, ay0, ay1;
        fx(a.h, ax0, ax1);
        fy(a.v, ay0, ay1);
        return UiAnchorBox{ p.x + p.w*ax0, p.y + p.h*ay0, p.x + p.w*ax1, p.y + p.h*ay1 };
    }

    bool LayoutSlot(const UiNode& parent, const UiRects& rects, const String& dragId, double x, double y, UiSlot& out)
    {
        std::vector<const UiNode*> others;
        for (auto& k : parent.children)
        {
            if (k.IsShown() && k.id != dragId && rects.Has(k.id))
                others.push_back(&k);
        }
        if (others.empty())
            return false;

        int i;
        if (parent.layout && *parent.layout == "grid")
        {
            int best = 0;
            double bestD = INFINITY;
            for (int j = 0; j < (int)others.size(); j++)
            {
                UiRect r = rects.Get(others[j]->id);
                double d = std::hypot(x - (r.x + r.w/2), y - (r.y + r.h/2));
                if (d < bestD)
                {
                    bestD = d;
                    best = j;
                }
            }
            UiRect r = rects.Get(others[best]->id);
            i = best + (x > r.x + r.w/2 ? 1 : 0);
        }
        else
        {
            bool row = parent.layout && *parent.layout == "row";
            i = -1;
            for (int j = 0; j < (int)others.size(); j++)
            {
                UiRect r = rects.Get(others[j]->id);
                if (row ? x < r.x + r.w/2 : y < r.y + r.h/2)
                {
                    i = j;
                    break;
                }
            }
            if (i < 0)
                i = (int)others.size();
        }

        out = UiSlot();
        if (i < (int)others.size())
            out.before = others[i]->id;
        else
            out.after = others.back()->id;
        return true;
    }

    bool MoveIsNoop(const UiDoc& doc, const UiOp& op)
    {
        const Json* id = JsGet(op, "id");
        const UiNode* parent = id && id->IsString() ? FindParent(doc, id->Str()) : nullptr;
        const Json* parentId = JsGet(op, "parent");
        if (!parent || !parentId || !parentId->IsString() || parent->id != parentId->Str())
            return false;

        std::vector<const UiNode*> kids;
        for (auto& k : parent->children)
        {
            if (k.IsShown())
                kids.push_back(&k);
        }
        int i = -1;
        for (int j = 0; j < (int)kids.size(); j++)
        {
            if (kids[j]->id == id->Str())
            {
                i = j;
                break;
            }
        }

        auto idAt = [&](int k) { return k >= 0 && k < (int)kids.size() ? &kids[k]->id : nullptr; };
        const Json* before = JsGet(op, "before");
        const Json* after = JsGet(op, "after");
        if (JsTruthy(before))
        {
            const String* at = idAt(i + 1);
            return at && before->IsString() && *at == before->Str();
        }
        if (JsTruthy(after))
        {
            const String* at = idAt(i - 1);
            return at && after->IsString() && *at == after->Str();
        }
        return false;
    }

    // --- snapping -----------------------------------------------------------------------------------------------

    UiSnapTargets SnapTargets(const UiDoc& doc, const UiRects& rects, const String& id)
    {
        UiSnapTargets t;
        const UiNode* parent = FindParent(doc, id);
        const UiRect* p = parent ? rects.Find(parent->id) : nullptr;
        auto add = [&](const UiRect& r) {
            t.xs.Add(r.x);
            t.xs.Add(r.x + r.w/2);
            t.xs.Add(r.x + r.w);
            t.ys.Add(r.y);
            t.ys.Add(r.y + r.h/2);
            t.ys.Add(r.y + r.h);
        };
        if (p)
            add(*p);
        if (parent)
        {
            for (auto& k : parent->children)
            {
                const UiRect* r = rects.Find(k.id);
                if (k.id != id && k.IsShown() && r)
                    add(*r);
            }
        }
        t.span = p ? *p : UiRect(0, 0, doc.width, doc.height);
        return t;
    }

    // The closest target within tol of any candidate: its distance (signed) and where it is
    static bool Nearest(const Vector<double>& cands, const Vector<double>& targets, double tol, double& d, double& at)
    {
        bool found = false;
        for (double c : cands)
        {
            for (double t : targets)
            {
                double dd = t - c;
                if (std::fabs(dd) <= tol && (!found || std::fabs(dd) < std::fabs(d)))
                {
                    d = dd;
                    at = t;
                    found = true;
                }
            }
        }
        return found;
    }

    static UiSnapLine LineOf(char axis, double at, const UiRect& r, const UiRect& span)
    {
        if (axis == 'x')
            return UiSnapLine{ axis, at, std::min(r.y, span.y), std::max(r.y + r.h, span.y + span.h) };
        return UiSnapLine{ axis, at, std::min(r.x, span.x), std::max(r.x + r.w, span.x + span.w) };
    }

    UiSnap SnapMove(const UiRect& r, const UiSnapTargets& t, double tol)
    {
        double dx = 0, atx = 0, dy = 0, aty = 0;
        bool sx = Nearest({ r.x, r.x + r.w/2, r.x + r.w }, t.xs, tol, dx, atx);
        bool sy = Nearest({ r.y, r.y + r.h/2, r.y + r.h }, t.ys, tol, dy, aty);
        UiSnap out;
        out.rect = UiRect(r.x + (sx ? dx : 0), r.y + (sy ? dy : 0), r.w, r.h);
        if (sx)
            out.lines.Add(LineOf('x', atx, out.rect, t.span));
        if (sy)
            out.lines.Add(LineOf('y', aty, out.rect, t.span));
        return out;
    }

    UiSnap SnapResize(const UiRect& r, const UiEdges& edges, const UiSnapTargets& t, double tol)
    {
        double x = r.x, y = r.y, w = r.w, h = r.h, d = 0, at = 0;
        UiSnap out;
        if (edges.e && Nearest({ x + w }, t.xs, tol, d, at))
        {
            w += d;
            out.lines.Add(LineOf('x', at, r, t.span));
        }
        if (edges.w && Nearest({ x }, t.xs, tol, d, at))
        {
            x += d;
            w -= d;
            out.lines.Add(LineOf('x', at, r, t.span));
        }
        if (edges.s && Nearest({ y + h }, t.ys, tol, d, at))
        {
            h += d;
            out.lines.Add(LineOf('y', at, r, t.span));
        }
        if (edges.n && Nearest({ y }, t.ys, tol, d, at))
        {
            y += d;
            h -= d;
            out.lines.Add(LineOf('y', at, r, t.span));
        }
        out.rect = UiRect(x, y, std::max(1.0, w), std::max(1.0, h));
        return out;
    }

    // --- keyboard -----------------------------------------------------------------------------------------------

    bool NudgeOp(const UiDoc& doc, const UiRects& rects, const String& id, double dx, double dy, UiOp& out)
    {
        const UiNode* n = FindNode(doc, id);
        const UiNode* parent = FindParent(doc, id);
        const UiRect* r = rects.Find(id);
        const UiRect* p = parent ? rects.Find(parent->id) : nullptr;
        if (!n || !parent || !r || !p)
            return false;

        if (parent->layout && !parent->layout->IsEmpty())
        {
            UiPatch patch;
            double nx = (n->x ? *n->x : 0) + dx, ny = (n->y ? *n->y : 0) + dy;
            if (dx != 0)
            {
                if (nx != 0) patch.set["x"] = nx;
                else patch.unset.Add("x");
            }
            if (dy != 0)
            {
                if (ny != 0) patch.set["y"] = ny;
                else patch.unset.Add("y");
            }
            return PatchOp(id, patch, out);
        }
        return PatchOp(id, PlaceAt(*n, *p, UiRect(r->x + dx, r->y + dy, r->w, r->h)), out);
    }

    Vector<UiOp> ReparentOps(const UiDoc& doc, const UiRects& rects, const String& id, const String& parentId,
                             const String& before /*= ""*/, const String& after /*= ""*/)
    {
        Json move = Json::Object();
        move["op"] = "move";
        move["id"] = id;
        move["parent"] = parentId;
        if (!before.IsEmpty())
            move["before"] = before;
        if (!after.IsEmpty())
            move["after"] = after;

        const UiNode* n = FindNode(doc, id);
        const UiNode* from = FindParent(doc, id);
        const UiNode* to = FindNode(doc, parentId);
        const UiRect* r = rects.Find(id);
        const UiRect* p = rects.Find(parentId);
        if (!n || !from || !to || from->id == to->id)
            return { move };

        // Into a row / column / grid: the layout places it, and offsets would only nudge it off its slot
        if (to->layout && !to->layout->IsEmpty())
        {
            Vector<String> unset;
            if (n->x) unset.Add("x");
            if (n->y) unset.Add("y");
            if (unset.IsEmpty())
                return { move };
            Json update = Json::Object();
            update["op"] = "update";
            update["id"] = id;
            update["unset"] = JsonStrings(unset);
            return { move, update };
        }

        if (!r || !p || !n->IsShown())
            return { move };

        UiOp fix;
        if (PatchOp(id, PlaceAt(*n, *p, *r, nullptr, true), fix))
            return { move, fix };
        return { move };
    }
}
