#include "o2/stdafx.h"
#include "ComponentLinkRef.h"

#include "o2/Scene/Scene.h"

namespace o2
{
    BaseComponentLinkRef::BaseComponentLinkRef()
    {}

    BaseComponentLinkRef::BaseComponentLinkRef(const BaseComponentLinkRef& other)
    {
        if (other.mRequiredResolveData)
        {
            mRequiredResolveData = other.mRequiredResolveData->Clone();
            mRequiredResolveData->RequireResolve(*this);
        }

        ActorRefResolver::RequireRemap(*this);
    }

    BaseComponentLinkRef::~BaseComponentLinkRef()
    {
        mRequiredResolveData = nullptr;
        ActorRefResolver::OnComponentDestroyed(this);
    }

    void BaseComponentLinkRef::Set(Component* component)
    {}

    Component* BaseComponentLinkRef::Get()
    {
        return nullptr;
    }

    const Component* BaseComponentLinkRef::Get() const
    {
        return nullptr;
    }

    void BaseComponentLinkRef::Destroy()
    {
        o2Scene.DestroyComponent(Ref(Get()));
    }

    const Type& BaseComponentLinkRef::GetComponentType() const
    {
        return TypeOf(Component);
    }

    const Type* BaseComponentLinkRef::GetComponentTypeStatic()
    {
        return &TypeOf(Component);
    }

    bool BaseComponentLinkRef::EqualsDelta(const BaseComponentLinkRef& obj, const BaseComponentLinkRef& origin)
    {
        if (obj.Get() == origin.Get())
            return true;

        if (obj.Get() && obj.Get()->mPrototypeLink.Lock().Get() == origin.Get())
            return true;

        return false;
    }

    void BaseComponentLinkRef::CopyWithoutRemap(const BaseComponentLinkRef& other)
    {}

    void BaseComponentLinkRef::OnSerialize(DataValue& node) const
    {
        if (auto component = Get())
        {
            auto actor = component->mOwner.Lock();
            if (actor)
            {
                if (actor->mIsAsset)
                    node.AddMember("AssetID") = actor->GetAssetID();
                else
                    node.AddMember("ActorID") = actor->GetID();

                node.AddMember("ComponentId") = component->mId;
            }
        }
    }

    void BaseComponentLinkRef::OnDeserialized(const DataValue& node)
    {
        if (auto assetIdNode = node.FindMember("AssetId"))
        {
            auto resolveData = mmake<AssetRequireResolveData>();
            resolveData->uid = (UID)*assetIdNode;
            resolveData->componentUID = (SceneUID)node.GetMember("ComponentId");
            resolveData->RequireResolve(*this);
            mRequiredResolveData = resolveData;
        }
        else if (auto sceneIdNode = node.FindMember("ActorID"))
        {
            auto resolveData = mmake<SceneRequireResolveData>();
            resolveData->uid = (SceneUID)*sceneIdNode;
            resolveData->componentUID = (SceneUID)node.GetMember("ComponentId");
            resolveData->RequireResolve(*this);
            mRequiredResolveData = resolveData;
        }
        else 
            Set(nullptr);
    }

    void BaseComponentLinkRef::SceneRequireResolveData::RequireResolve(BaseComponentLinkRef& ref)
    {
        ActorRefResolver::RequireResolve(ref, uid, componentUID);
    }

    Ref<BaseComponentLinkRef::IRequiredResolveData> BaseComponentLinkRef::SceneRequireResolveData::Clone() const
    {
        return mmake<SceneRequireResolveData>(*this);
    }

    void BaseComponentLinkRef::AssetRequireResolveData::RequireResolve(BaseComponentLinkRef& ref)
    {
        ActorRefResolver::RequireResolve(ref, uid, componentUID);
    }

    Ref<BaseComponentLinkRef::IRequiredResolveData> BaseComponentLinkRef::AssetRequireResolveData::Clone() const
    {
        return mmake<AssetRequireResolveData>(*this);
    }

}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::Component>);
// --- META ---

DECLARE_CLASS(o2::BaseComponentLinkRef, o2__BaseComponentLinkRef);
// --- END META ---
