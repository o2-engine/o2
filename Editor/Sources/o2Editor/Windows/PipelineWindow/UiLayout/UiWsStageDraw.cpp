#include "o2Editor/stdafx.h"
#include "UiWsStage.h"

#include <cmath>

#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/System/Time/Time.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsPins.h"

// The stage's drawing (UiWsStage.h): the board, the screen, what is drawn over it, the banners and the island
namespace Editor
{
    using namespace UiWsDraw;
    using UiDoc::UiRect;

    namespace
    {
        const float islandHeight = 36.0f;

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
            DrawReference(page);

            // Over the screen: its edge, the selection's parent, the hovered block, the gesture's ghost, guides and anchor dots
            const auto& doc = Shown();
            const auto& rects = ShownRects();
            Border(page, 0.0f, t.line);
            const UiDoc::UiNode* selNode = ctx->selected.IsEmpty() ? nullptr : UiDoc::FindNode(doc, ctx->selected);
            const UiDoc::UiNode* selParent = selNode ? UiDoc::FindParent(doc, selNode->id) : nullptr;
            const UiRect* sel = selNode ? rects.Find(selNode->id) : nullptr;
            const UiRect* parentRect = selParent ? rects.Find(selParent->id) : nullptr;
            // (Boxes are drawn turned with their blocks and parents: UiWsStageTurn.cpp)
            if (selParent && parentRect && selParent != &doc.root)
                TurnedFrame(*parentRect, BlockMatrix(selParent->id), A(t.accent, 0.6f), 1.0f, 5.0f, 4.0f);
            if (!mLive && !mHoverBlock.IsEmpty() && mHoverBlock != ctx->selected)
            {
                if (const UiRect* hov = rects.Find(mHoverBlock))
                    TurnedFrame(*hov, BlockMatrix(mHoverBlock), A(t.accent, 0.55f), 1.0f);
            }
            if (mLive && mLive->ghost)
            {
                UiDoc::UiMat m = BlockMatrix(ctx->selected, false);
                TurnedFill(*mLive->ghost, m, A(t.accent, 0.08f));
                TurnedFrame(*mLive->ghost, m, t.accent, 1.0f, 4.0f, 3.0f);
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
                            // The parent's nine anchor points, turned with it when it is
                            UiDoc::UiPoint q{ p.x + p.w*fx, p.y + p.h*fy };
                            if (mLive->anchorM)
                                q = UiDoc::Apply(*mLive->anchorM, q);
                            Vec2F c = DesignToWorld(q.x, q.y);
                            Circle(c, 4.1f, t.accent);
                            Circle(c, 2.9f, Color4::White());
                        }
                    }
                }
            }

            if (sel)
            {
                UiDoc::UiMat m = BlockMatrix(selNode->id);
                TurnedFrame(*sel, m, Color4(10, 8, 20, 140), 4.5f);
                TurnedFrame(*sel, m, t.accent, 2.0f);
            }

            // The anchor: Unity's four triangles, split along a stretched axis
            bool editable = ctx->Editable() && !ctx->Busy();
            std::array<std::array<Vec2F, 3>, 4> tris;
            if (AnchorTriangles(tris))
            {
                bool hot = editable && !mLive && GripAt(o2Input.GetCursorPos()) == "anchor";
                for (auto& tri : tris)
                {
                    Vec2F a = tri[0], b = tri[1], c = tri[2];
                    Convex({ a + Vec2F(0, -1.5f), b + Vec2F(0, -1.5f), c + Vec2F(0, -1.5f) }, Color4(0, 0, 0, 70));
                    Convex({ a, b, c }, hot ? t.portImage : Color4::White());
                    Color4 ink(26, 16, 48, 255);
                    Line(a, b, ink, 1.3f);
                    Line(b, c, ink, 1.3f);
                    Line(c, a, ink, 1.3f);
                }
            }

            // The handles: the corners always, the edge middles while the block is not small
            if (sel && editable && selNode)
            {
                RectF s = ToWorld(*sel);
                bool small = s.Width() < 28.0f || Math::Abs(s.Height()) < 28.0f;
                String grip = mLive ? String() : GripAt(o2Input.GetCursorPos());
                // The turning knob on its stalk above the top edge
                DrawKnob(grip == "turn" || mDrag == DragKind::Rotate);
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
                // Under the box the selection takes on the screen
                auto cs = WorldCorners(*sel, BlockMatrix(selNode->id));
                RectF s(cs[0].x, cs[0].y, cs[0].x, cs[0].y);
                for (auto& c : cs)
                {
                    s.left = Math::Min(s.left, c.x); s.right = Math::Max(s.right, c.x);
                    s.top = Math::Max(s.top, c.y); s.bottom = Math::Min(s.bottom, c.y);
                }
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
    {
        // The request's points over the screen
        if (ctx)
            UiWsPins::Draw(*this, *ctx);
    }

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
        // lines, kit | chip | ids, the reference's compare tools | - % + fit
        float w = 6 + 28 + 2 + 28 + 2 + 9 + 2 + chipW + 2 + 9 + 2 + 28 + 2 + RefToolsWidth() + 9 + 2 + 28 + 2 + 48 + 2 + 28 + 2 + 28 + 6;
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
        DrawRefTools(x, top, r);
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
        if (!ctx || OnRefClick(id))
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
