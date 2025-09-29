#include "o2Editor/stdafx.h"
#include "DefaultWidgetLayerPropertiesViewer.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2Editor/Actions/Transform.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/UI/SpoilerWithHead.h"
#include "o2Editor/Windows/WindowsManager.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"

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
            mViewer = o2EditorProperties.CreateObjectViewer(&TypeOf(WidgetLayer), "", THIS_FUNC(OnPropertyChangeCompleted), THIS_FUNC(OnPropertyChanged));
            mViewer->CheckCreateSpoiler(mSpoiler);
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
        o2EditorSceneWindow.DoneAction(action);
    }

    void DefaultWidgetLayerPropertiesViewer::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        IWidgetLayerPropertiesViewer::OnPropertyChanged(field, byUser);
    }

    void DefaultWidgetLayerPropertiesViewer::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after)
    {
        IWidgetLayerPropertiesViewer::OnPropertyChangeCompleted(path, before, after);

        o2EditorSceneWindow.DoneActorPropertyChangeAction(path, before, after);
    }

}
// --- META ---

DECLARE_CLASS(Editor::DefaultWidgetLayerPropertiesViewer, Editor__DefaultWidgetLayerPropertiesViewer);
// --- END META ---
