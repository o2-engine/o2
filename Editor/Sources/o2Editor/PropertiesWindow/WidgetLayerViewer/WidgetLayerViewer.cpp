#include "o2Editor/stdafx.h"
#include "WidgetLayerViewer.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Core/Properties/Properties.h"
#include "o2Editor/PropertiesWindow/WidgetLayerViewer/DefaultWidgetLayerHeadViewer.h"
#include "o2Editor/PropertiesWindow/WidgetLayerViewer/DefaultWidgetLayerLayoutViewer.h"
#include "o2Editor/PropertiesWindow/WidgetLayerViewer/DefaultWidgetLayerPropertiesViewer.h"
#include "o2Editor/PropertiesWindow/WidgetLayerViewer/IWidgetLayerHeadViewer.h"
#include "o2Editor/PropertiesWindow/WidgetLayerViewer/IWidgetLayerLayoutViewer.h"
#include "o2Editor/PropertiesWindow/WidgetLayerViewer/IWidgetLayerPropertiesViewer.h"

namespace Editor
{
    WidgetLayerViewer::WidgetLayerViewer()
    {
        PushEditorScopeOnStack scope;

        mHeaderViewer = mmake<DefaultWidgetLayerHeaderViewer>();
        mLayoutViewer = mmake<DefaultWidgetLayerLayoutViewer>();
        mPropertiesViewer = mmake<DefaultWidgetLayerPropertiesViewer>();

        auto scrollArea = o2UI.CreateScrollArea("backless");
        scrollArea->SetViewLayout(Layout::BothStretch(0, 0, 15, 0));
        scrollArea->SetClippingLayout(Layout::BothStretch());
        scrollArea->name = "widget layers scroll area";
        mContentWidget = scrollArea;

        mViewersLayout = o2UI.CreateVerLayout();
        mViewersLayout->name = "viewers layout";
        mViewersLayout->spacing = 0.0f;
        mViewersLayout->border = BorderF();
        mViewersLayout->expandHeight = false;
        mViewersLayout->expandWidth = true;
        mViewersLayout->fitByChildren = true;
        mViewersLayout->baseCorner = BaseCorner::Top;
        *mViewersLayout->layout = WidgetLayout::BothStretch();
        mContentWidget->AddChild(mViewersLayout);

        mViewersLayout->AddChild(mHeaderViewer->GetWidget());
        mViewersLayout->AddChild(mLayoutViewer->GetWidget());
        mViewersLayout->AddChild(mPropertiesViewer->GetWidget());

        o2Scene.onObjectsChanged += THIS_FUNC(OnSceneObjectsChanged);
    }

    WidgetLayerViewer::~WidgetLayerViewer()
    {
        if (Scene::IsSingletonInitialzed())
            o2Scene.onObjectsChanged -= THIS_FUNC(OnSceneObjectsChanged);
    }

    const Type* WidgetLayerViewer::GetViewingObjectType() const
    {
        return &TypeOf(WidgetLayer);
    }

    void WidgetLayerViewer::SetHeaderViewer(const Ref<IWidgetLayerHeaderViewer>& viewer)
    {
        mHeaderViewer = viewer;
    }

    void WidgetLayerViewer::SetLayoutViewer(const Ref<IWidgetLayerLayoutViewer>& viewer)
    {
        mLayoutViewer = viewer;
    }

    void WidgetLayerViewer::SetActorPropertiesViewer(const Ref<IWidgetLayerPropertiesViewer>& viewer)
    {
        mPropertiesViewer = viewer;
    }

    void WidgetLayerViewer::Refresh()
    {
        if (mTargetLayers.IsEmpty())
            return;

        mHeaderViewer->Refresh();
        mLayoutViewer->Refresh();
        mPropertiesViewer->Refresh();
    }

    void WidgetLayerViewer::OnSceneObjectsChanged(const Vector<Ref<SceneEditableObject>>& objects)
    {
        Refresh();
    }

    void WidgetLayerViewer::SetTargets(const Vector<IObject*>& targets)
    {
        PushEditorScopeOnStack scope;

        mTargetLayers = targets.Convert<WidgetLayer*>([](auto x) { return dynamic_cast<WidgetLayer*>(x); });

        // rebuild
        mHeaderViewer->SetTargetLayers(mTargetLayers);
        mLayoutViewer->SetTargetLayers(mTargetLayers);
        mPropertiesViewer->SetTargetLayers(mTargetLayers);
    }

    void WidgetLayerViewer::OnPropertiesEnabled()
    {
        mHeaderViewer->SetPropertiesEnabled(true);
        mLayoutViewer->SetPropertiesEnabled(true);
        mPropertiesViewer->SetPropertiesEnabled(true);
    }

    void WidgetLayerViewer::OnPropertiesDisabled()
    {
        mTargetLayers.Clear();

        mHeaderViewer->SetPropertiesEnabled(false);
        mLayoutViewer->SetPropertiesEnabled(false);
        mPropertiesViewer->SetPropertiesEnabled(false);
    }

    void WidgetLayerViewer::Update(float dt)
    {}

    void WidgetLayerViewer::Draw()
    {}
}
// --- META ---

DECLARE_CLASS(Editor::WidgetLayerViewer, Editor__WidgetLayerViewer);
// --- END META ---
