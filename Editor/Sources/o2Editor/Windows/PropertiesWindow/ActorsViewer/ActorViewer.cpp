#include "o2Editor/stdafx.h"
#include "ActorViewer.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Windows/PropertiesWindow/ActorsViewer/AddComponentPanel.h"
#include "o2Editor/Windows/PropertiesWindow/ActorsViewer/DefaultActorComponentViewer.h"
#include "o2Editor/Windows/PropertiesWindow/ActorsViewer/DefaultActorHeaderViewer.h"
#include "o2Editor/Windows/PropertiesWindow/ActorsViewer/DefaultActorPropertiesViewer.h"
#include "o2Editor/Windows/PropertiesWindow/ActorsViewer/DefaultActorTransformViewer.h"
#include "o2Editor/Windows/PropertiesWindow/ActorsViewer/IActorComponentViewer.h"
#include "o2Editor/Windows/PropertiesWindow/ActorsViewer/IActorHeaderViewer.h"
#include "o2Editor/Windows/PropertiesWindow/ActorsViewer/IActorPropertiesViewer.h"
#include "o2Editor/Windows/PropertiesWindow/ActorsViewer/IActorTransformViewer.h"

namespace Editor
{
    ActorViewer::ActorViewer(RefCounter* refCounter):
        IPropertiesViewer(refCounter)
    {
        PushEditorScopeOnStack scope;

        // Create viewers
        mHeaderViewer = mmake<DefaultActorHeaderViewer>();
        mHeaderViewer->onPropertyChanged = THIS_FUNC(OnPropertyChanged);
        mHeaderViewer->onPropertyChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);

        mTransformViewer = mmake<DefaultActorTransformViewer>();
        mTransformViewer->onPropertyChanged = THIS_FUNC(OnPropertyChanged);
        mTransformViewer->onPropertyChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);

        mDefaultComponentViewer = mmake<DefaultActorComponentViewer>();
        mDefaultActorPropertiesViewer = mmake<DefaultActorPropertiesViewer>();

        // Create available component and actors viewers
        auto componentsViewersTypes = TypeOf(IActorComponentViewer).GetDerivedTypes();
        for (auto& type : componentsViewersTypes)
        {
            if (type->GetName().Contains("TActorComponentViewer"))
                continue;

            mAvailableComponentsViewers.Add(DynamicCast<IActorComponentViewer>(type->CreateSampleRef()));
        }

        auto actorPropertiessViewersTypes = TypeOf(IActorPropertiesViewer).GetDerivedTypes();
        for (auto& type : actorPropertiessViewersTypes)
            mAvailableActorPropertiesViewers.Add(DynamicCast<IActorPropertiesViewer>(type->CreateSampleRef()));

        // Initialize content widget and viewers layout
        mContentWidget = mmake<Widget>();
        mContentWidget->name = "content";
        *mContentWidget->layout = WidgetLayout::BothStretch();

        auto scrollArea = o2UI.CreateScrollArea("backless");
        scrollArea->SetViewLayout(Layout::BothStretch(0, 0, 15, 0));
        scrollArea->SetClippingLayout(Layout::BothStretch());
        scrollArea->SetEnableScrollsHiding(false);
        scrollArea->name = "actors scroll area";
        *scrollArea->layout = WidgetLayout::BothStretch(0, 40, 0, 0);
        mContentWidget->AddChild(scrollArea);

        mViewersLayout = o2UI.CreateVerLayout();
        mViewersLayout->name = "viewers layout";
        mViewersLayout->spacing = 0.0f;
        mViewersLayout->border = BorderF();
        mViewersLayout->expandHeight = false;
        mViewersLayout->expandWidth = true;
        mViewersLayout->fitByChildren = true;
        mViewersLayout->baseCorner = BaseCorner::Top;
        *mViewersLayout->layout = WidgetLayout::BothStretch();
        scrollArea->AddChild(mViewersLayout);

        // Initialize add component panel
        mAddComponentPanel = mmake<AddComponentPanel>(Ref(this));
        mAddComponentPanel->name = "add component";
        *mAddComponentPanel->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, 40, 0);
        mContentWidget->AddChild(mAddComponentPanel);

        auto addComponentAnim = AnimationClip::EaseInOut<float>("child/add component/layout/offsetTop",
                                                                40, 400, 0.4f);

        *addComponentAnim->AddTrack<float>("child/actors scroll area/layout/offsetBottom") =
            AnimationTrack<float>::EaseInOut(40, 400, 0.4f);

        mContentWidget->AddState("add component", addComponentAnim);

        mAddComponentPanel->onCursorReleased = [&](auto curs) { mContentWidget->SetState("add component", true); };
        mAddComponentPanel->onCursorPressedOutside = [&](auto curs) { mContentWidget->SetState("add component", false); };

        o2Scene.onObjectsChanged += THIS_FUNC(OnSceneObjectsChanged);
    }

    ActorViewer::~ActorViewer()
    {
        if (Scene::IsSingletonInitialzed())
            o2Scene.onObjectsChanged -= THIS_FUNC(OnSceneObjectsChanged);
    }

    const Type* ActorViewer::GetViewingObjectType() const
    {
        return &TypeOf(Actor);
    }

    void ActorViewer::SetActorHeaderViewer(const Ref<IActorHeaderViewer>& viewer)
    {
        mHeaderViewer = viewer;
    }

    void ActorViewer::SetActorTransformViewer(const Ref<IActorTransformViewer>& viewer)
    {
        mTransformViewer = viewer;
    }

    void ActorViewer::AddComponentViewerType(const Ref<IActorComponentViewer>& viewer)
    {
        mAvailableComponentsViewers.Add(viewer);
    }

    void ActorViewer::AddActorPropertiesViewerType(const Ref<IActorPropertiesViewer>& viewer)
    {
        mAvailableActorPropertiesViewers.Add(viewer);
    }

    void ActorViewer::Refresh()
    {
        PushEditorScopeOnStack scope;

        if (mTargetActors.IsEmpty())
            return;

        auto currentComponentGroups = GetGroupedComponents();
        if (mComponentGroupsTypes != currentComponentGroups)
        {
            SetTargets(mTargetActors.DynamicCast<IObject*>());
            return;
        }

        for (auto& viewer : mComponentsViewers)
            viewer->Refresh();

        if (mActorPropertiesViewer)
            mActorPropertiesViewer->Refresh();

        mTransformViewer->Refresh();
        mHeaderViewer->Refresh();
    }

    void ActorViewer::OnSceneObjectsChanged(const Vector<Ref<SceneEditableObject>>& objects)
    {
        Refresh();
    }

    void ActorViewer::SetTargets(const Vector<IObject*>& targets)
    {
        PushEditorScopeOnStack scope;

        mTargetActors = targets.Convert<Actor*>([](auto x) { return dynamic_cast<Actor*>(x); });

        // clear 
        mViewersLayout->RemoveAllChildren();

        // rebuild
        Vector<Ref<Widget>> viewersWidgets;
        viewersWidgets.Add(mHeaderViewer->GetWidget());
        mHeaderViewer->SetTargetActors(mTargetActors);

        viewersWidgets.Add(mTransformViewer->GetWidget());
        mTransformViewer->SetTargetActors(mTargetActors);

        SetTargetsActorProperties(targets, viewersWidgets);
        SetTargetsComponents(targets, viewersWidgets);
        mViewersLayout->AddChildren(DynamicCastVector<Actor>(viewersWidgets));
    }

    void ActorViewer::SetTargetsActorProperties(const Vector<IObject*>& targets, Vector<Ref<Widget>>& viewersWidgets)
    {
        PushEditorScopeOnStack scope;

        const Type* type = &mTargetActors[0]->GetType();
        bool isAllSameType = mTargetActors.All([&](Actor* x) { return &x->GetType() == type; });

        if (!isAllSameType)
            return;

        bool usingDefaultViewer = false;

        auto viewerSample = mAvailableActorPropertiesViewers.FindOrDefault([&](auto& x) {
            return x->GetActorType() == type;
        });

        if (!viewerSample)
        {
            viewerSample = mDefaultActorPropertiesViewer;
            usingDefaultViewer = true;
        }

        if (!mActorPropertiesViewersPool.ContainsKey(type))
        {
            PushEditorScopeOnStack scope;

            auto newViewer = DynamicCast<IActorPropertiesViewer>(viewerSample->GetType().CreateSampleRef());

            if (usingDefaultViewer)
                DynamicCast<DefaultActorPropertiesViewer>(newViewer)->SpecializeActorType(type);

            mActorPropertiesViewersPool.Add(type, newViewer);
        }

        auto propertiesViewer = mActorPropertiesViewersPool[type];

        propertiesViewer->SetTargetActors(mTargetActors);
        propertiesViewer->onPropertyChanged = THIS_FUNC(OnPropertyChanged);
        propertiesViewer->onPropertyChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);

        if (!propertiesViewer->IsEmpty())
            viewersWidgets.Add(propertiesViewer->GetWidget());

        mActorPropertiesViewer = propertiesViewer;
    }

    Vector<Pair<const Type*, Vector<Ref<Component>>>> ActorViewer::GetGroupedComponents() const
    {
        Vector<Pair<const Type*, Vector<Ref<Component>>>> res;

        for (auto& actor : mTargetActors)
        {
            Map<const Type*, int> offsets;
            for (auto& component : actor->GetComponents())
            {
                auto type = &component->GetType();
                auto offset = offsets[type];
                offsets[type]++;

                Vector<Ref<Component>>* list = nullptr;
                for (int i = 0; i < res.Count(); i++)
                {
                    if (res[i].first == type)
                    {
                        if (offset == 0)
                        {
                            list = &res[i].second;
                            break;
                        }
                        else
                            offset--;
                    }
                }

                if (!list)
                {
                    res.Add({ type, {} });
                    list = &res.Last().second;
                }

                list->Add(component);
            }
        }

        return res;
    }

    void ActorViewer::SetTargetsComponents(const Vector<IObject*>& targets, Vector<Ref<Widget>>& viewersWidgets)
    {
        PushEditorScopeOnStack scope;

        auto lastComponentViewers = mComponentsViewers;

        Vector<Ref<IActorComponentViewer>> enableComponentViewers;

        for (auto& viewer : mComponentsViewers)
            mComponentViewersPool[viewer->GetComponentType()].Add(viewer);

        mComponentsViewers.Clear();

        mComponentGroupsTypes = GetGroupedComponents();
        for (auto& pair : mComponentGroupsTypes)
        {
            auto componentType = pair.first;
            auto viewerSample = mAvailableComponentsViewers.FindOrDefault([&](auto& x) {
                return x->GetComponentType() == componentType; });

            if (!viewerSample)
                viewerSample = mDefaultComponentViewer;

            if (!mComponentViewersPool.ContainsKey(componentType) || mComponentViewersPool[componentType].IsEmpty())
            {
                if (!mComponentViewersPool.ContainsKey(componentType))
                    mComponentViewersPool.Add(componentType, {});

                auto newViewer = DynamicCast<IActorComponentViewer>(viewerSample->GetType().CreateSampleRef());

                mComponentViewersPool[componentType].Add(newViewer);
            }

            auto componentViewer = mComponentViewersPool[componentType].PopBack();

            viewersWidgets.Add(componentViewer->GetWidget());
            mComponentsViewers.Add(componentViewer);

            componentViewer->SetTargetComponents(pair.second);
            componentViewer->onPropertyChanged = THIS_FUNC(OnPropertyChanged);
            componentViewer->onPropertyChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);

            if (lastComponentViewers.Contains(componentViewer))
                lastComponentViewers.Remove(componentViewer);
            else
                enableComponentViewers.Add(componentViewer);
        }

        for (auto& viewer : lastComponentViewers)
            viewer->SetPropertiesEnabled(false);

        for (auto& viewer : enableComponentViewers)
            viewer->SetPropertiesEnabled(true);
    }

    void ActorViewer::OnPropertiesEnabled()
    {
        mHeaderViewer->SetPropertiesEnabled(true);
        mTransformViewer->SetPropertiesEnabled(true);

        if (mActorPropertiesViewer)
            mActorPropertiesViewer->SetPropertiesEnabled(true);

        mComponentsViewers.ForEach([](auto x) { x->SetPropertiesEnabled(true); });
    }

    void ActorViewer::OnPropertiesDisabled()
    {
        mHeaderViewer->SetPropertiesEnabled(false);
        mTransformViewer->SetPropertiesEnabled(false);

        if (mActorPropertiesViewer)
            mActorPropertiesViewer->SetPropertiesEnabled(false);

        mComponentsViewers.ForEach([](auto x) { x->SetPropertiesEnabled(false); });

        mTargetActors.Clear();
    }

    void ActorViewer::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        IPropertiesViewer::OnPropertyChanged(field, byUser);
    }

    void ActorViewer::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after)
    {
        IPropertiesViewer::OnPropertyChangeCompleted(path, before, after);
    }
}
// --- META ---

DECLARE_CLASS(Editor::ActorViewer, Editor__ActorViewer);
// --- END META ---
