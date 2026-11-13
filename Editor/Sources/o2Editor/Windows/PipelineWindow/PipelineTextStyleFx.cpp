#include "o2Editor/stdafx.h"
#include "PipelineTextStyleFx.h"

#include <map>
#include <memory>

#include "o2Editor/Dialogs/ColorPickerDlg.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsPickers.h"

namespace Editor
{
    using namespace UiWsDraw;
    using UiDoc::TextEffect;
    using Style = UiDoc::TextStyle;

    namespace
    {
        const float wellW = 26.0f, gap = 5.0f, buttonW = 18.0f, switchW = 32.0f, nameW = 56.0f, subIndent = 36.0f;

        // ------------------------------------------------------------------
        // A number of an effect on a slider: its range, step and how it reads
        // ------------------------------------------------------------------
        struct Slider
        {
            const char* field;
            double      min, max, step;
            const char* prefix;
            const char* suffix;
            bool        percent = false;
        };

        // The colours of an effect's head row, its sliders there and on its second row
        struct EffectSpec
        {
            Vector<const char*> colors;
            Vector<Slider>      head;
            Vector<Slider>      sub;
        };

        const Slider opacity{ "opacity", 0.05, 1, 0.05, "", "", true };

        const EffectSpec& SpecOf(const String& type)
        {
            static const std::map<std::string, EffectSpec> specs = {
                { "stroke", { { "color" }, { { "width", 0.5, 30, 0.5, "", "px" } }, {} } },
                { "extrude", { { "color" }, { { "y", -30, 30, 1, "\xE2\x86\x93", "" }, { "x", -30, 30, 1, "\xE2\x86\x92", "" } }, {} } },
                { "glow", { { "color" }, { { "size", 1, 60, 1, "~", "" }, opacity }, {} } },
                { "shadow", { { "color" }, { opacity }, { { "x", -40, 40, 1, "\xE2\x86\x92", "" }, { "y", -40, 40, 1, "\xE2\x86\x93", "" },
                                                         { "blur", 0, 60, 1, "~", "" }, { "spread", 0, 30, 1, "+", "" } } } },
                { "innerShadow", { { "color" }, { opacity }, { { "x", -20, 20, 0.5, "\xE2\x86\x92", "" }, { "y", -20, 20, 0.5, "\xE2\x86\x93", "" },
                                                              { "blur", 0, 30, 0.5, "~", "" } } } },
                { "bevel", { { "highlight", "shade" }, { opacity }, { { "depth", 0.5, 20, 0.5, "\xE2\x86\x95", "" }, { "soft", 0, 20, 0.5, "~", "" } } } },
            };
            static const EffectSpec none;
            auto it = specs.find(type.Data());
            return it != specs.end() ? it->second : none;
        }

        double* FieldOf(TextEffect& e, const String& field)
        {
            if (field == "width") return &e.width;
            if (field == "x") return &e.x;
            if (field == "y") return &e.y;
            if (field == "blur") return &e.blur;
            if (field == "spread") return &e.spread;
            if (field == "size") return &e.size;
            if (field == "opacity") return &e.opacity;
            if (field == "depth") return &e.depth;
            if (field == "soft") return &e.soft;
            return nullptr;
        }

        String* ColorOf(TextEffect& e, const String& field)
        {
            return field == "highlight" ? &e.highlight : field == "shade" ? &e.shade : &e.color;
        }

        String FxName(const String& type)
        {
            return type == "stroke" ? "Outline" : type == "extrude" ? "Depth" : type == "shadow" ? "Shadow" : type == "glow" ? "Glow" :
                type == "innerShadow" ? "Inner" : type == "bevel" ? "Bevel" : type;
        }

        String FxHint(const String& type)
        {
            return type == "stroke" ? "An outline outside the letters; several nest from the letters out" :
                type == "extrude" ? "Solid depth: the outlined letters repeated towards the offset" :
                type == "shadow" ? "A drop shadow of the whole outlined shape; blur 0 = a hard copy" :
                type == "glow" ? "A soft halo around the letters" :
                type == "innerShadow" ? "A shade inside the letters along their edge; no offset = an inner glow" :
                "A light edge inside the top of the letters and a dark one inside the bottom";
        }

        String Format(const Slider& s, double v)
        {
            if (s.percent)
                return (String)(int)Math::Round((float)(v*100.0)) + "%";
            return String(s.prefix) + UiDoc::JsNum(v) + s.suffix;
        }

        double Snap(double v, double min, double max, double step)
        {
            double snapped = min + std::round((v - min)/step)*step;
            return std::round(Math::Clamp(snapped, min, max)*1000000.0)/1000000.0;
        }

        // The nearest earlier effect of the same kind, -1 for none: order matters only among those
        int Earlier(const Vector<TextEffect>& fx, int i)
        {
            for (int n = i - 1; n >= 0; n--)
            {
                if (fx[n].type == fx[i].type)
                    return n;
            }
            return -1;
        }

        String Mix(const String& a, const String& b, double k)
        {
            Color4 x, y;
            if (!PipelineUtils::ParseHexColor(a, x) || !PipelineUtils::ParseHexColor(b, y))
                return a;
            auto lerp = [k](int p, int q) { return (int)std::round(p + (q - p)*k); };
            return PipelineUtils::ColorToHex(Color4(lerp(x.r, y.r), lerp(x.g, y.g), lerp(x.b, y.b), 255)).ToUpperCase();
        }

        Color4 ColorOfHex(const String& hex)
        {
            Color4 color;
            if (!PipelineUtils::ParseHexColor(hex, color))
                color = Color4::White();
            return Color4(color.r, color.g, color.b, 255);
        }
    }

    PipelineTextFxPanel::PipelineTextFxPanel(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {}

    int PipelineTextFxPanel::EffectRows(const String& type)
    {
        return type == "shadow" || type == "innerShadow" || type == "bevel" ? 2 : 1;
    }

    int PipelineTextFxPanel::Rows(bool effects, const Style& style)
    {
        if (!effects)
            return style.fill ? 2 : 1;

        int rows = 1;
        for (auto& e : style.effects)
            rows += EffectRows(e.type);
        return rows;
    }

    float PipelineTextFxPanel::Height(bool effects, const Style& style)
    {
        int rows = Rows(effects, style);
        return rows*rowH + (rows - 1)*rowGap;
    }

    void PipelineTextFxPanel::WriteLook(DataValue& config, const Style& style)
    {
        auto next = UiDoc::NormalizeTextStyle(UiDoc::TextStyleToJson(style), style.name);
        UiDoc::WriteStyleLook(config, next);
    }

    UiDoc::TextGradient PipelineTextFxPanel::WithStopAt(const UiDoc::TextGradient& gradient, double at, int& index)
    {
        auto out = gradient;
        auto& stops = out.stops;
        double v = std::round(Math::Clamp(at, 0.0, 1.0)*100.0)/100.0;
        int i = stops.IndexOf([&](const UiDoc::GradientStop& s) { return s.at > v; });
        if (i < 0)
            i = stops.Count();
        const auto& a = stops[Math::Max(0, i - 1)];
        const auto& b = stops[Math::Min(stops.Count() - 1, i)];
        String color = b.at > a.at ? Mix(a.color, b.color, (v - a.at)/(b.at - a.at)) : a.color;
        stops.Insert(UiDoc::GradientStop{ color, v }, i);
        index = i;
        return out;
    }

    Vector<TextEffect> PipelineTextFxPanel::MovedUp(const Vector<TextEffect>& effects, int index)
    {
        auto out = effects;
        if (index < 0 || index >= out.Count())
            return out;

        int n = Earlier(out, index);
        if (n < 0)
            return out;

        auto moved = out[index];
        out[index] = out[n];
        out[n] = moved;
        return out;
    }

    void PipelineTextFxPanel::Look(const Style& style, bool done)
    {
        if (onLook)
            onLook(style, done);
    }

    // --- drawing ---------------------------------------------------------------------------------

    void PipelineTextFxPanel::DrawPanel(const Box& box)
    {
        if (!getStyle)
            return;

        auto style = getStyle();
        if (!effects)
            DrawFill(box, style);
        else
            DrawEffects(box, style);
    }

    void PipelineTextFxPanel::Well(const RectF& rect, const String& color, const String& id, bool dim)
    {
        auto& t = T();
        float alpha = dim ? 0.4f : 1.0f;
        RectF well(rect.left, rect.Center().y + 11.0f, rect.left + wellW, rect.Center().y - 11.0f);
        Fill(well, 5.0f, A(IsHover(id) ? t.active : t.field, alpha));
        Border(well, 5.0f, A(t.line, alpha));
        RectF swatch(well.left + 3.0f, well.top - 3.0f, well.right - 3.0f, well.bottom + 3.0f);
        Fill(swatch, 4.0f, A(ColorOfHex(color), alpha));
        Border(swatch, 4.0f, Color4(255, 255, 255, (int)(41*alpha)));
        AddHot(well, id);
    }

    void PipelineTextFxPanel::Bar(const RectF& rect, const String& key, float share, const String& text, bool dim)
    {
        auto& t = T();
        float alpha = dim ? 0.4f : 1.0f;
        String hot = "slider:" + key;
        bool active = mDrag == hot || IsHover(hot);
        float valueW = 30.0f;
        RectF track(rect.left, rect.Center().y + 5.0f, Math::Max(rect.left + 4.0f, rect.right - valueW - gap), rect.Center().y - 5.0f);
        Fill(track, 5.0f, A(t.dark ? Color4(255, 255, 255, 26) : A(t.fg, 0.1f), alpha));
        float w = track.Width()*Math::Clamp01(share);
        if (w > 0.5f)
            Fill(RectF(track.left, track.top, track.left + w, track.bottom), 5.0f,
                 A(active ? t.accent : (t.dark ? Color4(255, 255, 255, 71) : A(t.fg, 0.28f)), alpha));
        Text(text, RectF(rect.right - valueW, rect.top, rect.right, rect.bottom), TextStyle(10.0f, 400, A(t.fgDim, alpha), HorAlign::Right, true));
        AddHot(RectF(track.left, rect.top, track.right, rect.bottom), hot);
    }

    void PipelineTextFxPanel::Switch(const RectF& rect, bool on, const String& id, const String& caption)
    {
        auto& t = T();
        float w = switchW + (caption.IsEmpty() ? 0.0f : 6.0f + TextWidth(caption, 11.0f));
        RectF area(rect.left, rect.top, rect.left + w, rect.bottom);
        UiWsParts::Toggle(area, on, IsHover(id));
        if (!caption.IsEmpty())
            Text(caption, RectF(rect.left + switchW + 6.0f, rect.top, area.right + 2.0f, rect.bottom), TextStyle(11.0f, 400, t.fg));
        AddHot(area, id);
    }

    void PipelineTextFxPanel::SmallButton(const RectF& rect, const String& icon, const String& id, bool enabled)
    {
        auto& t = T();
        RectF b(rect.left, rect.Center().y + buttonW*0.5f, rect.left + buttonW, rect.Center().y - buttonW*0.5f);
        bool hover = enabled && IsHover(id);
        if (hover)
            Fill(b, 5.0f, t.field);
        Icon(icon, b.Center(), 10.0f, A(hover ? t.fg : t.fg3, enabled ? 1.0f : 0.35f));
        if (enabled)
            AddHot(b, id);
    }

    void PipelineTextFxPanel::DrawFill(const Box& box, const Style& st)
    {
        auto& t = T();
        float W = box.W(), x = labelW + 6.0f;
        Text("Fill", box.R(0, 0, labelW, rowH), TextStyle(11.0f, 400, t.fgDim));
        if (st.fill)
        {
            RectF r = box.R(x, 0, W - x, rowH);
            Switch(r, true, "gradient", "gradient");
            float after = switchW + 6.0f + TextWidth("gradient", 11.0f) + 2.0f + 6.0f;
            Bar(box.R(x + after, 0, W - x - after, rowH), "angle", (float)(st.fill->angle/345.0), UiDoc::JsNum(st.fill->angle) + "\xC2\xB0");

            // The stops on the bar, the selected one's colour, its removal
            float y = rowH + rowGap;
            Text("Colours", box.R(0, y, labelW, rowH), TextStyle(11.0f, 400, t.fgDim));
            auto& stops = st.fill->stops;
            int stop = Math::Clamp(mStop, 0, stops.Count() - 1);
            float barRight = W - wellW - gap - buttonW - gap - 6.0f;
            mBar = box.R(x + 6.0f, y + 6.0f, Math::Max(10.0f, barRight - x - 6.0f), 14.0f);
            int strips = 64;
            for (int i = 0; i < strips; i++)
            {
                float a = (float)i/strips, b = (float)(i + 1)/strips, mid = (a + b)*0.5f;
                String color = stops[0].color;
                for (int s = 0; s + 1 < stops.Count(); s++)
                {
                    if (mid >= stops[s].at && mid <= stops[s + 1].at)
                    {
                        double span = stops[s + 1].at - stops[s].at;
                        color = span > 0 ? Mix(stops[s].color, stops[s + 1].color, (mid - stops[s].at)/span) : stops[s + 1].color;
                        break;
                    }
                    if (mid > stops[s + 1].at)
                        color = stops[s + 1].color;
                }
                float l = mBar.left + mBar.Width()*a, r = mBar.left + mBar.Width()*b + 0.6f;
                PipelineDraw::Fill(RectF(l, mBar.top, Math::Min(r, mBar.right), mBar.bottom), PipelineDraw::Corners(0.0f), ColorOfHex(color));
            }
            Border(mBar, 0.0f, t.line);
            AddHot(mBar, "bar");
            for (int i = 0; i < stops.Count(); i++)
            {
                float cx = mBar.left + mBar.Width()*(float)stops[i].at;
                RectF handle(cx - 6.0f, mBar.Center().y + 9.0f, cx + 6.0f, mBar.Center().y - 9.0f);
                if (i == stop)
                    Fill(RectF(handle.left - 3.0f, handle.top + 3.0f, handle.right + 3.0f, handle.bottom - 3.0f), 6.0f, t.accent);
                Fill(handle, 4.0f, Color4(0, 0, 0, 140));
                Fill(RectF(handle.left + 1.0f, handle.top - 1.0f, handle.right - 1.0f, handle.bottom + 1.0f), 3.5f, Color4::White());
                Fill(RectF(handle.left + 3.0f, handle.top - 3.0f, handle.right - 3.0f, handle.bottom + 3.0f), 2.0f, ColorOfHex(stops[i].color));
                AddHot(RectF(handle.left - 2.0f, handle.top, handle.right + 2.0f, handle.bottom), "stop:" + (String)i);
            }
            Well(box.R(W - wellW - gap - buttonW, y, wellW, rowH), stops[stop].color, "color:stop");
            SmallButton(box.R(W - buttonW, y, buttonW, rowH), "uiws_close", "stopRemove", stops.Count() > 2);
        }
        else
        {
            Well(box.R(x, 0, wellW, rowH), st.color, "color:fill");
            Switch(box.R(x + wellW + 6.0f, 0, W - x - wellW - 6.0f, rowH), false, "gradient", "gradient");
        }
    }

    void PipelineTextFxPanel::DrawEffects(const Box& box, const Style& st)
    {
        auto& t = T();
        float W = box.W(), x = labelW + 6.0f;

        // The head: add an effect (a menu of the kinds)
        Text("Effects", box.R(0, 0, labelW, rowH), TextStyle(11.0f, 400, t.fgDim));
        bool full = st.effects.Count() >= UiDoc::TEXT_EFFECTS_MAX;
        RectF select = box.R(x, 0, W - x, rowH);
        Fill(select, 7.0f, A(t.field, full ? 0.5f : 1.0f));
        Border(select, 7.0f, IsHover("add") ? t.accent : t.line);
        Text(full ? "a style takes at most " + (String)UiDoc::TEXT_EFFECTS_MAX + " effects" : String("+ add an effect\xE2\x80\xA6"),
             RectF(select.left + 9.0f, select.top, select.right - 24.0f, select.bottom), TextStyle(11.5f, 400, full ? t.fg3 : t.fgDim));
        Icon("uiws_chevron_down", Vec2F(select.right - 13.0f, select.Center().y), 11.0f, t.fg3);
        if (!full)
            AddHot(select, "add");

        float y = rowH + rowGap;
        for (int i = 0; i < st.effects.Count(); i++)
        {
            auto e = st.effects[i];
            auto& spec = SpecOf(e.type);
            String id = "fx" + (String)i;
            bool dim = e.off;

            // The head row: on/off, the kind, its colours and numbers, before the previous one of its kind, remove
            Switch(box.R(0, y, switchW, rowH), !e.off, id + ":toggle");
            Text(FxName(e.type), box.R(switchW + gap, y, nameW, rowH), TextStyle(11.0f, 400, A(t.fg, dim ? 0.4f : 1.0f)));
            bool up = Earlier(st.effects, i) >= 0;
            float right = W - buttonW - (up ? buttonW + gap : 0.0f);
            if (up)
                SmallButton(box.R(right, y, buttonW, rowH), "al_cmp_up", id + ":up");
            SmallButton(box.R(W - buttonW, y, buttonW, rowH), "uiws_close", id + ":remove");

            float cx = switchW + gap + nameW + gap;
            for (auto color : spec.colors)
            {
                Well(box.R(cx, y, wellW, rowH), *ColorOf(e, color), id + ":" + color, dim);
                cx += wellW + gap;
            }
            auto bars = [&](const Vector<Slider>& sliders, float from, float to, float rowY)
            {
                if (sliders.IsEmpty())
                    return;
                float each = (to - from - gap*(sliders.Count() - 1))/sliders.Count();
                for (int s = 0; s < sliders.Count(); s++)
                {
                    auto& sl = sliders[s];
                    double v = *FieldOf(e, sl.field);
                    float share = (float)((v - sl.min)/(sl.max - sl.min));
                    Bar(box.R(from + s*(each + gap), rowY, each, rowH), id + ":" + sl.field, share, Format(sl, v), dim);
                }
            };
            bars(spec.head, cx, right - gap, y);
            y += rowH + rowGap;
            if (!spec.sub.IsEmpty())
            {
                bars(spec.sub, subIndent, W, y);
                y += rowH + rowGap;
            }
        }
    }

    // --- edits -----------------------------------------------------------------------------------

    void PipelineTextFxPanel::AddEffect(const String& type)
    {
        auto st = getStyle();
        if (st.effects.Count() >= UiDoc::TEXT_EFFECTS_MAX || !UiDoc::TextEffectTypes().Contains(type))
            return;

        st.effects.Add(UiDoc::DefaultEffect(type, st));
        Look(st, true);
        if (onRows)
            onRows();
    }

    void PipelineTextFxPanel::SetSlider(const String& key, float share, bool done)
    {
        auto st = getStyle();
        share = Math::Clamp01(share);
        if (key == "angle")
        {
            if (!st.fill)
                return;
            st.fill->angle = Snap(share*345.0, 0, 345, 15);
            Look(st, done);
            return;
        }

        int colon = key.Find(":");
        int i = colon > 2 ? atoi(key.SubStr(2, colon).Data()) : -1;
        if (i < 0 || i >= st.effects.Count())
            return;

        String field = key.SubStr(colon + 1);
        auto& spec = SpecOf(st.effects[i].type);
        for (auto list : { &spec.head, &spec.sub })
        {
            for (auto& sl : *list)
            {
                if (field == sl.field)
                {
                    *FieldOf(st.effects[i], field) = Snap(sl.min + share*(sl.max - sl.min), sl.min, sl.max, sl.step);
                    Look(st, done);
                    return;
                }
            }
        }
    }

    void PipelineTextFxPanel::MoveStop(float at, bool done)
    {
        auto st = getStyle();
        if (!st.fill || mStop < 0 || mStop >= st.fill->stops.Count())
            return;

        // A stop stays between its neighbours: the order of the colours is the list's
        auto& stops = st.fill->stops;
        double lo = mStop > 0 ? stops[mStop - 1].at : 0.0, hi = mStop + 1 < stops.Count() ? stops[mStop + 1].at : 1.0;
        double v = Math::Clamp(std::round(Math::Clamp01(at)*100.0)/100.0, lo, hi);
        if (v == stops[mStop].at && !done)
            return;

        stops[mStop].at = v;
        Look(st, done);
    }

    void PipelineTextFxPanel::PressBar(float at)
    {
        auto st = getStyle();
        if (!st.fill)
            return;

        // On a stop: it is selected and dragged
        auto& stops = st.fill->stops;
        for (int i = 0; i < stops.Count(); i++)
        {
            if (Math::Abs((float)stops[i].at - at)*Math::Max(1.0f, mBar.Width()) <= 7.0f)
            {
                mStop = i;
                mDrag = "stop";
                return;
            }
        }

        // Elsewhere: a new stop there, in the colour the gradient has there
        if (stops.Count() >= UiDoc::GRADIENT_STOPS_MAX)
            return;

        int i = 0;
        st.fill = WithStopAt(*st.fill, at, i);
        mStop = i;
        mDrag = "stop";
        Look(st, false);
    }

    void PipelineTextFxPanel::DragStopTo(float at)
    {
        MoveStop(at, true);
        mDrag = "";
    }

    void PipelineTextFxPanel::Slide(const String& key, float share)
    {
        SetSlider(key, share, true);
    }

    void PipelineTextFxPanel::PickColor(const String& key)
    {
        auto st = getStyle();
        String hex;
        if (key == "fill")
            hex = st.color;
        else if (key == "stop")
        {
            if (!st.fill)
                return;
            hex = st.fill->stops[Math::Clamp(mStop, 0, st.fill->stops.Count() - 1)].color;
        }
        else
        {
            int colon = key.Find(":");
            int i = colon > 2 ? atoi(key.SubStr(2, colon).Data()) : -1;
            if (i < 0 || i >= st.effects.Count())
                return;
            hex = *ColorOf(st.effects[i], key.SubStr(colon + 1));
        }

        WeakRef<PipelineTextFxPanel> weakThis(this);
        String target = key;
        auto apply = [weakThis, target](const Color4& value, bool done)
        {
            auto self = weakThis.Lock();
            if (!self || !self->getStyle)
                return;

            auto st = self->getStyle();
            String hex = PipelineUtils::ColorToHex(Color4(value.r, value.g, value.b, 255)).ToUpperCase();
            if (target == "fill")
                st.color = hex;
            else if (target == "stop")
            {
                if (!st.fill)
                    return;
                st.fill->stops[Math::Clamp(self->mStop, 0, st.fill->stops.Count() - 1)].color = hex;
            }
            else
            {
                int colon = target.Find(":");
                int i = atoi(target.SubStr(2, colon).Data());
                if (i < 0 || i >= st.effects.Count())
                    return;
                *ColorOf(st.effects[i], target.SubStr(colon + 1)) = hex;
            }
            self->Look(st, done);
        };
        Color4 start = ColorOfHex(hex);
        auto current = std::make_shared<Color4>(start);
        ColorPickerDlg::Show(start, [apply, current](const Color4& value, bool) { *current = value; apply(value, false); },
                             [apply, current]() { apply(*current, true); });
    }

    void PipelineTextFxPanel::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (!getStyle)
            return;

        auto st = getStyle();
        if (id == "gradient")
        {
            if (st.fill)
            {
                st.color = st.fill->stops[0].color;
                st.fill.reset();
            }
            else
                st.fill = UiDoc::TextGradient{ 90, { UiDoc::GradientStop{ st.color, 0 }, UiDoc::GradientStop{ st.gradient.value_or("#FF8C00"), 1 } } };
            mStop = 0;
            Look(st, true);
            if (onRows)
                onRows();
        }
        else if (id == "stopRemove")
        {
            if (!st.fill || st.fill->stops.Count() <= 2)
                return;
            st.fill->stops.RemoveAt(Math::Clamp(mStop, 0, st.fill->stops.Count() - 1));
            mStop = Math::Max(0, mStop - 1);
            Look(st, true);
        }
        else if (id.StartsWith("color:"))
            PickColor(id.SubStr(6));
        else if (id == "add")
        {
            auto owner = editor.Lock();
            if (!owner)
                return;

            PipelineOptionPickerRequest request;
            for (auto& type : UiDoc::TextEffectTypes())
                request.options.Add(PipelineOption{ type, FxName(type), FxHint(type), "" });
            RectF world = HotRect("add");
            Vec2F a = owner->LocalToScreenPoint(Vec2F(world.left, world.bottom));
            Vec2F b = owner->LocalToScreenPoint(Vec2F(world.right, world.top));
            request.anchor = RectF(Math::Min(a.x, b.x), Math::Max(a.y, b.y), Math::Max(a.x, b.x), Math::Min(a.y, b.y));
            request.field = Ref<Widget>(this);
            WeakRef<PipelineTextFxPanel> weakThis(this);
            request.onPick = [weakThis](const String& type) { if (auto self = weakThis.Lock()) self->AddEffect(type); };
            owner->ShowOptionPicker(request);
        }
        else if (id.StartsWith("fx"))
        {
            int colon = id.Find(":");
            int i = colon > 2 ? atoi(id.SubStr(2, colon).Data()) : -1;
            if (i < 0 || i >= st.effects.Count())
                return;

            String act = id.SubStr(colon + 1);
            if (act == "toggle")
                st.effects[i].off = !st.effects[i].off;
            else if (act == "remove")
                st.effects.RemoveAt(i);
            else if (act == "up")
            {
                if (Earlier(st.effects, i) < 0)
                    return;
                st.effects = MovedUp(st.effects, i);
            }
            else
            {
                PickColor(id.SubStr(0, colon) + ":" + act);
                return;
            }
            Look(st, true);
            if (act == "remove" && onRows)
                onRows();
        }
    }

    // --- dragging --------------------------------------------------------------------------------

    void PipelineTextFxPanel::OnCursorPressed(const Input::Cursor& cursor)
    {
        UiWsPanel::OnCursorPressed(cursor);
        if (mPressedHot.StartsWith("slider:"))
        {
            mDrag = mPressedHot;
            mDragTrack = HotRect(mPressedHot);
            SetSlider(mDrag.SubStr(7), (cursor.position.x - mDragTrack.left)/Math::Max(1.0f, mDragTrack.Width()), false);
        }
        else if (mPressedHot == "bar" || mPressedHot.StartsWith("stop:"))
        {
            if (mPressedHot.StartsWith("stop:"))
            {
                mStop = atoi(mPressedHot.SubStr(5).Data());
                mDrag = "stop";
            }
            else
                PressBar((cursor.position.x - mBar.left)/Math::Max(1.0f, mBar.Width()));
            mDragTrack = mBar;
        }
    }

    void PipelineTextFxPanel::OnCursorStillDown(const Input::Cursor& cursor)
    {
        float share = (cursor.position.x - mDragTrack.left)/Math::Max(1.0f, mDragTrack.Width());
        if (mDrag.StartsWith("slider:"))
            SetSlider(mDrag.SubStr(7), share, false);
        else if (mDrag == "stop")
            MoveStop(share, false);
    }

    void PipelineTextFxPanel::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (!mDrag.IsEmpty())
        {
            float share = (cursor.position.x - mDragTrack.left)/Math::Max(1.0f, mDragTrack.Width());
            if (mDrag.StartsWith("slider:"))
                SetSlider(mDrag.SubStr(7), share, true);
            else
                MoveStop(share, true);
            mDrag = "";
            mPressedHot = "";
            return;
        }
        UiWsPanel::OnCursorReleased(cursor);
    }
}
