#include "o2Editor/stdafx.h"
#include "SceneLayerRefProperty.h"

#include "o2/Scene/UI/Widgets/DropDown.h"


namespace Editor
{
    SceneLayerRefProperty::SceneLayerRefProperty(RefCounter* refCounter):
        TPropertyField<Ref<SceneLayer>>(refCounter)
    {
        mCommonValue = Ref<SceneLayer>();
    }

    SceneLayerRefProperty::SceneLayerRefProperty(RefCounter* refCounter, const SceneLayerRefProperty& other) :
        TPropertyField<Ref<SceneLayer>>(refCounter, other)
    {
        InitializeControls();
    }

    void SceneLayerRefProperty::SetSelectedInheritedValue(bool inherited)
    {
        if (mSelectedInheritedValue == inherited)
            return;

        mSelectedInheritedValue = inherited;

        UpdateValueView();
    }

    bool SceneLayerRefProperty::IsSelectedInheritedValue() const
    {
        return mSelectedInheritedValue;
    }

    void SceneLayerRefProperty::SetUseInheritedValue(bool use)
    {
        mUseInheritedValue = use;
    }

    bool SceneLayerRefProperty::IsUseInheritedValue() const
    {
        return mUseInheritedValue;
    }

    SceneLayerRefProperty& SceneLayerRefProperty::operator=(const SceneLayerRefProperty& other)
    {
        TPropertyField<Ref<SceneLayer>>::operator=(other);
        InitializeControls();
        return *this;
    }

    void SceneLayerRefProperty::InitializeControls()
    {
        mCommonValue = Ref<SceneLayer>();

        mDropDown = FindChildByType<DropDown>();
        if (mDropDown)
        {
            mDropDown->onSelectedText = THIS_FUNC(SelectLayer);
            mDropDown->onBeforeExpand = THIS_FUNC(UpdateLayersList);
            mDropDown->SetState("undefined", true);
        }
    }

    void SceneLayerRefProperty::UpdateValueView()
    {
        mUpdatingValue = true;

        UpdateLayersList();

        if (mValuesDifferent)
        {
            mDropDown->SelectItemAt(-1);
            mDropDown->SetState("undefined", true);
        }
        else
        {
            if (mSelectedInheritedValue)
                mDropDown->value = mInheritFromParentName;
            else if (mCommonValue)
                mDropDown->value = mCommonValue->GetName();

            mDropDown->SetState("undefined", false);
        }

        mUpdatingValue = false;
    }

    void SceneLayerRefProperty::UpdateLayersList()
    {
        auto layers = o2Scene.GetLayers();
        auto dropdownLayers = mDropDown->GetAllItemsText();
        for (auto itemName : dropdownLayers)
        {
            if (!layers.Contains([&](auto& x) { return x->GetName() == (String)itemName; }))
                mDropDown->RemoveItem(itemName);
        }

        if (mUseInheritedValue)
            mDropDown->AddItem(mInheritFromParentName);

        for (auto& layer : layers)
        {
            if (!dropdownLayers.Contains(layer->GetName()))
                mDropDown->AddItem(layer->GetName());
        }
    }

    void SceneLayerRefProperty::SelectLayer(const WString& name)
    {
        if (mUpdatingValue)
            return;

        if (String(name) == mInheritFromParentName)
        {
            mSelectedInheritedValue = true;
            onSelectedInheritedValue();
            return;
        }

        mSelectedInheritedValue = false;

        if (!mCommonValue)
            mCommonValue = o2Scene.GetDefaultLayer();

        //SetValueByUser(Ref<SceneLayer>(name));
    }

    bool SceneLayerRefProperty::IsAlwaysRefresh() const
    {
        return false;
    }

}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::Ref<o2::SceneLayer>>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::SceneLayerRefProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::Ref<o2::SceneLayer>>>);
// --- META ---

DECLARE_CLASS(Editor::SceneLayerRefProperty, Editor__SceneLayerRefProperty);
// --- END META ---
