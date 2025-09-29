#include "o2Editor/stdafx.h"
#include "DefaultWidgetLayerHeadViewer.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/Actions/PropertyChange.h"
#include "o2Editor/EditorApplication.h"
#include "o2Editor/Windows/WindowsManager.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Properties/Basic/BooleanProperty.h"
#include "o2Editor/Properties/Basic/StringProperty.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    DefaultWidgetLayerHeaderViewer::DefaultWidgetLayerHeaderViewer()
    {
        PushEditorScopeOnStack scope;

        mDataView = mmake<Widget>();
        mDataView->name = "actor head";
        mDataView->layout->minHeight = 21;

        mEnableProperty = o2UI.CreateWidget<BooleanProperty>("actor head enable");
        *mEnableProperty->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(1, 0));
        mEnableProperty->SetValuePath("enabled");
        mEnableProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mEnableProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mDataView->AddChild(mEnableProperty);

        mNameProperty = o2UI.CreateWidget<StringProperty>("actor head name");
        *mNameProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 21, 15, 17, 2);
        mNameProperty->SetValuePath("name");
        mNameProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mNameProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mDataView->AddChild(mNameProperty);

        mLockProperty = o2UI.CreateWidget<BooleanProperty>("actor head lock");
        *mLockProperty->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(2, -1));
        mLockProperty->SetValuePath("locked");
        mLockProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mLockProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mDataView->AddChild(mLockProperty);
    }

    DefaultWidgetLayerHeaderViewer::~DefaultWidgetLayerHeaderViewer()
    {}

    void DefaultWidgetLayerHeaderViewer::SetTargetLayers(const Vector<WidgetLayer*>& layers)
    {
        mLayers = layers;

        Vector<WidgetLayer*> prototypes = layers.Convert<WidgetLayer*>([](WidgetLayer* x) { return nullptr; });
//         auto prototypes = layers.Convert<WidgetLayer*>([](WidgetLayer* x) { 
//             return x->GetOwnerWidget()->GetPrototypeLink().Get(); });

        mEnableProperty->SelectValueAndPrototypeProperties<WidgetLayer, decltype(WidgetLayer::enabled)>(
            layers, prototypes, [](WidgetLayer* x) { return &x->enabled; });

        mNameProperty->SelectValueAndPrototypePointers<String, WidgetLayer>(
            layers, prototypes, [](WidgetLayer* x) { return &x->name; });

        mLockProperty->SelectValueAndPrototypeProperties<WidgetLayer, decltype(WidgetLayer::locked)>(
            layers, prototypes, [](WidgetLayer* x) { return &x->locked; });
    }

    Ref<Widget> DefaultWidgetLayerHeaderViewer::GetWidget() const
    {
        return mDataView;
    }

    void DefaultWidgetLayerHeaderViewer::Refresh()
    {
        mEnableProperty->Refresh();
        mNameProperty->Refresh();
        mLockProperty->Refresh();
    }

    void DefaultWidgetLayerHeaderViewer::OnPropertiesEnabled()
    {
        mEnableProperty->SetPropertyEnabled(true);
        mNameProperty->SetPropertyEnabled(true);
        mLockProperty->SetPropertyEnabled(true);
    }

    void DefaultWidgetLayerHeaderViewer::OnPropertiesDisabled()
    {
        mEnableProperty->SetPropertyEnabled(false);
        mNameProperty->SetPropertyEnabled(false);
        mLockProperty->SetPropertyEnabled(false);
    }

    void DefaultWidgetLayerHeaderViewer::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        IWidgetLayerHeaderViewer::OnPropertyChanged(field, byUser);
    }

    void DefaultWidgetLayerHeaderViewer::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after)
    {
        IWidgetLayerHeaderViewer::OnPropertyChangeCompleted(path, before, after);

        auto action = mmake<PropertyChangeAction>(o2EditorSceneScreen.GetSelectedObjects(), path, before, after);
        o2EditorSceneWindow.DoneAction(action);
    }

}
// --- META ---

DECLARE_CLASS(Editor::DefaultWidgetLayerHeaderViewer, Editor__DefaultWidgetLayerHeaderViewer);
// --- END META ---
