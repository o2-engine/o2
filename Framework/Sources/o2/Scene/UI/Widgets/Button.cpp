#include "o2/stdafx.h"
#include "Button.h"

#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetState.h"

namespace o2
{
    Button::Button(RefCounter* refCounter):
        Widget(refCounter), CursorAreaEventsListener()
    {}

    Button::Button(RefCounter* refCounter, const Button& other) :
        Widget(refCounter, other)
    {
        mCaptionText = GetLayerDrawable<Text>("caption");
        mIconSprite = GetLayerDrawable<Sprite>("icon");

        RetargetStatesAnimations();

        if (mInteractable)
            OnBecomeInteractable();
        else
            OnBecomeNotInteractable();
    }

    Button& Button::operator=(const Button& other)
    {
        Widget::operator=(other);

        mCaptionText = GetLayerDrawable<Text>("caption");
        mIconSprite = GetLayerDrawable<Sprite>("icon");
        RetargetStatesAnimations();

        return *this;
    }

    void Button::Draw()
    {
        Widget::Draw();
        CursorAreaEventsListener::OnDrawn();
    }

    void Button::SetCaption(const WString& text)
    {
        if (mCaptionText)
            mCaptionText.Lock()->SetText(text);
    }

    WString Button::GetCaption() const
    {
        if (mCaptionText)
            return mCaptionText.Lock()->GetText();

        return WString();
    }

    void Button::SetIcon(const Ref<Sprite>& sprite)
    {
        if (mIconSprite)
            mIconSprite = sprite;
    }

    Ref<Sprite> Button::GetIcon() const
    {
        if (mIconSprite)
            return mIconSprite.Lock();

        return nullptr;
    }

    bool Button::IsFocusable() const
    {
        return true;
    }

    bool Button::IsUnderPoint(const Vec2F& point)
    {
        if (isPointInside.IsEmpty())
            return Widget::IsUnderPoint(point);
        
        return mDrawingScissorRect.IsInside(point) && isPointInside(point);
    }

    String Button::GetCreateMenuGroup()
    {
        return "Basic";
    }

    void Button::OnCursorPressed(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = true;

        o2UI.FocusWidget(Ref(this));
    }

    void Button::OnCursorReleased(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;

        if (IsUnderPoint(cursor.position))
            onClick();
    }

    void Button::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;
    }

    void Button::OnCursorEnter(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = true;
    }

    void Button::OnCursorExit(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = false;
    }

    void Button::OnKeyPressed(const Input::Key& key)
    {
        if (mIsFocused && (key.keyCode == VK_SPACE || key.keyCode == VK_RETURN))
        {
            auto pressedState = state["pressed"];
            if (pressedState)
                *pressedState = true;
        }

        if (shortcut.IsPressed())
            onClick();
    }

    void Button::OnKeyReleased(const Input::Key& key)
    {
        if (mIsFocused && (key.keyCode == VK_SPACE || key.keyCode == VK_RETURN))
        {
            auto pressedState = state["pressed"];
            if (pressedState)
                *pressedState = false;

            onClick();
        }
    }

    void Button::OnLayerAdded(const Ref<WidgetLayer>& layer)
    {
        if (layer->name == "caption" && layer->GetDrawable() && layer->GetDrawable()->GetType() == TypeOf(Text))
            mCaptionText = DynamicCast<Text>(layer->GetDrawable());

        if (layer->name == "icon" && layer->GetDrawable() && layer->GetDrawable()->GetType() == TypeOf(Sprite))
            mIconSprite = DynamicCast<Sprite>(layer->GetDrawable());
    }

    void Button::OnEnabled()
    {
        Widget::OnEnabled();

        interactable = true;
    }

    void Button::OnDisabled()
    {
        Widget::OnDisabled();

        interactable = false;
    }

    void Button::OnBecomeInteractable()
    {
        if (auto inactiveState = state["inactive"])
            *inactiveState = false;
    }

    void Button::OnBecomeNotInteractable()
    {
        if (auto inactiveState = state["inactive"])
            *inactiveState = true;
    }

}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::Button>);
// --- META ---

DECLARE_CLASS(o2::Button, o2__Button);
// --- END META ---
