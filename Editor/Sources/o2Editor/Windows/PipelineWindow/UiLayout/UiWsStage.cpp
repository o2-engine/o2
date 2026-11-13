#include "o2Editor/stdafx.h"
#include "UiWsStage.h"

#include <chrono>
#include <cmath>

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/System/Time/Time.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsPins.h"

namespace Editor
{
    using namespace UiWsDraw;
    using UiDoc::UiOp;
    using UiDoc::UiRect;
    using UiDoc::UiRects;

    namespace
    {
        // Room around the fitted screen: the peek banner above, the island below
        const float padSide = 40.0f, padTop = 56.0f, padBottom = 84.0f;
        const float kMin = 0.05f, kMax = 6.0f;
        const float dragPixels = 3.0f;  // Pointer travel before a press becomes a drag
        const float snapPixels = 6.0f;

        float ClampK(float k) { return Math::Clamp(k, kMin, kMax); }

        bool InRect(const UiRect& r, const Vec2F& p) { return p.x >= r.x && p.y >= r.y && p.x <= r.x + r.w && p.y <= r.y + r.h; }

        // A point inside a triangle (any winding)
        bool InTriangle(const Vec2F& p, const Vec2F& a, const Vec2F& b, const Vec2F& c)
        {
            auto side = [](const Vec2F& p1, const Vec2F& p2, const Vec2F& p3) { return (p1.x - p3.x)*(p2.y - p3.y) - (p2.x - p3.x)*(p1.y - p3.y); };
            float d1 = side(p, a, b), d2 = side(p, b, c), d3 = side(p, c, a);
            bool neg = d1 < 0 || d2 < 0 || d3 < 0, pos = d1 > 0 || d2 > 0 || d3 > 0;
            return !(neg && pos);
        }

        // How far a point is from a segment
        float SegmentDistance(const Vec2F& p, const Vec2F& a, const Vec2F& b)
        {
            Vec2F d = b - a;
            float len2 = d.x*d.x + d.y*d.y;
            float f = len2 > 0.0f ? Math::Clamp01(((p.x - a.x)*d.x + (p.y - a.y)*d.y)/len2) : 0.0f;
            return (p - (a + d*f)).Length();
        }
    }

    UiWsStage::UiWsStage(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {
        PushEditorScopeOnStack scope;
        mSprite = mmake<Sprite>();
        mDevice = mmake<UiDevicePicker>();
        mDevice->name = "device chip";
        mDevice->chip = true;
        AddChild(mDevice);
    }

    // --- the camera ----------------------------------------------------------------------------

    Vec2F UiWsStage::FrameSize() const
    {
        if (!ctx)
            return Vec2F(1080, 1920);
        return Vec2F((float)ctx->preview.frame.w, (float)ctx->preview.frame.h);
    }

    UiWsStage::Cam UiWsStage::FitCam(float x, float y, float w, float h) const
    {
        Vec2F board = Vec2F(layout->GetWidth(), layout->GetHeight());
        float aw = Math::Max(40.0f, board.x - padSide*2.0f), ah = Math::Max(40.0f, board.y - padTop - padBottom);
        Cam c;
        c.k = ClampK(Math::Min(aw/Math::Max(1.0f, w), ah/Math::Max(1.0f, h)));
        c.x = (board.x - w*c.k)*0.5f - x*c.k;
        c.y = padTop + (ah - h*c.k)*0.5f - y*c.k;
        return c;
    }

    void UiWsStage::Fit()
    {
        mAuto = true;
        Vec2F frame = FrameSize();
        mCam = FitCam(0, 0, frame.x, frame.y);
        mBoardSize = Vec2F(layout->GetWidth(), layout->GetHeight());
        mFittedFrame = frame;
        mIdle = 0.0f;
    }

    void UiWsStage::Frame(const String& id)
    {
        const UiRect* r = id.IsEmpty() ? nullptr : ShownRects().Find(id);
        if (!r || r->w < 1 || r->h < 1)
        {
            Fit();
            return;
        }

        mAuto = false;
        float m = (float)Math::Max(r->w, r->h)*0.15f;
        Cam c = FitCam((float)r->x - m, (float)r->y - m, (float)r->w + m*2.0f, (float)r->h + m*2.0f);
        Vec2F board = Vec2F(layout->GetWidth(), layout->GetHeight());
        if (c.k > 4.0f)
            c = FitCam((float)(r->x + r->w*0.5) - board.x/8.0f, (float)(r->y + r->h*0.5) - board.y/8.0f, board.x/4.0f, board.y/4.0f);
        mCam = c;
        mIdle = 0.0f;
    }

    void UiWsStage::ZoomAt(const Vec2F& p, float factor)
    {
        mAuto = false;
        float k = ClampK(mCam.k*factor);
        mCam.x = p.x - (p.x - mCam.x)*k/mCam.k;
        mCam.y = p.y - (p.y - mCam.y)*k/mCam.k;
        mCam.k = k;
        mIdle = 0.0f;
    }

    void UiWsStage::ZoomBy(float factor)
    {
        Vec2F board = Vec2F(layout->GetWidth(), layout->GetHeight());
        ZoomAt(board*0.5f, factor);
    }

    Vec2F UiWsStage::ToBoard(const Vec2F& world) const
    {
        RectF r = layout->GetWorldRect();
        return Vec2F(world.x - r.left, r.top - world.y);
    }

    Vec2F UiWsStage::DesignToWorld(double x, double y) const
    {
        RectF r = layout->GetWorldRect();
        return Vec2F(r.left + mCam.x + (float)x*mCam.k, r.top - (mCam.y + (float)y*mCam.k));
    }

    Vec2F UiWsStage::WorldToDesign(const Vec2F& point) const
    {
        Vec2F b = ToBoard(point);
        float k = Math::Max(0.0001f, mCam.k);
        return Vec2F((b.x - mCam.x)/k, (b.y - mCam.y)/k);
    }

    RectF UiWsStage::ToWorld(const UiRect& r) const
    {
        Vec2F a = DesignToWorld(r.x, r.y), b = DesignToWorld(r.x + r.w, r.y + r.h);
        return RectF(a.x, a.y, b.x, b.y);
    }

    RectF UiWsStage::BlockRect(const String& id) const
    {
        const UiRect* r = ShownRects().Find(id);
        return r ? ToWorld(*r) : RectF();
    }

    const UiDoc::UiDoc& UiWsStage::Shown() const
    {
        return mLive ? mLive->doc : ctx->Doc();
    }

    const UiRects& UiWsStage::ShownRects() const
    {
        return mLive ? mLiveRects : ctx->rects;
    }

    void UiWsStage::OnScrolled(float scroll)
    {
        if (!ctx)
            return;

        float delta = Math::Clamp(scroll, -120.0f, 120.0f);
        bool fine = o2Input.IsKeyDown(VK_CONTROL);
        ZoomAt(ToBoard(o2Input.GetCursorPos()), std::exp(delta*(fine ? 0.01f : 0.002f)));
    }

    // --- handles -------------------------------------------------------------------------------

    Vec2F UiWsStage::HandlePoint(const String& handle) const
    {
        // On the block's own box, turned with it
        double fx = handle.Contains('w') ? 0.0 : handle.Contains('e') ? 1.0 : 0.5;
        double fy = handle.Contains('n') ? 0.0 : handle.Contains('s') ? 1.0 : 0.5;
        return SelectionAt(fx, fy);
    }

    Vec2F UiWsStage::AnchorGrip() const
    {
        // 5 px into the first triangle from its corner, along both of its sides (turned with the parent)
        std::array<std::array<Vec2F, 3>, 4> tris;
        if (!AnchorTriangles(tris))
            return Vec2F();

        auto& t = tris[0];
        return t[0] + (t[1] + t[2] - t[0]*2.0f)*(5.0f/18.5f);
    }

    String UiWsStage::GripAt(const Vec2F& point) const
    {
        if (!ctx || ctx->selected.IsEmpty() || !ctx->Editable() || ctx->Busy())
            return "";

        const auto& doc = Shown();
        const UiRect* sel = ShownRects().Find(ctx->selected);
        const UiDoc::UiNode* n = UiDoc::FindNode(doc, ctx->selected);
        if (!sel || !n)
            return "";

        RectF s = ToWorld(*sel);
        bool small = s.Width() < 28.0f || Math::Abs(s.Height()) < 28.0f;
        const char* handles[] = { "nw", "ne", "se", "sw", "n", "e", "s", "w" };
        for (auto h : handles)
        {
            if (small && String(h).Length() == 1)
                continue;
            if ((HandlePoint(h) - point).Length() <= 9.0f)
                return h;
        }

        // The whole edge pulls (the block's own edges, turned with it)
        static const struct { const char* h; double ax, ay, bx, by; } edges[] = {
            { "n", 0, 0, 1, 0 }, { "s", 0, 1, 1, 1 }, { "w", 0, 0, 0, 1 }, { "e", 1, 0, 1, 1 } };
        for (auto& e : edges)
        {
            if (SegmentDistance(point, SelectionAt(e.ax, e.ay), SelectionAt(e.bx, e.by)) <= 5.0f)
                return e.h;
        }

        // The turning knob above the top edge
        Vec2F knob;
        if (KnobPoint(knob) && (knob - point).Length() <= 10.0f)
            return "turn";

        // The anchor's triangles
        std::array<std::array<Vec2F, 3>, 4> tris;
        if (AnchorTriangles(tris))
        {
            for (auto& t : tris)
            {
                if (InTriangle(point, t[0], t[1], t[2]))
                    return "anchor";
            }
        }
        return "";
    }

    // --- gestures -------------------------------------------------------------------------------

    bool UiWsStage::BeginContext(const String& id)
    {
        mHasContext = false;
        const auto& doc = ctx->Doc();
        const UiDoc::UiNode* n = UiDoc::FindNode(doc, id);
        const UiDoc::UiNode* parent = UiDoc::FindParent(doc, id);
        const UiRect* r0 = mBaseRects.Find(id);
        const UiRect* p = parent ? mBaseRects.Find(parent->id) : nullptr;
        if (!n || !parent || !r0 || !p)
            return false;

        mR0 = *r0;
        mP = *p;
        mTargets = UiDoc::SnapTargets(doc, mBaseRects, id);
        mParentTurn = UiDoc::WorldTurn(doc, id, false);
        mParentM = UiDoc::WorldMatrix(doc, mBaseRects, id, false);
        mSelfM = UiDoc::WorldMatrix(doc, mBaseRects, id, true);
        mTurned = mParentTurn != 0 || UiDoc::TurnOf(*n) != 0;
        mHasContext = true;
        return true;
    }

    void UiWsStage::BeginPan(const Input::Cursor& cursor)
    {
        mDrag = DragKind::Pan;
        mPressWorld = cursor.position;
        mCam0 = mCam;
        mAuto = false;
        mMoved = false;
    }

    void UiWsStage::OnCursorPressed(const Input::Cursor& cursor)
    {
        if (!ctx)
            return;

        UiWsPanel::OnCursorPressed(cursor);
        if (OnRefPress(cursor) || !mPressedHot.IsEmpty())
            return;

        if (mSpace)
        {
            BeginPan(cursor);
            return;
        }

        // The request's points: a pin grabbed, or one placed with the point tool
        if (UiWsPins::Press(*this, *ctx, cursor.position))
            return;

        mPressWorld = cursor.position;
        mMoved = false;
        mBaseRects = ctx->rects;
        mLive.reset();
        bool editable = ctx->Editable() && !ctx->Busy();

        String grip = GripAt(cursor.position);
        if (!grip.IsEmpty() && BeginContext(ctx->selected))
        {
            mDragId = ctx->selected;
            mDrag = grip == "anchor" ? DragKind::Anchor : grip == "turn" ? DragKind::Rotate : DragKind::Resize;
            mHandle = grip;
            return;
        }

        Vec2F pt = WorldToDesign(cursor.position);
        Vec2F frame = FrameSize();
        UiRect page(0, 0, frame.x, frame.y);
        String hit = InRect(page, pt) ? UiDoc::HitTest(ctx->Doc(), mBaseRects, pt.x, pt.y) : String();
        bool onSelection = !ctx->selected.IsEmpty() && UiDoc::InBlock(ctx->Doc(), mBaseRects, ctx->selected, { pt.x, pt.y });
        String grab = onSelection ? ctx->selected : hit;
        if (grab.IsEmpty())
        {
            ctx->Select("");
            mDrag = DragKind::None;
            return;
        }

        mWasSelected = grab == ctx->selected;
        mHit = hit;
        mDragId = grab;
        if (!mWasSelected)
            ctx->Select(grab);

        mDrag = DragKind::Move;
        if (editable)
            BeginContext(grab);
        else
            mHasContext = false;
    }

    void UiWsStage::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (OnRefDrag(cursor) || (ctx && UiWsPins::Drag(*this, *ctx, cursor.position)))
            return;

        if (mDrag == DragKind::None)
            return;

        if (!mMoved && (cursor.position - mPressWorld).Length() < dragPixels)
            return;

        mMoved = true;
        DragTo(cursor);
    }

    void UiWsStage::DragTo(const Input::Cursor& cursor)
    {
        mIdle = 0.0f;
        Vec2F delta = cursor.position - mPressWorld;
        if (mDrag == DragKind::Pan)
        {
            mCam.x = mCam0.x + delta.x;
            mCam.y = mCam0.y - delta.y;
            o2Application.SetCursor(CursorType::SizeAll);
            return;
        }

        if (!mHasContext || !ctx->Editable() || ctx->Busy())
            return;

        const auto& doc = ctx->Doc();
        const UiDoc::UiNode* n = UiDoc::FindNode(doc, mDragId);
        const UiDoc::UiNode* parent = UiDoc::FindParent(doc, mDragId);
        if (!n || !parent)
            return;

        float k = Math::Max(0.0001f, mCam.k);
        double ddx = delta.x/k, ddy = -delta.y/k;
        bool shift = o2Input.IsKeyDown(VK_SHIFT), alt = o2Input.IsKeyDown(VK_MENU);
        bool noSnap = o2Input.IsKeyDown(VK_CTRL_CMD) || o2Input.IsKeyDown(VK_CONTROL);

        Live live;
        live.doc = doc;
        UiOp op;
        bool hasOp = false;
        if (mDrag == DragKind::Move)
        {
            // In a turned parent the offsets run along its axes
            if (mParentTurn)
            {
                UiDoc::UiPoint along = UiDoc::ApplyLinear(UiDoc::Invert(mParentM), { ddx, ddy });
                ddx = along.x;
                ddy = along.y;
            }
            if (shift)
            {
                if (std::fabs(ddx) > std::fabs(ddy)) ddy = 0;
                else ddx = 0;
            }
            UiRect moved(mR0.x + ddx, mR0.y + ddy, mR0.w, mR0.h);
            if (parent->layout)
            {
                // A row / column / grid places its children: the drag picks the slot
                Vec2F pt = WorldToDesign(cursor.position);
                UiDoc::UiSlot slot;
                if (UiDoc::LayoutSlot(*parent, mBaseRects, mDragId, pt.x, pt.y, slot))
                {
                    op = UiDoc::MakeMoveOp(mDragId, parent->id, std::nullopt, slot.after, slot.before);
                    hasOp = !UiDoc::MoveIsNoop(doc, op);
                }
                live.label = mDragId + ": order";
                live.ghost = moved;
            }
            else
            {
                // Snapping lines are the screen's axes: a turned block (or one in a turned parent) is not snapped
                UiDoc::UiSnap s;
                if (noSnap || mTurned)
                    s.rect = moved;
                else
                    s = UiDoc::SnapMove(moved, mTargets, snapPixels/k);
                hasOp = UiDoc::PatchOp(mDragId, UiDoc::PlaceAt(*n, mP, s.rect), op);
                live.label = mDragId + ": move";
                live.lines = s.lines;
                live.badge = UiDoc::JsNum(UiDoc::JsRound(s.rect.x - mP.x)) + ", " + UiDoc::JsNum(UiDoc::JsRound(s.rect.y - mP.y));
            }
        }
        else if (mDrag == DragKind::Resize)
        {
            UiDoc::UiEdges edges{ mHandle.Contains('n'), mHandle.Contains('s'), mHandle.Contains('e'), mHandle.Contains('w') };
            // Along the block's own axes (its turn and its parents' undone)
            if (mTurned)
            {
                UiDoc::UiPoint along = UiDoc::ApplyLinear(UiDoc::Invert(mSelfM), { ddx, ddy });
                ddx = along.x;
                ddy = along.y;
            }
            double sym = alt ? 2 : 1;
            double x = mR0.x, y = mR0.y, w = mR0.w, h = mR0.h;
            if (edges.e) w = mR0.w + ddx*sym;
            if (edges.w) w = mR0.w - ddx*sym;
            if (edges.s) h = mR0.h + ddy*sym;
            if (edges.n) h = mR0.h - ddy*sym;
            if (shift && mR0.w > 0 && mR0.h > 0)
            {
                double ratio = mR0.w/mR0.h;
                bool corner = (edges.e || edges.w) && (edges.n || edges.s);
                if (corner ? std::fabs(w/mR0.w - 1) >= std::fabs(h/mR0.h - 1) : (edges.e || edges.w))
                    h = w/ratio;
                else
                    w = h*ratio;
            }
            w = std::max(1.0, w);
            h = std::max(1.0, h);
            // The side not pulled stays put (both move with Alt; a proportional edge pull keeps the other axis centred)
            String sideX = alt || (!edges.e && !edges.w) ? "mid" : edges.w ? "right" : "left";
            String sideY = alt || (!edges.n && !edges.s) ? "mid" : edges.n ? "bottom" : "top";
            x = sideX == "left" ? mR0.x : sideX == "right" ? mR0.x + mR0.w - w : mR0.x + (mR0.w - w)/2;
            y = sideY == "top" ? mR0.y : sideY == "bottom" ? mR0.y + mR0.h - h : mR0.y + (mR0.h - h)/2;
            UiDoc::UiSnap s;
            s.rect = UiRect(x, y, w, h);
            if (!(noSnap || shift || alt || mTurned))
                s = UiDoc::SnapResize(s.rect, edges, mTargets, snapPixels/k);
            double own = UiDoc::TurnOf(*n);
            if (own && !parent->layout)
            {
                // The box turns about its centre, and a resize moves the centre: shifted by (R - I)(c' - c) the side not
                // pulled stays where it is on the screen
                UiDoc::UiPoint c0 = UiDoc::Centre(mR0), c1 = UiDoc::Centre(s.rect);
                UiDoc::UiPoint d{ c1.x - c0.x, c1.y - c0.y };
                UiDoc::UiPoint rd = UiDoc::ApplyLinear(UiDoc::RotationAbout(own, 0, 0), d);
                s.rect = UiRect(s.rect.x + rd.x - d.x, s.rect.y + rd.y - d.y, s.rect.w, s.rect.h);
            }
            auto patch = parent->layout ? UiDoc::SizeInLayout(*n, *parent, mR0, s.rect) : UiDoc::PlaceAt(*n, mP, s.rect, nullptr, true);
            hasOp = UiDoc::PatchOp(mDragId, patch, op);
            live.label = mDragId + ": size";
            live.lines = s.lines;
            live.badge = UiDoc::JsNum(UiDoc::JsRound(s.rect.w)) + " \xC3\x97 " + UiDoc::JsNum(UiDoc::JsRound(s.rect.h));
        }
        else if (mDrag == DragKind::Anchor)
        {
            // The pointer in the parent's own (unturned) frame, where its anchor points are
            Vec2F pt = WorldToDesign(cursor.position);
            UiDoc::UiPoint q{ pt.x, pt.y };
            if (mParentTurn)
                q = UiDoc::Apply(UiDoc::Invert(mParentM), q);
            UiDoc::Anchor a = UiDoc::AnchorAt(mP, q.x, q.y);
            hasOp = UiDoc::PatchOp(mDragId, UiDoc::PlaceAt(*n, mP, mR0, &a), op);
            live.label = mDragId + ": anchor";
            live.anchorDots = mP;
            if (mParentTurn)
                live.anchorM = mParentM;
            o2Application.SetCursor(CursorType::SizeAll);
        }
        else if (mDrag == DragKind::Rotate)
        {
            RotateTo(cursor.position, live, op, hasOp);
            o2Application.SetCursor(CursorType::Hand);
        }

        if (hasOp)
        {
            live.ops.Add(op);
            live.doc = UiDoc::ApplyOps(doc, live.ops).doc;
        }

        Vec2F frame = FrameSize();
        mLiveRects = UiDoc::SolveLayout(live.doc, frame.x, frame.y);
        mLive = live;
        mLiveVersion++;
    }

    void UiWsStage::EndDrag(bool commit)
    {
        DragKind drag = mDrag;
        mDrag = DragKind::None;
        if (drag == DragKind::Pan)
            return;

        if (commit && mMoved && mLive && !mLive->ops.IsEmpty() && ctx->Editable() && !ctx->Busy())
        {
            auto live = *mLive;
            mLive.reset();
            mLiveVersion++;
            ctx->Gesture(live.ops, live.label);
            return;
        }

        if (mLive)
        {
            mLive.reset();
            mLiveVersion++;
        }

        // A click inside the selection reaches the block under the pointer
        if (!mMoved && drag == DragKind::Move && mWasSelected && !mHit.IsEmpty() && mHit != mDragId)
            ctx->Select(mHit);
    }

    void UiWsStage::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (OnRefRelease(cursor))
            return;

        if (!mPressedHot.IsEmpty())
        {
            UiWsPanel::OnCursorReleased(cursor);
            return;
        }

        if (ctx && UiWsPins::Release(*this, *ctx, cursor.position))
            return;

        EndDrag(true);
        UpdateCursor(cursor.position);
    }

    void UiWsStage::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        mRefLive = -1.0;
        mRefSlide = "";
        EndDrag(false);
    }

    void UiWsStage::OnCursorMoved(const Input::Cursor& cursor)
    {
        UiWsPanel::OnCursorMoved(cursor);
        if (!ctx || mDrag != DragKind::None)
            return;

        Vec2F pt = WorldToDesign(cursor.position);
        Vec2F frame = FrameSize();
        bool onPage = pt.x >= 0 && pt.y >= 0 && pt.x <= frame.x && pt.y <= frame.y && mHover.IsEmpty();
        mHoverBlock = onPage ? UiDoc::HitTest(ctx->Doc(), ctx->rects, pt.x, pt.y) : String();
        mOverSelection = !ctx->selected.IsEmpty() && UiDoc::InBlock(ctx->Doc(), ctx->rects, ctx->selected, { pt.x, pt.y });
        UpdateCursor(cursor.position);
    }

    void UiWsStage::OnCursorExit(const Input::Cursor& cursor)
    {
        UiWsPanel::OnCursorExit(cursor);
        mHoverBlock = "";
        mOverSelection = false;
        if (mDrag == DragKind::None)
            o2Application.SetCursor(CursorType::Arrow);
    }

    void UiWsStage::UpdateCursor(const Vec2F& point)
    {
        if (!ctx)
            return;

        if (mSpace)
        {
            o2Application.SetCursor(CursorType::Hand);
            return;
        }

        String grip = mHover.IsEmpty() ? GripAt(point) : String();
        CursorType type = CursorType::Arrow;
        if (grip == "anchor") type = CursorType::SizeAll;
        else if (grip == "turn") type = CursorType::Hand;
        else if (!grip.IsEmpty()) type = HandleCursor(grip);
        else if (mOverSelection && mHover.IsEmpty() && ctx->Editable() && !ctx->Busy()) type = CursorType::SizeAll;
        o2Application.SetCursor(type);
    }

    void UiWsStage::OnCursorRightMousePressed(const Input::Cursor& cursor) { BeginPan(cursor); }
    void UiWsStage::OnCursorRightMouseStayDown(const Input::Cursor& cursor) { if (mDrag == DragKind::Pan) DragTo(cursor); }
    void UiWsStage::OnCursorRightMouseReleased(const Input::Cursor& cursor) { if (mDrag == DragKind::Pan) mDrag = DragKind::None; }
    void UiWsStage::OnCursorMiddleMousePressed(const Input::Cursor& cursor) { BeginPan(cursor); }
    void UiWsStage::OnCursorMiddleMouseStayDown(const Input::Cursor& cursor) { if (mDrag == DragKind::Pan) DragTo(cursor); }
    void UiWsStage::OnCursorMiddleMouseReleased(const Input::Cursor& cursor) { if (mDrag == DragKind::Pan) mDrag = DragKind::None; }

    // --- update and drawing -------------------------------------------------------------------------

    void UiWsStage::Update(float dt)
    {
        UiWsPanel::Update(dt);
        mIdle += dt;
        if (!ctx)
            return;

        // Fitted until the user pans or zooms; a new screen size fits again
        Vec2F board = Vec2F(layout->GetWidth(), layout->GetHeight());
        Vec2F frame = FrameSize();
        if (board.x > 1 && board.y > 1)
        {
            if (mCam.k <= 0.0f || frame != mFittedFrame)
                Fit();
            else if (mAuto && board != mBoardSize)
                Fit();
        }
    }

    void UiWsStage::FlushRender()
    {
        mIdle = 10.0f;
        mForceRender = true;
    }

    void UiWsStage::Upload(const UiLayout::UiImage& image, const String& key, float scale, float ms)
    {
        if (image.IsEmpty())
            return;

        auto bitmap = image.ToBitmap();
        mTexture = TextureRef(*bitmap);
        mTexture->SetFilter(Texture::Filter::Linear);
        mTextureSize = bitmap->GetSize();
        mSprite->SetTexture(mTexture);
        mSprite->SetTextureSrcRect(RectI(Vec2I(), mTextureSize));
        mRenderedKey = key;
        mRenderedScale = scale;
        mLastRenderMs = ms;
        mRenderCount++;
    }

    void UiWsStage::EnsureRender(const Box& box)
    {
        const auto& doc = Shown();
        Vec2F frame = FrameSize();
        // What the picture depends on, read again only when the context or the gesture changed
        String versionKey = (String)ctx->version + "|" + (String)mLiveVersion + "|" + (String)ctx->peek;
        if (versionKey != mVersionKey)
        {
            mVersionKey = versionKey;
            mDocKey = UiDoc::NodeToJson(doc.root).Dump() + "|" + doc.name + "|" + UiDoc::JsNum(doc.width) + "x" + UiDoc::JsNum(doc.height) +
                "|" + (doc.dpi ? UiDoc::JsNum(*doc.dpi) : String());
        }
        String key = mDocKey + "|" + ctx->view + "|" + (ctx->showIds ? "ids" : "") + "|" + (String)ctx->kitVersion + "|" +
            (String)frame.x + "x" + (String)frame.y;

        float dpr = 1.0f/Math::Max(0.05f, PipelineDraw::Pixel());
        float longest = Math::Max(1.0f, Math::Max(frame.x, frame.y));
        float full = Math::Max(0.01f, Math::Min(mCam.k*dpr, 4096.0f/longest));
        bool interacting = mDrag != DragKind::None || mIdle < 0.15f;

        // The worker's picture, when it is still the one wanted
        if (mJob && mJob->picture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            auto image = mJob->picture.get();
            if (mJob->key == key)
                Upload(image, mJob->key, mJob->scale, (o2Time.GetApplicationTime() - mJob->started)*1000.0f);
            mJob = nullptr;
        }

        auto options = [&](float scale)
        {
            UiDoc::UiRenderOptions o;
            o.mode = ctx->view;
            o.width = frame.x;
            o.height = frame.y;
            o.showIds = ctx->showIds;
            o.scale = scale;
            return o;
        };

        bool stale = key != mRenderedKey;
        if (mForceRender || !mTexture || (interacting && stale))
        {
            // Right away: the first picture, a test's flush, a gesture's live result (smaller while it lasts)
            float want = mForceRender || !mTexture ? full : Math::Min(full, 720.0f/longest);
            mForceRender = false;
            auto o = options(want);
            o.kit = ctx->hasKit ? &ctx->kit : nullptr;
            o.rects = &ShownRects();
            auto started = std::chrono::steady_clock::now();
            auto image = UiDoc::RenderLayout(doc, o);
            Upload(image, key, want, std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - started).count());
            return;
        }

        if (interacting || mJob)
            return;

        if (stale || Math::Abs(full - mRenderedScale) > full*0.01f)
        {
            // At rest: the full size on a worker, from copies of what it draws
            auto job = std::make_shared<RenderJob>();
            job->key = key;
            job->scale = full;
            job->started = o2Time.GetApplicationTime();
            auto docCopy = std::make_shared<UiDoc::UiDoc>(doc);
            auto kitCopy = ctx->hasKit ? std::make_shared<UiDoc::UiKit>(ctx->kit) : nullptr;
            auto rectsCopy = std::make_shared<UiDoc::UiRects>(ShownRects());
            auto o = options(full);
            job->picture = std::async(std::launch::async, [docCopy, kitCopy, rectsCopy, o]() mutable
            {
                o.kit = kitCopy.get();
                o.rects = rectsCopy.get();
                return UiDoc::RenderLayout(*docCopy, o);
            });
            mJob = job;
        }
    }
}
