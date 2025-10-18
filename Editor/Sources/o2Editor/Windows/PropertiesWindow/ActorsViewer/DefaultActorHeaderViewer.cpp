#include "o2Editor/stdafx.h"
#include "DefaultActorHeaderViewer.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Image.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Actions/PropertyChange.h"
#include "o2Editor/EditorApplication.h"
#include "o2Editor/Windows/WindowsManager.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"
#include "o2Editor/Properties/Basic/AssetProperty.h"
#include "o2Editor/Properties/Basic/BooleanProperty.h"
#include "o2Editor/Properties/Basic/FloatProperty.h"
#include "o2Editor/Properties/Basic/SceneLayerRefProperty.h"
#include "o2Editor/Properties/Basic/StringProperty.h"
#include "o2Editor/Properties/Basic/TagProperty.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    DefaultActorHeaderViewer::DefaultActorHeaderViewer()
    {
        PushEditorScopeOnStack scope;

        mDataView = mmake<Widget>();
        mDataView->name = "actor head";
        mDataView->layout->minHeight = 62;

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


        auto prototypeRoot = mDataView->AddChildWidget(mmake<Widget>());
        prototypeRoot->name = "prototype";
        *prototypeRoot->layout = WidgetLayout::BothStretch();

        prototypeRoot->AddState("visible", AnimationClip::EaseInOut("transparency", 0.0f, 1.0f, 0.1f));

        auto linkImg = o2UI.CreateImage("ui/UI4_prefab_link_big.png");
        *linkImg->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(1, -20));
        prototypeRoot->AddChild(linkImg);

        mPrototypeProperty = o2UI.CreateWidget<AssetProperty>("actor head asset property");
        *mPrototypeProperty->layout = WidgetLayout::HorStretch(VerAlign::Top, 20, 65, 17, 25);
        prototypeRoot->AddChild(mPrototypeProperty);

        mPrototypeApplyBtn = o2UI.CreateWidget<Button>("accept prototype");
        *mPrototypeApplyBtn->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(25, 25), Vec2F(-40, -18));
        mPrototypeApplyBtn->onClick = THIS_FUNC(OnApplyPrototypePressed);
        prototypeRoot->AddChild(mPrototypeApplyBtn);

        mPrototypeRevertBtn = o2UI.CreateWidget<Button>("revert prototype");
        *mPrototypeRevertBtn->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(25, 25), Vec2F(-20, -18));
        mPrototypeRevertBtn->onClick = THIS_FUNC(OnRevertPrototypePressed);
        prototypeRoot->AddChild(mPrototypeRevertBtn);

        mPrototypeBreakBtn = o2UI.CreateWidget<Button>("break prototype");
        *mPrototypeBreakBtn->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(25, 25), Vec2F(0, -18));
        mPrototypeBreakBtn->onClick = THIS_FUNC(OnBreakPrototypePressed);
        prototypeRoot->AddChild(mPrototypeBreakBtn);

        auto tagsImg = o2UI.CreateImage("ui/UI4_tag_big.png");
        *tagsImg->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(20, 20), Vec2F(1, 21));
        mDataView->AddChild(tagsImg);

        mTagsProperty = o2UI.CreateWidget<TagsProperty>("actor head tags");
        *mTagsProperty->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 21, 3, 17, 23);
        mTagsProperty->SetValuePath("tags");
        mTagsProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mTagsProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mDataView->AddChild(mTagsProperty);

        auto layerImg = o2UI.CreateImage("ui/UI4_layer_big.png");
        *layerImg->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(20, 20), Vec2F(1, 0));
        mDataView->AddChild(layerImg);

        mLayerProperty = o2UI.CreateWidget<SceneLayerRefProperty>("actor head layer");
        *mLayerProperty->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 21, 75, 17, 3);
        mLayerProperty->name = "layer";
        mLayerProperty->SetValuePath("layer");
        mLayerProperty->SetUseInheritedValue(true);
        mLayerProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mLayerProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mLayerProperty->onSelectedInheritedValue = THIS_FUNC(OnSelectedInheritedLayer);
        mDataView->AddChild(mLayerProperty);

        mDepthProperty = o2UI.CreateWidget<FloatProperty>("actor head depth");
        *mDepthProperty->layout = WidgetLayout::Based(BaseCorner::RightBottom, Vec2F(70, 17), Vec2F(-3, 3));
        mDepthProperty->name = "depth";
        mDepthProperty->SetValuePath("depth");
        mDepthProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mDepthProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mDataView->AddChild(mDepthProperty);

        auto depthInheritStateAnim = AnimationClip::EaseInOut("child/layer/layout/offsetRight", -75.0f, -3.0f, 0.2f);
        *depthInheritStateAnim->AddTrack<float>("child/depth/layout/offsetLeft") = AnimationTrack<float>::Linear(-70.0f, 0.0f, 0.2f);
        *depthInheritStateAnim->AddTrack<bool>("child/depth/enabled") = AnimationTrack<bool>::Linear(true, false, 0.2f);
        mDataView->AddState("depth inherit", depthInheritStateAnim);

        auto protoStateAnim = AnimationClip::EaseInOut("layout/minHeight", 62.0f, 84.0f, 0.1f);
        *protoStateAnim->AddTrack<bool>("child/prototype/enabled") = AnimationTrack<bool>::Linear(false, true, 0.1f);
        mDataView->AddState("prototype", protoStateAnim);
    }

    DefaultActorHeaderViewer::~DefaultActorHeaderViewer()
    {}

    void DefaultActorHeaderViewer::SetTargetActors(const Vector<Actor*>& actors)
    {
        mActors = actors;

        auto prototypes = actors.Convert<Actor*>([](Actor* x) { return x->GetPrototypeLink().Get(); });

        mEnableProperty->SelectValueAndPrototypeProperties<Actor, decltype(Actor::enabled)>(
            actors, prototypes, [](Actor* x) { return &x->enabled; });

        mNameProperty->SelectValueAndPrototypeProperties<Actor, decltype(Actor::name)>(
            actors, prototypes, [](Actor* x) { return &x->name; });

        mLockProperty->SelectValueAndPrototypeProperties<Actor, decltype(Actor::locked)>(
            actors, prototypes, [](Actor* x) { return &x->locked; });

        mPrototypeProperty->SelectValuesProperties<Actor, decltype(Actor::prototype)>(
            actors, [](Actor* x) { return &x->prototype; });

        mDataView->state["prototype"]->SetState(mPrototypeProperty->GetCommonValue().IsValid());

        mTagsProperty->SelectValueAndPrototypePointers<TagGroup, Actor>(
            actors, prototypes, [](Actor* x) { return &x->tags; });

        mLayerProperty->SelectValueAndPrototypeFunctional<Ref<SceneLayer>, Actor>(
            actors, prototypes, [](Actor* x) { return x->GetLayer(); },
            [](Actor* x, const Ref<SceneLayer>& l) { if (l) x->SetLayer(l->GetName()); });

        mDepthProperty->SelectValueAndPrototypeProperties<Actor, decltype(Actor::drawDepth)>(
            actors, prototypes, [](Actor* x) { return &x->drawDepth; });

        RefreshLayer();
    }

    Ref<Widget> DefaultActorHeaderViewer::GetWidget() const
    {
        return mDataView;
    }

    void DefaultActorHeaderViewer::Refresh()
    {
        mEnableProperty->Refresh();
        mNameProperty->Refresh();
        mLockProperty->Refresh();
        mPrototypeProperty->Refresh();
        mTagsProperty->Refresh();
        mDepthProperty->Refresh();

        mDataView->state["prototype"]->SetState(mPrototypeProperty->GetCommonValue().IsValid());

        RefreshLayer();
    }

    void DefaultActorHeaderViewer::OnPropertiesEnabled()
    {
        mEnableProperty->SetPropertyEnabled(true);
        mNameProperty->SetPropertyEnabled(true);
        mLockProperty->SetPropertyEnabled(true);
        mPrototypeProperty->SetPropertyEnabled(true);
        mTagsProperty->SetPropertyEnabled(true);
        mLayerProperty->SetPropertyEnabled(true);
        mDepthProperty->SetPropertyEnabled(true);
    }

    void DefaultActorHeaderViewer::OnPropertiesDisabled()
    {
        mEnableProperty->SetPropertyEnabled(false);
        mNameProperty->SetPropertyEnabled(false);
        mLockProperty->SetPropertyEnabled(false);
        mPrototypeProperty->SetPropertyEnabled(false);
        mTagsProperty->SetPropertyEnabled(false);
        mLayerProperty->SetPropertyEnabled(false);
        mDepthProperty->SetPropertyEnabled(false);
    }

    void DefaultActorHeaderViewer::RefreshLayer()
    {
        mLayerProperty->Refresh();

        bool commonInheritedValue = mActors[0]->IsDrawingDepthInheritedFromParent();
        bool differentsInheritedValue = false;

        for (int i = 1; i < mActors.Count(); i++)
        {
            bool inheritedValue = mActors[i]->IsDrawingDepthInheritedFromParent();
            if (inheritedValue != commonInheritedValue)
                differentsInheritedValue = true;
        }

        if (differentsInheritedValue)
            mLayerProperty->SetUnknownValue(mActors[0]->GetLayer());
        else
            mLayerProperty->SetSelectedInheritedValue(commonInheritedValue);

        mDataView->state["depth inherit"]->SetState(commonInheritedValue);
    }

    void DefaultActorHeaderViewer::OnApplyPrototypePressed()
    {
        Vector<Actor*> actors = GetRootApplyActors();

        for (auto& actor : actors)
            actor->ApplyChangesToPrototype();
    }

    void DefaultActorHeaderViewer::OnRevertPrototypePressed()
    {
        bool areViewActorsAssets = mActors[0]->IsAsset();
        o2EditorPropertiesWindow.SetTarget(nullptr);

        Vector<Actor*> revertingActors = GetRootApplyActors();

        for (auto& actor : revertingActors)
            actor->RevertToPrototype();

        mActors.Clear();
        if (areViewActorsAssets)
        {
            Vector<UID> viewActors = mActors.Convert<UID>([](Actor* x) { return x->GetAssetID(); });
            for (auto& id : viewActors)
                mActors.Add(AssetRef<ActorAsset>(id)->GetActor().Get());
        }
        else
        {
            Vector<UInt64> viewActors = mActors.Convert<UInt64>([](Actor* x) { return x->GetID(); });
            for (auto& id : viewActors)
                mActors.Add(o2Scene.GetActorByID(id).Get());
        }

        o2EditorPropertiesWindow.SetTargets(mActors.Cast<IObject*>());
    }

    void DefaultActorHeaderViewer::OnBreakPrototypePressed()
    {
        Vector<Actor*> actors = GetRootApplyActors();

        for (auto& actor : actors)
            actor->BreakPrototypeLink();

        *mDataView->state["prototype"] = false;
    }

    void DefaultActorHeaderViewer::OnSelectedInheritedLayer()
    {
        for (auto& actor : mActors)
            actor->SetDrawingDepthInheritFromParent(true);
    }

    Vector<Actor*> DefaultActorHeaderViewer::GetRootApplyActors()
    {
        Vector<Actor*> applyActors;

        for (auto& actor : mActors)
        {
            Actor* applyActor = actor;
            if (!applyActor->GetPrototype())
                continue;

            while (applyActor && !applyActor->GetPrototypeDirectly().IsValid())
                applyActor = applyActor->GetParent().Lock().Get();

            if (applyActor)
                applyActors.Add(applyActor);
        }

        return applyActors;
    }
    void DefaultActorHeaderViewer::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        IActorHeaderViewer::OnPropertyChanged(field, byUser);
    }

    void DefaultActorHeaderViewer::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after)
    {
        IActorHeaderViewer::OnPropertyChangeCompleted(path, before, after);
    }

}
// --- META ---

DECLARE_CLASS(Editor::DefaultActorHeaderViewer, Editor__DefaultActorHeaderViewer);
// --- END META ---
