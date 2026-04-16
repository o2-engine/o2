#include "o2/stdafx.h"
#include "Toggle.h"

#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"

namespace o2
{
    Toggle::Toggle(RefCounter* refCounter):
        Widget(refCounter), DrawableCursorEventsListener(this)
    {}

    Toggle::Toggle(RefCounter* refCounter, const Toggle& other) :
        Widget(refCounter, other), DrawableCursorEventsListener(this), mToggleGroup(nullptr)
    {
        mCaptionText = GetLayerDrawable<Text>("caption");
        mBackLayer = FindLayer("back");

        RetargetStatesAnimations();

        mValue = !other.mValue;
        SetValue(other.mValue);

        if (other.IsValueUnknown())
            SetValueUnknown();
    }

    Toggle& Toggle::operator=(const Toggle& other)
    {
        Widget::operator=(other);

        mCaptionText = GetLayerDrawable<Text>("caption");
        mBackLayer = FindLayer("back");

        mValue = !other.mValue;
        SetValue(other.mValue);

        if (other.IsValueUnknown())
            SetValueUnknown();

        RetargetStatesAnimations();

        return *this;
    }

    Toggle::~Toggle()
    {}

    void Toggle::Update(float dt)
    {
        PROFILE_SAMPLE_FUNC();

        Widget::Update(dt);

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        if (mToggleGroup && mToggleGroup->mPressed && mToggleGroup->mPressedValue != mValue &&
            (mToggleGroup->mType == ToggleGroup::Type::VerOneClick || mToggleGroup->mType == ToggleGroup::Type::HorOneClick))
        {
            Vec2F cursor = o2Input.GetCursorPos();
            bool underPoint = false;

            if (mToggleGroup->mType == ToggleGroup::Type::VerOneClick)
                underPoint = cursor.y > layout->worldBottom && cursor.y < layout->worldTop;
            else
                underPoint = cursor.x > layout->worldLeft && cursor.x < layout->worldRight;

            if (underPoint)
            {
                SetValue(!mValue);
                onClick();
                onToggleByUser(mValue);
                mToggleGroup->mToggled.Add(Ref(this));
            }
        }
    }

    void Toggle::SetCaption(const WString& text)
    {
        if (mCaptionText)
            mCaptionText->SetText(text);
    }

    WString Toggle::GetCaption() const
    {
        if (mCaptionText)
            return mCaptionText->GetText();

        return WString();
    }

    void Toggle::SetValue(bool value)
    {
        if (mValue == value && !mValueUnknown)
            return;

        mValue = value;
        mValueUnknown = false;

        auto valueState = state["value"];
        if (valueState)
            *valueState = mValue;

        auto unknownState = state["unknown"];
        if (unknownState)
            *unknownState = false;

        onToggle(mValue);

        if (mToggleGroup)
            mToggleGroup->OnToggled(Ref(this));
    }

    void Toggle::SetValueUnknown()
    {
        mValueUnknown = true;

        auto unknownState = state["unknown"];
        if (unknownState)
            *unknownState = true;

        auto valueState = state["value"];
        if (valueState)
            *valueState = false;
    }

    bool Toggle::IsValueUnknown() const
    {
        return mValueUnknown;
    }

    bool Toggle::GetValue() const
    {
        return mValue;
    }

    bool Toggle::IsFocusable() const
    {
        return true;
    }

    void Toggle::SetToggleGroup(const Ref<ToggleGroup>& toggleGroup)
    {
        if (mToggleGroup == toggleGroup)
            return;

        if (mToggleGroup)
            mToggleGroup->mToggles.Remove(Ref(this));

        mToggleGroup = toggleGroup;
        mToggleGroup->mToggles.Add(Ref(this));
    }

    const Ref<ToggleGroup>& Toggle::GetToggleGroup() const
    {
        return mToggleGroup;
    }

    void Toggle::OnCursorPressed(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = true;

        o2UI.FocusWidget(Ref(this));

        if (mToggleGroup)
        {
            SetValue(!mValue);
            onClick();

            mToggleGroup->mPressed = true;
            mToggleGroup->mPressedValue = mValue;
            mToggleGroup->mToggled.Clear();
            mToggleGroup->mToggled.Add(Ref(this));
            mToggleGroup->onPressed(mValue);
        }
    }

    void Toggle::OnCursorReleased(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;

        if (Widget::IsUnderPoint(cursor.position) &&
            !(mToggleGroup && (mToggleGroup->mType == ToggleGroup::Type::VerOneClick ||
            mToggleGroup->mType == ToggleGroup::Type::HorOneClick) &&
            mToggleGroup->mPressed))
        {
            SetValue(!mValue);
            onClick();
            onToggleByUser(mValue);
        }

        if (mToggleGroup && (mToggleGroup->mType == ToggleGroup::Type::VerOneClick ||
            mToggleGroup->mType == ToggleGroup::Type::HorOneClick))
        {
            mToggleGroup->mPressed = false;
            mToggleGroup->onReleased(mValue);
        }
    }

    void Toggle::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        auto pressedState = state["pressed"];
        if (pressedState)
            *pressedState = false;

        if (mToggleGroup && (mToggleGroup->mType == ToggleGroup::Type::VerOneClick ||
            mToggleGroup->mType == ToggleGroup::Type::HorOneClick))
        {
            mToggleGroup->mPressed = false;
            mToggleGroup->onReleased(mValue);
        }
    }

    void Toggle::OnCursorEnter(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = true;
    }

    void Toggle::OnCursorExit(const Input::Cursor& cursor)
    {
        auto selectState = state["hover"];
        if (selectState)
            *selectState = false;
    }

    void Toggle::OnKeyPressed(const Input::Key& key)
    {
        if (mIsFocused && (key.keyCode == VK_SPACE || key.keyCode == VK_RETURN))
        {
            auto pressedState = state["pressed"];
            if (pressedState)
                *pressedState = true;
        }

        bool isFocusedEditBox = o2UI.GetFocusedWidget() && o2UI.GetFocusedWidget()->GetType() == TypeOf(EditBox);
        if (shortcut.IsPressed() && !isFocusedEditBox)
        {
            SetValue(!mValue);
            onClick();
            onToggleByUser(mValue);
        }
    }

    void Toggle::OnKeyReleased(const Input::Key& key)
    {
        if (mIsFocused && (key.keyCode == VK_SPACE || key.keyCode == VK_RETURN))
        {
            auto pressedState = state["pressed"];
            if (pressedState)
                *pressedState = false;

            SetValue(!mValue);
            onClick();
            onToggleByUser(mValue);
        }

    }

    void Toggle::OnDeserialized(const DataValue& node)
    {
        Widget::OnDeserialized(node);

        mCaptionText = GetLayerDrawable<Text>("caption");
        mBackLayer = FindLayer("back");

        RetargetStatesAnimations();
    }

    void Toggle::OnLayerAdded(const Ref<WidgetLayer>& layer)
    {
        if (layer->name == "caption" && layer->GetDrawable() && layer->GetDrawable()->GetType() == TypeOf(Text))
            mCaptionText = DynamicCast<Text>(layer->GetDrawable());

        if (layer->name == "back")
            mBackLayer = layer;
    }

    String Toggle::GetCreateMenuGroup()
    {
        return "Basic";
    }

    void Toggle::OnEnabled()
    {
        Widget::OnEnabled();

        interactable = true;
    }

    void Toggle::OnDisabled()
    {
        Widget::OnDisabled();

        interactable = false;
    }

    ToggleGroup::ToggleGroup(Type type):
        mType(type), mPressed(false)
    {}

    ToggleGroup::~ToggleGroup()
    {}

    void ToggleGroup::AddToggle(const Ref<Toggle>& toggle)
    {
        mToggles.Add(toggle);
        toggle->mToggleGroup = Ref(this);
        toggle->SetValue(true);
    }

    void ToggleGroup::RemoveToggle(Toggle* toggle)
    {
        mToggles.RemoveFirst([&](auto& x) { return x == toggle; });
        toggle->mToggleGroup = nullptr;
    }

    const Vector<WeakRef<Toggle>>& ToggleGroup::GetToggles() const
    {
        return mToggles;
    }

    const Vector<WeakRef<Toggle>>& ToggleGroup::GetToggled() const
    {
        return mToggled;
    }

    void ToggleGroup::OnToggled(const Ref<Toggle>& toggle)
    {
        if (mType == Type::OnlySingleTrue)
        {
            if (toggle->GetValue())
            {
                for (auto& ctoggle : mToggles)
                {
                    if (ctoggle == toggle)
                        continue;

                    ctoggle.Lock()->SetValue(false);
                }
            }
            else
            {
                if (!mToggles.Any([&](auto& x) { return x.Lock()->GetValue(); }))
                    toggle->SetValue(true);
            }
        }
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::Toggle>);
// --- META ---

ENUM_META(o2::ToggleGroup::Type, o2__ToggleGroup__Type)
{
    ENUM_ENTRY(HorOneClick);
    ENUM_ENTRY(OnlySingleTrue);
    ENUM_ENTRY(VerOneClick);
}
END_ENUM_META;

DECLARE_CLASS(o2::Toggle, o2__Toggle);
// --- END META ---
