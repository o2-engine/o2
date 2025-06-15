#include "o2Editor/stdafx.h"
#include "StringProperty.h"

#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/EditBoxDropDown.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Utils/Editor/Attributes/ItemsSourceAttribute.h"
#include "../PropertiesContext.h"

namespace Editor
{
    StringProperty::StringProperty(RefCounter* refCounter):
        TPropertyField<String>(refCounter)
    {}

    StringProperty::StringProperty(RefCounter* refCounter, const StringProperty& other) :
        TPropertyField<String>(refCounter, other)
    {
        InitializeControls();
    }

    StringProperty& StringProperty::operator=(const StringProperty& other)
    {
        TPropertyField<String>::operator=(other);
        InitializeControls();
        return *this;
    }

    void StringProperty::SetFieldInfo(const FieldInfo* fieldInfo)
    {
        TPropertyField::SetFieldInfo(fieldInfo);
        
        mUsingDropDown = (fieldInfo && fieldInfo->GetAttribute<ItemsSourceAttribute>());
        
        if (mUsingDropDown)
            UpdateDropDownItems();
    }
    
    void StringProperty::Refresh(bool forcible /*= false*/)
    {
        TPropertyField<String>::Refresh(forcible);
        
        if (mUsingDropDown)
            UpdateDropDownItems();
    }

    void StringProperty::InitializeControls()
    {
        mEditBox = FindChildByType<EditBox>();
        mEditBoxDropDown = FindChildByType<EditBoxDropDown>();
        
        if (mEditBox)
        {
            mEditBox->onChangeCompleted = THIS_FUNC(OnEdited);
            mEditBox->text = "--";
        }
        
        if (mEditBoxDropDown)
        {
            mEditBoxDropDown->onChangeCompleted = THIS_FUNC(OnEdited);
            mEditBoxDropDown->text = "--";
            
            if (mFieldInfo && mUsingDropDown)
                UpdateDropDownItems();
        }
    }

    void StringProperty::UpdateValueView()
    {
        if (mValuesDifferent)
        {
            if (mUsingDropDown)
                mEditBoxDropDown->text = "--";
            else
                mEditBox->text = "--";
        }
        else
        {
            if (mUsingDropDown)
                mEditBoxDropDown->text = mCommonValue;
            else
                mEditBox->text = mCommonValue;
        }
        
        if (mEditBox)
            mEditBox->SetEnabled(!mUsingDropDown);
            
        if (mEditBoxDropDown)
            mEditBoxDropDown->SetEnabled(mUsingDropDown);
    }

    void StringProperty::OnEdited(const WString& data)
    {
        if (mValuesDifferent && data == "--")
            return;

        SetValueByUser(data);
    }
    
    void StringProperty::UpdateDropDownItems()
    {
        if (!mFieldInfo || !mEditBoxDropDown)
            return;
            
        auto itemsSourceAttribute = mFieldInfo->GetAttribute<ItemsSourceAttribute>();
        if (!itemsSourceAttribute || itemsSourceAttribute->methodName.IsEmpty())
            return;
        
        auto parentContext = mParentContext.Lock();
        if (!parentContext || parentContext->targets.IsEmpty())
            return;

        auto& target = parentContext->targets[0];
        auto& targetType = target.first->GetType();
        auto& targetObjType = dynamic_cast<const ObjectType&>(targetType);

        Vector<String> items = targetObjType.Invoke<Vector<String>>(itemsSourceAttribute->methodName,
                                                                    targetObjType.DynamicCastFromIObject(target.first));
        
        bool itemsChanged = (items != mCachedItems);
        if (itemsChanged)
        {
            mCachedItems = items;
			mEditBoxDropDown->RemoveAllItems();
			for (const auto& item : items)
				mEditBoxDropDown->AddItem(item);
        }
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::String>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::StringProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::String>>);
// --- META ---

DECLARE_CLASS(Editor::StringProperty, Editor__StringProperty);
// --- END META ---
