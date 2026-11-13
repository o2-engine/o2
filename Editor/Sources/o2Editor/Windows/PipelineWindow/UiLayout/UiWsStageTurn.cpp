#include "o2Editor/stdafx.h"
#include "UiWsStage.h"

#include <cmath>

#include "o2/Application/Input.h"
#include "o2Editor/Pipeline/UiDoc/UiDocOps.h"

// The stage's turned blocks (UiWsStage.h, port of the turned parts of AssetsLine uiLayout/Stage.tsx): where a block's
// box is on the screen with its turns and its parents' (UiDocRotate.h), the selection, its handles, the anchor's
// triangles and the turning knob drawn turned, the knob's gesture
namespace Editor
{
    using namespace UiWsDraw;
    using UiDoc::UiMat;
    using UiDoc::UiPoint;
    using UiDoc::UiRect;

    namespace
    {
        constexpr double kPi = 3.141592653589793;

        // The knob's distance above the top edge, its radius and its grip, pixels
        const float knobStalk = 26.0f, knobRadius = 5.5f;

        bool IsIdentity(const UiMat& m)
        {
            return m == UiDoc::UI_IDENTITY;
        }

        // A y-down offset on the screen turned clockwise by deg -> a world offset (y up)
        Vec2F TurnedOffset(double x, double y, double deg)
        {
            double a = (deg*kPi)/180, c = std::cos(a), s = std::sin(a);
            return Vec2F((float)(c*x - s*y), (float)-(s*x + c*y));
        }
    }

    UiMat UiWsStage::BlockMatrix(const String& id, bool self /*= true*/) const
    {
        return UiDoc::WorldMatrix(Shown(), ShownRects(), id, self);
    }

    std::array<Vec2F, 4> UiWsStage::WorldCorners(const UiRect& r, const UiMat& m) const
    {
        auto cs = UiDoc::Corners(r, m);
        std::array<Vec2F, 4> out;
        for (int i = 0; i < 4; i++)
            out[i] = DesignToWorld(cs[i].x, cs[i].y);
        return out;
    }

    Vec2F UiWsStage::SelectionAt(double fx, double fy) const
    {
        const UiRect* sel = ctx && !ctx->selected.IsEmpty() ? ShownRects().Find(ctx->selected) : nullptr;
        if (!sel)
            return Vec2F();

        UiPoint p = UiDoc::Apply(BlockMatrix(ctx->selected), { sel->x + sel->w*fx, sel->y + sel->h*fy });
        return DesignToWorld(p.x, p.y);
    }

    bool UiWsStage::KnobPoints(Vec2F& foot, Vec2F& knob) const
    {
        if (!ctx || ctx->selected.IsEmpty())
            return false;

        const auto& doc = Shown();
        const UiDoc::UiNode* n = UiDoc::FindNode(doc, ctx->selected);
        if (!n || n == &doc.root || !ShownRects().Find(n->id))
            return false;

        // Above the top edge's middle, along the block's own "up"
        double turn = UiDoc::WorldTurn(doc, n->id);
        foot = SelectionAt(0.5, 0);
        knob = foot + TurnedOffset(0, -knobStalk, turn);
        return true;
    }

    bool UiWsStage::KnobPoint(Vec2F& point) const
    {
        Vec2F foot;
        return KnobPoints(foot, point);
    }

    bool UiWsStage::AnchorTriangles(std::array<std::array<Vec2F, 3>, 4>& out) const
    {
        if (!ctx || ctx->selected.IsEmpty())
            return false;

        const auto& doc = Shown();
        const UiDoc::UiNode* n = UiDoc::FindNode(doc, ctx->selected);
        const UiDoc::UiNode* parent = n ? UiDoc::FindParent(doc, n->id) : nullptr;
        const UiRect* p = parent ? ShownRects().Find(parent->id) : nullptr;
        if (!n || !parent || parent->layout || !p || !n->IsShown())
            return false;

        // On the parent's box: turned with it
        auto ab = UiDoc::AnchorBox(*n, *p);
        UiMat pm = BlockMatrix(parent->id);
        double turn = UiDoc::WorldTurn(doc, parent->id);
        auto corner = [&](double x, double y, double sx, double sy)
        {
            UiPoint q = UiDoc::Apply(pm, { x, y });
            Vec2F a = DesignToWorld(q.x, q.y);
            return std::array<Vec2F, 3>{ a, a + TurnedOffset(sx*4.5, sy*14.0, turn), a + TurnedOffset(sx*14.0, sy*4.5, turn) };
        };
        out = { corner(ab.x0, ab.y0, -1, -1), corner(ab.x1, ab.y0, 1, -1), corner(ab.x0, ab.y1, -1, 1), corner(ab.x1, ab.y1, 1, 1) };
        return true;
    }

    CursorType UiWsStage::HandleCursor(const String& handle) const
    {
        static const char* const order[] = { "n", "ne", "e", "se", "s", "sw", "w", "nw" };
        int i0 = 0;
        for (int i = 0; i < 8; i++)
        {
            if (handle == order[i])
                i0 = i;
        }
        double turn = ctx && !ctx->selected.IsEmpty() ? UiDoc::WorldTurn(Shown(), ctx->selected) : 0.0;
        int steps = (int)UiDoc::JsRound(std::fmod(std::fmod(turn, 360.0) + 360.0, 360.0)/45.0);
        String h = order[(i0 + steps) % 8];
        if (h == "n" || h == "s") return CursorType::SizeNS;
        if (h == "e" || h == "w") return CursorType::SizeWE;
        if (h == "ne" || h == "sw") return CursorType::SizeNeSw;
        return CursorType::SizeNwSe;
    }

    void UiWsStage::TurnedFrame(const UiRect& r, const UiMat& m, const Color4& color, float width, float dash /*= 0*/,
                                float gap /*= 0*/) const
    {
        if (IsIdentity(m))
        {
            if (dash > 0)
                DashedFrame(ToWorld(r), color, dash, gap, width);
            else
                UiWsDraw::Frame(ToWorld(r), 0.0f, color, width);
            return;
        }

        auto cs = WorldCorners(r, m);
        for (int i = 0; i < 4; i++)
        {
            Vec2F a = cs[i], b = cs[(i + 1)%4];
            if (dash > 0)
            {
                DashedLine(a, b, color, dash, gap, width);
                continue;
            }
            // Each side reaches half the width past its ends, so the corners close
            Vec2F d = b - a;
            float len = d.Length();
            Vec2F ext = len > 0.0f ? d*(width*0.5f/len) : Vec2F();
            Line(a - ext, b + ext, color, width);
        }
    }

    void UiWsStage::TurnedFill(const UiRect& r, const UiMat& m, const Color4& color) const
    {
        if (IsIdentity(m))
        {
            Fill(ToWorld(r), 0.0f, color);
            return;
        }

        auto cs = WorldCorners(r, m);
        Vector<Vec2F> points = { cs[0], cs[1], cs[2], cs[3] };
        float area = 0.0f;
        for (int i = 0; i < 4; i++)
            area += points[i].x*points[(i + 1)%4].y - points[(i + 1)%4].x*points[i].y;
        if (area < 0.0f)
            points.Reverse();
        Polygon(points, color);
    }

    void UiWsStage::DrawKnob(bool hot) const
    {
        Vec2F foot, knob;
        if (!KnobPoints(foot, knob))
            return;

        auto& t = T();
        Line(foot, knob, t.accent, 1.2f);
        Circle(knob + Vec2F(0, -1), knobRadius + 1.0f, Color4(0, 0, 0, 80));
        Circle(knob, knobRadius + 0.8f, Color4::White());
        Circle(knob, knobRadius - 0.8f, hot ? Color4::White() : t.accent);
        if (hot)
            Circle(knob, knobRadius - 2.6f, t.accent);
    }

    void UiWsStage::RotateTo(const Vec2F& world, Live& live, UiDoc::UiOp& op, bool& hasOp) const
    {
        const auto& doc = ctx->Doc();
        const UiDoc::UiNode* n = UiDoc::FindNode(doc, mDragId);
        if (!n)
            return;

        // The block's centre on the screen; the pointer's direction from it is the block's "up"
        UiPoint mid = UiDoc::Apply(mSelfM, UiDoc::Centre(mR0));
        Vec2F q = WorldToDesign(world);
        double deg = (std::atan2(q.y - mid.y, q.x - mid.x)*180.0)/kPi + 90 - mParentTurn;
        if (o2Input.IsKeyDown(VK_SHIFT))
            deg = UiDoc::JsRound(deg/15)*15;
        else
        {
            double right = UiDoc::JsRound(deg/90)*90;
            if (std::abs(deg - right) < 4)
                deg = right;
        }

        auto turn = UiDoc::CleanTurn(deg);
        if (turn)
        {
            UiDoc::Json set = UiDoc::Json::Object();
            set["rotation"] = *turn;
            op = UiDoc::MakeUpdateOp(mDragId, set);
        }
        else
            op = UiDoc::MakeUpdateOp(mDragId, UiDoc::Json::Object(), { "rotation" });
        hasOp = UiDoc::TurnOf(*n) != (turn ? *turn : 0.0);
        live.label = mDragId + ": rotation";
        live.badge = UiDoc::JsNum(turn ? *turn : 0.0) + "\xC2\xB0";
    }
}
