#include "o2Editor/stdafx.h"
#include "PipelineKitCard.h"

#include <cstdlib>

#include "o2/Render/Render.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"

namespace Editor
{
    using namespace UiWsDraw;
    using UiDoc::Json;

    namespace
    {
        const float rowHeight = 30.0f, groupTitle = 24.0f, groupGap = 6.0f, customRow = 36.0f;
        // The menu zooms with the canvas (it hangs off the chip in canvas units), so it shows every device at once
        const float menuMinWidth = 320.0f, menuMaxWidth = 600.0f, menuMaxHeight = 800.0f;
        const double sideMin = 64.0, sideMax = 8192.0;

        const char* const kinds[] = { "phone", "tablet", "desktop" };
        const char* const kindTitles[] = { "PHONES", "TABLETS", "DESKTOPS" };

        String FootText(const UiDoc::UiTarget& target)
        {
            return "The kit is made for a " + UiDoc::JsNum(target.w) + "\xC3\x97" + UiDoc::JsNum(target.h) + " screen at " +
                UiDoc::JsNum(UiDoc::JsRound(target.dpi)) + " dpi (" + UiDoc::DensityName(target.dpi) + "): element sizes are measured on "
                "it, textures are brought to its density";
        }

        float ListHeight()
        {
            float h = 6.0f;
            for (int g = 0; g < 3; g++)
            {
                int count = UiDoc::UiDevices().Count([&](const UiDoc::UiDevice& d) { return d.kind == kinds[g]; });
                h += (g ? groupGap : 0.0f) + groupTitle + count*rowHeight;
            }
            return h + groupGap + groupTitle + customRow + 6.0f;
        }
    }

    // ------------------------------------------------------------------------------------------------
    // The device menu of the kit (KitDevicePicker.tsx): the known devices by kind, the custom screen's
    // W H DPI fields, what the kit is made for at the foot
    // ------------------------------------------------------------------------------------------------
    class PipelineKitDeviceMenu : public UiWsPanel
    {
    public:
        WeakRef<PipelineKitDeviceChip> chip;

    public:
        explicit PipelineKitDeviceMenu(RefCounter* refCounter): UiWsPanel(refCounter) {}

        float FootHeight(float width) const
        {
            auto owner = chip.Lock();
            TextStyle style(11.0f, 400, T().fg3);
            style.wrap = true;
            style.lineHeight = 15.4f;
            return 16.0f + (owner ? TextHeight(FootText(owner->Target()), width - 20.0f, style) : 15.0f);
        }

        float GetContentHeight() const
        {
            return ListHeight() + FootHeight(Math::Max(menuMinWidth, layout->GetWidth()));
        }

        // The custom screen's fields with the values now; they apply on Enter or when left, Escape drops what was typed
        void SyncFields()
        {
            PushEditorScopeOnStack scope;
            auto owner = chip.Lock();
            if (!owner)
                return;

            auto custom = owner->CustomScreen();
            const char* keys[] = { "w", "h", "dpi" };
            const char* letters[] = { "W", "H", "DPI" };
            double values[] = { custom.w, custom.h, custom.dpi };
            for (int i = 0; i < 3; i++)
            {
                if (mFields[i])
                    RemoveChild(mFields[i]);

                String key = keys[i];
                double min = key == "dpi" ? 36.0 : sideMin;
                mFields[i] = UiWsControls::NumberField(letters[i], values[i], "", {}, min);
                mFields[i]->name = String("custom ") + keys[i];
                String shown = UiWsControls::NumberText(values[i]);
                WeakRef<PipelineKitDeviceMenu> weak(this);
                mFields[i]->onChangeCompleted = [weak, key, shown, min](const WString& text)
                {
                    auto self = weak.Lock();
                    auto owner = self ? self->chip.Lock() : nullptr;
                    String s = ((String)text).Trimed(" \t");
                    // Escape drops what was typed: the menu closes on it, which takes the focus away while it is held
                    if (!owner || s == shown || s.IsEmpty() || o2Input.IsKeyDown(VK_ESCAPE))
                        return;

                    char* end = nullptr;
                    double v = std::strtod(s.Data(), &end);
                    if (end && *end == '\0' && std::isfinite(v) && v >= min)
                        owner->SetCustom(key, v);
                };
                AddChild(mFields[i]);
            }
        }

        Ref<EditBox> GetField(int index) const { return mFields[index]; }

        // The fields show the screen again on the next update (never from inside a field's own callback)
        void MarkDirty() { mDirty = true; }

        void Update(float dt) override
        {
            UiWsPanel::Update(dt);
            if (mDirty)
            {
                mDirty = false;
                SyncFields();
            }
        }

    protected:
        Ref<EditBox> mFields[3];
        bool         mDirty = false;

    protected:
        void DrawPanel(const Box& box) override
        {
            auto owner = chip.Lock();
            if (!owner)
                return;

            auto& t = T();
            float W = box.W();
            auto target = owner->Target();
            float foot = FootHeight(W);
            float listBottom = box.H() - foot;
            float y = 6.0f;
            auto row = [&](const UiDoc::UiDevice& d)
            {
                RectF r = box.R(6, y, W - 12, rowHeight);
                bool on = target.id == d.id;
                String hot = "device:" + d.id;
                if (on)
                    Fill(r, 7.0f, t.accentSoft);
                else if (IsHover(hot))
                    Fill(r, 7.0f, t.active);
                Icon(DeviceIcon(d.kind), Vec2F(r.left + 8 + 7.5f, r.Center().y), 15.0f, on ? t.accent : t.fgDim);
                String meta = PipelineKitCards::ScreenText(d.Screen());
                float metaW = TextWidth(meta, 10.5f, 400, true);
                Text(d.name, RectF(r.left + 31, r.top, r.right - 8 - metaW - 8, r.bottom), TextStyle(12.0f, 400, t.fg));
                Text(meta, RectF(r.right - 8 - metaW - 2, r.top, r.right - 8, r.bottom), TextStyle(10.5f, 400, t.fg3, HorAlign::Right, true));
                AddHot(r, hot);
                y += rowHeight;
            };

            for (int g = 0; g < 3; g++)
            {
                if (g)
                    y += groupGap;
                Text(kindTitles[g], box.R(14, y + 6, W - 28, 14), TextStyle(10.0f, 600, t.fg3));
                y += groupTitle;
                for (auto& d : UiDoc::UiDevices())
                {
                    if (d.kind == kinds[g])
                        row(d);
                }
            }

            // The custom screen: a click on the row takes it, so does a number typed into it
            y += groupGap;
            Text("CUSTOM", box.R(14, y + 6, W - 28, 14), TextStyle(10.0f, 600, t.fg3));
            y += groupTitle;
            RectF custom = box.R(6, y, W - 12, customRow);
            bool isCustom = target.id.IsEmpty();
            if (isCustom)
                Fill(custom, 7.0f, t.accentSoft);
            else if (IsHover("custom"))
                Fill(custom, 7.0f, t.active);
            Icon(DeviceIcon("custom"), Vec2F(custom.left + 8 + 7.5f, custom.Center().y), 15.0f, isCustom ? t.accent : t.fgDim);
            AddHot(custom, "custom");
            float fx = 6 + 8 + 15 + 6, fw = (W - 12 - 16 - 15 - 6*3)/3.0f;
            for (int i = 0; i < 3; i++)
            {
                if (!mFields[i])
                    continue;
                WidgetLayout next(Vec2F(0, 1), Vec2F(0, 1), Vec2F(fx + i*(fw + 6), -(y + 4 + 28)), Vec2F(fx + i*(fw + 6) + fw, -(y + 4)));
                if (mFields[i]->layout->GetOffsetMin() != next.GetOffsetMin() || mFields[i]->layout->GetOffsetMax() != next.GetOffsetMax())
                    *mFields[i]->layout = next;
            }

            // The foot: what the kit is made for
            Line(box.P(0, listBottom + 0.5f), box.P(W, listBottom + 0.5f), t.line);
            TextStyle style(11.0f, 400, t.fg3);
            style.wrap = true;
            style.ver = VerAlign::Top;
            style.lineHeight = 15.4f;
            Text(FootText(target), box.R(10, listBottom + 8, W - 20, foot - 8), style);
        }

        void OnHotClick(const String& id, const Input::Cursor& cursor) override
        {
            auto owner = chip.Lock();
            if (!owner)
                return;

            if (id.StartsWith("device:"))
            {
                owner->Pick(id.SubStr(7));
                owner->ToggleMenu();
            }
            else if (id == "custom" && !owner->Target().id.IsEmpty())
                owner->SetCustom("", std::nullopt);
        }
    };

    // --- the chip ------------------------------------------------------------------------------------

    PipelineKitDeviceChip::PipelineKitDeviceChip(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {}

    void PipelineKitDeviceChip::Bind(const Ref<PipelineUiKitBody>& body)
    {
        mBody = body;
    }

    UiDoc::UiTarget PipelineKitDeviceChip::Target() const
    {
        auto body = mBody.Lock();
        if (!body)
            return UiDoc::DefaultTarget();

        // Only the two keys the device is read from (the config holds the kit's items too)
        Json config = Json::Object();
        for (auto key : { "kitDevice", "kitDeviceCustom" })
        {
            if (auto value = body->GetNode()->GetConfigValue(key))
                config[key] = UiDoc::FromDataValue(*value);
        }
        return UiDoc::TargetOfConfig(config);
    }

    UiDoc::UiScreen PipelineKitDeviceChip::CustomScreen() const
    {
        auto target = Target();
        if (target.id.IsEmpty())
            return target.Screen();

        // The screen typed before, else the device chosen now (so "Custom" starts from it)
        auto body = mBody.Lock();
        UiDoc::UiScreen screen = target.Screen();
        auto raw = body ? body->GetNode()->GetConfigValue("kitDeviceCustom") : nullptr;
        if (raw && raw->IsObject())
        {
            Json stored = UiDoc::FromDataValue(*raw);
            auto side = [&](const char* key, double def)
            {
                auto v = stored.Get(key);
                double n = v && v->IsNumber() ? v->Num() : 0.0;
                return n >= sideMin ? Math::Min(sideMax, UiDoc::JsRound(n)) : def;
            };
            screen.w = side("w", target.w);
            screen.h = side("h", target.h);
            screen.dpi = UiDoc::CleanDpi(stored.Get("dpi")).value_or(target.dpi);
        }
        return screen;
    }

    float PipelineKitDeviceChip::GetPreferredWidth() const
    {
        auto target = Target();
        float nameW = Math::Min(150.0f, TextWidth(PipelineKitCards::TargetName(target), 11.5f));
        float w = 8.0f + 14.0f + 7.0f + nameW + 1.0f + 7.0f + TextWidth(PipelineKitCards::ScreenText(target.Screen()), 10.5f, 400, true) +
            4.0f + 7.0f + 11.0f + 8.0f;
        return Math::Min(320.0f, Math::Ceil(w));
    }

    void PipelineKitDeviceChip::DrawPanel(const Box& box)
    {
        auto& t = T();
        RectF rect = box.world;
        bool open = IsMenuOpen();
        Fill(rect, 8.0f, t.field);
        Border(rect, 8.0f, open || IsHover("trigger") ? t.accent : t.line);

        auto target = Target();
        float x = rect.left + 8.0f, cy = rect.Center().y;
        Icon(DeviceIcon(PipelineKitCards::TargetKind(target)), Vec2F(x + 7.0f, cy), 14.0f, t.fgDim);
        x += 14.0f + 7.0f;
        float right = rect.right - 8.0f;
        Icon("uiws_chevron_down", Vec2F(right - 5.5f, cy), 11.0f, t.fg3);
        right -= 11.0f + 7.0f;

        String name = PipelineKitCards::TargetName(target), meta = PipelineKitCards::ScreenText(target.Screen());
        float metaW = TextWidth(meta, 10.5f, 400, true);
        float nameW = Math::Min(TextWidth(name, 11.5f) + 1.0f, Math::Max(30.0f, right - x - metaW - 7.0f));
        Text(name, RectF(x, rect.top, x + nameW, rect.bottom), TextStyle(11.5f, 400, t.fg));
        x += nameW + 7.0f;
        Text(meta, RectF(x, rect.top, Math::Max(x, right), rect.bottom), TextStyle(10.5f, 400, t.fg3, HorAlign::Left, true));
        AddHot(rect, "trigger");
    }

    void PipelineKitDeviceChip::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (id == "trigger")
            ToggleMenu();
    }

    void PipelineKitDeviceChip::ToggleMenu()
    {
        PushEditorScopeOnStack scope;
        if (!mMenu)
        {
            mMenu = mmake<UiWsPopup>();
            mMenu->name = "kit device menu";
            auto panel = mmake<PipelineKitDeviceMenu>();
            panel->name = "kit device menu panel";
            mMenuPanel = panel;
            mMenu->SetContent(mMenuPanel);
            AddChild(mMenu);
        }

        if (mMenu->IsOpen() || UiWsPopup::ClosedThisFrame())
        {
            mMenu->Close();
            return;
        }

        auto panel = DynamicCast<PipelineKitDeviceMenu>(mMenuPanel);
        panel->chip = Ref(this);
        panel->SyncFields();

        // The chip sits on the canvas and the popover is drawn in the layer it was drawn in: the menu hangs off the chip in
        // canvas units, zooming with the card
        RectF anchor = layout->GetWorldRect();
        float width = Math::Clamp(anchor.Width(), menuMinWidth, menuMaxWidth);
        mMenu->OpenAt(anchor, width, Math::Min(menuMaxHeight, panel->GetContentHeight()));
    }

    bool PipelineKitDeviceChip::IsMenuOpen() const
    {
        return mMenu && mMenu->IsOpen();
    }

    Ref<UiWsPanel> PipelineKitDeviceChip::GetMenuPanel() const
    {
        return mMenuPanel;
    }

    void PipelineKitDeviceChip::Pick(const String& deviceId)
    {
        auto body = mBody.Lock();
        if (!body || Target().id == deviceId)
            return;

        body->GetNode()->SetConfigString("kitDevice", deviceId);
        body->Notify("kitDevice", true);
        body->OnConfigChanged();
    }

    void PipelineKitDeviceChip::SetCustom(const String& key, std::optional<double> value)
    {
        auto body = mBody.Lock();
        if (!body)
            return;

        auto screen = CustomScreen();
        if (value)
        {
            if (key == "dpi")
            {
                auto dpi = UiDoc::CleanDpi(*value);
                if (!dpi)
                    return;
                screen.dpi = *dpi;
            }
            else
            {
                double side = Math::Min(sideMax, UiDoc::JsRound(*value));
                if (side < sideMin)
                    return;
                (key == "w" ? screen.w : screen.h) = side;
            }
        }

        Json custom = Json::Object();
        custom["w"] = screen.w;
        custom["h"] = screen.h;
        custom["dpi"] = screen.dpi;
        body->GetNode()->SetConfigString("kitDevice", "custom");
        UiDoc::SetConfigJson(body->GetNode()->config, "kitDeviceCustom", custom);
        body->Notify("kitDevice", true);
        body->OnConfigChanged();
        if (auto panel = DynamicCast<PipelineKitDeviceMenu>(mMenuPanel))
            panel->MarkDirty();
    }
}
