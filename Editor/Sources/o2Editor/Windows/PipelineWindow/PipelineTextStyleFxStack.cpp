#include "o2Editor/stdafx.h"
#include "PipelineTextStyleFx.h"

#include <map>

#include "o2/Render/Resources/Texture.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTextStyleFxParts.h"

// The effects block of the Text style card (AssetsLine TextStyleFx.tsx EffectsBlock): a frame of its own; on its head the
// caption, the count and a palette of six buttons - a letter drawn with each kind of effect - that adds one where the
// canonical stack has it; then a plate per effect, its first one on top: a grip that drags it to another place in the
// stack (a line shows where it lands, one undo step), an eye that hides it, its name and controls, duplicate and remove; an
// outline may be a gradient - its angle in its row, its stops on a bar of their own under it
namespace Editor
{
    using namespace UiWsDraw;
    using namespace TextFx;
    using UiDoc::TextEffect;
    using Style = UiDoc::TextStyle;

    namespace
    {
        const float nameW = 50.0f, gripW = 8.0f, subIndent = 36.0f, letterW = 22.0f, letterH = 20.0f, letterGap = 3.0f;

        // ---------------------------------------------
        // A letter of the palette, drawn once per kind
        // ---------------------------------------------
        struct Letter
        {
            TextureRef texture;
            Vec2I      size;
        };

        // The look of each kind's letter (styles.css .fxi-*): a capital A in Russo One on the button's plate, 100 px tall
        UiDoc::Json LetterStyle(const String& type)
        {
            String json =
                type == "stroke" ? R"({"color":"#FFD23A","effects":[{"type":"stroke","color":"#5A2100","width":6}]})" :
                type == "extrude" ? R"({"color":"#FFD23A","effects":[{"type":"extrude","color":"#8A3400","y":14}]})" :
                type == "shadow" ? R"({"color":"#FFFFFF","effects":[{"type":"shadow","color":"#000000","x":9,"y":9,"blur":8,"opacity":0.95}]})" :
                type == "glow" ? R"({"color":"#FFFFFF","effects":[{"type":"glow","color":"#19D3FF","size":22,"opacity":1}]})" :
                type == "innerShadow" ? R"({"color":"#C9CDD6","effects":[{"type":"innerShadow","color":"#1C2029","y":14,"blur":6,"opacity":0.95}]})" :
                R"({"color":"#9AA2B1","effects":[{"type":"bevel","highlight":"#FFFFFF","shade":"#2E333D","depth":7,"soft":3,"opacity":0.9}]})";
            UiDoc::Json style;
            UiDoc::Json::Parse(json, style);
            style["name"] = "fx";
            style["font"] = "Russo One";
            style["size"] = 72;
            return style;
        }

        const Letter& LetterOf(const String& type)
        {
            static auto* cache = new std::map<std::string, Letter>();
            auto it = cache->find(type.Data());
            if (it != cache->end())
                return it->second;

            UiDoc::UiKit kit;
            kit.name = "k";
            kit.textStyles.Add(UiDoc::NormalizeTextStyle(LetterStyle(type), "fx"));
            UiDoc::UiDoc doc = UiDoc::EmptyDoc("A", 110, 100);
            doc.root = UiDoc::UiNode();
            doc.root.id = "root";
            doc.root.type = "screen";
            doc.root.Set("color", UiDoc::Json(String("#2A2E38")));
            UiDoc::UiNode label;
            label.id = "a";
            label.type = "text";
            label.Set("anchor", UiDoc::Json(String("stretch")));
            label.Set("style", UiDoc::Json(String("fx")));
            label.Set("text", UiDoc::Json(String("A")));
            doc.root.children.push_back(label);

            UiDoc::UiRenderOptions o;
            o.mode = "kit";
            o.kit = &kit;
            o.pixelLong = 88;
            Letter letter;
            auto image = UiDoc::RenderLayout(doc, o);
            if (!image.IsEmpty())
            {
                auto bitmap = image.ToBitmap();
                letter.texture = TextureRef(*bitmap);
                letter.texture->SetFilter(Texture::Filter::Linear);
                letter.size = bitmap->GetSize();
            }
            return (*cache)[type.Data()] = letter;
        }
    }

    void PipelineTextFxPanel::DrawEffects(const Box& box, const Style& st)
    {
        // .uts-fxbox: the frame, its padding and border (boxExtra of the card's height)
        auto& t = T();
        Fill(box.world, 8.0f, A(t.field, 0.45f));
        Border(box.world, 8.0f, t.line);
        float x = 6.0f, w = box.W() - 12.0f, y = 5.0f;
        DrawEffectsHead(box, st, x, y, w);
        y += rowH + rowGap;

        Vector<float> tops;
        mPlates.Clear();
        mFxBars.Clear();
        for (auto& e : st.effects)
        {
            int rows = EffectRows(e);
            float h = rows*rowH + (rows - 1)*rowGap;
            tops.Add(y);
            mPlates.Add(box.R(x - 3.0f, y, w + 6.0f, h));
            y += h + rowGap;
        }

        bool dragging = mFxDrag.from >= 0 && mFxDrag.from < st.effects.Count();
        for (int i = 0; i < st.effects.Count(); i++)
        {
            if (!dragging || i != mFxDrag.from)
                DrawPlate(box, st, i, x, tops[i], w, false);
        }
        if (dragging)
        {
            if (mFxDrag.to != mFxDrag.from)
            {
                RectF line(box.world.left + 6.0f, mFxDrag.lineY + 1.0f, box.world.right - 6.0f, mFxDrag.lineY - 1.0f);
                Shadow(line, 1.0f, A(t.accent, 0.6f), 6.0f);
                Fill(line, 1.0f, t.accent);
            }
            DrawPlate(box, st, mFxDrag.from, x, tops[mFxDrag.from] + mFxDrag.dy, w, true);
        }
    }

    void PipelineTextFxPanel::DrawEffectsHead(const Box& box, const Style& st, float x, float y, float w)
    {
        auto& t = T();
        Text("Effects", box.R(x, y, 70.0f, rowH), TextStyle(11.0f, 600, t.fg));
        int count = st.effects.Count();
        bool full = count >= UiDoc::TEXT_EFFECTS_MAX;
        if (count > 0)
        {
            float after = TextWidth("Effects", 11.0f, 600) + 6.0f;
            Text((String)count + "/" + (String)UiDoc::TEXT_EFFECTS_MAX, box.R(x + after, y, 40.0f, rowH), TextStyle(10.0f, 400, t.fg3));
        }

        // .uts-fxadd: a letter drawn with each kind; the hovered one rises a pixel
        auto& types = UiDoc::TextEffectTypes();
        float left = x + w - (types.Count()*letterW + (types.Count() - 1)*letterGap);
        for (int k = 0; k < types.Count(); k++)
        {
            String id = "add:" + types[k];
            bool hover = !full && IsHover(id);
            RectF b = box.R(left + k*(letterW + letterGap), y + (rowH - letterH)*0.5f - (hover ? 1.0f : 0.0f), letterW, letterH);
            float alpha = full ? 0.4f : 1.0f;
            Fill(b, 5.0f, Color4(0x2A, 0x2E, 0x38, (int)(255*alpha)));
            auto& letter = LetterOf(types[k]);
            if (letter.texture)
                Picture(letter.texture, letter.size, RectF(b.left + 1.0f, b.top - 1.0f, b.right - 1.0f, b.bottom + 1.0f), alpha);
            Border(b, 5.0f, hover ? t.accent : Color4(255, 255, 255, (int)(26*alpha)));
            if (!full)
                AddHot(b, id);
        }
    }

    void PipelineTextFxPanel::DrawPlate(const Box& box, const Style& st, int i, float x, float y, float w, bool lifted)
    {
        auto& t = T();
        auto e = st.effects[i];
        String id = "fx" + (String)i;
        int count = st.effects.Count();
        bool full = count >= UiDoc::TEXT_EFFECTS_MAX, still = count < 2, dim = e.off;
        int rows = EffectRows(e);
        RectF plate = box.R(x - 3.0f, y, w + 6.0f, rows*rowH + (rows - 1)*rowGap);
        bool hover = !lifted && mFxDrag.from < 0 && mHoverInside && Inside(plate, mCursor);
        if (lifted)
        {
            Shadow(plate, 6.0f, Color4(0, 0, 0, 90), 14.0f);
            Fill(plate, 6.0f, t.cardSolid);
            Border(plate, 6.0f, t.accent);
        }
        else if (hover)
            Fill(plate, 6.0f, A(t.fg, 0.05f));

        // The grip: two columns of three dots
        float cx = x;
        RectF grip = box.R(cx, y, gripW, rowH);
        Color4 dot = still ? A(t.fg3, 0.4f) : lifted || IsHover(id + ":grip") ? t.fg : hover ? t.fgDim : t.fg3;
        for (int r = 0; r < 3; r++)
        {
            for (int c = 0; c < 2; c++)
                Fill(box.R(cx + 1.0f + c*4.0f, y + rowH*0.5f - 5.0f + r*4.0f, 2.0f, 2.0f), 1.0f, dot);
        }
        if (!still)
            AddHot(grip, id + ":grip");
        cx += gripW + gap;

        // The eye: shown or hidden (.uts-fx-b.eye)
        RectF eye = box.R(cx, y + (rowH - buttonW)*0.5f, buttonW, buttonW);
        bool eyeHover = IsHover(id + ":eye");
        if (eyeHover)
            Fill(eye, 5.0f, t.field);
        Icon(e.off ? "uiws_eye_off" : "uiws_eye", eye.Center(), 12.0f, eyeHover ? t.fg : e.off ? t.fg3 : t.fgDim);
        AddHot(eye, id + ":eye");
        cx += buttonW + gap;

        Text(FxName(e.type), box.R(cx, y, nameW, rowH), TextStyle(11.0f, 400, A(t.fg, dim ? 0.4f : 1.0f)));
        cx += nameW + gap;

        float right = x + w;
        SmallButton(box.R(right - buttonW*2 - gap, y, buttonW, rowH), "al_cmp_copy", id + ":dup", !full, false, 11.0f);
        SmallButton(box.R(right - buttonW, y, buttonW, rowH), "uiws_close", id + ":remove");
        float controlsRight = right - buttonW*2 - gap*2;

        // The sliders of a row, sharing its width
        auto bars = [&](const Vector<Slider>& sliders, float from, float to, float rowY)
        {
            if (sliders.IsEmpty())
                return;
            float each = (to - from - gap*(sliders.Count() - 1))/sliders.Count();
            for (int s = 0; s < sliders.Count(); s++)
            {
                auto& sl = sliders[s];
                double* v = FieldOf(e, sl.field);
                if (!v)
                    continue;
                float share = (float)((*v - sl.min)/(sl.max - sl.min));
                Bar(box.R(from + s*(each + gap), rowY, each, rowH), id + ":" + sl.field, share, Format(sl, *v), dim);
            }
        };

        auto& spec = SpecOf(e.type);
        float subY = y + rowH + rowGap, subX = x + subIndent;
        if (e.type == "stroke")
        {
            // An outline's paint: a colour, or a gradient (its angle here, its stops on the row below)
            SmallButton(box.R(controlsRight - buttonW, y, buttonW, rowH), "uiws_gradient", id + ":grad", true, e.fill.has_value(), 12.0f);
            float to = controlsRight - buttonW - gap;
            if (e.fill)
                bars({ AngleSlider(), spec.head[0] }, cx, to, y);
            else
            {
                Well(box.R(cx, y, wellW, rowH), e.color, id + ":color", dim);
                bars(spec.head, cx + wellW + gap, to, y);
            }

            if (e.fill)
            {
                auto& stops = e.fill->stops;
                int stop = Math::Clamp(GetSelectedStop(id), 0, stops.Count() - 1);
                float barRight = right - buttonW - gap - wellW - gap - 6.0f;
                RectF bar = box.R(subX + 6.0f, subY + 6.0f, Math::Max(10.0f, barRight - subX - 6.0f), 14.0f);
                if (!lifted)
                    mFxBars[i] = bar;
                DrawStopsBar(bar, *e.fill, stop, id + ":", dim);
                Well(box.R(right - buttonW - gap - wellW, subY, wellW, rowH), stops[stop].color, id + ":stop", dim);
                SmallButton(box.R(right - buttonW, subY, buttonW, rowH), "uiws_close", id + ":stopRemove", stops.Count() > 2);
            }
            return;
        }

        for (auto color : spec.colors)
        {
            Well(box.R(cx, y, wellW, rowH), *ColorOf(e, color), id + ":" + color, dim);
            cx += wellW + gap;
        }
        bars(spec.head, cx, controlsRight, y);
        if (!spec.sub.IsEmpty())
            bars(spec.sub, subX, right, subY);
    }

    // --- edits -----------------------------------------------------------------------------------

    bool PipelineTextFxPanel::EffectClick(Style& st, int i, const String& act)
    {
        auto& e = st.effects[i];
        bool rows = false;
        if (act == "eye")
            e.off = !e.off;
        else if (act == "dup")
        {
            if (st.effects.Count() >= UiDoc::TEXT_EFFECTS_MAX)
                return false;
            st.effects.Insert(TextEffect(e), i + 1);
            rows = true;
        }
        else if (act == "remove")
        {
            st.effects.RemoveAt(i);
            rows = true;
        }
        else if (act == "grad")
        {
            if (e.type != "stroke")
                return false;
            if (e.fill)
            {
                e.color = e.fill->stops[0].color;
                e.fill.reset();
            }
            else
                e.fill = UiDoc::TextGradient{ 90, { UiDoc::GradientStop{ e.color, 0 },
                                                    UiDoc::GradientStop{ Mix(e.color, e.color == "#FFFFFF" ? "#000000" : "#FFFFFF", 0.6), 1 } } };
            rows = true;
        }
        else if (act == "stopRemove")
        {
            int& stop = StopOf("fx" + (String)i);
            if (!e.fill || e.fill->stops.Count() <= 2)
                return false;
            e.fill->stops.RemoveAt(Math::Clamp(stop, 0, e.fill->stops.Count() - 1));
            e.color = e.fill->stops[0].color;
            stop = Math::Max(0, stop - 1);
        }
        else if (act == "color" || act == "highlight" || act == "shade" || act == "stop")
        {
            PickColor("fx" + (String)i + ":" + act);
            return true;
        }
        else
            return false;

        if (rows)
            mFxStops.Clear();
        Look(st, true);
        if (rows && onRows)
            onRows();
        return true;
    }

    void PipelineTextFxPanel::AddEffect(const String& type)
    {
        auto st = getStyle();
        if (st.effects.Count() >= UiDoc::TEXT_EFFECTS_MAX || !UiDoc::TextEffectTypes().Contains(type))
            return;

        st.effects = UiDoc::InsertEffect(st.effects, UiDoc::DefaultEffect(type, st));
        mFxStops.Clear();
        Look(st, true);
        if (onRows)
            onRows();
    }

    void PipelineTextFxPanel::DragEffect(int from, int to)
    {
        mFxDrag = FxDrag();
        auto st = getStyle();
        if (from < 0 || from >= st.effects.Count() || to == from)
            return;

        st.effects = Moved(st.effects, from, to);
        mFxStops.Clear();
        Look(st, true);
    }

    void PipelineTextFxPanel::UpdateFxSlot(float y)
    {
        int from = mFxDrag.from;
        if (from < 0 || from >= mPlates.Count())
            return;

        Vector<RectF> others;
        for (int k = 0; k < mPlates.Count(); k++)
        {
            if (k != from)
                others.Add(mPlates[k]);
        }

        // Below the middle of a plate (world y goes up): the effect lands under it
        int to = 0;
        for (auto& plate : others)
        {
            if (y < plate.Center().y)
                to++;
        }
        mFxDrag.to = to;
        mFxDrag.lineY = to < others.Count() ? others[to].top + 3.0f : !others.IsEmpty() ? others.Last().bottom - 2.0f : mPlates[from].top;
    }
}
