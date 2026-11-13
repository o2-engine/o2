#include "o2Editor/stdafx.h"
#include "UiWsWidgets.h"

#include <cmath>

#include "o2/Assets/Types/FontAsset.h"
#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/System/Time/Time.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"

namespace Editor
{
    using namespace UiWsDraw;

    // --- the panel ----------------------------------------------------------------------------

    UiWsPanel::UiWsPanel(RefCounter* refCounter):
        Widget(refCounter)
    {}

    void UiWsPanel::Draw()
    {
        if (!mResEnabledInHierarchy || mIsClipped)
        {
            Widget::Draw();
            return;
        }

        DrawLayers();
        ISceneDrawable::OnDrawn();

        Box box(layout->GetWorldRect());
        mHot.Clear();
        DrawPanel(box);
        mDrawnHot = mHot;
        CursorAreaEventsListener::OnDrawn();

        if (mClipChildren)
            o2Render.EnableScissorTest((RectI)mChildClip);
        DrawInheritedDepthChildren();
        DrawInternalChildren();
        if (mClipChildren)
            o2Render.DisableScissorTest();
        DrawTopLayers();
        DrawOverlay(box);
    }

    bool UiWsPanel::IsUnderPoint(const Vec2F& point)
    {
        return Inside(layout->GetWorldRect(), point);
    }

    String UiWsPanel::HotAt(const Vec2F& point) const
    {
        for (int i = mDrawnHot.Count() - 1; i >= 0; i--)
        {
            if (Inside(mDrawnHot[i].rect, point))
                return mDrawnHot[i].id;
        }
        return "";
    }

    RectF UiWsPanel::HotRect(const String& id) const
    {
        for (int i = mDrawnHot.Count() - 1; i >= 0; i--)
        {
            if (mDrawnHot[i].id == id)
                return mDrawnHot[i].rect;
        }
        return RectF();
    }

    void UiWsPanel::ClickHot(const String& id)
    {
        Input::Cursor cursor;
        cursor.position = HotRect(id).Center();
        OnHotClick(id, cursor);
    }

    void UiWsPanel::AddHot(const RectF& rect, const String& id)
    {
        mHot.Add(Hot{ rect, id });
    }

    void UiWsPanel::OnCursorPressed(const Input::Cursor& cursor)
    {
        mPressedHot = HotAt(cursor.position);
        mPressPoint = cursor.position;
    }

    void UiWsPanel::OnCursorReleased(const Input::Cursor& cursor)
    {
        String hot = HotAt(cursor.position);
        String pressed = mPressedHot;
        mPressedHot = "";
        if (!pressed.IsEmpty() && hot == pressed)
            OnHotClick(hot, cursor);
    }

    void UiWsPanel::OnCursorMoved(const Input::Cursor& cursor)
    {
        mHover = HotAt(cursor.position);
        mHoverInside = true;
    }

    void UiWsPanel::OnCursorExit(const Input::Cursor& cursor)
    {
        mHover = "";
        mHoverInside = false;
    }

    void UiWsPanel::OnCursorDblClicked(const Input::Cursor& cursor)
    {
        String hot = HotAt(cursor.position);
        if (!hot.IsEmpty())
            OnHotDoubleClick(hot, cursor);
    }

    void UiWsPanel::Tool(const RectF& rect, const String& icon, const String& id, bool active /*= false*/, bool enabled /*= true*/,
                         float iconSize /*= 15.0f*/)
    {
        auto& t = T();
        bool hover = enabled && IsHover(id);
        if (active)
            Fill(rect, 5.0f, t.accentSoft);
        else if (hover)
            Fill(rect, 5.0f, t.active);

        Color4 color = active ? t.accent : hover ? t.fg : t.fgDim;
        if (!enabled)
            color = A(color, 0.35f);
        UiWsDraw::Icon(icon, rect.Center(), iconSize, color);
        if (enabled)
            AddHot(rect, id);
    }

    void UiWsPanel::Button(const RectF& rect, const String& caption, const String& id, bool primary /*= false*/,
                           bool enabled /*= true*/, const String& icon /*= ""*/)
    {
        auto& t = T();
        bool hover = enabled && IsHover(id);
        float alpha = enabled ? 1.0f : 0.45f;
        if (primary)
        {
            Fill(rect, 8.0f, A(hover ? t.accentHover : t.accent, alpha));
        }
        else
        {
            Fill(rect, 8.0f, A(hover ? t.active : t.field, alpha));
            Border(rect, 8.0f, A(t.line, alpha));
        }

        Color4 ink = primary ? t.accentInk : t.fg;
        ink = A(ink, alpha);
        float textW = TextWidth(caption, 12.0f, primary ? 600 : 400);
        float iconW = icon.IsEmpty() ? 0.0f : 13.0f + (caption.IsEmpty() ? 0.0f : 6.0f);
        float x = rect.Center().x - (textW + iconW)*0.5f;
        if (!icon.IsEmpty())
            UiWsDraw::Icon(icon, Vec2F(x + 6.5f, rect.Center().y), 13.0f, primary ? ink : A(t.fgDim, alpha));
        if (!caption.IsEmpty())
            Text(caption, RectF(x + iconW, rect.top, x + iconW + textW + 2.0f, rect.bottom), TextStyle(12.0f, primary ? 600 : 400, ink));
        if (enabled)
            AddHot(rect, id);
    }

    void UiWsPanel::Segments(const RectF& rect, const Vector<Pair<String, String>>& options, const String& value, const String& prefix,
                             bool enabled /*= true*/)
    {
        auto& t = T();
        float alpha = enabled ? 1.0f : 0.45f;
        Fill(rect, 8.0f, A(t.field, alpha));
        Border(rect, 8.0f, A(t.line, alpha));
        int n = Math::Max(1, options.Count());
        float inner = rect.Width() - 4.0f - (n - 1)*2.0f;
        float part = inner/n;
        for (int i = 0; i < options.Count(); i++)
        {
            float left = rect.left + 2.0f + i*(part + 2.0f);
            RectF r(left, rect.top - 2.0f, left + part, rect.bottom + 2.0f);
            String id = prefix + options[i].first;
            bool on = options[i].first == value;
            if (on)
                Fill(r, 6.0f, A(t.active, alpha));
            Color4 color = on || IsHover(id) ? t.fg : t.fgDim;
            Text(options[i].second, RectF(r.left + 3.0f, r.top, r.right - 3.0f, r.bottom), TextStyle(11.5f, 400, A(color, alpha), HorAlign::Middle));
            if (enabled)
                AddHot(r, id);
        }
    }

    void UiWsPanel::Separator(const Vec2F& center, float height /*= 16.0f*/)
    {
        Fill(RectF(center.x - 0.5f, center.y + height*0.5f, center.x + 0.5f, center.y - height*0.5f), 0.0f, T().line);
    }

    // --- the popover ---------------------------------------------------------------------------

    namespace
    {
        int openPopups = 0;
        int closedFrame = -10;
    }

    UiWsPopup::UiWsPopup(RefCounter* refCounter):
        PopupWidget(refCounter)
    {
        PushEditorScopeOnStack scope;
        fitByChildren = false;
        AddLayer("back", mmake<PipelineMenuBack>(), Layout::BothStretch(-2, -2, -2, -2));
        SetViewLayout(Layout::BothStretch(0, 0, 0, 0));
        SetClippingLayout(Layout::BothStretch(0, 0, 0, 0));
    }

    void UiWsPopup::OpenAt(const RectF& anchor, float width, float height, bool alignRight /*= false*/)
    {
        PushEditorScopeOnStack scope;
        mAnchor = anchor;
        mWidth = width;
        mHeight = height;
        mAlignRight = alignRight;
        PopupWidget::Show(Vec2F());
        Place();
    }

    void UiWsPopup::Resize(float width, float height)
    {
        mWidth = width;
        mHeight = height;
        if (IsOpen())
            Place();
    }

    void UiWsPopup::Close()
    {
        if (IsOpen())
            HideWithParent();
    }

    bool UiWsPopup::IsOpen() const
    {
        return IsEnabled();
    }

    bool UiWsPopup::ClosedThisFrame()
    {
        return closedFrame >= o2Time.GetCurrentFrame() - 1;
    }

    bool UiWsPopup::AnyOpen()
    {
        return openPopups > 0;
    }

    void UiWsPopup::SetContent(const Ref<Widget>& content)
    {
        PushEditorScopeOnStack scope;
        *content->layout = WidgetLayout::BothStretch();
        AddChild(content);
    }

    void UiWsPopup::FitSizeAndPosition(const Vec2F& position)
    {
        Place();
    }

    void UiWsPopup::OnEnabled()
    {
        PopupWidget::OnEnabled();
        openPopups++;
    }

    void UiWsPopup::OnDisabled()
    {
        PopupWidget::OnDisabled();
        openPopups = Math::Max(0, openPopups - 1);
        closedFrame = o2Time.GetCurrentFrame();
        if (onClosed)
            onClosed();
    }

    void UiWsPopup::Place()
    {
        Vec2F resolution = (Vec2F)o2Render.GetResolution();
        RectF screen(-resolution.x*0.5f, resolution.y*0.5f, resolution.x*0.5f, -resolution.y*0.5f);
        const float margin = 8.0f, gap = 6.0f;
        float height = Math::Min(mHeight, resolution.y - margin*2.0f);
        float below = mAnchor.bottom - gap - (screen.bottom + margin);
        float above = (screen.top - margin) - (mAnchor.top + gap);
        bool down = below >= height || below >= above;
        float left = mAlignRight ? mAnchor.right - mWidth : mAnchor.left;
        left = Math::Clamp(left, screen.left + margin, screen.right - margin - mWidth);
        float top = down ? mAnchor.bottom - gap : mAnchor.top + gap + height;
        top = Math::Min(top, screen.top - margin);
        top = Math::Max(top, screen.bottom + margin + height);
        layout->worldRect = RectF(Math::Round(left), Math::Round(top), Math::Round(left + mWidth), Math::Round(top - height));
        UpdateSelfTransform();
        UpdateChildrenTransforms();
    }

    // --- controls ---------------------------------------------------------------------------------

    // ----------------------------------------------------------------------------------------------
    // A number field of the workspace: its letter inside on the left, the placeholder after the letter
    // ----------------------------------------------------------------------------------------------
    class UiWsNumberBox : public PipelineField
    {
    public:
        float     pad = 8.0f;  // Where the text starts
        Ref<o2::Text> placeholder; // Shown while the field is empty and not focused

    public:
        explicit UiWsNumberBox(RefCounter* refCounter): PipelineField(refCounter) {}

        void Draw() override
        {
            EditBox::Draw();
            if (!placeholder || !mResEnabledInHierarchy || mIsClipped || !GetText().IsEmpty() || IsFocused())
                return;

            RectF rect = layout->GetWorldRect();
            placeholder->SetRect(RectF(rect.left + pad, rect.top, rect.right - 8.0f, rect.bottom));
            placeholder->transparency = GetResTransparency();
            placeholder->Draw();
        }
    };

    namespace UiWsControls
    {
        String NumberText(double value)
        {
            if (std::isnan(value))
                return "";
            if (value == std::floor(value) && std::fabs(value) < 1e15)
                return String(std::to_string((long long)value).c_str());
            return UiDoc::JsNum(std::round(value*10000.0)/10000.0);
        }

        // Number(text) of JavaScript for what a number field takes: NaN for anything else
        static double ParseNumber(const String& raw)
        {
            String text = raw.Trimed(" \t");
            if (text.IsEmpty())
                return NAN;

            char* end = nullptr;
            double v = std::strtod(text.Data(), &end);
            if (!end || *end != '\0' || !std::isfinite(v))
                return NAN;
            return v;
        }

        Ref<EditBox> NumberField(const String& letter, std::optional<double> value, const String& placeholder,
                                 const Function<void(std::optional<double>)>& onValue, std::optional<double> min /*= std::nullopt*/)
        {
            PushEditorScopeOnStack scope;
            auto& t = PipelineTheme::Get();
            auto field = mmake<UiWsNumberBox>();
            field->SetCaretBlinkingDelay(1.15f);
            field->SetMultiLine(false);
            field->layout->minSize = Vec2F(30, 20);
            PipelineControls::SkinField(field, false);
            field->SetText(value ? NumberText(*value) : String());
            field->layout->minHeight = 28.0f;
            if (auto text = field->GetTextDrawable())
            {
                text->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::MonoFont()));
                text->height = PipelineTheme::Size(12.0f);
            }

            float pad = 8.0f;
            if (!letter.IsEmpty())
            {
                auto mark = PipelineControls::MakeThemeText(letter, 10.0f, 400, t.fg3);
                mark->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::MonoFont()));
                mark->dotsEngings = false;
                field->AddLayer("letter", mark, Layout::BothStretch(8, 0, 0, 0), 1.0f);
                pad = 8.0f + UiWsDraw::TextWidth(letter, 10.0f, 400, true) + 6.0f;
            }
            field->SetViewLayout(Layout::BothStretch(pad, 0, 8, 0));
            field->pad = pad;
            if (!placeholder.IsEmpty())
            {
                field->placeholder = PipelineControls::MakeThemeText(placeholder, 12.0f, 400, t.fg3);
                field->placeholder->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::MonoFont()));
            }

            WeakRef<EditBox> weak(field);
            field->onChanged = [onValue, min](const WString& text)
            {
                String s = (String)text;
                if (s.Trimed(" \t").IsEmpty())
                {
                    if (onValue)
                        onValue(std::nullopt);
                    return;
                }
                double v = ParseNumber(s);
                if (!std::isnan(v) && (!min || v >= *min) && onValue)
                    onValue(v);
            };
            return field;
        }

        Ref<EditBox> LazyNumberField(const String& letter, std::optional<double> value, const String& placeholder,
                                     const Function<void(std::optional<double>)>& onValue, std::optional<double> min /*= std::nullopt*/)
        {
            auto field = NumberField(letter, value, placeholder, {}, min);
            field->onChanged = Function<void(const WString&)>();
            // Escape puts the text back before the field is left, so what is applied then is what was there
            field->onChangeCompleted = [onValue, min](const WString& text)
            {
                String s = (String)text;
                if (s.Trimed(" \t").IsEmpty())
                {
                    if (onValue)
                        onValue(std::nullopt);
                    return;
                }
                double v = ParseNumber(s);
                if (!std::isnan(v) && (!min || v >= *min) && onValue)
                    onValue(v);
            };
            return field;
        }

        void SetNumber(const Ref<EditBox>& field, std::optional<double> value)
        {
            if (!field || field->IsFocused())
                return;

            WString text = value ? (WString)NumberText(*value) : WString();
            if (field->GetText() != text)
                SetTextQuiet(field, text);
        }

        void SetTextQuiet(const Ref<EditBox>& field, const String& text)
        {
            if (!field)
                return;

            auto changed = field->onChanged;
            auto completed = field->onChangeCompleted;
            field->onChanged = Function<void(const WString&)>();
            field->onChangeCompleted = Function<void(const WString&)>();
            field->SetText(text);
            field->onChanged = changed;
            field->onChangeCompleted = completed;
        }

        Ref<EditBox> TextField(const String& text, const String& placeholder, bool multiline,
                               const Function<void(const String&)>& onText, const Function<void(const String&)>& onDone /*= {}*/)
        {
            PushEditorScopeOnStack scope;
            auto field = PipelineControls::MakeEditBox(text, multiline, placeholder);
            field->layout->minHeight = 28.0f;
            if (auto drawable = field->GetTextDrawable())
                drawable->height = PipelineTheme::Size(12.0f);
            if (onText)
                field->onChanged = [onText](const WString& t) { onText((String)t); };
            if (onDone)
                field->onChangeCompleted = [onDone](const WString& t) { onDone((String)t); };
            return field;
        }
    }
}
