#include "o2Editor/stdafx.h"
#include "IntegerProperty.h"

#include "o2/Application/Application.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/Widgets/EditBox.h"

namespace Editor
{
    IntegerProperty::IntegerProperty(RefCounter* refCounter):
        TPropertyField<int>(refCounter)
    {}

    IntegerProperty::IntegerProperty(RefCounter* refCounter, const IntegerProperty& other) :
        TPropertyField<int>(refCounter, other)
    {
        InitializeControls();
    }

    IntegerProperty& IntegerProperty::operator=(const IntegerProperty& other)
    {
        TPropertyField<int>::operator=(other);
        InitializeControls();
        return *this;
    }

    void IntegerProperty::InitializeControls()
    {
        SetValueChangeAppliedByAction(true);

        mEditBox = FindChildByType<EditBox>();
        if (mEditBox)
        {
            mEditBox->onChangeCompleted = THIS_FUNC(OnEdited);
            mEditBox->text = "--";
            mEditBox->SetFilterInteger();

            auto handleLayer = mEditBox->FindLayer("arrows");
            if (handleLayer)
            {
                mEditBox->onDraw += [&]() { mDragHangle->OnDrawn(); };

                mDragHangle = mmake<CursorEventsArea>();
                mDragHangle->cursorType = CursorType::SizeNS;
                mDragHangle->isUnderPoint = [=](const Vec2F& point) { return handleLayer->IsUnderPoint(point); };
                mDragHangle->onMoved = THIS_FUNC(OnDragHandleMoved);
                mDragHangle->onCursorPressed = THIS_FUNC(OnMoveHandlePressed);
                mDragHangle->onCursorReleased = THIS_FUNC(OnMoveHandleReleased);
            }
        }
    }

    void IntegerProperty::UpdateValueView()
    {
        if (mValuesDifferent)
            mEditBox->text = "--";
        else
            mEditBox->text = (WString)mCommonValue;
    }

    void IntegerProperty::OnEdited(const WString& data)
    {
        if (mValuesDifferent && data == "--")
            return;

        SetValueByUserAndComplete((const int)data);
    }

    void IntegerProperty::OnDragHandleMoved(const Input::Cursor& cursor)
    {
        SetValue(mCommonValue + (int)cursor.delta.y, true);
    }

    void IntegerProperty::OnKeyReleased(const Input::Key& key)
    {
        if (!mEditBox)
            return;

        if (mEditBox && !mEditBox->IsFocused())
            return;

        if (key == VK_UP)
        {
            SetValueByUserAndComplete(mCommonValue + 1);
            mEditBox->SelectAll();
        }

        if (key == VK_DOWN)
        {
            SetValueByUserAndComplete(mCommonValue - 1);
            mEditBox->SelectAll();
        }
    }

    void IntegerProperty::OnMoveHandlePressed(const Input::Cursor& cursor)
    {
		BeginUserChanging();
        o2Application.SetCursorInfiniteMode(true);
    }

    void IntegerProperty::OnMoveHandleReleased(const Input::Cursor& cursor)
    {
        o2Application.SetCursorInfiniteMode(false);
        EndUserChanging();
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<int>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::IntegerProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<int>>);
// --- META ---

DECLARE_CLASS(Editor::IntegerProperty, Editor__IntegerProperty);
// --- END META ---
