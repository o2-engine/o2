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
        SetValueChangeAppliedByAction(true);

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

    void WStringProperty::TransformByUser(const Function<WString(const WString&)>& transform)
    {
        if (mValuesProxies.IsEmpty())
            return;

        Vector<WString> values;
        for (auto& ptr : mValuesProxies)
            values.Add(transform(GetProxy(ptr.first)));

        Vector<DataDocument> before, after;
        StoreValues(before);
        for (auto& value : values)
        {
            after.Add(DataDocument());
            after.Last() = value;
        }

        if (before == after)
            return;

        if (mApplyChangeWithAction && !onChangeCompleted.IsEmpty())
        {
            onBeforeChange(Ref(this), true);

            mCommonValue = values[0];
            mValuesDifferent = values.Any([&](const WString& x) { return x != values[0]; });

            UpdateValueView();
            OnValueChanged(true);

            onChangeCompleted(mValuesPath, before, after);
        }
        else
        {
            mBeforeChangeValues = before;
            for (int i = 0; i < mValuesProxies.Count(); i++)
                SetProxy(mValuesProxies[i].first, values[i]);

            Refresh(true);
            CheckValueChangeCompleted();
        }
    }

    Vector<WString> WStringProperty::GetValues() const
    {
        Vector<WString> res;
        for (auto& ptr : mValuesProxies)
            res.Add(GetProxy(ptr.first));

        return res;
    }

    void WStringProperty::OnEdited(const WString& data)
    {
        if (mValuesDifferent && data == "--")
            return;

        SetValueByUserAndComplete(data);
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::WString>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::WStringProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::WString>>);
// --- META ---

DECLARE_CLASS(Editor::WStringProperty, Editor__WStringProperty);
// --- END META ---
