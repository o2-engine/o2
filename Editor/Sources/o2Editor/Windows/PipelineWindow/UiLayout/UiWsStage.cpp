#include "o2Editor/stdafx.h"
#include "UiWsStage.h"

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
        const float islandHeight = 36.0f;

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

        // A convex polygon in the order PipelineDraw::Polygon wants (counter-clockwise, y up)
        void Convex(Vector<Vec2F> points, const Color4& color)
        {
            float area = 0.0f;
            for (int i = 0; i < points.Count(); i++)
            {
                const Vec2F& a = points[i];
                const Vec2F& b = points[(i + 1) % points.Count()];
                area += a.x*b.y - b.x*a.y;
            }
            if (area < 0.0f)
                points.Reverse();
            Polygon(points, color);
        }

        // The board's dots: one tile with a dot in its corner, repeated (.ulw-board's radial-gradient dots every 20 px)
        void DrawDots(const RectF& area, const Vec2F& origin, float step, const Color4& color)
        {
            static TextureRef tile;
            static int tileTheme = -1;
            static const int tileSize = 40;
            if (!tile || tileTheme != PipelineTheme::GetVersion())
            {
                Bitmap bitmap(PixelFormat::R8G8B8A8, Vec2I(tileSize, tileSize));
                UInt8* data = bitmap.GetData();
                for (int y = 0; y < tileSize; y++)
                {
                    for (int x = 0; x < tileSize; x++)
                    {
                        // A dot of radius 1 px (2 texels) at (1, 1) px: covered amount by distance
                        float dx = x + 0.5f - 2.0f, dy = y + 0.5f - 2.0f;
                        float d = std::sqrt(dx*dx + dy*dy);
                        float a = Math::Clamp01(2.6f - d);
                        UInt8* px = data + (y*tileSize + x)*4;
                        px[0] = color.r; px[1] = color.g; px[2] = color.b;
                        px[3] = (UInt8)Math::Clamp(Math::RoundToInt(a*color.a), 0, 255);
                    }
                }
                tile = TextureRef(bitmap);
                tile->SetFilter(Texture::Filter::Linear);
                tile->SetWrap(Texture::Wrap::Repeat);
                tileTheme = PipelineTheme::GetVersion();
            }

            ULong white = Color4::White().ABGR();
            auto uv = [&](float x, float y) { return Vec2F((x - origin.x)/step, (origin.y - y)/step); };
            Vec2F a = uv(area.left, area.top), b = uv(area.right, area.top), c = uv(area.right, area.bottom), d = uv(area.left, area.bottom);
            Vertex vertices[4] = {
                Vertex(Vec2F(area.left, area.top), white, a.x, a.y),
                Vertex(Vec2F(area.right, area.top), white, b.x, b.y),
                Vertex(Vec2F(area.right, area.bottom), white, c.x, c.y),
                Vertex(Vec2F(area.left, area.bottom), white, d.x, d.y)
            };
            VertexIndex indices[6] = { 0, 1, 2, 0, 2, 3 };
            o2Render.DrawBuffer(PrimitiveType::Polygon, vertices, 4, indices, 2, o2Render.GetDefaultMaterial(), tile);
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
        RectF s = ctx && !ctx->selected.IsEmpty() ? BlockRect(ctx->selected) : RectF();
        float x = handle.Contains('w') ? s.left : handle.Contains('e') ? s.right : (s.left + s.right)*0.5f;
        float y = handle.Contains('n') ? s.top : handle.Contains('s') ? s.bottom : (s.top + s.bottom)*0.5f;
        return Vec2F(x, y);
    }

    Vec2F UiWsStage::AnchorGrip() const
    {
        const auto& doc = Shown();
        const UiDoc::UiNode* n = ctx->selected.IsEmpty() ? nullptr : UiDoc::FindNode(doc, ctx->selected);
        const UiDoc::UiNode* parent = n ? UiDoc::FindParent(doc, n->id) : nullptr;
        const UiRect* p = parent ? ShownRects().Find(parent->id) : nullptr;
        if (!n || !p)
            return Vec2F();

        auto ab = UiDoc::AnchorBox(*n, *p);
        Vec2F corner = DesignToWorld(ab.x0, ab.y0);
        return corner + Vec2F(-5.0f, 5.0f);
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

        // The whole edge pulls
        float top = s.top, bottom = s.bottom;
        if (point.x >= s.left - 5 && point.x <= s.right + 5)
        {
            if (Math::Abs(point.y - top) <= 5.0f) return "n";
            if (Math::Abs(point.y - bottom) <= 5.0f) return "s";
        }
        if (point.y <= top + 5 && point.y >= bottom - 5)
        {
            if (Math::Abs(point.x - s.left) <= 5.0f) return "w";
            if (Math::Abs(point.x - s.right) <= 5.0f) return "e";
        }

        // The anchor's triangles
        const UiDoc::UiNode* parent = UiDoc::FindParent(doc, n->id);
        const UiRect* p = parent ? ShownRects().Find(parent->id) : nullptr;
        if (parent && !parent->layout && p && n->IsShown())
        {
            auto ab = UiDoc::AnchorBox(*n, *p);
            Vec2F b = ToBoard(point);
            auto tri = [&](double x, double y, float sx, float sy)
            {
                Vec2F a(mCam.x + (float)x*mCam.k, mCam.y + (float)y*mCam.k);
                return InTriangle(b, a, a + Vec2F(sx*4.5f, sy*14.0f), a + Vec2F(sx*14.0f, sy*4.5f));
            };
            if (tri(ab.x0, ab.y0, -1, -1) || tri(ab.x1, ab.y0, 1, -1) || tri(ab.x0, ab.y1, -1, 1) || tri(ab.x1, ab.y1, 1, 1))
                return "anchor";
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
        if (!mPressedHot.IsEmpty())
            return;

        if (mSpace)
        {
            BeginPan(cursor);
            return;
        }

        mPressWorld = cursor.position;
        mMoved = false;
        mBaseRects = ctx->rects;
        mLive.reset();
        bool editable = ctx->Editable() && !ctx->Busy();

        String grip = GripAt(cursor.position);
        if (!grip.IsEmpty() && BeginContext(ctx->selected))
        {
            mDragId = ctx->selected;
            mDrag = grip == "anchor" ? DragKind::Anchor : DragKind::Resize;
            mHandle = grip;
            return;
        }

        Vec2F pt = WorldToDesign(cursor.position);
        Vec2F frame = FrameSize();
        UiRect page(0, 0, frame.x, frame.y);
        String hit = InRect(page, pt) ? UiDoc::HitTest(ctx->Doc(), mBaseRects, pt.x, pt.y) : String();
        const UiRect* selRect = ctx->selected.IsEmpty() ? nullptr : mBaseRects.Find(ctx->selected);
        String grab = selRect && InRect(*selRect, pt) ? ctx->selected : hit;
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
                UiDoc::UiSnap s;
                if (noSnap)
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
            if (!(noSnap || shift || alt))
                s = UiDoc::SnapResize(s.rect, edges, mTargets, snapPixels/k);
            auto patch = parent->layout ? UiDoc::SizeInLayout(*n, *parent, mR0, s.rect) : UiDoc::PlaceAt(*n, mP, s.rect, nullptr, true);
            hasOp = UiDoc::PatchOp(mDragId, patch, op);
            live.label = mDragId + ": size";
            live.lines = s.lines;
            live.badge = UiDoc::JsNum(UiDoc::JsRound(s.rect.w)) + " \xC3\x97 " + UiDoc::JsNum(UiDoc::JsRound(s.rect.h));
        }
        else if (mDrag == DragKind::Anchor)
        {
            Vec2F pt = WorldToDesign(cursor.position);
            UiDoc::Anchor a = UiDoc::AnchorAt(mP, pt.x, pt.y);
            hasOp = UiDoc::PatchOp(mDragId, UiDoc::PlaceAt(*n, mP, mR0, &a), op);
            live.label = mDragId + ": anchor";
            live.anchorDots = mP;
            o2Application.SetCursor(CursorType::SizeAll);
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
        if (!mPressedHot.IsEmpty())
        {
            UiWsPanel::OnCursorReleased(cursor);
            return;
        }

        EndDrag(true);
        UpdateCursor(cursor.position);
    }

    void UiWsStage::OnCursorPressBreak(const Input::Cursor& cursor)
    {
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
        const UiRect* sel = ctx->selected.IsEmpty() ? nullptr : ctx->rects.Find(ctx->selected);
        mOverSelection = sel && InRect(*sel, pt);
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
        if (grip == "n" || grip == "s") type = CursorType::SizeNS;
        else if (grip == "e" || grip == "w") type = CursorType::SizeWE;
        else if (grip == "ne" || grip == "sw") type = CursorType::SizeNeSw;
        else if (grip == "nw" || grip == "se") type = CursorType::SizeNwSe;
        else if (grip == "anchor") type = CursorType::SizeAll;
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
        float full = Math::Min(mCam.k*dpr, 4096.0f/longest);
        bool interacting = mDrag != DragKind::None || mIdle < 0.15f;
        float want = interacting && !mForceRender ? Math::Min(full, 1600.0f/longest) : full;
        bool stale = key != mRenderedKey;
        bool sharper = !interacting && Math::Abs(want - mRenderedScale) > want*0.01f;
        if (!stale && !sharper && !mForceRender)
            return;

        mForceRender = false;
        UiDoc::UiRenderOptions options;
        options.mode = ctx->view;
        options.kit = ctx->hasKit ? &ctx->kit : nullptr;
        options.width = frame.x;
        options.height = frame.y;
        options.rects = &ShownRects();
        options.showIds = ctx->showIds;
        options.scale = Math::Max(0.01f, want);
        auto bitmap = UiDoc::RenderLayoutBitmap(doc, options);
        if (!bitmap)
            return;

        mTexture = TextureRef(*bitmap);
        mTexture->SetFilter(Texture::Filter::Linear);
        mTextureSize = bitmap->GetSize();
        mSprite->SetTexture(mTexture);
        mSprite->SetTextureSrcRect(RectI(Vec2I(), mTextureSize));
        mRenderedKey = key;
        mRenderedScale = want;
        mRenderCount++;
    }

    void UiWsStage::DrawPanel(const Box& box)
    {
        if (!ctx)
            return;

        auto& t = T();
        RectF area = box.world;
        o2Render.EnableScissorTest((RectI)area);

        // The board: the canvas gradient and its dots every 20 px, moving with the camera
        PipelineDraw::Fill(area, PipelineDraw::Corners(0.0f), PipelineDraw::Gradient(t.canvasInner, 0.42f, t.canvasMid, t.canvasOuter));
        Color4 dot = t.dark ? Color4(255, 255, 255, 18) : A(t.fg, 0.12f);
        DrawDots(area, box.P(std::fmod(mCam.x, 20.0f) - 20.0f, std::fmod(mCam.y, 20.0f) - 20.0f), 20.0f, dot);

        if (mCam.k > 0.0f)
        {
            Vec2F frame = FrameSize();
            RectF page = ToWorld(UiRect(0, 0, frame.x, frame.y));
            Shadow(RectF(page.left, page.top - (t.dark ? 18.0f : 12.0f), page.right, page.bottom - (t.dark ? 18.0f : 12.0f)), 0.0f,
                   t.dark ? Color4(0, 0, 0, 115) : Color4(38, 50, 56, 41), t.dark ? 48.0f : 32.0f);
            Fill(page, 0.0f, t.bg2);
            EnsureRender(box);
            if (mTexture)
            {
                mSprite->SetRect(page);
                mSprite->Draw();
            }

            // Over the screen: its edge, the selection's parent, the hovered block, the gesture's ghost, guides and anchor dots
            const auto& doc = Shown();
            const auto& rects = ShownRects();
            Border(page, 0.0f, t.line);
            const UiDoc::UiNode* selNode = ctx->selected.IsEmpty() ? nullptr : UiDoc::FindNode(doc, ctx->selected);
            const UiDoc::UiNode* selParent = selNode ? UiDoc::FindParent(doc, selNode->id) : nullptr;
            const UiRect* sel = selNode ? rects.Find(selNode->id) : nullptr;
            const UiRect* parentRect = selParent ? rects.Find(selParent->id) : nullptr;
            if (selParent && parentRect && selParent != &doc.root)
                DashedFrame(ToWorld(*parentRect), A(t.accent, 0.6f), 5.0f, 4.0f);
            if (!mLive && !mHoverBlock.IsEmpty() && mHoverBlock != ctx->selected)
            {
                if (const UiRect* hov = rects.Find(mHoverBlock))
                    UiWsDraw::Frame(ToWorld(*hov), 0.0f, A(t.accent, 0.55f), 1.0f);
            }
            if (mLive && mLive->ghost)
            {
                RectF g = ToWorld(*mLive->ghost);
                Fill(g, 0.0f, A(t.accent, 0.08f));
                DashedFrame(g, t.accent, 4.0f, 3.0f);
            }
            if (mLive)
            {
                for (auto& l : mLive->lines)
                {
                    Vec2F a = l.axis == 'x' ? DesignToWorld(l.at, l.from) : DesignToWorld(l.from, l.at);
                    Vec2F b = l.axis == 'x' ? DesignToWorld(l.at, l.to) : DesignToWorld(l.to, l.at);
                    Line(a, b, SnapColor(), 1.0f);
                }
                if (mLive->anchorDots)
                {
                    auto& p = *mLive->anchorDots;
                    for (double fx : { 0.0, 0.5, 1.0 })
                    {
                        for (double fy : { 0.0, 0.5, 1.0 })
                        {
                            Vec2F c = DesignToWorld(p.x + p.w*fx, p.y + p.h*fy);
                            Circle(c, 4.1f, t.accent);
                            Circle(c, 2.9f, Color4::White());
                        }
                    }
                }
            }

            if (sel)
            {
                RectF s = ToWorld(*sel);
                UiWsDraw::Frame(s, 0.0f, Color4(10, 8, 20, 140), 4.5f);
                UiWsDraw::Frame(s, 0.0f, t.accent, 2.0f);
            }

            // The anchor: Unity's four triangles, split along a stretched axis
            bool editable = ctx->Editable() && !ctx->Busy();
            if (selNode && selParent && !selParent->layout && parentRect && selNode->IsShown())
            {
                auto ab = UiDoc::AnchorBox(*selNode, *parentRect);
                bool hot = editable && !mLive && GripAt(o2Input.GetCursorPos()) == "anchor";
                auto tri = [&](double x, double y, float sx, float sy)
                {
                    Vec2F a = DesignToWorld(x, y);
                    Vec2F b = a + Vec2F(sx*4.5f, -sy*14.0f), c = a + Vec2F(sx*14.0f, -sy*4.5f);
                    Convex({ a + Vec2F(0, -1.5f), b + Vec2F(0, -1.5f), c + Vec2F(0, -1.5f) }, Color4(0, 0, 0, 70));
                    Convex({ a, b, c }, hot ? t.portImage : Color4::White());
                    Color4 ink(26, 16, 48, 255);
                    Line(a, b, ink, 1.3f);
                    Line(b, c, ink, 1.3f);
                    Line(c, a, ink, 1.3f);
                };
                tri(ab.x0, ab.y0, -1, -1);
                tri(ab.x1, ab.y0, 1, -1);
                tri(ab.x0, ab.y1, -1, 1);
                tri(ab.x1, ab.y1, 1, 1);
            }

            // The handles: the corners always, the edge middles while the block is not small
            if (sel && editable && selNode)
            {
                RectF s = ToWorld(*sel);
                bool small = s.Width() < 28.0f || Math::Abs(s.Height()) < 28.0f;
                String grip = mLive ? String() : GripAt(o2Input.GetCursorPos());
                for (auto h : { "nw", "n", "ne", "e", "se", "s", "sw", "w" })
                {
                    if (small && String(h).Length() == 1)
                        continue;
                    Vec2F c = HandlePoint(h);
                    bool hot = grip == h;
                    Circle(c + Vec2F(0, -1), 6.5f, Color4(0, 0, 0, 80));
                    Circle(c, 6.0f, hot ? Color4::White() : t.accent);
                    Circle(c, 4.0f, hot ? t.accent : Color4::White());
                }
            }

            if (mLive && !mLive->badge.IsEmpty() && sel)
            {
                RectF s = ToWorld(*sel);
                float w = TextWidth(mLive->badge, 10.5f, 400, true) + 14.0f;
                float cx = (s.left + s.right)*0.5f;
                RectF b(cx - w*0.5f, s.bottom - 10.0f, cx + w*0.5f, s.bottom - 10.0f - 18.0f);
                Fill(b, 5.0f, t.accent);
                Text(mLive->badge, b, TextStyle(10.5f, 400, Color4::White(), HorAlign::Middle, true));
            }
        }

        DrawBanners(box);
        DrawIsland(box);
        o2Render.DisableScissorTest();
    }

    void UiWsStage::DrawOverlay(const Box& box)
    {}

    void UiWsStage::DrawBanners(const Box& box)
    {
        auto& t = T();
        float W = box.W();
        if (ctx->peek >= 0)
        {
            String text = "Showing the screen after step " + (String)ctx->peek + " of " + (String)(ctx->timeline.Count() - 1);
            float textW = TextWidth(text, 12.0f);
            float backW = TextWidth("Back to now", 12.0f) + 20.0f;
            float revertW = TextWidth("Revert to here", 12.0f) + 20.0f + 19.0f;
            float w = 12 + 14 + 8 + textW + 8 + backW + 8 + revertW + 6;
            RectF r = box.R((W - w)*0.5f, 14, w, 36);
            Island(r, 12.0f, true);
            Border(r, 12.0f, t.accent);
            float x = r.left + 12;
            UiWsDraw::Icon("uiws_peek", Vec2F(x + 7, r.Center().y), 14.0f, t.accent);
            x += 14 + 8;
            Text(text, RectF(x, r.top, x + textW + 2, r.bottom), TextStyle(12.0f, 400, t.fg));
            x += textW + 8;
            Button(RectF(x, r.top - 4, x + backW, r.bottom + 4), "Back to now", "peek:back");
            x += backW + 8;
            Button(RectF(x, r.top - 4, x + revertW, r.bottom + 4), "Revert to here", "peek:revert", false, true, "uiws_revert");
        }

        if (ctx->Busy())
        {
            String text = "The agent is working\xE2\x80\xA6";
            float w = 12 + 11 + 8 + TextWidth(text, 11.5f) + 12;
            RectF r = box.R((W - w)*0.5f, ctx->peek >= 0 ? 58.0f : 14.0f, w, 26);
            Island(r, 13.0f, true);
            float angle = o2Time.GetApplicationTime()*7.85f;
            Vec2F c(r.left + 12 + 5.5f, r.Center().y);
            PipelineDraw::Ring(c, 4.5f, 2.0f, t.accentSoft);
            for (int i = 0; i < 6; i++)
            {
                float a0 = angle + i*0.26f, a1 = angle + (i + 1)*0.26f;
                Line(c + Vec2F(std::cos(a0), std::sin(a0))*4.5f, c + Vec2F(std::cos(a1), std::sin(a1))*4.5f, t.accent, 2.0f);
            }
            Text(text, RectF(r.left + 31, r.top, r.right - 8, r.bottom), TextStyle(11.5f, 400, t.fg));
        }
    }

    void UiWsStage::DrawIsland(const Box& box)
    {
        auto& t = T();
        float W = box.W(), H = box.H();
        float chipW = mDevice->GetPreferredWidth();
        // lines, kit | chip | ids | - % + fit
        float w = 6 + 28 + 2 + 28 + 2 + 9 + 2 + chipW + 2 + 9 + 2 + 28 + 2 + 9 + 2 + 28 + 2 + 48 + 2 + 28 + 2 + 28 + 6;
        RectF r = box.R((W - w)*0.5f, H - 16 - islandHeight, w, islandHeight);
        Island(r, 12.0f, true);
        AddHot(r, "island");

        float x = r.left + 6, top = r.top - 4;
        auto tool = [&](const String& icon, const String& id, bool active, float size = 16.0f)
        {
            Tool(RectF(x, top, x + 28, top - 28), icon, id, active, true, size);
            x += 28 + 2;
        };
        auto sep = [&]()
        {
            Separator(Vec2F(x + 4.5f, r.Center().y));
            x += 9 + 2;
        };

        tool("uiws_wire", "view:lines", ctx->view == "lines");
        tool("uiws_skin", "view:kit", ctx->view == "kit");
        sep();
        Vec2F origin(box.world.left, box.world.top);
        *mDevice->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(x - origin.x, top - 28 - origin.y), Vec2F(x + chipW - origin.x, top - origin.y));
        x += chipW + 2;
        sep();
        tool("uiws_tag", "ids", ctx->showIds, 15.0f);
        sep();
        tool("uiws_minus", "zoom:out", false, 15.0f);
        RectF pct(x, top, x + 48, top - 28);
        if (IsHover("zoom:100"))
            Fill(pct, 7.0f, t.active);
        Text((String)Math::RoundToInt(mCam.k*100.0f) + "%", pct, TextStyle(11.0f, 400, IsHover("zoom:100") ? t.fg : t.fgDim, HorAlign::Middle, true));
        AddHot(pct, "zoom:100");
        x += 48 + 2;
        tool("uiws_plus", "zoom:in", false, 15.0f);
        tool("uiws_fit", "zoom:fit", mAuto, 15.0f);
    }

    void UiWsStage::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (!ctx)
            return;

        if (id == "view:lines" || id == "view:kit")
            ctx->SetUi("uiView", UiDoc::Json(id.SubStr(5)));
        else if (id == "ids")
            ctx->SetUi("uiShowIds", UiDoc::Json(!ctx->showIds));
        else if (id == "zoom:out")
            ZoomBy(1.0f/1.25f);
        else if (id == "zoom:in")
            ZoomBy(1.25f);
        else if (id == "zoom:100")
            ZoomBy(1.0f/Math::Max(0.0001f, mCam.k));
        else if (id == "zoom:fit")
            Fit();
        else if (id == "peek:back")
            ctx->SetPeek(-1);
        else if (id == "peek:revert")
            ctx->Revert(ctx->peek);
    }
}
