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

            for (auto& kv : mEntries)
                mDropDown->AddItem(kv.second);
        }
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

        SetValueByUser(mEntries.FindValue(name).first);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::EnumProperty>);
// --- META ---

DECLARE_CLASS(Editor::EnumProperty, Editor__EnumProperty);
// --- END META ---
