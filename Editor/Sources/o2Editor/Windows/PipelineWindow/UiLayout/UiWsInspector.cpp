#include "o2Editor/stdafx.h"
#include "UiWsInspector.h"

#include <cmath>

#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Dialogs/ColorPickerDlg.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRotate.h"

namespace Editor
{
    using namespace UiWsDraw;
    using UiDoc::Json;
    using UiDoc::UiNode;
    using UiDoc::UiOp;
    namespace Ws = UiDoc::Ws;

    namespace
    {
        const float pad = 12.0f;
        const float rowH = 28.0f;
        const float rowGap = 6.0f;
        const float labelW = 100.0f;

        std::optional<double> Opt(const std::optional<double>& v) { return v; }

        String ColorText(const std::optional<String>& value, const String& fallback)
        {
            return value && !value->IsEmpty() ? *value : fallback;
        }
    }

    UiWsInspector::UiWsInspector(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {
        mClipChildren = true;
    }

    const UiNode* UiWsInspector::Shown() const
    {
        if (!ctx)
            return nullptr;
        const auto& doc = ctx->Doc();
        return ctx->selected.IsEmpty() ? &doc.root : UiDoc::FindNode(doc, ctx->selected);
    }

    Ref<EditBox> UiWsInspector::GetField(const String& key) const
    {
        Ref<EditBox> field;
        if (mFields.TryGetValue(key, field) && field && field->IsEnabled() && mUsed.Contains(key))
            return field;
        return nullptr;
    }

    Vector<String> UiWsInspector::GetFieldKeys() const
    {
        return mUsed;
    }

    Ref<EditBox> UiWsInspector::PlaceField(const String& key, const RectF& rect, const Function<Ref<EditBox>()>& make)
    {
        PushEditorScopeOnStack scope;
        Ref<EditBox> field;
        if (!mFields.TryGetValue(key, field) || !field)
        {
            field = make();
            field->name = key;
            AddChild(field);
            mFields[key] = field;
        }

        RectF me = layout->GetWorldRect();
        *field->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(rect.left - me.left, rect.bottom - me.top),
                                      Vec2F(rect.right - me.left, rect.top - me.top));
        if (!field->IsEnabled())
            field->SetEnabledForcible(true);
        field->interactable = ctx && ctx->Editable();
        mUsed.Add(key);
        return field;
    }

    void UiWsInspector::Set(const String& key, const Json& value)
    {
        const UiNode* n = Shown();
        if (!n || !ctx->Editable())
            return;

        String label;
        UiOp op = Ws::SetFieldOp(*n, key, value, label);
        ctx->Op({ op }, label);
    }

    void UiWsInspector::Screen(const UiOp& op, const String& label)
    {
        if (ctx && ctx->Editable())
            ctx->Op({ op }, label);
    }

    void UiWsInspector::OnScrolled(float scroll)
    {
        mScroll = Math::Clamp(mScroll - scroll*0.5f, 0.0f, Math::Max(0.0f, mContentHeight - layout->GetHeight()));
    }

    void UiWsInspector::Update(float dt)
    {
        UiWsPanel::Update(dt);
    }

    // --- drawing ---------------------------------------------------------------------------------

    void UiWsInspector::DrawPanel(const Box& box)
    {
        mUsed.Clear();
        const UiNode* n = Shown();
        auto& t = T();
        mChildClip = box.world;
        if (!ctx || !n)
        {
            for (auto& kv : mFields)
                kv.second->SetEnabledForcible(false);
            TextStyle hint(12.0f, 400, t.fg3);
            hint.wrap = true;
            hint.ver = VerAlign::Top;
            Text("Select a block on the screen or in the hierarchy", box.R(20, 28, box.W() - 40, 40), hint);
            return;
        }

        const auto& doc = ctx->Doc();
        bool isRoot = n == &doc.root;
        const UiNode* parent = isRoot ? nullptr : UiDoc::FindParent(doc, n->id);
        bool inLayout = parent && parent->layout;
        bool editable = ctx->Editable();
        float W = box.W();
        float ctrlX = pad + labelW + 8, ctrlW = W - pad - ctrlX;
        bool narrow = W < 390.0f;
        bool newNode = mFieldsFor != n->id;
        mFieldsFor = n->id;
        WeakRef<UiWsInspector> weak(this);

        mContentHeight = Math::Max(mContentHeight, 0.0f);
        mScroll = Math::Clamp(mScroll, 0.0f, Math::Max(0.0f, mContentHeight - box.H()));
        o2Render.EnableScissorTest((RectI)box.world);
        float y = pad - mScroll;
        bool firstRow = true;

        auto section = [&](const String& title)
        {
            y += 16;
            Text(title.ToUpperCase(), box.R(pad, y, W - pad*2, 14), TextStyle(10.0f, 600, t.fg3));
            y += 14 + 8;
            firstRow = true;
        };
        // A row: its label and the control's rectangle
        auto row = [&](const String& label, float height = rowH) -> RectF
        {
            if (!firstRow)
                y += rowGap;
            firstRow = false;
            Text(label, box.R(pad, y, labelW, rowH), TextStyle(12.0f, 400, t.fgDim));
            RectF r = box.R(ctrlX, y, ctrlW, height);
            y += height;
            return r;
        };
        // Number fields sharing the control column in equal parts (a lone field takes the first of two)
        struct Num { String key, letter, placeholder; std::optional<double> value, min; Function<void(std::optional<double>)> on; };
        auto nums = [&](const RectF& r, const Vector<Num>& items, int extra = 0) -> RectF
        {
            int cols = Math::Max(2, items.Count() + extra);
            float cw = (r.Width() - (cols - 1)*6.0f)/cols;
            for (int i = 0; i < items.Count(); i++)
            {
                const Num& it = items[i];
                RectF fr(r.left + i*(cw + 6), r.top, r.left + i*(cw + 6) + cw, r.bottom);
                auto on = it.on;
                auto field = PlaceField("num:" + it.key, fr, [&]()
                {
                    return UiWsControls::NumberField(it.letter, it.value, it.placeholder, [on](std::optional<double> v) { if (on) on(v); }, it.min);
                });
                if (newNode || !field->IsFocused())
                    UiWsControls::SetNumber(field, it.value);
            }
            float ex = r.left + items.Count()*(cw + 6);
            return RectF(ex, r.top, r.right, r.bottom);
        };
        auto setter = [weak](const String& key)
        {
            return [weak, key](std::optional<double> v) { if (auto self = weak.Lock()) self->Set(key, v ? Json(*v) : Json()); };
        };
        auto segments = [&](const RectF& r, const String& key, const Vector<Pair<String, String>>& options, const String& value)
        {
            Segments(r, options, value, "seg:" + key + ":", editable);
        };
        auto select = [&](const RectF& r, const String& key, const String& label)
        {
            bool hover = IsHover("sel:" + key);
            Fill(r, 8.0f, t.field);
            Border(r, 8.0f, hover && editable ? t.accent : t.line);
            Text(label, RectF(r.left + 8, r.top, r.right - 24, r.bottom), TextStyle(12.0f, 400, A(t.fg, editable ? 1.0f : 0.5f)));
            UiWsDraw::Icon("uiws_chevron_down", Vec2F(r.right - 12, r.Center().y), 11.0f, t.fg3);
            if (editable)
                AddHot(r, "sel:" + key);
        };
        auto textField = [&](const RectF& r, const String& key, const String& value, const String& placeholder, bool multiline)
        {
            String fkey = (multiline ? "area:" : "text:") + key;
            auto field = PlaceField(fkey, r, [&]()
            {
                return UiWsControls::TextField(value, placeholder, multiline, [weak, key](const String& s)
                {
                    if (auto self = weak.Lock())
                        self->Set(key, s.IsEmpty() ? Json() : Json(s));
                });
            });
            if ((newNode || !field->IsFocused()) && (String)field->GetText() != value)
                UiWsControls::SetTextQuiet(field, value);
        };
        auto colorField = [&](const RectF& r, const String& key, const std::optional<String>& value, const String& fallback)
        {
            String hex = ColorText(value, fallback);
            Color4 c;
            PipelineUtils::ParseHexColor(hex, c);
            float clearW = value ? 28 + 4 : 0;
            RectF swatch(r.left, r.top, r.left + 28, r.bottom);
            Checker(swatch, 8.0f, 5.0f, t.checkerA, t.checkerB);
            Fill(swatch, 8.0f, Color4(c.r, c.g, c.b, 255));
            Border(swatch, 8.0f, t.line);
            if (editable)
                AddHot(swatch, "swatch:" + key);
            auto field = PlaceField("hex:" + key, RectF(r.left + 32, r.top, r.right - clearW, r.bottom), [&]()
            {
                auto f = UiWsControls::TextField(hex, "", false, {}, [weak, key](const String& s)
                {
                    auto self = weak.Lock();
                    if (!self)
                        return;
                    Color4 parsed;
                    String text = s.Trimed(" ");
                    if (!text.StartsWith("#"))
                        text = "#" + text;
                    if (PipelineUtils::ParseHexColor(text, parsed))
                        self->Set(key, Json(PipelineUtils::ColorToHex(Color4(parsed.r, parsed.g, parsed.b, 255)).ToUpperCase()));
                });
                return f;
            });
            if ((newNode || !field->IsFocused()) && (String)field->GetText() != hex)
                UiWsControls::SetTextQuiet(field, hex);
            if (value)
                Tool(RectF(r.right - 28, r.top, r.right, r.bottom), "uiws_close", "clear:" + key, false, editable, 12.0f);
        };
        auto kitPicker = [&](const RectF& r, const String& key, const std::optional<String>& value, const UiDoc::UiKitElement* autoEl)
        {
            bool hover = IsHover("kitpick:" + key);
            Fill(r, 8.0f, t.field);
            Border(r, 8.0f, hover && editable ? t.accent : t.line);
            const UiDoc::UiKitElement* cur = value ? UiDoc::KitElement(ctx->hasKit ? &ctx->kit : nullptr, *value) : nullptr;
            RectF thumb(r.left + 4, r.Center().y + 10, r.left + 24, r.Center().y - 10);
            if (cur || autoEl)
                KitThumb(cur ? *cur : *autoEl, thumb, 3.0f);
            else
                DashedFrame(thumb, t.fg3, 2.0f, 2.0f);
            String label = cur ? cur->name : value ? *value : autoEl ? "auto \xC2\xB7 " + autoEl->name : String("no graphic");
            Color4 color = !cur && value ? t.yellow : t.fg;
            Text(label, RectF(r.left + 31, r.top, r.right - 22, r.bottom), TextStyle(11.5f, 400, color));
            UiWsDraw::Icon("uiws_chevron_down", Vec2F(r.right - 13, r.Center().y), 12.0f, t.fg3);
            if (editable)
                AddHot(r, "kitpick:" + key);
        };

        // --- the head: the icon, the name, the type ---
        Fill(box.R(pad, y, 28, 28), 7.0f, t.accentSoft);
        UiWsDraw::Icon(TypeIcon(Ws::TypeWord(*n)), box.P(pad + 14, y + 14), 18.0f, t.accent);
        float nameRight = isRoot ? W - pad : W - pad - 116 - 6;
        {
            String nameValue = isRoot ? doc.name : n->id;
            auto field = PlaceField("name", box.R(pad + 34, y, nameRight - pad - 34, 28), [&]()
            {
                auto f = UiWsControls::TextField(nameValue, "", false, [weak](const String& s)
                {
                    auto self = weak.Lock();
                    if (!self || !self->ctx || !self->ctx->selected.IsEmpty())
                        return;
                    // The screen's name follows every key, as the web's field does
                    self->Screen(UiDoc::MakeScreenOp(s, std::nullopt, std::nullopt, std::nullopt), "screen name");
                }, [weak](const String& s)
                {
                    auto self = weak.Lock();
                    if (!self || !self->ctx || self->ctx->selected.IsEmpty())
                        return;
                    String id = self->ctx->selected;
                    String v = UiDoc::CleanId(s);
                    if (v.IsEmpty() || v == id || UiDoc::FindNode(self->ctx->Doc(), v))
                    {
                        if (auto field = self->GetField("name"))
                            UiWsControls::SetTextQuiet(field, id);
                        return;
                    }
                    Json set = Json::Object();
                    set["id"] = v;
                    self->ctx->Op({ UiDoc::MakeUpdateOp(id, set) }, id + " \xE2\x86\x92 " + v);
                    self->ctx->Select(v);
                });
                if (auto text = f->GetTextDrawable())
                    text->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::Font(600)));
                return f;
            });
            if ((newNode || !field->IsFocused()) && (String)field->GetText() != nameValue)
                UiWsControls::SetTextQuiet(field, nameValue);
        }
        if (!isRoot)
            select(box.R(W - pad - 116, y, 116, 28), "type", Ws::TypeLabel(Ws::TypeWord(*n)));
        y += 28 + 12;
        Line(box.P(pad, y - 0.5f), box.P(W - pad, y - 0.5f), t.line);

        if (isRoot)
        {
            section("Screen");
            DrawRootScreen(box, y);
            firstRow = false;
            RectF r;
            r = row("Backdrop");
            kitPicker(r, "kit", n->kit, nullptr);
            r = row("Colour");
            colorField(r, "color", n->color, "#20232B");
        }
        else
        {
            section(inLayout ? "Placed by the parent's " + Ws::TypeLabel(*parent->layout) : String("Placement"));
            if (!inLayout)
            {
                // The anchor presets: columns left / centre / right / ↔, rows top / middle / bottom / ↕
                RectF r = row("Anchor", narrow ? 166.0f : 200.0f);
                float cell = narrow ? 32.0f : 40.0f, gap = narrow ? 2.0f : 3.0f, head = narrow ? 26.0f : 30.0f;
                float gw = 4 + head + 4*(cell + gap) + 8 - gap + 3, gh = 6 + 14 + 4*(cell + gap) + 8 - gap + 3;
                RectF grid(r.left, r.top, r.left + gw, r.top - gh);
                Fill(grid, 9.0f, t.field);
                Border(grid, 9.0f, t.line);
                auto a = UiDoc::ParseAnchor(n->anchor);
                const char* hs[] = { "left", "center", "right", "stretch" };
                const char* vs[] = { "top", "middle", "bottom", "stretch" };
                for (int c = 0; c < 4; c++)
                {
                    float cx = grid.left + 4 + head + gap + c*(cell + gap);
                    TextStyle head(c == 3 ? 14.0f : 9.5f, 400, c == 3 ? t.fgDim : t.fg3, HorAlign::Middle, c == 3);
                    head.dots = false;
                    Text(Ws::AnchorColumnName(hs[c]), RectF(cx - 6, grid.top - 6, cx + cell + 6, grid.top - 20), head);
                }
                for (int rr = 0; rr < 4; rr++)
                {
                    float cy = grid.top - 6 - 14 - gap - rr*(cell + gap);
                    TextStyle rowHead(rr == 3 ? 14.0f : 9.5f, 400, rr == 3 ? t.fgDim : t.fg3, HorAlign::Right, rr == 3);
                    rowHead.dots = false;
                    Text(Ws::AnchorRowName(vs[rr]), RectF(grid.left - 30, cy, grid.left + 4 + head - 2, cy - cell), rowHead);
                    for (int c = 0; c < 4; c++)
                    {
                        float cx = grid.left + 4 + head + gap + c*(cell + gap);
                        RectF cr(cx, cy, cx + cell, cy - cell);
                        String name = UiDoc::AnchorName(UiDoc::Anchor{ hs[c], vs[rr] });
                        bool on = a.h == hs[c] && a.v == vs[rr];
                        String id = "anchor:" + name;
                        if (on)
                        {
                            Fill(cr, 7.0f, t.accentSoft);
                            Border(cr, 7.0f, t.accent);
                        }
                        else if (editable && IsHover(id))
                            Fill(cr, 7.0f, t.active);
                        UiWsParts::AnchorCell(cr, hs[c], vs[rr], on, editable && IsHover(id));
                        if (editable)
                            AddHot(cr, id);
                    }
                }
                // The anchor in words, its name and the hint
                float capY = gh + 8;
                float wordsW = TextWidth(Ws::AnchorWords(a), 12.0f, 600);
                Text(Ws::AnchorWords(a), RectF(r.left, r.top - capY, r.left + wordsW + 2, r.top - capY - 16), TextStyle(12.0f, 600, t.fg));
                Text(UiDoc::AnchorName(a), RectF(r.left + wordsW + 10, r.top - capY, r.right, r.top - capY - 16),
                     TextStyle(10.5f, 400, t.fg3, HorAlign::Left, true));
                TextStyle hint(11.0f, 400, t.fg3);
                hint.wrap = true;
                hint.ver = VerAlign::Top;
                hint.lineHeight = 14.9f;
                String hintText = "The block stays where it is on the screen. Alt+click: move it to the anchor too.";
                float hintH = TextHeight(hintText, r.Width(), hint);
                Text(hintText, RectF(r.left, r.top - capY - 20, r.right, r.top - capY - 20 - hintH - 2), hint);
                y += gh + 8 + 20 + hintH - r.Height();
            }
            RectF r = row(inLayout ? "Nudge" : "Offset");
            nums(r, { { "x", "X", "0", Opt(n->x), std::nullopt, setter("x") }, { "y", "Y", "0", Opt(n->y), std::nullopt, setter("y") } });
            r = row("Size");
            nums(r, { { "w", "W", "auto", Opt(n->w), std::nullopt, setter("w") }, { "h", "H", "auto", Opt(n->h), std::nullopt, setter("h") } });
            auto anchor = UiDoc::ParseAnchor(n->anchor);
            if (!inLayout && (anchor.h == "stretch" || anchor.v == "stretch"))
            {
                r = row("Margins");
                UiDoc::Box4 m = n->margin ? *n->margin : UiDoc::Box4{ 0, 0, 0, 0 };
                Vector<Num> items;
                const char* letters[] = { "T", "R", "B", "L" };
                for (int i = 0; i < 4; i++)
                {
                    items.Add(Num{ "margin:" + (String)i, letters[i], "0", m[i] ? std::optional<double>(m[i]) : std::nullopt, std::nullopt,
                                   [weak, i](std::optional<double> v)
                                   {
                                       auto s = weak.Lock();
                                       const UiNode* node = s ? s->Shown() : nullptr;
                                       if (!node) return;
                                       String label;
                                       UiOp op = Ws::SetBoxOp(*node, "margin", i, v, label);
                                       s->ctx->Op({ op }, label);
                                   } });
                }
                nums(r, items);
            }
            if (inLayout && *parent->layout != "grid")
            {
                r = row("Grow");
                nums(r, { { "grow", "", "0", Opt(n->grow), std::nullopt, setter("grow") } });
            }
            // Turned about its centre, degrees clockwise; the blocks inside turn with it, the layout does not change
            r = row("Rotation");
            RectF turns = nums(r, { { "rotation", "\xC2\xB0", "0", Opt(n->rotation), std::nullopt, [weak](std::optional<double> v)
            {
                auto turn = v ? UiDoc::CleanTurn(*v) : std::nullopt;
                if (auto self = weak.Lock())
                    self->Set("rotation", turn ? Json(*turn) : Json());
            } } }, 1);
            if (editable)
            {
                Tool(RectF(turns.left, r.top, turns.left + 28, r.bottom), "uiws_undo", "turn:left", false, true, 14.0f);
                Tool(RectF(turns.left + 30, r.top, turns.left + 58, r.bottom), "uiws_redo", "turn:right", false, true, 14.0f);
            }
            if (n->type != "group" && n->type != "text")
            {
                section("Look");
                r = row("Graphic");
                auto skin = UiDoc::SkinOf(*n, ctx->hasKit ? &ctx->kit : nullptr);
                kitPicker(r, "kit", n->kit, n->kit ? nullptr : skin.el);
                r = row("Variant");
                textField(r, "variant", n->variant ? *n->variant : String(), "primary, secondary\xE2\x80\xA6", false);
                r = row("Tint");
                colorField(r, "color", n->color, "#FFFFFF");
                if (n->type == "image" || n->type == "icon" || n->type == "background")
                {
                    r = row("Fit");
                    String fit = n->fit ? *n->fit : String(n->type == "background" ? "cover" : "contain");
                    segments(r, "fit", { { "contain", "Contain" }, { "cover", "Cover" }, { "stretch", "Stretch" } }, fit);
                }
            }
            r = row("Opacity");
            float opacity = n->opacity ? (float)*n->opacity : 1.0f;
            UiWsParts::SliderBar(r, opacity, (String)Math::RoundToInt(opacity*100) + "%", mSliderKey == "opacity" || IsHover("slider:opacity"));
            if (editable)
                AddHot(RectF(r.left, r.top, r.right - 48, r.bottom), "slider:opacity");
            r = row("Visible");
            UiWsParts::Toggle(r, n->IsShown(), IsHover("toggle:visible"));
            if (editable)
                AddHot(RectF(r.left, r.top, r.left + 32, r.bottom), "toggle:visible");
        }

        // --- the children's layout ---
        section("Children");
        RectF r = row("Layout");
        segments(r, "layout", { { "none", "\xE2\x80\x94" }, { "row", "Row" }, { "column", "Column" }, { "grid", "Grid" } }, n->layout ? *n->layout : String("none"));
        if (n->layout)
        {
            bool grid = *n->layout == "grid";
            r = row("Gap");
            if (grid)
                nums(r, { { "gap", "X", "0", Opt(n->gap), std::nullopt, setter("gap") }, { "gapY", "Y", "0", Opt(n->gapY), std::nullopt, setter("gapY") } });
            else
                nums(r, { { "gap", "", "0", Opt(n->gap), std::nullopt, setter("gap") } });
            r = row("Padding");
            UiDoc::Box4 p = n->pad ? *n->pad : UiDoc::Box4{ 0, 0, 0, 0 };
            Vector<Num> items;
            const char* letters[] = { "T", "R", "B", "L" };
            for (int i = 0; i < 4; i++)
            {
                items.Add(Num{ "pad:" + (String)i, letters[i], "0", p[i] ? std::optional<double>(p[i]) : std::nullopt, std::nullopt,
                               [weak, i](std::optional<double> v)
                               {
                                   auto s = weak.Lock();
                                   const UiNode* node = s ? s->Shown() : nullptr;
                                   if (!node) return;
                                   String label;
                                   UiOp op = Ws::SetBoxOp(*node, "pad", i, v, label);
                                   s->ctx->Op({ op }, label);
                               } });
            }
            nums(r, items);
            r = row("Align");
            segments(r, "align", { { "start", "Start" }, { "center", "Center" }, { "end", "End" }, { "stretch", "Stretch" } },
                     n->align ? *n->align : String(grid ? "start" : "stretch"));
            r = row("Justify");
            select(r, "justify", Ws::JustifyLabel(n->justify ? *n->justify : String("start")));
            if (grid)
            {
                r = row("Columns");
                nums(r, { { "cols", "#", "3", Opt(n->cols), std::nullopt, setter("cols") }, { "cellW", "W", "auto", Opt(n->cellW), std::nullopt, setter("cellW") },
                          { "cellH", "H", "auto", Opt(n->cellH), std::nullopt, setter("cellH") } });
            }
        }

        // --- the content ---
        static const Vector<String> labelled = { "button", "tab", "badge", "text", "input", "progress" };
        if (!isRoot && labelled.Contains(n->type))
        {
            section("Content");
            r = row(n->type == "input" ? "Value" : "Text", 56.0f);
            textField(r, "text", n->text ? *n->text : String(), "", true);
            if (n->type == "input")
            {
                r = row("Placeholder");
                textField(r, "placeholder", n->placeholder ? *n->placeholder : String(), "", false);
            }
            r = row("Text style");
            UiWsStylePicker::DrawButton(r, *n, ctx->hasKit ? &ctx->kit : nullptr, IsHover("stylepick"), editable);
            if (editable)
                AddHot(r, "stylepick");
            r = row("Text align");
            segments(r, "textAlign", { { "auto", "auto" }, { "left", "Start" }, { "center", "Center" }, { "right", "End" } },
                     n->textAlign ? *n->textAlign : String("auto"));
            r = row("Font size");
            RectF extra = nums(r, { { "fontSize", "", "auto", Opt(n->fontSize), std::nullopt, setter("fontSize") } }, 1);
            colorField(extra, "textColor", n->textColor, "#FFFFFF");
        }
        if (!isRoot && (n->type == "progress" || n->type == "slider"))
        {
            r = row("Value");
            float value = n->value ? (float)*n->value : (n->type == "progress" ? 0.6f : 0.5f);
            UiWsParts::SliderBar(r, value, (String)Math::RoundToInt(value*100) + "%", mSliderKey == "value" || IsHover("slider:value"));
            if (editable)
                AddHot(RectF(r.left, r.top, r.right - 48, r.bottom), "slider:value");
        }
        if (!isRoot && (n->type == "toggle" || n->type == "tab"))
        {
            r = row("On");
            UiWsParts::Toggle(r, n->on && *n->on, IsHover("toggle:on"));
            if (editable)
                AddHot(RectF(r.left, r.top, r.left + 32, r.bottom), "toggle:on");
        }

        section("Note");
        textField(box.R(pad, y, W - pad*2, 56), "note", n->note ? *n->note : String(), "what it is for (the agent reads it)", true);
        y += 56 + 16;

        mContentHeight = y + mScroll;
        o2Render.DisableScissorTest();

        for (auto& kv : mFields)
        {
            if (!mUsed.Contains(kv.first) && kv.second->IsEnabled())
                kv.second->SetEnabledForcible(false);
        }
    }

    // --- clicks ------------------------------------------------------------------------------------

    void UiWsInspector::OnCursorPressed(const Input::Cursor& cursor)
    {
        UiWsPanel::OnCursorPressed(cursor);
        mAltClick = o2Input.IsKeyDown(VK_MENU);
        if (mPressedHot.StartsWith("slider:"))
        {
            mSliderKey = mPressedHot.SubStr(7);
            mSliderTrack = HotRect(mPressedHot);
            SlideTo(cursor.position.x);
        }
    }

    void UiWsInspector::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (!mSliderKey.IsEmpty())
            SlideTo(cursor.position.x);
    }

    void UiWsInspector::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (!mSliderKey.IsEmpty())
        {
            mSliderKey = "";
            mPressedHot = "";
            return;
        }
        UiWsPanel::OnCursorReleased(cursor);
    }

    void UiWsInspector::SlideTo(float x)
    {
        float f = Math::Clamp01((x - mSliderTrack.left)/Math::Max(1.0f, mSliderTrack.Width()));
        SetSlider(mSliderKey, f);
    }

    void UiWsInspector::SetSlider(const String& key, float value)
    {
        const UiNode* n = Shown();
        if (!n)
            return;

        if (key == "opacity")
        {
            double v = std::round(value/0.05)*0.05;
            v = std::round(v*100.0)/100.0;
            double current = n->opacity ? *n->opacity : 1.0;
            if (v != current)
                Set("opacity", v >= 1 ? Json() : Json(v));
        }
        else if (key == "value")
        {
            double v = std::round(value*100.0)/100.0;
            if (!n->value || *n->value != v)
                Set("value", Json(v));
        }
    }

    void UiWsInspector::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (!ctx)
            return;

        const UiNode* n = Shown();
        if (!n)
            return;

        if (id.StartsWith("sel:"))
        {
            String key = id.SubStr(4);
            RectF anchor = HotRect(id);
            if (key == "type")
            {
                Vector<Pair<String, String>> options;
                for (auto& type : UiDoc::UiNodeTypes())
                {
                    if (type != "screen")
                        options.Add(Pair<String, String>(type, Ws::TypeLabel(type)));
                }
                for (auto word : { "row", "column", "grid" })
                    options.Add(Pair<String, String>(word, Ws::TypeLabel(word)));
                OpenOptions(key, anchor, options, Ws::TypeWord(*n));
            }
            else if (key == "justify")
            {
                Vector<Pair<String, String>> options;
                for (auto v : { "start", "center", "end", "between", "around", "evenly" })
                    options.Add(Pair<String, String>(v, Ws::JustifyLabel(v)));
                OpenOptions(key, anchor, options, n->justify ? *n->justify : String("start"));
            }
            else if (key == "style")
            {
                Vector<Pair<String, String>> options = { Pair<String, String>("", "auto") };
                if (ctx->hasKit)
                {
                    for (auto& s : ctx->kit.textStyles)
                        options.Add(Pair<String, String>(s.name, s.name + " \xC2\xB7 " + s.role));
                }
                OpenOptions(key, anchor, options, n->style ? *n->style : String());
            }
            return;
        }

        if (id.StartsWith("kitpick:"))
        {
            OpenKitGrid(HotRect(id), n->kit ? *n->kit : String());
            return;
        }

        if (id == "stylepick")
        {
            OpenStyles(HotRect(id));
            return;
        }

        if (OnRootClick(id))
            return;

        if (id.StartsWith("swatch:"))
        {
            String key = id.SubStr(7);
            std::optional<String> value = key == "color" ? n->color : n->textColor;
            Color4 color;
            PipelineUtils::ParseHexColor(ColorText(value, key == "color" && n == &ctx->Doc().root ? "#20232B" : "#FFFFFF"), color);
            WeakRef<UiWsInspector> weak(this);
            ColorPickerDlg::Show(color, [weak, key](const Color4& c, bool byUser)
            {
                if (auto self = weak.Lock())
                    self->Set(key, Json(PipelineUtils::ColorToHex(Color4(c.r, c.g, c.b, 255)).ToUpperCase()));
            }, []() {});
            return;
        }

        Choose(id, mAltClick);
    }

    void UiWsInspector::Choose(const String& id, bool alt /*= false*/)
    {
        const UiNode* n = Shown();
        if (!ctx || !n || !ctx->Editable())
            return;

        auto rest = [&](int prefix) { return id.SubStr(prefix); };
        if (id.StartsWith("seg:") || id.StartsWith("sel:"))
        {
            String tail = rest(4);
            int colon = tail.Find(":");
            String key = tail.SubStr(0, colon), value = tail.SubStr(colon + 1);
            if (key == "layout")
                Set("layout", value == "none" ? Json() : Json(value));
            else if (key == "textAlign")
                Set("textAlign", value == "auto" ? Json() : Json(value));
            else
                Set(key, value.IsEmpty() ? Json() : Json(value));
        }
        else if (id.StartsWith("kit:"))
        {
            String tail = rest(4);
            int colon = tail.Find(":");
            Set(tail.SubStr(0, colon), tail.SubStr(colon + 1).IsEmpty() ? Json() : Json(tail.SubStr(colon + 1)));
        }
        else if (id.StartsWith("anchor:"))
        {
            UiOp op;
            String label;
            if (Ws::AnchorOp(ctx->Doc(), ctx->rects, n->id, rest(7), alt, op, label))
                ctx->Op({ op }, label);
        }
        else if (id == "turn:left" || id == "turn:right")
        {
            // A quarter turn counter-clockwise / clockwise
            auto turn = UiDoc::CleanTurn((n->rotation ? *n->rotation : 0.0) + (id == "turn:left" ? -90.0 : 90.0));
            Set("rotation", turn ? Json(*turn) : Json());
        }
        else if (id == "toggle:visible")
            Set("visible", n->IsShown() ? Json(false) : Json());
        else if (id == "toggle:on")
            Set("on", n->on && *n->on ? Json() : Json(true));
        else if (id.StartsWith("clear:"))
            Set(rest(6), Json());
        else if (OnRootClick(id))
            return;
    }

    void UiWsInspector::OpenOptions(const String& key, const RectF& anchor, const Vector<Pair<String, String>>& options, const String& current)
    {
        PushEditorScopeOnStack scope;
        if (!mPopup)
        {
            mPopup = mmake<UiWsPopup>();
            mPopup->name = "inspector popup";
            AddChild(mPopup);
            mOptions = mmake<UiWsOptionsPanel>();
            mOptions->name = "options";
            mKitGrid = mmake<UiWsKitGridPanel>();
            mKitGrid->name = "kit grid";
            mStyles = mmake<UiWsStyleListPanel>();
            mStyles->name = "style list";
            mPopup->SetContent(mOptions);
            mPopup->SetContent(mKitGrid);
            mPopup->SetContent(mStyles);
        }
        if (mPopup->IsOpen())
        {
            mPopup->Close();
            if (mPopupFor == key)
                return;
        }

        mPopupFor = key;
        mOptions->SetEnabledForcible(true);
        mKitGrid->SetEnabledForcible(false);
        mStyles->SetEnabledForcible(false);
        mOptions->options = options;
        mOptions->current = current;
        WeakRef<UiWsInspector> weak(this);
        mOptions->onPick = [weak, key](const String& value)
        {
            auto self = weak.Lock();
            if (!self)
                return;
            self->mPopup->Close();
            self->Choose("sel:" + key + ":" + value);
        };
        mPopup->OpenAt(anchor, Math::Max(anchor.Width(), 180.0f), Math::Min(320.0f, mOptions->ContentHeight()));
    }

    void UiWsInspector::OpenKitGrid(const RectF& anchor, const String& current)
    {
        OpenOptions("kit", anchor, {}, "");
        if (!mPopup->IsOpen())
            return;

        mOptions->SetEnabledForcible(false);
        mKitGrid->SetEnabledForcible(true);
        mKitGrid->ctx = ctx;
        mKitGrid->current = current;
        WeakRef<UiWsInspector> weak(this);
        mKitGrid->onPick = [weak](const String& name)
        {
            auto self = weak.Lock();
            if (!self)
                return;
            self->mPopup->Close();
            self->Choose("kit:kit:" + name);
        };
        mPopup->OpenAt(anchor, 312.0f, Math::Min(340.0f, mKitGrid->ContentHeight()), true);
    }

    void UiWsInspector::OpenStyles(const RectF& anchor)
    {
        const UiNode* n = Shown();
        OpenOptions("style", anchor, {}, "");
        if (!n || !mPopup->IsOpen())
            return;

        mOptions->SetEnabledForcible(false);
        mStyles->SetEnabledForcible(true);
        mStyles->ctx = ctx;
        mStyles->blockId = n->id;
        WeakRef<UiWsInspector> weak(this);
        mStyles->onPick = [weak](const String& name)
        {
            auto self = weak.Lock();
            if (!self)
                return;
            self->mPopup->Close();
            self->Set("style", name.IsEmpty() ? Json() : Json(name));
        };
        mPopup->OpenAt(anchor, 320.0f, Math::Min(420.0f, mStyles->ContentHeight()), true);
    }
}
