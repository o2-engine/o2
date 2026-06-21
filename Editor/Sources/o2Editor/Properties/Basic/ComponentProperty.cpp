#include "o2Editor/stdafx.h"
#include "ComponentProperty.h"

#include "o2/Application/Application.h"
#include "o2/Assets/Assets.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2Editor/Windows/AssetsWindow/AssetsIconsScroll.h"
#include "o2Editor/Windows/AssetsWindow/AssetsWindow.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/Windows/TreeWindow/SceneHierarchyTree.h"
#include "o2Editor/Windows/TreeWindow/TreeWindow.h"


namespace Editor
{
    ComponentProperty::ComponentProperty(RefCounter* refCounter):
        TPropertyField<LinkRef<Component>>(refCounter)
    {
    }

    ComponentProperty::ComponentProperty(RefCounter* refCounter, const ComponentProperty& other) :
        TPropertyField<LinkRef<Component>>(refCounter, other)
    {
        InitializeControls();
    }

    ComponentProperty& ComponentProperty::operator=(const ComponentProperty& other)
    {
        TPropertyField<LinkRef<Component>>::operator=(other);
        InitializeControls();
        return *this;
    }

    void ComponentProperty::InitializeControls()
    {
        SetValueChangeAppliedByAction(true);

        mCommonValue = nullptr;
        mComponentType = &TypeOf(Component);

        mBox = GetChildWidget("container/layout/box");
        if (mBox)
        {
            mBox->onDraw += [&]() { DragDropArea::OnDrawn(); };
            mBox->SetFocusable(true);

            mNameText = mBox->GetLayerDrawable<Text>("caption");
            if (mNameText)
                mNameText->text = "--";
        }
    }

    void ComponentProperty::Revert()
    {
        auto propertyObjects = o2EditorPropertiesWindow.GetTargets();

        for (int i = 0; i < mValuesProxies.Count() && i < propertyObjects.Count(); i++)
            RevertoToPrototype(mValuesProxies[i].first, mValuesProxies[i].second, propertyObjects[i]);

        Refresh();
    }

    const Type* ComponentProperty::GetValueType() const
    {
        return GetValueTypeStatic();
    }

    const Type* ComponentProperty::GetValueTypeStatic()
    {
        return &TypeOf(BaseComponentLinkRef);
    }

    bool ComponentProperty::IsUnderPoint(const Vec2F& point)
    {
        return mBox->IsUnderPoint(point);
    }

    void ComponentProperty::OnTypeSpecialized(const Type& type)
    {
        TPropertyField<LinkRef<Component>>::OnTypeSpecialized(type);

        mComponentType = type.InvokeStatic<const Type*>("GetComponentTypeStatic");
    }

    LinkRef<Component> ComponentProperty::GetProxy(const Ref<IAbstractValueProxy>& proxy) const
    {
        auto proxyType = dynamic_cast<const ObjectType*>(&proxy->GetType());
        auto proxySample = proxyType->CreateSample();
        proxy->GetValuePtr(proxySample);
        auto objectSample = proxyType->DynamicCastToIObject(proxySample);
        BaseComponentLinkRef* baseComponentLinkSample = dynamic_cast<BaseComponentLinkRef*>(objectSample);
        LinkRef<Component> res = LinkRef<Component>(const_cast<Component*>(baseComponentLinkSample->Get()));
        delete baseComponentLinkSample;
        return res;
    }

    void ComponentProperty::SetProxy(const Ref<IAbstractValueProxy>& proxy, const LinkRef<Component>& value)
    {
        auto proxyType = dynamic_cast<const ObjectType*>(&proxy->GetType());
        auto proxySample = proxyType->CreateSample();
        auto objectSample = proxyType->DynamicCastToIObject(proxySample);
        BaseComponentLinkRef* baseComponentLinkSample = dynamic_cast<BaseComponentLinkRef*>(objectSample);
        baseComponentLinkSample->Set(const_cast<Component*>(value.Get()));
        proxy->SetValuePtr(proxySample);
        delete baseComponentLinkSample;
    }

    bool ComponentProperty::IsValueRevertable() const
    {
        for (auto& ptr : mValuesProxies)
        {
            if (ptr.second)
            {
                Ref<Component> value = GetProxy(ptr.first);
                Ref<Component> proto = GetProxy(ptr.second);

                if (value && value->GetPrototypeLink())
                {
                    if (value->GetPrototypeLink() != proto)
                        return true;
                }
                else
                {
                    if (value != proto)
                        return true;
                }
            }
        }

        return false;
    }

    void ComponentProperty::UpdateValueView()
    {
        if (mValuesDifferent)
        {
            mNameText->text = "--";
            mBox->layer["caption"]->transparency = 1.0f;
        }
        else
        {
            if (!mCommonValue)
            {
                mNameText->text = "Null:" + mComponentType->GetName();
                mBox->layer["caption"]->transparency = 0.5f;
            }
            else
            {
                mNameText->text = mCommonValue->GetActor()->GetName();
                mBox->layer["caption"]->transparency = 1.0f;
            }
        }
    }

    void ComponentProperty::RevertoToPrototype(const Ref<IAbstractValueProxy>& target, const Ref<IAbstractValueProxy>& source, 
                                               IObject* targetOwner)
    {
        if (!source || !targetOwner || targetOwner->GetType().IsBasedOn(TypeOf(Component)))
            return;

        Ref<Component> sourceComponent = GetProxy(source);
        auto topSourceActor = sourceComponent->GetActor();
        while (topSourceActor->GetParent())
            topSourceActor = topSourceActor->GetParent().Lock();

        auto actorOwner = Ref(dynamic_cast<Actor*>(targetOwner));

        if (actorOwner)
        {
            auto topTargetActor = actorOwner;
            while (topTargetActor->GetPrototypeLink() != topSourceActor && topTargetActor->GetParent())
                topTargetActor = topTargetActor->GetParent().Lock();

            auto sameToProtoSourceActor = topTargetActor->FindLinkedActor(sourceComponent->GetActor());

            if (sameToProtoSourceActor)
            {
                auto sameToProtoSourceComponent = sameToProtoSourceActor->GetComponents().FindOrDefault(
                    [&](auto& x) { return x->GetPrototypeLink() == sourceComponent; });

                if (sameToProtoSourceComponent)
                {
                    SetProxy(target, sameToProtoSourceComponent);
                    return;
                }
            }
        }

        if (sourceComponent->GetActor()->IsOnScene())
            SetProxy(target, sourceComponent);
    }

    void ComponentProperty::OnCursorEnter(const Input::Cursor& cursor)
    {
        mBox->SetState("hover", true);
    }

    void ComponentProperty::OnCursorExit(const Input::Cursor& cursor)
    {
        mBox->SetState("hover", false);
    }

    void ComponentProperty::OnCursorPressed(const Input::Cursor& cursor)
    {
        o2UI.FocusWidget(mBox);

        if (mCommonValue)
        {
            auto ownerActor = mCommonValue->GetActor();

            if (ownerActor)
            {
                if (ownerActor->IsAsset())
                    o2EditorAssets.ShowAssetIcon(o2Assets.GetAssetPath(ownerActor->GetAssetID()));
                else if (ownerActor->IsOnScene())
                    o2EditorTree.HighlightObjectTreeNode(ownerActor);
            }
        }
    }

    void ComponentProperty::OnKeyPressed(const Input::Key& key)
    {
        if (mBox && mBox->IsFocused() && (key == VK_DELETE || key == VK_BACK))
            SetValueByUserAndComplete(nullptr);
    }

    void ComponentProperty::OnDropped(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        if (auto actorsTree = DynamicCast<SceneHierarchyTree>(group))
            OnDroppedFromActorsTree(actorsTree);
        else if (auto assetsScroll = DynamicCast<AssetsIconsScrollArea>(group))
            OnDroppedFromAssetsScroll(assetsScroll);
    }

    void ComponentProperty::OnDragEnter(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        if (auto actorsTree = DynamicCast<SceneHierarchyTree>(group))
            OnDragEnterFromActorsTree(actorsTree);
        else if (auto assetsScroll = DynamicCast<AssetsIconsScrollArea>(group))
            OnDragEnterFromAssetsScroll(assetsScroll);
    }

    void ComponentProperty::OnDragExit(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        if (auto actorsTree = DynamicCast<SceneHierarchyTree>(group))
            OnDragExitFromActorsTree(actorsTree);
        else if (auto assetsScroll = DynamicCast<AssetsIconsScrollArea>(group))
            OnDragExitFromAssetsScroll(assetsScroll);
    }

    void ComponentProperty::OnDroppedFromActorsTree(const Ref<SceneHierarchyTree>& actorsTree)
    {
        if (actorsTree->GetSelectedObjects().Count() > 1)
            return;

        if (auto actor = DynamicCast<Actor>(actorsTree->GetSelectedObjects()[0]))
            SetValueByUserAndComplete(actor->GetComponent(mComponentType));
        else
            return;

        o2Application.SetCursor(CursorType::Arrow);
        mBox->Focus();
    }

    void ComponentProperty::OnDragEnterFromActorsTree(const Ref<SceneHierarchyTree>& actorsTree)
    {
        if (actorsTree->GetSelectedObjects().Count() != 1)
            return;

        auto actor = DynamicCast<Actor>(actorsTree->GetSelectedObjects()[0]);
        if (!actor)
            return;

        if (!actor->GetComponent(mComponentType))
            return;

        o2Application.SetCursor(CursorType::Hand);
        mBox->SetState("focused", true);
    }

    void ComponentProperty::OnDragExitFromActorsTree(const Ref<SceneHierarchyTree>& actorsTree)
    {
        o2Application.SetCursor(CursorType::Arrow);
        mBox->SetState("focused", false);
    }

    void ComponentProperty::OnDroppedFromAssetsScroll(const Ref<AssetsIconsScrollArea>& assetsIconsScroll)
    {
        if (assetsIconsScroll->GetSelectedAssets().Count() > 1)
            return;

        auto actor = o2Scene.GetAssetActorByID(assetsIconsScroll->GetSelectedAssets().Last()->meta->ID());
        SetValueByUserAndComplete(actor->GetComponent(mComponentType));

        o2Application.SetCursor(CursorType::Arrow);
        mBox->Focus();
    }

    void ComponentProperty::OnDragEnterFromAssetsScroll(const Ref<AssetsIconsScrollArea>& assetsIconsScroll)
    {
        if (assetsIconsScroll->GetSelectedAssets().Count() > 1)
            return;

        auto actor = o2Scene.GetAssetActorByID(assetsIconsScroll->GetSelectedAssets().Last()->meta->ID());
        if (actor)
        {
            auto component = actor->GetComponent(mComponentType);

            if (!component)
                return;

            o2Application.SetCursor(CursorType::Hand);
            mBox->SetState("focused", true);
        }
    }

    void ComponentProperty::OnDragExitFromAssetsScroll(const Ref<AssetsIconsScrollArea>& assetsIconsScroll)
    {
        o2Application.SetCursor(CursorType::Arrow);
        mBox->SetState("focused", false);
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::LinkRef<Component>>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::ComponentProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::LinkRef<Component>>>);
// --- META ---

DECLARE_CLASS(Editor::ComponentProperty, Editor__ComponentProperty);
// --- END META ---
