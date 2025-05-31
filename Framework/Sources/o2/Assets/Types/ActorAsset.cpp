#include "o2/stdafx.h"
#include "ActorAsset.h"

#include "o2/Assets/Assets.h"
#include "o2/Scene/Actor.h"

#include "o2/Scripts/ScriptValue.h"

namespace o2
{
    ActorAsset::ActorAsset():
        mActor(mmake<Actor>(ActorCreateMode::NotInScene))
    {
        mActor->mIsAsset = true;
        mActor->mAssetId = ID();
    }

    ActorAsset::ActorAsset(const ActorAsset& other):
        AssetWithDefaultMeta<ActorAsset>(other)
    {
        mActor = other.mActor;
    }

    ActorAsset::ActorAsset(const Ref<Actor>& actor):
        mActor(actor)
    {
        if (!actor->IsAsset())
        {
            mActor->RemoveFromScene();
            mActor->mIsAsset = true;
            mActor->mAssetId = ID();
        }
        else
            ID() = mActor->mAssetId;
    }

    ActorAsset::~ActorAsset()
    {
    }

    ActorAsset& ActorAsset::operator=(const ActorAsset& other)
    {
        Asset::operator=(other);

        mActor = other.mActor;

        return *this;
    }

    Ref<Actor> ActorAsset::Instantiate() const
    {
        if (!mActor)
            return nullptr;

        return mActor->CloneAsRef<Actor>();
    }

    Vector<String> ActorAsset::GetFileExtensions()
    {
        return { "proto" };
    }

    void ActorAsset::OnUIDChanged(const UID& oldUID)
    {
        if (mActor)
            mActor->mAssetId = ID();
    }

    void ActorAsset::OnSerialize(DataValue& node) const
    {
        mActor->mIsAsset = false;
        node["mActor"] = mActor;
        mActor->mIsAsset = true;
    }

    void ActorAsset::OnDeserialized(const DataValue& node)
    {
        node["mActor"].Get(mActor);

        ActorRefResolver::ResolveRefs(true);

        if (mActor)
        {
            mActor->RemoveFromScene();
            mActor->mIsAsset = true;
            mActor->mAssetId = GetUID();
        }
    }

    const Ref<Actor>& ActorAsset::GetActor() const
    {
        return mActor;
    }

    void ActorAsset::SetActor(const Ref<Actor>& actor)
    {
        mActor = actor;

        if (mActor)
        {
            if (!actor->IsAsset())
            {
                mActor->RemoveFromScene();
                mActor->mIsAsset = true;
                mActor->mAssetId = ID();
            }
            else
                ID() = mActor->mAssetId;
        }
    }

}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::ActorAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::ActorAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::ActorAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AssetWithDefaultMeta<o2::ActorAsset>>);
// --- META ---

DECLARE_CLASS(o2::ActorAsset, o2__ActorAsset);
// --- END META ---
