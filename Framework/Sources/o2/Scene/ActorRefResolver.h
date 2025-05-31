#pragma once

#include "o2/Scene/ActorLinkRef.h"
#include "o2/Scene/ComponentLinkRef.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Singleton.h"

namespace o2
{
    // -------------------------
    // Actor data node converter
    // -------------------------
    class ActorRefResolver : public Singleton<ActorRefResolver>
    {
    public:
        // Default constructor
        ActorRefResolver(RefCounter* refCounter);

        // Requires to resolve actor reference by actor id
        static void RequireResolve(BaseActorLinkRef& ref, SceneUID actorId);

        // Requires to resolve actor reference by asset id
        static void RequireResolve(BaseActorLinkRef& ref, const UID& assetId);

        // Requires to resolve component reference
        static void RequireResolve(BaseComponentLinkRef& ref, SceneUID actorId, SceneUID id);

        // Requires to resolve component reference
        static void RequireResolve(BaseComponentLinkRef& ref, const UID& assetId, SceneUID id);

        // Requires to remap actor reference
        static void RequireRemap(BaseActorLinkRef& ref);

        // Requires to remap component reference
        static void RequireRemap(BaseComponentLinkRef& ref);

        // Locks references resolving depth
        static void LockResolving(int depth = 1);

        // Unlocks references resolving depth
        static void UnlockResolving(int depth = 1);

        // Resolves references. Works when lock depth is 0
        static void ResolveRefs(bool forcible = false);

        // Returns is resolver depth locked
        static bool IsLocked();

        // Returns lock depth
        static int GetLockDepth();

        // Remaps required refs 
        static void RemapReferences(const Map<const Actor*, Actor*>& actors, const Map<const Component*, Component*>& components);

        // Called when new actor was created
        static void ActorCreated(Actor* actor);

        // Called when actor id was changed
        static void OnActorIdChanged(Actor* actor, SceneUID prevId);

        // Called when new component was created
        static void ComponentCreated(Component* component);

        // Called when component id was changed
        static void OnComponentIdChanged(Component* component, SceneUID prevId);

        // Called when actor reference was destroyed, removes it from unresolved list
        static void OnActorDestroyed(const BaseActorLinkRef* ref);

        // Called when component reference was destroyed, removes it from unresolved list
        static void OnComponentDestroyed(const BaseComponentLinkRef* ref);

    protected:
        struct UnresolvedActor
        {
            BaseActorLinkRef* target;
            SceneUID      sourceId;

        public:
            UnresolvedActor();
            UnresolvedActor(BaseActorLinkRef* target, SceneUID actorId);
            bool operator==(const UnresolvedActor& other) const;
        };

        struct UnresolvedAssetActor
        {
            BaseActorLinkRef* target;
            UID           sourceAssetId;

        public:
            UnresolvedAssetActor();
            UnresolvedAssetActor(BaseActorLinkRef* target, const UID& assetId);

            bool operator==(const UnresolvedAssetActor& other) const;
        };

        struct UnresolvedComponent
        {
            BaseComponentLinkRef* target;
            SceneUID          sourceId;

        public:
            UnresolvedComponent();
            UnresolvedComponent(BaseComponentLinkRef* target, SceneUID id);

            bool operator==(const UnresolvedComponent& other) const;
        };

    protected:
        Vector<UnresolvedActor>      mUnresolvedActorsRefs;
        Vector<UnresolvedAssetActor> mUnresolvedAssetActorsRefs;
        Map<SceneUID, Actor*>        mNewActors;

        Vector<UnresolvedComponent> mUnresolvedComponentsRefs;
        Map<SceneUID, Component*>   mNewComponents;

        Vector<BaseActorLinkRef*>     mRemapActors;
        Vector<BaseComponentLinkRef*> mRemapComponents;

        int mLockDepth = 0;

        friend class Actor;
        friend class Component;
        friend class Scene;
    };
}
