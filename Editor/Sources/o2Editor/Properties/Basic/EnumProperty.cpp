#include "o2Editor/stdafx.h"
#include "EnumProperty.h"

#include "o2/Scene/UI/Widgets/DropDown.h"

namespace Editor
{
    EnumProperty::EnumProperty(RefCounter* refCounter):
        TPropertyField<int>(refCounter)
    {}

    EnumProperty::EnumProperty(RefCounter* refCounter, const EnumProperty& other) :
        TPropertyField<int>(refCounter, other)
    {
        InitializeControls();
    }

    EnumProperty& EnumProperty::operator=(const EnumProperty& other)
    {
        TPropertyField<int>::operator=(other);
        InitializeControls();
        return *this;
    }

    void EnumProperty::InitializeControls()
    {
        SetValueChangeAppliedByAction(true);

        mDropDown = FindChildByType<DropDown>();
        if (mDropDown)
        {
            mDropDown->onSelectedText = THIS_FUNC(OnSelectedItem);
            mDropDown->SetState("undefined", true);
        }
    }

    const Type* EnumProperty::GetValueType() const
    {
        return mEnumType;
    }

    void EnumProperty::SpecializeType(const Type* type)
    {
        if (type->GetUsage() == Type::Usage::Property)
            mEnumType = dynamic_cast<const EnumType*>(((const PropertyType*)type)->GetValueType());
        else
            mEnumType = dynamic_cast<const EnumType*>(type);

        if (mEnumType)
        {
            mEntries = mEnumType->GetEntries();

            if (mDropDown)
            {
                for (auto& kv : mEntries)
                    mDropDown->AddItem(kv.second);
            }
        }
    }

    void EnumProperty::StoreValues(Vector<DataDocument>& data) const
    {
        data.Clear();
        for (auto& ptr : mValuesProxies)
        {
            data.Add(DataDocument());
            StoreEnumValue(data.Last(), GetProxy(ptr.first));
        }
    }

    void EnumProperty::StoreValuesOfValue(Vector<DataDocument>& data, const int& value) const
    {
        data.Clear();
        for (auto& ptr : mValuesProxies)
        {
            data.Add(DataDocument());
            StoreEnumValue(data.Last(), value);
        }
    }

    void EnumProperty::StoreEnumValue(DataDocument& data, int value) const
    {
        // The name matches the enum field serialization; unspecialized fields (no entries
        // known) fall back to the raw int, which the enum converter also accepts
        if (mEntries.ContainsKey(value))
            data = mEntries.Get(value);
        else
            data = value;
    }

    const Type* EnumProperty::GetValueTypeStatic()
    {
        return nullptr;
    }

    void EnumProperty::UpdateValueView()
    {
        mUpdatingValue = true;

        if (mValuesDifferent)
        {
            mDropDown->value = (mEntries).Get(mCommonValue);
            mDropDown->SetState("undefined", true);
        }
        else
        {
            mDropDown->value = (mEntries).Get(mCommonValue);
            mDropDown->SetState("undefined", false);
        }

        mUpdatingValue = false;
    }

    void EnumProperty::OnSelectedItem(const WString& name)
    {
        if (mUpdatingValue)
            return;

        SetValueByUserAndComplete(mEntries.FindValue(name).first);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::EnumProperty>);
// --- META ---

DECLARE_CLASS(Editor::EnumProperty, Editor__EnumProperty);
// --- END META ---
