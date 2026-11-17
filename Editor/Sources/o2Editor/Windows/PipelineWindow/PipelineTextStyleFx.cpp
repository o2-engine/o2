#include "o2Editor/stdafx.h"
#include "PipelineTextStyleFx.h"

#include <map>
#include <memory>

#include "o2Editor/Dialogs/ColorPickerDlg.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTextStyleFxParts.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsPickers.h"

namespace Editor
{
    using namespace UiWsDraw;
    using namespace TextFx;
    using UiDoc::TextEffect;
    using Style = UiDoc::TextStyle;

    namespace TextFx
    {
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

        const Slider& AngleSlider()
        {
            static const Slider angle{ "angle", 0, 345, 15, "", "\xC2\xB0" };
            return angle;
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
            if (field == "angle") return e.fill ? &e.fill->angle : nullptr;
            return nullptr;
        }

        String* ColorOf(TextEffect& e, const String& field)
        {
            return field == "highlight" ? &e.highlight : field == "shade" ? &e.shade : &e.color;
        }

        String FxName(const String& type)
        {
            return type == "stroke" ? "Outline" : type == "extrude" ? "Depth" : type == "shadow" ? "Shadow" : type == "glow" ? "Glow" :
                type == "innerShadow" ? "Inner" : type == "bevel" ? "Bevel" : type.Data();
        }

        String FxHint(const String& type)
        {
            return type == "stroke" ? "An outline outside the letters. Several nest: the first lies next to the letters, the next goes around it" :
                type == "extrude" ? "Solid depth: the outlined letters repeated towards the offset" :
                type == "shadow" ? "A drop shadow of the whole outlined shape; blur 0 = a hard copy" :
                type == "glow" ? "A soft halo around the letters" :
                type == "innerShadow" ? "An inner shadow: a shade inside the letters along their edge; no offset = an inner glow" :
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

        int FxIndexOf(const String& id)
        {
            if (!id.StartsWith("fx"))
                return -1;
            int colon = id.Find(":");
            return colon > 2 ? atoi(id.SubStr(2, colon).Data()) : -1;
        }
    }

    PipelineTextFxPanel::PipelineTextFxPanel(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {}

    int PipelineTextFxPanel::EffectRows(const TextEffect& e)
    {
        if (e.type == "stroke")
            return e.fill ? 2 : 1;
        return e.type == "extrude" || e.type == "glow" ? 1 : 2;
    }

    int PipelineTextFxPanel::Rows(bool effects, const Style& style)
    {
        if (!effects)
            return style.fill ? 2 : 1;

        int rows = 1;
        for (auto& e : style.effects)
            rows += EffectRows(e);
        return rows;
    }

    float PipelineTextFxPanel::Height(bool effects, const Style& style)
    {
        int rows = Rows(effects, style);
        return rows*rowH + (rows - 1)*rowGap + (effects ? boxExtra : 0.0f);
    }

    void PipelineTextFxPanel::WriteLook(DataValue& config, const Style& style)
    {
        auto raw = UiDoc::TextStyleToJson(style);
        raw["fxStack"] = true;
        UiDoc::WriteStyleLook(config, UiDoc::NormalizeTextStyle(raw, style.name));
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

    Vector<TextEffect> PipelineTextFxPanel::Moved(const Vector<TextEffect>& effects, int from, int to)
    {
        auto out = effects;
        if (from < 0 || from >= out.Count())
            return out;

        auto moved = out[from];
        out.RemoveAt(from);
        out.Insert(moved, Math::Clamp(to, 0, out.Count()));
        return out;
    }

    void PipelineTextFxPanel::Look(const Style& style, bool done)
    {
        if (onLook)
            onLook(style, done);
    }

    UiDoc::TextGradient* PipelineTextFxPanel::GradientOf(Style& style, const String& owner)
    {
        if (owner.IsEmpty())
            return style.fill ? &*style.fill : nullptr;

        int i = FxIndexOf(owner + ":");
        if (i < 0 || i >= style.effects.Count() || !style.effects[i].fill)
            return nullptr;
        return &*style.effects[i].fill;
    }

    int& PipelineTextFxPanel::StopOf(const String& owner)
    {
        if (owner.IsEmpty())
            return mStop;
        return mFxStops[FxIndexOf(owner + ":")];
    }

    int PipelineTextFxPanel::GetSelectedStop(const String& owner /*= ""*/) const
    {
        if (owner.IsEmpty())
            return mStop;

        int stop = 0;
        mFxStops.TryGetValue(FxIndexOf(owner + ":"), stop);
        return stop;
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

    void PipelineTextFxPanel::SmallButton(const RectF& rect, const String& icon, const String& id, bool enabled, bool on,
                                          float iconSize)
    {
        // .uts-fx-b: a wash on hover, the accent while on
        auto& t = T();
        RectF b(rect.left, rect.Center().y + buttonW*0.5f, rect.left + buttonW, rect.Center().y - buttonW*0.5f);
        bool hover = enabled && IsHover(id);
        if (on)
            Fill(b, 5.0f, A(t.accent, 0.16f));
        else if (hover)
            Fill(b, 5.0f, t.field);
        Color4 color = on ? t.accent : hover ? t.fg : t.fg3;
        Icon(icon, b.Center(), iconSize, A(color, enabled ? 1.0f : 0.35f));
        if (enabled)
            AddHot(b, id);
    }

    void PipelineTextFxPanel::DrawStopsBar(const RectF& bar, const UiDoc::TextGradient& gradient, int selected, const String& prefix,
                                           bool dim)
    {
        auto& t = T();
        float alpha = dim ? 0.4f : 1.0f;
        auto& stops = gradient.stops;
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
            float l = bar.left + bar.Width()*a, r = bar.left + bar.Width()*b + 0.6f;
            PipelineDraw::Fill(RectF(l, bar.top, Math::Min(r, bar.right), bar.bottom), PipelineDraw::Corners(0.0f), A(ColorOfHex(color), alpha));
        }
        Border(bar, 0.0f, A(t.line, alpha));
        AddHot(bar, prefix + "bar");
        for (int i = 0; i < stops.Count(); i++)
        {
            float cx = bar.left + bar.Width()*(float)stops[i].at;
            RectF handle(cx - 6.0f, bar.Center().y + 9.0f, cx + 6.0f, bar.Center().y - 9.0f);
            if (i == selected)
                Fill(RectF(handle.left - 3.0f, handle.top + 3.0f, handle.right + 3.0f, handle.bottom - 3.0f), 6.0f, A(t.accent, alpha));
            Fill(handle, 4.0f, Color4(0, 0, 0, (int)(140*alpha)));
            Fill(RectF(handle.left + 1.0f, handle.top - 1.0f, handle.right - 1.0f, handle.bottom + 1.0f), 3.5f, A(Color4::White(), alpha));
            Fill(RectF(handle.left + 3.0f, handle.top - 3.0f, handle.right - 3.0f, handle.bottom + 3.0f), 2.0f, A(ColorOfHex(stops[i].color), alpha));
            AddHot(RectF(handle.left - 2.0f, handle.top, handle.right + 2.0f, handle.bottom), prefix + "stop:" + (String)i);
        }
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
            DrawStopsBar(mBar, *st.fill, stop, "");
            Well(box.R(W - wellW - gap - buttonW, y, wellW, rowH), stops[stop].color, "color:stop");
            SmallButton(box.R(W - buttonW, y, buttonW, rowH), "uiws_close", "stopRemove", stops.Count() > 2);
        }
        else
        {
            Well(box.R(x, 0, wellW, rowH), st.color, "color:fill");
            Switch(box.R(x + wellW + 6.0f, 0, W - x - wellW - 6.0f, rowH), false, "gradient", "gradient");
        }
    }

    // --- edits -----------------------------------------------------------------------------------

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

        int i = FxIndexOf(key);
        if (i < 0 || i >= st.effects.Count())
            return;

        String field = key.SubStr(key.Find(":") + 1);
        Vector<Slider> sliders = SpecOf(st.effects[i].type).head;
        sliders.Add(SpecOf(st.effects[i].type).sub);
        sliders.Add(AngleSlider());
        for (auto& sl : sliders)
        {
            double* value = field == sl.field ? FieldOf(st.effects[i], field) : nullptr;
            if (value)
            {
                *value = Snap(sl.min + share*(sl.max - sl.min), sl.min, sl.max, sl.step);
                Look(st, done);
                return;
            }
        }
    }

    void PipelineTextFxPanel::MoveStop(float at, bool done)
    {
        auto st = getStyle();
        String owner = mDrag.StartsWith("stop:") ? mDrag.SubStr(5) : String();
        auto gradient = GradientOf(st, owner);
        int& selected = StopOf(owner);
        if (!gradient || selected < 0 || selected >= gradient->stops.Count())
            return;

        // A stop stays between its neighbours: the order of the colours is the list's
        auto& stops = gradient->stops;
        double lo = selected > 0 ? stops[selected - 1].at : 0.0, hi = selected + 1 < stops.Count() ? stops[selected + 1].at : 1.0;
        double v = Math::Clamp(std::round(Math::Clamp01(at)*100.0)/100.0, lo, hi);
        if (v == stops[selected].at && !done)
            return;

        stops[selected].at = v;
        Look(st, done);
    }

    void PipelineTextFxPanel::PressBar(float at, const String& owner /*= ""*/)
    {
        auto st = getStyle();
        auto gradient = GradientOf(st, owner);
        if (!gradient)
            return;

        // On a stop: it is selected and dragged
        RectF bar = mBar;
        if (!owner.IsEmpty())
            mFxBars.TryGetValue(FxIndexOf(owner + ":"), bar);
        auto& stops = gradient->stops;
        for (int i = 0; i < stops.Count(); i++)
        {
            if (Math::Abs((float)stops[i].at - at)*(bar.Width() > 1.0f ? bar.Width() : 100.0f) <= 7.0f)
            {
                StopOf(owner) = i;
                mDrag = "stop:" + owner;
                return;
            }
        }

        // Elsewhere: a new stop there, in the colour the gradient has there
        if (stops.Count() >= UiDoc::GRADIENT_STOPS_MAX)
            return;

        int i = 0;
        *gradient = WithStopAt(*gradient, at, i);
        StopOf(owner) = i;
        mDrag = "stop:" + owner;
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
        auto colorOf = [](PipelineTextFxPanel& self, Style& style, const String& target) -> String*
        {
            if (target == "fill")
                return &style.color;

            int i = FxIndexOf(target);
            String owner = i >= 0 ? target.SubStr(0, target.Find(":")) : String();
            String field = i >= 0 ? target.SubStr(target.Find(":") + 1) : target;
            if (field == "stop")
            {
                auto gradient = GradientOf(style, owner);
                return gradient ? &gradient->stops[Math::Clamp(self.StopOf(owner), 0, gradient->stops.Count() - 1)].color : nullptr;
            }
            if (i < 0 || i >= style.effects.Count())
                return nullptr;
            return ColorOf(style.effects[i], field);
        };

        String* start = colorOf(*this, st, key);
        if (!start)
            return;

        WeakRef<PipelineTextFxPanel> weakThis(this);
        String target = key;
        auto apply = [weakThis, target, colorOf](const Color4& value, bool done)
        {
            auto self = weakThis.Lock();
            if (!self || !self->getStyle)
                return;

            auto st = self->getStyle();
            String* color = colorOf(*self, st, target);
            if (!color)
                return;

            *color = PipelineUtils::ColorToHex(Color4(value.r, value.g, value.b, 255)).ToUpperCase();
            self->Look(st, done);
        };
        Color4 from = ColorOfHex(*start);
        auto current = std::make_shared<Color4>(from);
        ColorPickerDlg::Show(from, [apply, current](const Color4& value, bool) { *current = value; apply(value, false); },
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
        else if (id.StartsWith("add:"))
            AddEffect(id.SubStr(4));
        else if (FxIndexOf(id) >= 0)
        {
            int i = FxIndexOf(id);
            if (i < st.effects.Count())
                EffectClick(st, i, id.SubStr(id.Find(":") + 1));
        }
    }

    // --- dragging --------------------------------------------------------------------------------

    void PipelineTextFxPanel::OnCursorMoved(const Input::Cursor& cursor)
    {
        UiWsPanel::OnCursorMoved(cursor);
        mCursor = cursor.position;
    }

    void PipelineTextFxPanel::OnCursorPressed(const Input::Cursor& cursor)
    {
        UiWsPanel::OnCursorPressed(cursor);
        mCursor = cursor.position;
        if (mPressedHot.StartsWith("slider:"))
        {
            mDrag = mPressedHot;
            mDragTrack = HotRect(mPressedHot);
            SetSlider(mDrag.SubStr(7), (cursor.position.x - mDragTrack.left)/Math::Max(1.0f, mDragTrack.Width()), false);
            return;
        }

        // A gradient's bar or one of its stops: the fill's, or an outline's (fx<i>:bar, fx<i>:stop:<n>)
        int i = FxIndexOf(mPressedHot);
        String owner = i >= 0 ? mPressedHot.SubStr(0, mPressedHot.Find(":")) : String();
        String part = i >= 0 ? mPressedHot.SubStr(mPressedHot.Find(":") + 1) : mPressedHot;
        if (part == "bar" || part.StartsWith("stop:"))
        {
            mDragTrack = mBar;
            if (!owner.IsEmpty())
                mFxBars.TryGetValue(i, mDragTrack);
            if (part.StartsWith("stop:"))
            {
                StopOf(owner) = atoi(part.SubStr(5).Data());
                mDrag = "stop:" + owner;
            }
            else
                PressBar((cursor.position.x - mDragTrack.left)/Math::Max(1.0f, mDragTrack.Width()), owner);
            return;
        }

        // An effect's grip: it is lifted and follows the cursor over the others
        if (part == "grip" && i >= 0 && i < mPlates.Count())
        {
            mFxDrag = FxDrag();
            mFxDrag.from = i;
            mFxDrag.startY = cursor.position.y;
            UpdateFxSlot(cursor.position.y);
        }
    }

    void PipelineTextFxPanel::OnCursorStillDown(const Input::Cursor& cursor)
    {
        mCursor = cursor.position;
        float share = (cursor.position.x - mDragTrack.left)/Math::Max(1.0f, mDragTrack.Width());
        if (mDrag.StartsWith("slider:"))
            SetSlider(mDrag.SubStr(7), share, false);
        else if (mDrag.StartsWith("stop:"))
            MoveStop(share, false);
        else if (mFxDrag.from >= 0)
        {
            mFxDrag.dy = mFxDrag.startY - cursor.position.y;
            UpdateFxSlot(cursor.position.y);
        }
    }

    void PipelineTextFxPanel::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (mFxDrag.from >= 0)
        {
            UpdateFxSlot(cursor.position.y);
            DragEffect(mFxDrag.from, mFxDrag.to);
            mPressedHot = "";
            return;
        }
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
