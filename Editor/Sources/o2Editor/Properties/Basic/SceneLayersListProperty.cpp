#include "o2Editor/stdafx.h"
#include "SceneLayersListProperty.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"

namespace Editor
{
    SceneLayersListProperty::SceneLayersListProperty(RefCounter* refCounter):
        TPropertyField<SceneLayersList>(refCounter)
    {}

    SceneLayersListProperty::SceneLayersListProperty(RefCounter* refCounter, const SceneLayersListProperty& other):
        TPropertyField<SceneLayersList>(refCounter, other)
    {
        InitializeControls();
    }

    SceneLayersListProperty& SceneLayersListProperty::operator=(const SceneLayersListProperty& other)
    {
        TPropertyField<SceneLayersList>::operator=(other);
        InitializeControls();
        return *this;
    }

    void SceneLayersListProperty::InitializeControls()
    {
        mPropertyButton = GetChildByType<Button>("container/layout/box");
        if (mPropertyButton)
        {
            mPropertyButton->SetFocusable(true);
            mPropertyButton->onClick = THIS_FUNC(OpenContext);

            mPropertyText = mPropertyButton->GetLayerDrawable<Text>("caption");
            if (mPropertyText)
                mPropertyText->text = "--";

            mLayersContext = o2UI.CreateWidget<ContextMenu>();
            mPropertyButton->AddChild(mLayersContext);
        }
    }

    void SceneLayersListProperty::OpenContext()
    {
        mLayersContext->RemoveAllItems();

        mLayersContext->AddToggleItem("All layers", o2Scene.GetLayers().All([&](auto& x) { return mCommonValue.HasLayer(x); }),
                                      [&](bool b) { if (b) SetValueByUser(o2Scene.GetLayers()); else SetValueByUser({}); });

        mLayersContext->AddItem("---");

        for (auto& layer : o2Scene.GetLayers())
            mLayersContext->AddToggleItem(layer->GetName(), mCommonValue.HasLayer(layer), 
                                          [=](bool b) { 
            SceneLayersList copy = mCommonValue;
            if (b)
                copy.AddLayer(layer);
            else
                copy.RemoveLayer(layer);

            SetValueByUser(copy);
        });

        mLayersContext->Show(mPropertyButton->layout->GetWorldLeftBottom());
    }

    void SceneLayersListProperty::UpdateValueView()
    {
        String layers;
        for (auto& layer : mCommonValue.GetLayersNames())
        {
            if (!layers.IsEmpty())
                layers += ", ";

            layers += layer;
        }

        mPropertyText->text = layers;
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::SceneLayersList>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::SceneLayersListProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::SceneLayersList>>);
// --- META ---

DECLARE_CLASS(Editor::SceneLayersListProperty, Editor__SceneLayersListProperty);
// --- END META ---
