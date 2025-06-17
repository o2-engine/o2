#include "o2/stdafx.h"
#include "ActorRefResolver.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/ActorLinkRef.h"
#include "o2/Scene/Scene.h"

namespace o2
{
    CREATE_SINGLETON(ActorRefResolver);

    ActorRefResolver::ActorRefResolver(RefCounter* refCounter):
        Singleton<ActorRefResolver>(refCounter)
    {}

    void ActorRefResolver::RequireResolve(BaseActorLinkRef& ref, SceneUID actorId)
    {
        if (!mInstance)
            return;

        if (IsLocked())
            mInstance->mUnresolvedActorsRefs.Add(UnresolvedActor(&ref, actorId));
        else
            ref.Set(o2Scene.GetActorByID(actorId).Get());
    }

    void ActorRefResolver::RequireResolve(BaseActorLinkRef& ref, const UID& assetId)
    {
        if (!mInstance)
            return;

        if (IsLocked())
            mInstance->mUnresolvedAssetActorsRefs.Add(UnresolvedAssetActor(&ref, assetId));
        else
            ref.Set(o2Scene.GetAssetActorByID(assetId).Get());
    }

    void ActorRefResolver::RequireResolve(BaseComponentLinkRef& ref, SceneUID actorId, SceneUID id)
    {
        if (!mInstance)
            return;

        if (IsLocked())
            mInstance->mUnresolvedComponentsRefs.Add(UnresolvedComponent(&ref, id));
        else
        {
            if (auto actor = o2Scene.GetActorByID(actorId))
                ref.Set(actor->GetComponent(id).Get());
        }
    }

    void ActorRefResolver::RequireResolve(BaseComponentLinkRef& ref, const UID& assetId, SceneUID id)
    {
        if (!mInstance)
            return;

        if (IsLocked())
            mInstance->mUnresolvedComponentsRefs.Add(UnresolvedComponent(&ref, id));
        else
        {
            if (auto actor = o2Scene.GetAssetActorByID(assetId))
                ref.Set(actor->GetComponent(id).Get());
        }
    }

    bool ActorRefResolver::IsLocked()
    {
        if (!mInstance)
            return false;

        return mInstance->mLockDepth != 0;
    }

    int ActorRefResolver::GetLockDepth()
    {
        if (!mInstance)
            return 0;

        return mInstance->mLockDepth;
    }

    void ActorRefResolver::RemapReferences(const Map<const Actor*, Actor*>& actors,
                                           const Map<const Component*, Component*>& components)
    {
        if (!mInstance)
            return;

		auto remapActors = mInstance->mRemapActors;
		mInstance->mRemapActors.Clear();

        for (auto& ref : remapActors)
        {
            Actor* res = nullptr;
            if (actors.TryGetValue(ref->Get(), res))
                ref->Set(res);
        }

		auto remapComponents = mInstance->mRemapComponents;
		mInstance->mRemapComponents.Clear();

        for (auto& ref : remapComponents)
        {
            Component* res = nullptr;
            if (components.TryGetValue(ref->Get(), res))
                ref->Set(res);
        }
    }

    void ActorRefResolver::RequireRemap(BaseActorLinkRef& ref)
    {
        if (!mInstance)
            return;

        if (!IsLocked())
            return;

        mInstance->mRemapActors.Add(&ref);
    }

    void ActorRefResolver::RequireRemap(BaseComponentLinkRef& ref)
    {
        if (!mInstance)
            return;

        if (!IsLocked())
            return;

        mInstance->mRemapComponents.Add(&ref);
    }

    void ActorRefResolver::LockResolving(int depth /*= 1*/)
    {
        if (!mInstance)
            return;

        mInstance->mLockDepth += depth;
    }

    void ActorRefResolver::UnlockResolving(int depth /*= 1*/)
    {
        if (!mInstance)
            return;

        mInstance->mLockDepth -= depth;

        if (mInstance->mLockDepth == 0)
            ResolveRefs();

        if (mInstance->mLockDepth < 0)
        {
            o2Debug.LogWarning("ActorRefResolver lock depth is less than zero. Something going wrong");
            mInstance->mLockDepth = 0;
        }
    }

    void ActorRefResolver::ResolveRefs(bool forcible /*= false*/)
    {
        if (!mInstance)
            return;

        if (mInstance->mLockDepth > 0 && !forcible)
            return;

        for (auto it = mInstance->mUnresolvedActorsRefs.Begin(); it != mInstance->mUnresolvedActorsRefs.End();)
        {
            auto& unresolvedActor = *it;
            Actor* res = nullptr;
            if (mInstance->mNewActors.TryGetValue(unresolvedActor.sourceId, res))
            {
                unresolvedActor.target->Set(res);
                it = mInstance->mUnresolvedActorsRefs.Remove(it);
            }
            else if (auto actor = o2Scene.GetActorByID(unresolvedActor.sourceId))
            {
                unresolvedActor.target->Set(actor.Get());
                it = mInstance->mUnresolvedActorsRefs.Remove(it);
            }
            else
                ++it;
        }

        auto unresolvedAssetActorsRefs = mInstance->mUnresolvedAssetActorsRefs;
        for (auto it = unresolvedAssetActorsRefs.Begin(); it != unresolvedAssetActorsRefs.End();)
        {
            auto& unresolvedAssetActor = *it;
            if (auto actor = o2Scene.GetAssetActorByID(unresolvedAssetActor.sourceAssetId))
            {
                unresolvedAssetActor.target->Set(actor.Get());
                it = unresolvedAssetActorsRefs.Remove(it);
            }
            else
                ++it;
        }
        
        mInstance->mUnresolvedAssetActorsRefs = unresolvedAssetActorsRefs;

        auto unresolvedComponentsRefs = mInstance->mUnresolvedComponentsRefs;
        for (auto it = unresolvedComponentsRefs.Begin(); it != unresolvedComponentsRefs.End();)
        {
            auto& unresolvedComponent = *it;
            Component* res = nullptr;
            if (mInstance->mNewComponents.TryGetValue(unresolvedComponent.sourceId, res))
            {
                unresolvedComponent.target->Set(res);
                it = unresolvedComponentsRefs.Remove(it);
            }
            else
                ++it;
        }
        
        mInstance->mUnresolvedComponentsRefs = unresolvedComponentsRefs;

        if (mInstance->mLockDepth == 0)
        {
            mInstance->mNewActors.Clear();
            mInstance->mNewComponents.Clear();
            mInstance->mUnresolvedActorsRefs.Clear();
            mInstance->mUnresolvedComponentsRefs.Clear();
        }
    }

    void ActorRefResolver::ActorCreated(Actor* actor)
    {
        if (!IsSingletonInitialzed())
            return;

        if (mInstance->mLockDepth < 1)
            return;

        mInstance->mNewActors[actor->GetID()] = actor;
    }

    void ActorRefResolver::OnActorIdChanged(Actor* actor, SceneUID prevId)
    {
        if (!IsSingletonInitialzed())
            return;

        if (mInstance->mLockDepth < 1)
            return;

        mInstance->mNewActors.Remove(prevId);
        mInstance->mNewActors[actor->GetID()] = actor;
    }

    void ActorRefResolver::ComponentCreated(Component* component)
    {
        if (!IsSingletonInitialzed())
            return;

        if (mInstance->mLockDepth < 1)
            return;

        mInstance->mNewComponents[component->GetID()] = component;
    }

    void ActorRefResolver::OnComponentIdChanged(Component* component, SceneUID prevId)
    {
        if (!IsSingletonInitialzed())
            return;

        if (mInstance->mLockDepth < 1)
            return;

        mInstance->mNewComponents.Remove(prevId);
        mInstance->mNewComponents[component->GetID()] = component;
    }

    void ActorRefResolver::OnActorDestroyed(const BaseActorLinkRef* ref)
    {
        mInstance->mUnresolvedActorsRefs.RemoveFirst([&](auto& x) { return x.target == ref; });
        mInstance->mUnresolvedAssetActorsRefs.RemoveFirst([&](auto& x) { return x.target == ref; });

        mInstance->mRemapActors.Remove(const_cast<BaseActorLinkRef*>(ref));
    }

    void ActorRefResolver::OnComponentDestroyed(const BaseComponentLinkRef* ref)
    {
        mInstance->mUnresolvedComponentsRefs.RemoveFirst([&](auto& x) { return x.target == ref; });
        mInstance->mRemapComponents.Remove(const_cast<BaseComponentLinkRef*>(ref));
    }

    bool ActorRefResolver::UnresolvedActor::operator==(const UnresolvedActor& other) const
    {
        return target == other.target;
    }

    ActorRefResolver::UnresolvedActor::UnresolvedActor(BaseActorLinkRef* target, SceneUID actorId) :
        target(target), sourceId(actorId)
    {}

    ActorRefResolver::UnresolvedActor::UnresolvedActor()
    {}

    ActorRefResolver::UnresolvedComponent::UnresolvedComponent()
    {}

    ActorRefResolver::UnresolvedComponent::UnresolvedComponent(BaseComponentLinkRef* target, SceneUID id):
        target(target), sourceId(id)
    {}

    bool ActorRefResolver::UnresolvedComponent::operator==(const UnresolvedComponent& other) const
    {
        return target == other.target;
    }

    ActorRefResolver::UnresolvedAssetActor::UnresolvedAssetActor()
    {}

    ActorRefResolver::UnresolvedAssetActor::UnresolvedAssetActor(BaseActorLinkRef* target, const UID& assetId):
        target(target), sourceAssetId(assetId)
    {}

    bool ActorRefResolver::UnresolvedAssetActor::operator==(const UnresolvedAssetActor& other) const
    {
        return target == other.target;
    }

}
