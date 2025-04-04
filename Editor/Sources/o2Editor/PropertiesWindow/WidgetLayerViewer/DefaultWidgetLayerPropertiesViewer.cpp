#include "o2Editor/stdafx.h"
#include "DefaultWidgetLayerPropertiesViewer.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2Editor/Core/Actions/Transform.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Core/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Core/Properties/Properties.h"
#include "o2Editor/Core/UI/SpoilerWithHead.h"

namespace Editor
{

    DefaultWidgetLayerPropertiesViewer::DefaultWidgetLayerPropertiesViewer()
    {
        PushEditorScopeOnStack scope;
        mFitSizeButton = o2UI.CreateButton("Fit size by drawable", THIS_FUNC(FitLayerByDrawable));
    }

    DefaultWidgetLayerPropertiesViewer::~DefaultWidgetLayerPropertiesViewer()
    {}

    void DefaultWidgetLayerPropertiesViewer::SetTargetLayers(const Vector<WidgetLayer*>& layers)
    {
        mLayers = layers;
        Refresh();
    }

    const Type* DefaultWidgetLayerPropertiesViewer::GetDrawableType() const
    {
        return mDrawableType;
    }

    void DefaultWidgetLayerPropertiesViewer::Refresh()
    {
        if (!mViewer)
        {
            mViewer = o2EditorProperties.CreateObjectViewer(&TypeOf(WidgetLayer), "");
            mViewer->CreateSpoiler(mSpoiler);
			mViewer->SetHeaderEnabled(false);
            mFitSizeButton->SetParent(mSpoiler);
        }

        if (mViewer)
        {
            mViewer->Refresh(mLayers.Convert<Pair<IObject*, IObject*>>([](WidgetLayer* x) {
                return Pair<IObject*, IObject*>(dynamic_cast<IObject*>(x), nullptr);
            }));
        }
    }

    bool DefaultWidgetLayerPropertiesViewer::IsEmpty() const
    {
        return mSpoiler->GetChildren().Count() == 0;
    }

    void DefaultWidgetLayerPropertiesViewer::OnPropertiesEnabled()
    {
        if (mViewer)
            mViewer->OnPropertiesEnabled();
    }

    void DefaultWidgetLayerPropertiesViewer::OnPropertiesDisabled()
    {
        if (mViewer)
            mViewer->OnPropertiesDisabled();
    }

    void DefaultWidgetLayerPropertiesViewer::FitLayerByDrawable()
    {
        auto action = mmake<TransformAction>(mLayers.Convert<Ref<SceneEditableObject>>([](WidgetLayer* layer) { return Ref(dynamic_cast<SceneEditableObject*>(layer)); }));

        for (auto& layer : mLayers)
        {
            if (auto sprite = DynamicCast<Sprite>(layer->GetDrawable()))
                layer->layout.size = sprite->GetImageAsset()->GetSize();

            if (auto text = DynamicCast<Text>(layer->GetDrawable()))
                layer->layout.size = text->GetRealSize();
        }

        action->Completed();
        o2EditorApplication.DoneAction(action);
    }

}
// --- META ---

DECLARE_CLASS(Editor::DefaultWidgetLayerPropertiesViewer, Editor__DefaultWidgetLayerPropertiesViewer);
// --- END META ---
