#include "o2Editor/stdafx.h"
#include "UiDevicePicker.h"

#include "o2/Render/Render.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"

namespace Editor
{
    using namespace UiWsDraw;
    using UiDoc::Json;
    namespace Ws = UiDoc::Ws;

    namespace
    {
        const float rowHeight = 30.0f;
        const float groupTitle = 24.0f;
        const float groupGap = 6.0f;
        const float customRow = 36.0f;
        const float menuMinWidth = 340.0f;
        const float menuMaxWidth = 600.0f;
        const float menuMaxHeight = 540.0f;

        const char* const kinds[] = { "phone", "tablet", "desktop" };
        const char* const kindTitles[] = { "PHONES", "TABLETS", "DESKTOPS" };

        const char* const hintText = "Another screen changes only the aspect ratio: the layout stays in its device's pixels and DPI";

        TextStyle Wrapped(float size, const Color4& color)
        {
            TextStyle style(size, 400, color);
            style.wrap = true;
            style.ver = VerAlign::Top;
            style.lineHeight = Math::Round(size*1.35f*10.0f)/10.0f;
            return style;
        }

        // The kit graphics drawn soft on the shown screen
        Vector<String> Soft(const UiWsContext& ctx)
        {
            return Ws::SoftGraphics(ctx.Doc(), ctx.hasKit ? &ctx.kit : nullptr, ctx.preview.frame);
        }

        String SoftText(const Vector<String>& soft)
        {
            String names;
            for (int i = 0; i < soft.Count() && i < 4; i++)
                names += (i ? ", " : "") + soft[i];
            if (soft.Count() > 4)
                names += "\xE2\x80\xA6";
            return "Enlarged on this screen (soft): " + (String)soft.Count() + " kit graphics \xE2\x80\x94 " + names;
        }

        // The foot's lines: the canvas, what the screen adds, the comparison; the soft graphics as a warning
        Vector<String> FootLines(const UiWsContext& ctx)
        {
            const auto& doc = ctx.Doc();
            double dpi = UiDoc::DocDpi(doc);
            Vector<String> lines = { "Layout canvas " + Ws::SizeText(ctx.preview.frame.w, ctx.preview.frame.h) + " at " + UiDoc::JsNum(dpi) +
                " dpi (" + UiDoc::DensityName(dpi) + ")" };
            String added = Ws::AddedText(doc, ctx.preview);
            if (!added.IsEmpty())
                lines.Add("The design is " + Ws::SizeText(doc.width, doc.height) + "; this screen adds " + added);
            if (ctx.preview.comparing)
                lines.Add("While the reference is laid over the layout, it is shown on the reference's screen");
            return lines;
        }

        float FootHeight(const UiWsContext& ctx, float width)
        {
            float h = 8.0f;
            for (auto& line : FootLines(ctx))
                h += TextHeight(line, width - 20.0f, Wrapped(11.0f, Color4::White())) + 4.0f;
            auto soft = Soft(ctx);
            if (!soft.IsEmpty())
                h += TextHeight(SoftText(soft), width - 20.0f - 17.0f, Wrapped(11.0f, Color4::White())) + 4.0f;
            return h - 4.0f + 8.0f;
        }

        float HeadHeight(float width)
        {
            return 8.0f + Math::Max(28.0f, TextHeight(hintText, width - 16.0f - 6.0f - 28.0f - 4.0f, Wrapped(11.0f, Color4::White()))) + 8.0f;
        }

        float ListHeight()
        {
            float h = 12.0f + rowHeight*3.0f;
            for (int g = 0; g < 3; g++)
            {
                int count = UiDoc::UiDevices().Count([&](const UiDoc::UiDevice& d) { return d.kind == kinds[g]; });
                h += groupGap + groupTitle + count*rowHeight;
            }
            h += groupGap + groupTitle + customRow;
            return h;
        }

        UiDoc::UiScreen Turned(const UiDoc::UiScreen& s, bool landscape)
        {
            return landscape ? UiDoc::UiScreen{ s.h, s.w, s.dpi } : s;
        }
    }

    // --- the trigger ------------------------------------------------------------------------------

    UiDevicePicker::UiDevicePicker(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {}

    void UiDevicePicker::Bind(const Ref<PipelineEditor>& editor, const String& nodeId)
    {
        auto context = mmake<UiWsContext>();
        context->editor = editor;
        context->nodeId = nodeId;
        ctx = context;
        mOwnContext = true;
        onCanvas = true;
        ctx->Refresh();
    }

    void UiDevicePicker::Bind(const Ref<UiWsContext>& context)
    {
        ctx = context;
        mOwnContext = false;
    }

    void UiDevicePicker::Refresh()
    {
        if (ctx)
            ctx->Refresh();
    }

    void UiDevicePicker::Update(float dt)
    {
        UiWsPanel::Update(dt);
        mPoll += dt;
        if (mOwnContext && ctx && mPoll > 0.25f)
        {
            mPoll = 0.0f;
            ctx->RefreshIfChanged();
        }
    }

    float UiDevicePicker::GetPreferredWidth() const
    {
        if (!ctx)
            return 120.0f;

        String name = Ws::PreviewName(ctx->preview);
        String meta = Ws::SizeText(ctx->preview.frame.w, ctx->preview.frame.h);
        String added = Ws::AddedText(ctx->Doc(), ctx->preview);
        float nameW = Math::Min(150.0f, TextWidth(name, 11.5f));
        float w = (chip ? 6.0f : 8.0f) + (chip ? 16.0f : 14.0f) + 7.0f + nameW + 1.0f + 7.0f + TextWidth(meta, 10.5f, 400, true) + 4.0f + 7.0f + 11.0f + 8.0f;
        if (!added.IsEmpty())
            w += TextWidth(added, 10.5f) + 7.0f;
        if (!Soft(*ctx).IsEmpty())
            w += 12.0f + 7.0f;
        return Math::Ceil(w);
    }

    void UiDevicePicker::DrawPanel(const Box& box)
    {
        if (!ctx)
            return;

        auto& t = T();
        RectF rect = box.world;
        bool open = IsMenuOpen();
        bool hover = IsHover("trigger");
        if (chip)
        {
            if (open || hover)
                Fill(rect, 8.0f, t.active);
        }
        else
        {
            Fill(rect, 8.0f, t.field);
            Border(rect, 8.0f, open || hover ? t.accent : t.line);
        }

        float x = rect.left + (chip ? 6.0f : 8.0f);
        float cy = rect.Center().y;
        float iconSize = chip ? 16.0f : 14.0f;
        UiWsDraw::Icon(DeviceIcon(Ws::PreviewIconKind(ctx->preview, ctx->target)), Vec2F(x + iconSize*0.5f, cy), iconSize, t.fgDim);
        x += iconSize + 7.0f;

        float right = rect.right - 8.0f;
        UiWsDraw::Icon("uiws_chevron_down", Vec2F(right - 5.5f, cy), 11.0f, t.fg3);
        right -= 11.0f + 7.0f;
        if (!Soft(*ctx).IsEmpty())
        {
            UiWsDraw::Icon("uiws_warn", Vec2F(right - 6.0f, cy), 12.0f, t.yellow);
            right -= 12.0f + 7.0f;
        }

        // The name, the canvas and what the screen adds: the addition gives way first, then the name
        String name = Ws::PreviewName(ctx->preview);
        String meta = Ws::SizeText(ctx->preview.frame.w, ctx->preview.frame.h);
        String added = Ws::AddedText(ctx->Doc(), ctx->preview);
        float metaW = TextWidth(meta, 10.5f, 400, true) + 1.0f;
        float nameW = Math::Min(TextWidth(name, 11.5f) + 1.0f, chip ? 150.0f : Math::Max(30.0f, right - x - metaW - 7.0f));
        Text(name, RectF(x, rect.top, x + nameW, rect.bottom), TextStyle(11.5f, 400, t.fg));
        x += nameW + 7.0f;
        Text(meta, RectF(x, rect.top, Math::Min(x + metaW, Math::Max(x, right)), rect.bottom), TextStyle(10.5f, 400, t.fg3, HorAlign::Left, true));
        x += metaW + 7.0f;
        if (!added.IsEmpty() && right - x > 12.0f)
            Text(added, RectF(x, rect.top, right, rect.bottom), TextStyle(10.5f, 400, t.fg3));
        AddHot(rect, "trigger");
    }

    void UiDevicePicker::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (id == "trigger")
            ToggleMenu();
    }

    void UiDevicePicker::ToggleMenu()
    {
        PushEditorScopeOnStack scope;
        if (!mMenu)
        {
            mMenu = mmake<UiWsPopup>();
            mMenu->name = "screen menu";
            mMenuPanel = mmake<UiDeviceMenuPanel>();
            mMenuPanel->name = "screen menu panel";
            mMenu->SetContent(mMenuPanel);
            AddChild(mMenu);
        }

        if (mMenu->IsOpen() || UiWsPopup::ClosedThisFrame())
        {
            mMenu->Close();
            return;
        }

        mMenuPanel->ctx = ctx;
        mMenuPanel->picker = Ref(this);
        mMenuPanel->SyncFields();
        RectF anchor = layout->GetWorldRect();
        // On a card the picker sits on the editor's canvas: the menu opens in screen space at the field
        if (onCanvas && ctx)
        {
            if (auto editor = ctx->editor.Lock())
            {
                Vec2F a = editor->LocalToScreenPoint(Vec2F(anchor.left, anchor.bottom));
                Vec2F b = editor->LocalToScreenPoint(Vec2F(anchor.right, anchor.top));
                anchor = RectF(Math::Min(a.x, b.x), Math::Max(a.y, b.y), Math::Max(a.x, b.x), Math::Min(a.y, b.y));
            }
        }
        float width = Math::Clamp(anchor.Width(), menuMinWidth, menuMaxWidth);
        mMenu->OpenAt(anchor, width, Math::Min(menuMaxHeight, mMenuPanel->GetContentHeight()));
    }

    bool UiDevicePicker::IsMenuOpen() const
    {
        return mMenu && mMenu->IsOpen();
    }

    Ref<UiDeviceMenuPanel> UiDevicePicker::GetMenuPanel() const
    {
        return mMenuPanel;
    }

    void UiDevicePicker::Pick(const String& device)
    {
        if (ctx)
            ctx->SetUi("uiDevice", Json(device));
    }

    String UiDevicePicker::Chosen() const
    {
        if (!ctx)
            return "kit";

        const String& device = ctx->setting.device;
        if (device == "reference" && !ctx->reference)
            return "kit";
        if (device == "design" || device == "reference" || device == "custom" || UiDoc::DeviceById(device))
            return device;
        return "kit";
    }

    void UiDevicePicker::ToggleLandscape()
    {
        String chosen = Chosen();
        if (!ctx || chosen == "design" || chosen == "reference")
            return;

        ctx->SetUi("uiLandscape", Json(!ctx->setting.landscape));
    }

    void UiDevicePicker::SetCustom(const String& key, double value)
    {
        if (!ctx || (key != "w" && key != "h"))
            return;

        // The custom screen in its order: w, h, dpi
        auto custom = ctx->setting.custom;
        if ((key == "w" ? custom.w : custom.h) == value)
            return;

        Json obj = Json::Object();
        obj["w"] = key == "w" ? value : custom.w;
        obj["h"] = key == "h" ? value : custom.h;
        obj["dpi"] = custom.dpi;
        ctx->SetUi({ Pair<String, Json>("uiDevice", Json("custom")), Pair<String, Json>("uiDeviceCustom", obj) });
    }

    // --- the menu -------------------------------------------------------------------------------

    UiDeviceMenuPanel::UiDeviceMenuPanel(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {}

    float UiDeviceMenuPanel::GetContentHeight() const
    {
        if (!ctx)
            return 200.0f;

        float width = Math::Max(menuMinWidth, layout->GetWidth());
        return HeadHeight(width) + 1.0f + ListHeight() + 1.0f + FootHeight(*ctx, width);
    }

    Ref<EditBox> UiDeviceMenuPanel::GetCustomField(const String& key) const
    {
        return mFields[key == "w" ? 0 : 1];
    }

    String UiDeviceMenuPanel::RowText(const String& id) const
    {
        String text;
        mRowTexts.TryGetValue(id, text);
        return text;
    }

    void UiDeviceMenuPanel::SyncFields()
    {
        PushEditorScopeOnStack scope;
        if (!ctx)
            return;

        const char* keys[] = { "w", "h" };
        const char* letters[] = { "W", "H" };
        double values[] = { ctx->setting.custom.w, ctx->setting.custom.h };
        for (int i = 0; i < 2; i++)
        {
            if (!mFields[i])
            {
                String key = keys[i];
                WeakRef<UiDeviceMenuPanel> weak(this);
                // Applied on Enter or when the field is left, not while it is typed: "1000" must not apply 100 on the way
                mFields[i] = UiWsControls::LazyNumberField(letters[i], values[i], "", [weak, key](std::optional<double> v)
                {
                    auto self = weak.Lock();
                    auto owner = self ? self->picker.Lock() : nullptr;
                    if (!owner || !v || std::round(*v) == 0.0)
                        return;
                    owner->SetCustom(key, std::round(*v));
                    self->SyncFields();
                }, 64.0);
                mFields[i]->name = String("custom ") + keys[i];
                AddChild(mFields[i]);
            }
            UiWsControls::SetNumber(mFields[i], values[i]);
        }
    }

    void UiDeviceMenuPanel::OnScrolled(float scroll)
    {
        float width = layout->GetWidth();
        float visible = layout->GetHeight() - HeadHeight(width) - (ctx ? FootHeight(*ctx, width) : 0.0f);
        mScroll = Math::Clamp(mScroll - scroll*0.5f, 0.0f, Math::Max(0.0f, mListHeight - visible));
    }

    void UiDeviceMenuPanel::DrawPanel(const Box& box)
    {
        auto owner = picker.Lock();
        if (!ctx || !owner)
            return;

        auto& t = T();
        float W = box.W();
        const auto& doc = ctx->Doc();
        const auto& setting = ctx->setting;
        const auto& target = ctx->target;
        String chosen = owner->Chosen();
        bool turnable = chosen != "design" && chosen != "reference";
        auto turned = [&](const UiDoc::UiScreen& s) { return Turned(s, setting.landscape); };

        // The head: what another screen changes, and the turn button
        float head = HeadHeight(W);
        Text(hintText, box.R(10, 8 + Math::Max(0.0f, (28.0f - TextHeight(hintText, W - 54.0f, Wrapped(11.0f, t.fg3)))*0.5f), W - 16 - 6 - 28 - 4, head - 16),
             Wrapped(11.0f, t.fg3));
        Tool(box.R(W - 8 - 28, (head - 28)*0.5f, 28, 28), "uiws_rotate", "turn", setting.landscape, turnable);
        Line(box.P(0, head - 0.5f), box.P(W, head - 0.5f), t.line);

        float foot = FootHeight(*ctx, W);
        float listTop = head, listBottom = box.H() - foot;
        mListHeight = ListHeight();
        mScroll = Math::Clamp(mScroll, 0.0f, Math::Max(0.0f, mListHeight - (listBottom - listTop)));
        o2Render.EnableScissorTest((RectI)box.R(0, listTop, W, listBottom - listTop));

        mRowTexts.Clear();
        float y = listTop + 6.0f - mScroll;
        // A row: the screen's name and its aspect ratio - what choosing it changes - and the canvas it gives
        auto row = [&](const String& id, const String& kind, const String& name, const UiDoc::UiScreen* s, const String& meta = "")
        {
            RectF r = box.R(6, y, W - 12, rowHeight);
            bool on = chosen == id;
            bool disabled = s == nullptr;
            String hot = "device:" + id;
            if (on)
                Fill(r, 7.0f, t.accentSoft);
            else if (!disabled && IsHover(hot))
                Fill(r, 7.0f, t.active);
            float alpha = disabled ? 0.5f : 1.0f;
            UiWsDraw::Icon(DeviceIcon(kind), Vec2F(r.left + 8 + 7.5f, r.Center().y), 15.0f, A(on ? t.accent : t.fgDim, alpha));
            String right = s ? (meta.IsEmpty() ? Ws::CanvasText(doc, s->w, s->h) : meta) : String("connect a picture");
            String aspect = s ? UiDoc::AspectName(s->w, s->h) : String();
            float rightW = TextWidth(right, 10.5f, 400, true);
            float nameRight = r.right - 8 - rightW - 8;
            float nameW = Math::Min(TextWidth(name, 12.0f) + 1.0f, Math::Max(20.0f, nameRight - (r.left + 31) - (aspect.IsEmpty() ? 0.0f : 7.0f + TextWidth(aspect, 10.5f, 400, true))));
            Text(name, RectF(r.left + 31, r.top, r.left + 31 + nameW, r.bottom), TextStyle(12.0f, 400, A(t.fg, alpha)));
            if (!aspect.IsEmpty())
                Text(aspect, RectF(r.left + 31 + nameW + 7, r.top, nameRight, r.bottom), TextStyle(10.5f, 400, A(t.fgDim, alpha), HorAlign::Left, true));
            Text(right, RectF(r.right - 8 - rightW - 2, r.top, r.right - 8, r.bottom), TextStyle(10.5f, 400, A(t.fg3, alpha), HorAlign::Right, true));
            mRowTexts[id] = name + (aspect.IsEmpty() ? String() : " " + aspect) + " \xC2\xB7 " + right;
            if (!disabled && y + rowHeight > listTop && y < listBottom)
                AddHot(r, hot);
            y += rowHeight;
        };

        UiDoc::UiScreen kitScreen = turned(target.Screen());
        row("kit", Ws::TargetKind(target), "Kit's device \xC2\xB7 " + target.name, &kitScreen);
        UiDoc::UiScreen design{ doc.width, doc.height, UiDoc::DocDpi(doc) };
        row("design", "design", "Design", &design, Ws::SizeText(doc.width, doc.height));
        row("reference", "reference", "Reference", ctx->reference ? &*ctx->reference : nullptr);
        for (int g = 0; g < 3; g++)
        {
            y += groupGap;
            Text(kindTitles[g], box.R(14, y + 6, W - 28, 14), TextStyle(10.0f, 600, t.fg3));
            y += groupTitle;
            for (auto& d : UiDoc::UiDevices())
            {
                if (d.kind == kinds[g])
                {
                    auto s = turned(d.Screen());
                    row(d.id, d.kind, d.name, &s);
                }
            }
        }

        // The custom screen: its icon, the W and H fields, its aspect ratio and the canvas
        y += groupGap;
        Text("CUSTOM", box.R(14, y + 6, W - 28, 14), TextStyle(10.0f, 600, t.fg3));
        y += groupTitle;
        RectF custom = box.R(6, y, W - 12, customRow);
        bool customOn = chosen == "custom";
        if (customOn)
            Fill(custom, 7.0f, t.accentSoft);
        UiWsDraw::Icon(DeviceIcon("custom"), Vec2F(custom.left + 8 + 7.5f, custom.Center().y), 15.0f, customOn ? t.accent : t.fgDim);
        float fx = 6 + 8 + 15 + 6;
        bool fieldsShown = y >= listTop - 1 && y + customRow <= listBottom + 1;
        for (int i = 0; i < 2; i++)
        {
            if (!mFields[i])
                continue;
            float left = fx + i*(84 + 6);
            *mFields[i]->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(left, -(y + 4 + 28)), Vec2F(left + 84, -(y + 4)));
            if (mFields[i]->IsEnabled() != fieldsShown)
                mFields[i]->SetEnabledForcible(fieldsShown);
        }
        auto cs = turned(setting.custom);
        String customMeta = UiDoc::AspectName(cs.w, cs.h) + " \xC2\xB7 " + Ws::CanvasText(doc, cs.w, cs.h);
        float metaLeft = fx + 2*(84 + 6) + 4;
        Text(customMeta, box.R(metaLeft, y, W - 6 - 8 - metaLeft, customRow), TextStyle(10.5f, 400, t.fg3, HorAlign::Left, true));
        mRowTexts["custom"] = customMeta;
        o2Render.DisableScissorTest();

        // The foot: the canvas the layout gets, what the screen adds, the comparison, the soft graphics
        Line(box.P(0, listBottom + 0.5f), box.P(W, listBottom + 0.5f), t.line);
        float fy = listBottom + 8;
        for (auto& line : FootLines(*ctx))
        {
            float h = TextHeight(line, W - 20, Wrapped(11.0f, t.fg3));
            Text(line, box.R(10, fy, W - 20, h), Wrapped(11.0f, t.fg3));
            fy += h + 4;
        }
        auto soft = Soft(*ctx);
        if (!soft.IsEmpty())
        {
            String warn = SoftText(soft);
            float h = TextHeight(warn, W - 20 - 17, Wrapped(11.0f, t.yellow));
            UiWsDraw::Icon("uiws_warn", box.P(10 + 6, fy + 8), 12.0f, t.yellow);
            Text(warn, box.R(10 + 17, fy, W - 20 - 17, h), Wrapped(11.0f, t.yellow));
        }
    }

    void UiDeviceMenuPanel::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        auto owner = picker.Lock();
        if (!owner)
            return;

        if (id.StartsWith("device:"))
            owner->Pick(id.SubStr(7));
        else if (id == "turn")
            owner->ToggleLandscape();
        SyncFields();
    }
}
