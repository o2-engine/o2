#include "o2Editor/stdafx.h"
#include "UiWsPickers.h"

#include "o2/Render/Render.h"

namespace Editor
{
    using namespace UiWsDraw;
    namespace Ws = UiDoc::Ws;

    // --- options ------------------------------------------------------------------------------------

    void UiWsOptionsPanel::OnScrolled(float scroll)
    {
        mScroll = Math::Clamp(mScroll - scroll*0.5f, 0.0f, Math::Max(0.0f, ContentHeight() - layout->GetHeight()));
    }

    void UiWsOptionsPanel::DrawPanel(const Box& box)
    {
        auto& t = T();
        o2Render.EnableScissorTest((RectI)box.world);
        for (int i = 0; i < options.Count(); i++)
        {
            RectF r = box.R(6, 6 + i*28 - mScroll, box.W() - 12, 28);
            String id = "opt:" + options[i].first;
            bool on = options[i].first == current;
            if (on)
                Fill(r, 6.0f, t.accentSoft);
            else if (IsHover(id))
                Fill(r, 6.0f, t.active);
            Text(options[i].second, RectF(r.left + 10, r.top, r.right - 26, r.bottom), TextStyle(12.0f, 400, t.fg));
            if (on)
                UiWsDraw::Icon("uiws_check", Vec2F(r.right - 13, r.Center().y), 13.0f, t.accent);
            AddHot(r, id);
        }
        o2Render.DisableScissorTest();
    }

    void UiWsOptionsPanel::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (id.StartsWith("opt:") && onPick)
            onPick(id.SubStr(4));
    }

    // --- kit graphics ---------------------------------------------------------------------------------

    float UiWsKitGridPanel::ContentHeight() const
    {
        int count = (ctx ? ctx->kit.elements.Count() : 0) + 1;
        int rows = (count + 2)/3;
        return 16.0f + rows*84.0f + (rows - 1)*6.0f;
    }

    void UiWsKitGridPanel::OnScrolled(float scroll)
    {
        mScroll = Math::Clamp(mScroll - scroll*0.5f, 0.0f, Math::Max(0.0f, ContentHeight() - layout->GetHeight()));
    }

    void UiWsKitGridPanel::DrawPanel(const Box& box)
    {
        if (!ctx)
            return;

        auto& t = T();
        o2Render.EnableScissorTest((RectI)box.world);
        float cellW = (box.W() - 16 - 12)/3.0f;
        int count = ctx->kit.elements.Count() + 1;
        for (int i = 0; i < count; i++)
        {
            float x = 8 + (i % 3)*(cellW + 6), y = 8 + (i/3)*90 - mScroll;
            RectF r = box.R(x, y, cellW, 84);
            const UiDoc::UiKitElement* e = i > 0 ? &ctx->kit.elements[i - 1] : nullptr;
            String name = e ? e->name : String();
            String id = "kit:" + name;
            bool on = e ? current.ToLowerCase() == name.ToLowerCase() : current.IsEmpty();
            if (on)
            {
                Fill(r, 8.0f, t.accentSoft);
                Border(r, 8.0f, t.accent);
            }
            else if (IsHover(id))
                Fill(r, 8.0f, t.active);

            RectF pic = box.R(x + 4, y + 6, cellW - 8, 56);
            Checker(pic, 6.0f, 5.0f, t.checkerA, t.checkerB);
            if (e)
                KitThumb(*e, RectF(pic.left + pic.Width()*0.06f, pic.top - 4, pic.right - pic.Width()*0.06f, pic.bottom + 4), 3.0f, false);
            else
                UiWsDraw::Icon("uiws_sparkle", pic.Center(), 14.0f, t.accent);
            Text(e ? name : String("auto"), box.R(x + 4, y + 66, cellW - 8, 14),
                 TextStyle(10.5f, 400, on || IsHover(id) ? t.fg : t.fgDim, HorAlign::Middle));
            AddHot(r, id);
        }
        o2Render.DisableScissorTest();
    }

    void UiWsKitGridPanel::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (id.StartsWith("kit:") && onPick)
            onPick(id.SubStr(4));
    }

    // --- coverage ------------------------------------------------------------------------------------

    float UiWsCoveragePanel::ContentHeight(float width)
    {
        Box box(RectF(0, 0, width, -1000));
        return Lay(box, false);
    }

    void UiWsCoveragePanel::DrawPanel(const Box& box)
    {
        Lay(box, true);
    }

    float UiWsCoveragePanel::Lay(const Box& box, bool draw)
    {
        if (!ctx)
            return 40.0f;

        auto& t = T();
        float W = box.W(), y = 12.0f;
        float inner = W - 24;
        TextStyle noteStyle(11.5f, 400, t.fgDim);
        noteStyle.wrap = true;
        noteStyle.ver = VerAlign::Top;
        noteStyle.lineHeight = 16.7f;

        auto note = [&](const String& icon, const Color4& iconColor, const String& text)
        {
            float h = Math::Max(16.7f, TextHeight(text, inner - 21, noteStyle));
            if (draw)
            {
                UiWsDraw::Icon(icon, box.P(12 + 6.5f, y + 2 + 6.5f), 13.0f, iconColor);
                Text(text, box.R(12 + 21, y, inner - 21, h + 2), noteStyle);
            }
            y += h + 12;
        };

        // Chips flow left to right and wrap
        auto chips = [&](const String& title, const Vector<Pair<String, String>>& items, bool blocks)
        {
            if (draw)
                Text(title, box.R(12, y, inner, 14), TextStyle(11.0f, 600, t.fgDim));
            y += 14 + 6;
            float x = 12, rowY = y;
            for (auto& item : items)
            {
                float w = 7 + (blocks ? 12 : 16) + 5 + TextWidth(item.first, 10.5f, 400, true) + 7;
                if (x + w > W - 12 && x > 12)
                {
                    x = 12;
                    rowY += 22 + 4;
                }
                if (draw)
                {
                    RectF r = box.R(x, rowY, w, 22);
                    String id = (blocks ? "block:" : "unused:") + item.first;
                    Fill(r, 6.0f, t.field);
                    Border(r, 6.0f, blocks && IsHover(id) ? t.accent : t.line);
                    if (blocks)
                        UiWsDraw::Icon(TypeIcon(item.second), Vec2F(r.left + 7 + 6, r.Center().y), 12.0f, t.fgDim);
                    else if (auto e = UiDoc::KitElement(&ctx->kit, item.first))
                        KitThumb(*e, RectF(r.left + 7, r.Center().y + 8, r.left + 23, r.Center().y - 8), 3.0f);
                    Text(item.first, RectF(r.left + 7 + (blocks ? 12 : 16) + 5, r.top, r.right - 4, r.bottom),
                         TextStyle(10.5f, 400, t.fg, HorAlign::Left, true));
                    if (blocks)
                        AddHot(r, id);
                }
                x += w + 4;
            }
            y = rowY + (items.IsEmpty() ? 0 : 22) + 12;
        };

        auto& report = ctx->report;
        if (ctx->undescribed > 0)
            note("uiws_warn", t.yellow, (String)ctx->undescribed + " kit graphics have no name or description \xE2\x80\x94 the agent can only guess what "
                 "they are. Run \xE2\x80\x9C" "AI: describe all\xE2\x80\x9D in the UI kit first.");

        if (!report.missing.IsEmpty())
        {
            Vector<Pair<String, String>> items;
            for (auto& m : report.missing)
                items.Add(Pair<String, String>(m.first, m.second));
            chips("No kit graphic \xC2\xB7 " + (String)report.missing.Count(), items, true);
        }
        else
            note("uiws_check", t.green, "Every block is drawn with a kit graphic");

        if (!report.unused.IsEmpty())
        {
            Vector<Pair<String, String>> items;
            for (auto& name : report.unused)
                items.Add(Pair<String, String>(name, ""));
            chips("Kit graphics not used \xC2\xB7 " + (String)report.unused.Count(), items, false);
        }
        else
            note("uiws_check", t.green, "Every kit graphic is used");

        if (!report.glyphs.IsEmpty())
        {
            String ids;
            for (int i = 0; i < report.glyphs.Count(); i++)
                ids += (i ? ", " : "") + report.glyphs[i];
            note("uiws_warn", t.yellow, "Icons drawn as characters instead of kit graphics: " + ids);
        }
        return y;
    }

    void UiWsCoveragePanel::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (id.StartsWith("block:") && onPickBlock)
            onPickBlock(id.SubStr(6));
    }

    // --- parts ---------------------------------------------------------------------------------

    namespace UiWsParts
    {
        void AnchorCell(const RectF& rect, const String& h, const String& v, bool on, bool hover)
        {
            auto& t = T();
            // The web's 40-unit picture scaled into the cell
            float s = rect.Width()/40.0f;
            auto P = [&](float x, float y) { return Vec2F(rect.left + x*s, rect.top - y*s); };
            const float pb = 7.0f, size = 26.0f;
            auto span = [&](const String& k, float& from, float& len)
            {
                if (k == "left" || k == "top") { from = pb + 3; len = 9; }
                else if (k == "right" || k == "bottom") { from = pb + size - 12; len = 9; }
                else if (k == "stretch") { from = pb + 3; len = size - 6; }
                else { from = pb + size/2 - 4.5f; len = 9; }
            };
            Color4 outline = on ? t.accent : A(t.fg3, 0.8f);
            Color4 block = on ? t.accent : hover ? t.fgDim : A(t.fg3, 0.75f);
            Frame(RectF(P(pb, pb).x, P(pb, pb).y, P(pb + size, pb + size).x, P(pb + size, pb + size).y), 2.5f*s, outline, 1.0f);
            if (h == "stretch")
            {
                float y = pb + size + 3.5f;
                Line(P(pb + 1.5f, y), P(pb + size - 1.5f, y), outline, 1.0f);
                Line(P(pb + 4, y - 2), P(pb + 1.5f, y), outline, 1.0f);
                Line(P(pb + 4, y + 2), P(pb + 1.5f, y), outline, 1.0f);
                Line(P(pb + size - 4, y - 2), P(pb + size - 1.5f, y), outline, 1.0f);
                Line(P(pb + size - 4, y + 2), P(pb + size - 1.5f, y), outline, 1.0f);
            }
            if (v == "stretch")
            {
                float x = pb + size + 3.5f;
                Line(P(x, pb + 1.5f), P(x, pb + size - 1.5f), outline, 1.0f);
                Line(P(x - 2, pb + 4), P(x, pb + 1.5f), outline, 1.0f);
                Line(P(x + 2, pb + 4), P(x, pb + 1.5f), outline, 1.0f);
                Line(P(x - 2, pb + size - 4), P(x, pb + size - 1.5f), outline, 1.0f);
                Line(P(x + 2, pb + size - 4), P(x, pb + size - 1.5f), outline, 1.0f);
            }
            float bx, bw, by, bh;
            span(h, bx, bw);
            span(v, by, bh);
            Vec2F a = P(bx, by), b = P(bx + bw, by + bh);
            Fill(RectF(a.x, a.y, b.x, b.y), 1.5f*s, block);

            Vector<float> ax = h == "left" ? Vector<float>{ pb } : h == "right" ? Vector<float>{ pb + size } :
                h == "center" ? Vector<float>{ pb + size/2 } : Vector<float>{ pb, pb + size };
            Vector<float> ay = v == "top" ? Vector<float>{ pb } : v == "bottom" ? Vector<float>{ pb + size } :
                v == "middle" ? Vector<float>{ pb + size/2 } : Vector<float>{ pb, pb + size };
            for (float px : ax)
            {
                for (float py : ay)
                {
                    Circle(P(px, py), 3.2f*s, t.field.a < 200 ? t.cardSolid : t.field);
                    Circle(P(px, py), 2.2f*s, t.portImage);
                }
            }
        }

        void Toggle(const RectF& rect, bool on, bool hover)
        {
            auto& t = T();
            RectF track(rect.left, rect.Center().y + 9, rect.left + 32, rect.Center().y - 9);
            Fill(track, 9.0f, on ? t.accent : (t.dark ? Color4(255, 255, 255, hover ? 46 : 36) : t.fg3));
            float x = on ? track.left + 16 : track.left + 2;
            Circle(Vec2F(x + 7, track.Center().y), 7.0f, on ? Color4::White() : (t.dark ? t.fgDim : t.cardSolid));
        }

        void SliderBar(const RectF& rect, float value, const String& text, bool active)
        {
            auto& t = T();
            RectF track(rect.left, rect.Center().y + 5, rect.right - 48, rect.Center().y - 5);
            Fill(track, 5.0f, t.dark ? Color4(255, 255, 255, 26) : A(t.fg, 0.1f));
            float w = track.Width()*Math::Clamp01(value);
            if (w > 0.5f)
                Fill(RectF(track.left, track.top, track.left + w, track.bottom), 5.0f, active ? t.accent : (t.dark ? Color4(255, 255, 255, 71) : A(t.fg, 0.28f)));
            Text(text, RectF(rect.right - 44, rect.top, rect.right, rect.bottom), TextStyle(11.0f, 400, t.fgDim, HorAlign::Right, true));
        }
    }
}
