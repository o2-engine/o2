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
        const float headHeight = 44.0f;
        const float rowHeight = 30.0f;
        const float groupTitle = 24.0f;
        const float groupGap = 6.0f;
        const float customRow = 36.0f;
        const float menuMinWidth = 320.0f;
        const float menuMaxWidth = 600.0f;
        const float menuMaxHeight = 540.0f;

        const char* const kinds[] = { "phone", "tablet", "desktop" };
        const char* const kindTitles[] = { "PHONES", "TABLETS", "DESKTOPS" };

        // The kit graphics drawn soft on the shown screen
        Vector<String> Soft(const UiWsContext& ctx)
        {
            return Ws::SoftGraphics(ctx.Doc(), ctx.hasKit ? &ctx.kit : nullptr, ctx.preview.frame);
        }

        float FootHeight(const UiWsContext& ctx, float width)
        {
            auto soft = Soft(ctx);
            float h = 16.0f + 15.0f;
            if (!soft.IsEmpty())
            {
                String names;
                for (int i = 0; i < soft.Count() && i < 4; i++)
                    names += (i ? ", " : "") + soft[i];
                if (soft.Count() > 4)
                    names += "\xE2\x80\xA6";
                String warn = "Enlarged on this screen (soft): " + (String)soft.Count() + " kit graphics \xE2\x80\x94 " + names;
                TextStyle style(11.0f, 400, T().yellow);
                style.wrap = true;
                style.lineHeight = 15.4f;
                h += 4.0f + TextHeight(warn, width - 20.0f - 17.0f, style);
            }
            return h;
        }

        float ListHeight()
        {
            float h = 12.0f + rowHeight*2.0f;
            for (int g = 0; g < 3; g++)
            {
                int count = UiDoc::UiDevices().Count([&](const UiDoc::UiDevice& d) { return d.kind == kinds[g]; });
                h += groupGap + groupTitle + count*rowHeight;
            }
            h += groupGap + groupTitle + customRow;
            return h;
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
        String meta = Ws::ScreenText(ctx->preview.screen);
        float nameW = Math::Min(150.0f, TextWidth(name, 11.5f));
        float w = (chip ? 6.0f : 8.0f) + (chip ? 16.0f : 14.0f) + 7.0f + nameW + 1.0f + 7.0f + TextWidth(meta, 10.5f, 400, true) + 4.0f + 7.0f + 11.0f + 8.0f;
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
        UiWsDraw::Icon(DeviceIcon(ctx->preview.kind), Vec2F(x + iconSize*0.5f, cy), iconSize, t.fgDim);
        x += iconSize + 7.0f;

        float right = rect.right - 8.0f;
        UiWsDraw::Icon("uiws_chevron_down", Vec2F(right - 5.5f, cy), 11.0f, t.fg3);
        right -= 11.0f + 7.0f;
        if (!Soft(*ctx).IsEmpty())
        {
            UiWsDraw::Icon("uiws_warn", Vec2F(right - 6.0f, cy), 12.0f, t.yellow);
            right -= 12.0f + 7.0f;
        }

        String name = Ws::PreviewName(ctx->preview);
        String meta = Ws::ScreenText(ctx->preview.screen);
        float metaW = TextWidth(meta, 10.5f, 400, true);
        float nameW = Math::Min(TextWidth(name, 11.5f) + 1.0f, chip ? 150.0f : Math::Max(30.0f, right - x - metaW - 7.0f));
        Text(name, RectF(x, rect.top, x + nameW, rect.bottom), TextStyle(11.5f, 400, t.fg));
        x += nameW + 7.0f;
        Text(meta, RectF(x, rect.top, Math::Max(x, right), rect.bottom), TextStyle(10.5f, 400, t.fg3, HorAlign::Left, true));
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
        if (mOwnContext && ctx)
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

    void UiDevicePicker::SetScale(const String& scale)
    {
        if (ctx)
            ctx->SetUi("uiScale", Json(scale));
    }

    void UiDevicePicker::ToggleLandscape()
    {
        if (!ctx || ctx->preview.kind == "design" || ctx->preview.kind == "reference")
            return;

        ctx->SetUi("uiLandscape", Json(!ctx->setting.landscape));
    }

    void UiDevicePicker::SetCustom(const String& key, double value)
    {
        if (!ctx)
            return;

        // The custom screen in its order: w, h, dpi
        auto custom = ctx->setting.custom;
        Json obj = Json::Object();
        obj["w"] = key == "w" ? value : custom.w;
        obj["h"] = key == "h" ? value : custom.h;
        obj["dpi"] = key == "dpi" ? value : custom.dpi;
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
        return headHeight + ListHeight() + FootHeight(*ctx, width);
    }

    Ref<EditBox> UiDeviceMenuPanel::GetCustomField(const String& key) const
    {
        int i = key == "w" ? 0 : key == "h" ? 1 : 2;
        return mFields[i];
    }

    void UiDeviceMenuPanel::SyncFields()
    {
        PushEditorScopeOnStack scope;
        if (!ctx)
            return;

        const char* keys[] = { "w", "h", "dpi" };
        const char* letters[] = { "W", "H", "DPI" };
        double values[] = { ctx->setting.custom.w, ctx->setting.custom.h, ctx->setting.custom.dpi };
        for (int i = 0; i < 3; i++)
        {
            if (!mFields[i])
            {
                String key = keys[i];
                WeakRef<UiDeviceMenuPanel> weak(this);
                mFields[i] = UiWsControls::NumberField(letters[i], values[i], "", [weak, key](std::optional<double> v)
                {
                    auto self = weak.Lock();
                    auto owner = self ? self->picker.Lock() : nullptr;
                    if (!owner || !v)
                        return;
                    if (key == "dpi")
                    {
                        auto dpi = UiDoc::CleanDpi(*v);
                        if (dpi)
                            owner->SetCustom(key, *dpi);
                    }
                    else if (std::round(*v) != 0.0)
                        owner->SetCustom(key, std::round(*v));
                }, key == "dpi" ? 36.0 : 64.0);
                mFields[i]->name = String("custom ") + keys[i];
                AddChild(mFields[i]);
            }
            UiWsControls::SetNumber(mFields[i], values[i]);
        }
    }

    void UiDeviceMenuPanel::OnScrolled(float scroll)
    {
        float visible = layout->GetHeight() - headHeight - (ctx ? FootHeight(*ctx, layout->GetWidth()) : 0.0f);
        mScroll = Math::Clamp(mScroll - scroll*0.5f, 0.0f, Math::Max(0.0f, mListHeight - visible));
    }

    void UiDeviceMenuPanel::DrawPanel(const Box& box)
    {
        if (!ctx)
            return;

        auto& t = T();
        float W = box.W();
        auto& preview = ctx->preview;
        auto& setting = ctx->setting;
        bool turnable = preview.kind != "design" && preview.kind != "reference";

        // The head: By DPI | By screen and the turn button
        Segments(box.R(8, 8, W - 16 - 34, 28), { { "dpi", "By DPI" }, { "fit", "By screen" } }, setting.scale, "scale:");
        Tool(box.R(W - 8 - 28, 8, 28, 28), "uiws_rotate", "turn", setting.landscape, turnable);
        Line(box.P(0, headHeight - 0.5f), box.P(W, headHeight - 0.5f), t.line);

        float foot = FootHeight(*ctx, W);
        float listTop = headHeight, listBottom = box.H() - foot;
        mListHeight = ListHeight();
        mScroll = Math::Clamp(mScroll, 0.0f, Math::Max(0.0f, mListHeight - (listBottom - listTop)));
        o2Render.EnableScissorTest((RectI)box.R(0, listTop, W, listBottom - listTop));

        float y = listTop + 6.0f - mScroll;
        auto row = [&](const String& id, const String& kind, const String& name, const UiDoc::UiScreen* s, bool disabled)
        {
            RectF r = box.R(6, y, W - 12, rowHeight);
            bool on = (preview.id == id && !preview.fallback) || (id == "design" && preview.kind == "design");
            String hot = "device:" + id;
            if (on)
                Fill(r, 7.0f, t.accentSoft);
            else if (!disabled && IsHover(hot))
                Fill(r, 7.0f, t.active);
            float alpha = disabled ? 0.5f : 1.0f;
            UiWsDraw::Icon(DeviceIcon(kind), Vec2F(r.left + 8 + 7.5f, r.Center().y), 15.0f, A(on ? t.accent : t.fgDim, alpha));
            String meta = s ? Ws::ScreenText(*s) : String("connect a picture");
            float metaW = TextWidth(meta, 10.5f, 400, true);
            Text(name, RectF(r.left + 31, r.top, r.right - 8 - metaW - 8, r.bottom), TextStyle(12.0f, 400, A(t.fg, alpha)));
            Text(meta, RectF(r.right - 8 - metaW - 2, r.top, r.right - 8, r.bottom), TextStyle(10.5f, 400, A(t.fg3, alpha), HorAlign::Right, true));
            if (!disabled && y + rowHeight > listTop && y < listBottom)
                AddHot(r, hot);
            y += rowHeight;
        };

        UiDoc::UiScreen design{ ctx->Doc().width, ctx->Doc().height, UiDoc::DocDpi(ctx->Doc()) };
        row("design", "design", "Design", &design, false);
        row("reference", "reference", "Reference", ctx->reference ? &*ctx->reference : nullptr, !ctx->reference);
        for (int g = 0; g < 3; g++)
        {
            y += groupGap;
            Text(kindTitles[g], box.R(14, y + 6, W - 28, 14), TextStyle(10.0f, 600, t.fg3));
            y += groupTitle;
            for (auto& d : UiDoc::UiDevices())
            {
                if (d.kind == kinds[g])
                {
                    auto s = d.Screen();
                    row(d.id, d.kind, d.name, &s, false);
                }
            }
        }

        // The custom screen: its icon and the W H DPI fields
        y += groupGap;
        Text("CUSTOM", box.R(14, y + 6, W - 28, 14), TextStyle(10.0f, 600, t.fg3));
        y += groupTitle;
        RectF custom = box.R(6, y, W - 12, customRow);
        if (preview.kind == "custom")
            Fill(custom, 7.0f, t.accentSoft);
        UiWsDraw::Icon(DeviceIcon("custom"), Vec2F(custom.left + 8 + 7.5f, custom.Center().y), 15.0f, preview.kind == "custom" ? t.accent : t.fgDim);
        float fx = 6 + 8 + 15 + 6, fw = (W - 12 - 16 - 15 - 6*3)/3.0f;
        bool fieldsShown = y >= listTop - 1 && y + customRow <= listBottom + 1;
        for (int i = 0; i < 3; i++)
        {
            if (!mFields[i])
                continue;
            *mFields[i]->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(fx + i*(fw + 6), -(y + 4 + 28)), Vec2F(fx + i*(fw + 6) + fw, -(y + 4)));
            if (mFields[i]->IsEnabled() != fieldsShown)
                mFields[i]->SetEnabledForcible(fieldsShown);
        }
        o2Render.DisableScissorTest();

        // The foot: the canvas the layout gets, the soft graphics
        Line(box.P(0, listBottom + 0.5f), box.P(W, listBottom + 0.5f), t.line);
        String canvas = "Layout canvas " + UiDoc::JsNum(preview.frame.w) + "\xC3\x97" + UiDoc::JsNum(preview.frame.h) + " at " +
            UiDoc::JsNum(UiDoc::DocDpi(ctx->Doc())) + " dpi (" + UiDoc::DensityName(UiDoc::DocDpi(ctx->Doc())) + ")";
        Text(canvas, box.R(10, listBottom + 8, W - 20, 15), TextStyle(11.0f, 400, t.fg3));
        auto soft = Soft(*ctx);
        if (!soft.IsEmpty())
        {
            String names;
            for (int i = 0; i < soft.Count() && i < 4; i++)
                names += (i ? ", " : "") + soft[i];
            if (soft.Count() > 4)
                names += "\xE2\x80\xA6";
            String warn = "Enlarged on this screen (soft): " + (String)soft.Count() + " kit graphics \xE2\x80\x94 " + names;
            UiWsDraw::Icon("uiws_warn", box.P(10 + 6, listBottom + 8 + 15 + 4 + 8), 12.0f, t.yellow);
            TextStyle style(11.0f, 400, t.yellow, HorAlign::Left);
            style.wrap = true;
            style.ver = VerAlign::Top;
            style.lineHeight = 15.4f;
            Text(warn, box.R(10 + 17, listBottom + 8 + 15 + 4, W - 20 - 17, box.H() - listBottom - 27), style);
        }
    }

    void UiDeviceMenuPanel::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        auto owner = picker.Lock();
        if (!owner)
            return;

        if (id.StartsWith("device:"))
            owner->Pick(id.SubStr(7));
        else if (id.StartsWith("scale:"))
            owner->SetScale(id.SubStr(6));
        else if (id == "turn")
            owner->ToggleLandscape();
        SyncFields();
    }
}
