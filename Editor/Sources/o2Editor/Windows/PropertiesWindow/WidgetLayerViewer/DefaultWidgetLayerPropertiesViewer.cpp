#include "o2Editor/stdafx.h"
#include "DefaultWidgetLayerPropertiesViewer.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayerLayout.h"
#include "o2/Utils/Math/Layout.h"
#include "o2Editor/Actions/WidgetLayerLayout.h"
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
        Vector<Ref<SceneEditableObject>> objects;
        Vector<Layout> doneLayouts;

        for (auto& layer : mLayers)
        {
            Vec2F target;
            if (auto sprite = DynamicCast<Sprite>(layer->GetDrawable()))
                target = sprite->GetImageAsset()->GetSize();
            else if (auto text = DynamicCast<Text>(layer->GetDrawable()))
                target = text->GetRealSize();
            else
                continue;

            Layout done = layer->GetLayout();
            Vec2F delta = target - layer->layout.size;
            done.offsetMin -= delta*0.5f;
            done.offsetMax += delta*0.5f;

            objects.Add(Ref(dynamic_cast<SceneEditableObject*>(layer)));
            doneLayouts.Add(done);
        }

        if (objects.IsEmpty())
            return;

        auto action = mmake<WidgetLayerLayoutAction>(objects, doneLayouts);
        action->Redo();
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
    }

}
// --- META ---

DECLARE_CLASS(Editor::DefaultWidgetLayerPropertiesViewer, Editor__DefaultWidgetLayerPropertiesViewer);
// --- END META ---
