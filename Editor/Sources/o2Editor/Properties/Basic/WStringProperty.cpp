#include "o2Editor/stdafx.h"
#include "WStringProperty.h"

#include "o2/Scene/UI/Widgets/EditBox.h"

namespace Editor
{
    WStringProperty::WStringProperty(RefCounter* refCounter):
        TPropertyField<WString>(refCounter)
    {}

    WStringProperty::WStringProperty(RefCounter* refCounter, const WStringProperty& other):
        TPropertyField<WString>(refCounter, other)
    {
        InitializeControls();
    }

    WStringProperty& WStringProperty::operator=(const WStringProperty& other)
    {
        TPropertyField<WString>::operator=(other);
        InitializeControls();
        return *this;
    }

    void WStringProperty::InitializeControls()
    {
        mEditBox = FindChildByType<EditBox>();
        if (mEditBox)
        {
            mEditBox->onChangeCompleted = THIS_FUNC(OnEdited);
            mEditBox->text = "--";
        }
    }

    void WStringProperty::UpdateValueView()
    {
        if (mValuesDifferent)
            mEditBox->text = "--";
        else
            mEditBox->text = mCommonValue;
    }

    void WStringProperty::OnEdited(const WString& data)
    {
        if (mValuesDifferent && data == "--")
            return;

        SetValueByUser(data);
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::WString>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::WStringProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::WString>>);
// --- META ---

DECLARE_CLASS(Editor::WStringProperty, Editor__WStringProperty);
// --- END META ---
