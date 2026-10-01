#include "o2Editor/stdafx.h"
#include "PipelineOptionPicker.h"

#include "o2/Animation/AnimationClip.h"
#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/System/Time/Time.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"

namespace Editor
{
    static const float padding = 6.0f;
    static const float rowPadding = 4.0f;
    static const float labelHeight = 18.0f;
    static const float iconColumn = 28.0f;
    static const float checkColumn = 26.0f;
    static const int   hintFontHeight = 10;
    static const float minMenuWidth = 300.0f;
    static const float maxMenuWidth = 420.0f;
    static const float screenMargin = 8.0f;
    static const float fieldGap = 4.0f;

    static Ref<Text> MakeText(const String& text, const Color4& color, int height = 11)
    {
        auto drawable = mmake<Text>("stdFont.ttf");
        drawable->text = text;
        drawable->color = color;
        drawable->horAlign = HorAlign::Left;
        drawable->verAlign = VerAlign::Middle;
        drawable->wordWrap = false;
        drawable->dotsEngings = true;
        drawable->height = height;
        return drawable;
    }

    PipelineOptionPicker::PipelineOptionPicker(RefCounter* refCounter):
        PopupWidget(refCounter)
    {
        PushEditorScopeOnStack scope;
        fitByChildren = false;
        AddLayer("back", mmake<Sprite>("ui/UI4_Context_menu.png"), Layout::BothStretch(-20, -19, -20, -19));
        SetViewLayout(Layout::BothStretch(0, 0, 0, 0));
        SetClippingLayout(Layout::BothStretch(0, 0, 0, 0));
        mMeasure = mmake<Text>("stdFont.ttf");
    }

    void PipelineOptionPicker::Open(const PipelineOptionPickerRequest& request)
    {
        PushEditorScopeOnStack scope;

        mRequest = request;
        mClosedByField = nullptr;
        mClosedByFieldTime = -1.0f;

        Vec2F resolution = (Vec2F)o2Render.GetResolution();
        mWidth = Math::Min(resolution.x - screenMargin*2.0f, Math::Clamp(request.anchor.Width(), minMenuWidth, maxMenuWidth));
        RebuildRows();

        // Below the field when the menu fits there or there is more room below than above
        RectF screen(-resolution.x*0.5f, resolution.y*0.5f, resolution.x*0.5f, -resolution.y*0.5f);
        float needed = mContentHeight + padding*2.0f;
        float below = request.anchor.bottom - fieldGap - (screen.bottom + screenMargin);
        float above = (screen.top - screenMargin) - (request.anchor.top + fieldGap);
        mOpensDown = below >= needed || below >= above;

        PopupWidget::Show(Vec2F());

        mHighlight = Math::Max(0, request.options.IndexOf([&](const PipelineOption& o) { return o.value == request.current; }));
        UpdateHighlight();

        // The keys belong to the menu while it is open, not to the canvas shortcuts
        o2UI.FocusWidget(nullptr);
    }

    void PipelineOptionPicker::Close()
    {
        if (IsOpen())
            HideWithParent();
    }

    bool PipelineOptionPicker::IsOpen() const
    {
        return IsEnabled();
    }

    bool PipelineOptionPicker::IsOpenFor(const Ref<Widget>& field) const
    {
        return IsOpen() && field && mRequest.field.Lock() == field;
    }

    bool PipelineOptionPicker::ConsumeClosedByField(const Ref<Widget>& field)
    {
        // The click ends the press that closed the menu: it follows within moments, never after a new press
        bool result = field && mClosedByField.Lock() == field && mClosedByFieldTime >= 0.0f &&
            o2Time.GetApplicationTime() - mClosedByFieldTime < 2.0f;

        mClosedByField = nullptr;
        mClosedByFieldTime = -1.0f;
        return result;
    }

    void PipelineOptionPicker::Pick(const String& value)
    {
        if (value.IsEmpty())
            return;

        auto request = mRequest;
        Close();
        if (request.onPick)
            request.onPick(value);
    }

    void PipelineOptionPicker::PickHighlighted()
    {
        Pick(GetHighlightedValue());
    }

    void PipelineOptionPicker::MoveHighlight(int step)
    {
        int count = mRows.Count();
        if (count == 0)
            return;

        mHighlight = ((mHighlight + step) % count + count) % count;
        UpdateHighlight();
    }

    String PipelineOptionPicker::GetHighlightedValue() const
    {
        return mHighlight >= 0 && mHighlight < mRequest.options.Count() ? mRequest.options[mHighlight].value : String();
    }

    Ref<Button> PipelineOptionPicker::FindRow(const String& value) const
    {
        int index = mRequest.options.IndexOf([&](const PipelineOption& o) { return o.value == value; });
        return index >= 0 && index < mRows.Count() ? mRows[index] : nullptr;
    }

    void PipelineOptionPicker::Update(float dt)
    {
        if (mResEnabledInHierarchy)
        {
            Vec2F cursor = o2Input.GetCursorPos();
            bool inside = layout->IsPointInside(cursor);

            // A press on the field closes the menu here; the click it ends must not open it again
            bool pressed = o2Input.IsCursorPressed() || o2Input.IsRightMousePressed();
            if (pressed && !inside && !mShownAtFrame)
            {
                mClosedByField = mRequest.field;
                mClosedByFieldTime = mRequest.anchor.IsInside(cursor) ? o2Time.GetApplicationTime() : -1.0f;
            }

            // The canvas zooms under a wheel outside, and the menu would stay behind its field
            if (!inside && !mShownAtFrame && Math::Abs(o2Input.GetMouseWheelDelta()) > 0.1f)
            {
                Close();
                return;
            }

            if (o2Input.GetCursorDelta() != Vec2F() && inside)
            {
                for (int i = 0; i < mRows.Count(); i++)
                {
                    if (mRows[i]->layout->IsPointInside(cursor) && mHighlight != i)
                    {
                        mHighlight = i;
                        UpdateHighlight();
                        break;
                    }
                }
            }
        }

        PopupWidget::Update(dt);
    }

    void PipelineOptionPicker::OnKeyPressed(const Input::Key& key)
    {
        if (!IsOpen())
            return;

        if (key.keyCode == VK_ESCAPE)
            Close();
        else if (key.keyCode == VK_UP)
            MoveHighlight(-1);
        else if (key.keyCode == VK_DOWN)
            MoveHighlight(1);
        else if (key.keyCode == VK_RETURN)
            PickHighlighted();
    }

    void PipelineOptionPicker::OnKeyStayDown(const Input::Key& key)
    {
        if (!IsOpen() || !o2Input.IsKeyRepeating(key.keyCode))
            return;

        if (key.keyCode == VK_UP)
            MoveHighlight(-1);
        else if (key.keyCode == VK_DOWN)
            MoveHighlight(1);
    }

    void PipelineOptionPicker::OnDisabled()
    {
        PopupWidget::OnDisabled();

        // Back to the canvas, never to the field: the Enter that picked would press the focused field and open the menu again
        auto owner = GetParentWidget().Lock();
        if (!o2UI.GetFocusedWidget() && owner && owner->IsFocusable())
            o2UI.FocusWidget(owner);
    }

    void PipelineOptionPicker::OnCursorReleasedOutside(const Input::Cursor& cursor)
    {}

    void PipelineOptionPicker::FitSizeAndPosition(const Vec2F& position)
    {
        Place();
    }

    float PipelineOptionPicker::MeasureHint(const String& text, float width) const
    {
        if (text.IsEmpty())
            return 0.0f;

        // Glyphs the font has not rasterized yet measure too narrow
        mMeasure->GetFont()->CheckCharacters(text, hintFontHeight);
        return Text::GetTextSize(text, mMeasure->GetFont(), hintFontHeight, Vec2F(width, 0.0f), HorAlign::Left, VerAlign::Top, true).y;
    }

    void PipelineOptionPicker::RebuildRows()
    {
        PushEditorScopeOnStack scope;

        for (auto& row : mRows)
            RemoveChild(row);
        mRows.Clear();

        float width = mWidth - padding*2.0f;
        float hintWidth = width - iconColumn - checkColumn;
        float y = padding;
        for (auto& option : mRequest.options)
        {
            float hintHeight = Math::Ceil(MeasureHint(option.hint, hintWidth));
            float height = rowPadding*2.0f + labelHeight + hintHeight;
            auto row = MakeRow(option, width, height, hintHeight);
            *row->layout = WidgetLayout::HorStretch(VerAlign::Top, padding, padding, height, y);
            AddChild(row);
            mRows.Add(row);
            y += height;
        }
        mContentHeight = y - padding;
    }

    Ref<Button> PipelineOptionPicker::MakeRow(const PipelineOption& option, float width, float height, float hintHeight)
    {
        bool current = option.value == mRequest.current;
        auto row = mmake<Button>();
        row->name = option.value;
        row->AddLayer("select", mmake<Sprite>("ui/UI4_Context_menu_select.png"), Layout::BothStretch(-10, -16, -10, -16))->transparency = 0.0f;

        auto icon = mmake<Sprite>(option.icon);
        icon->color = current ? PipelineControls::accentColor : PipelineControls::dimTextColor;
        float iconTop = -rowPadding - (labelHeight - 16.0f)*0.5f;
        row->AddLayer("icon", icon, Layout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(6, iconTop - 16.0f), Vec2F(22, iconTop)));

        row->AddLayer("label", MakeText(option.label, current ? PipelineControls::accentColor : PipelineControls::textColor),
                      Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(iconColumn, -rowPadding - labelHeight), Vec2F(-checkColumn, -rowPadding)));

        auto hint = MakeText(option.hint, PipelineControls::dimTextColor, hintFontHeight);
        hint->wordWrap = true;
        hint->dotsEngings = false;
        hint->verAlign = VerAlign::Top;
        row->AddLayer("hint", hint, Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(iconColumn, -rowPadding - labelHeight - hintHeight),
                                           Vec2F(-checkColumn, -rowPadding - labelHeight)));

        float checkTop = -rowPadding - (labelHeight - 20.0f)*0.5f;
        row->AddLayer("check", mmake<Sprite>("ui/UI4_Ckeck.png"), Layout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-24, checkTop - 20.0f), Vec2F(-4, checkTop)))
            ->transparency = current ? 1.0f : 0.0f;

        String value = option.value;
        row->onClick = [this, value]() { Pick(value); };
        return row;
    }

    void PipelineOptionPicker::Place()
    {
        Vec2F resolution = (Vec2F)o2Render.GetResolution();
        RectF screen(-resolution.x*0.5f, resolution.y*0.5f, resolution.x*0.5f, -resolution.y*0.5f);
        const RectF& anchor = mRequest.anchor;

        float height = Math::Min(mContentHeight + padding*2.0f, resolution.y - screenMargin*2.0f);
        float left = Math::Clamp(anchor.left, screen.left + screenMargin, screen.right - screenMargin - mWidth);
        float top = mOpensDown ? anchor.bottom - fieldGap : anchor.top + fieldGap + height;
        top = Math::Min(top, screen.top - screenMargin);
        top = Math::Max(top, screen.bottom + screenMargin + height);

        layout->worldRect = RectF(Math::Round(left), Math::Round(top), Math::Round(left + mWidth), Math::Round(top - height));
        UpdateSelfTransform();
        UpdateChildrenTransforms();
    }

    void PipelineOptionPicker::UpdateHighlight()
    {
        for (int i = 0; i < mRows.Count(); i++)
        {
            if (auto select = mRows[i]->FindLayer("select"))
                select->transparency = i == mHighlight ? 1.0f : 0.0f;
        }
    }

    namespace PipelineControls
    {
        Ref<Button> MakeOptionField()
        {
            // The layers of the model field, with the option's icon before its name
            auto field = mmake<Button>();
            field->name = "option";
            field->layout->minSize = Vec2F(20, 20);
            field->AddLayer("back", mmake<Sprite>("ui/UI4_Editbox_regular.png"), Layout::BothStretch(-9, -9, -9, -9));
            field->AddLayer("hover", mmake<Sprite>("ui/UI4_Editbox_select.png"), Layout::BothStretch(-9, -9, -9, -9))->transparency = 0.0f;
            field->AddLayer("pressed", mmake<Sprite>("ui/UI4_Editbox_pressed.png"), Layout::BothStretch(-9, -9, -9, -9))->transparency = 0.0f;
            field->AddLayer("arrow", mmake<Sprite>("ui/UI4_Down_icn.png"), Layout(Vec2F(1.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(-20, -10), Vec2F(0, 10)));
            auto icon = mmake<Sprite>();
            icon->color = accentColor;
            field->AddLayer("icon", icon, Layout(Vec2F(0.0f, 0.5f), Vec2F(0.0f, 0.5f), Vec2F(5, -7), Vec2F(19, 7)));
            field->AddLayer("caption", MakeText("", textColor), Layout::BothStretch(24, 0, 20, 0));

            field->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.05f))->offStateAnimationSpeed = 0.5f;
            field->AddState("pressed", AnimationClip::EaseInOut("layer/pressed/transparency", 0.0f, 1.0f, 0.05f))->offStateAnimationSpeed = 0.5f;
            return field;
        }

        void SetOptionFieldValue(const Ref<Button>& field, const PipelineOption& option)
        {
            if (!field)
                return;

            if (auto caption = field->GetLayerDrawable<Text>("caption"))
                caption->text = option.label;
            if (auto icon = field->GetLayerDrawable<Sprite>("icon"))
            {
                icon->imageName = option.icon;
                icon->color = accentColor;
            }
            field->SetLayoutDirty();
        }
    }
}
// --- META ---

DECLARE_CLASS(Editor::PipelineOptionPicker, Editor__PipelineOptionPicker);
// --- END META ---
