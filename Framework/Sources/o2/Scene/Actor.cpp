#include "o2/stdafx.h"
#include "Actor.h"

#include "o2/Scene/ActorRefResolver.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Components/ScriptableComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    FORWARD_REF_IMPL(Component);

    ActorCreateMode Actor::mDefaultCreationMode = ActorCreateMode::InScene;

    Actor::Actor(RefCounter* refCounter, ActorTransform* transform, bool onScene /*= true*/, const String& name /*= "unnamed"*/, 
                 bool enabled /*= true*/, SceneUID id /*= Math::Random()*/, UID assetId /*= UID(0)*/) :
        SceneEditableObject(refCounter), transform(transform), mName(name), mEnabled(enabled),
        mResEnabled(enabled), mResEnabledInHierarchy(false), mId(id), mAssetId(assetId), mState(State::Initializing),
        mIsOnScene(onScene)
    {
        tags.onTagAdded = [&](const Ref<Tag>& tag) { tag->mActors.Add(WeakRef(this)); };
        tags.onTagRemoved = [&](const Ref<Tag>& tag) { tag->mActors.Remove(WeakRef(this)); };

        auto thisRef = Ref<Actor>(this);
        transform->SetOwner(thisRef);

        Scene::OnActorCreated(thisRef);
        ActorRefResolver::ActorCreated(this);
    }

    Actor::Actor(RefCounter* refCounter, ActorTransform* transform, ActorCreateMode mode /*= ActorCreateMode::Default*/) :
        Actor(refCounter, transform, IsModeOnScene(mode))
    {}

    Actor::Actor(RefCounter* refCounter, ActorTransform* transform, const Actor& other, ActorCreateMode mode /*= ActorCreateMode::Default*/) :
        Actor(refCounter, transform, IsModeOnScene(mode), other.mName, other.mEnabled, Math::Random(), other.mAssetId)
    {
        ActorRefResolver::LockResolving();

        ISceneDrawable::operator=(other);
        mSceneLayer = other.mSceneLayer;

        if (other.mIsAsset)
        {
            other.mCopyVisitor = mmake<InstantiatePrototypeCloneVisitor>();
            SetPrototype(AssetRef<ActorAsset>(other.GetAssetID()));
        }

        transform->CopyFrom(*other.transform);
        mAssetId = other.mAssetId;
        mPrototypeLink = other.mPrototypeLink.Lock();

        if (!other.mCopyVisitor)
            other.mCopyVisitor = mmake<SourceToTargetMapCloneVisitor>();

        other.mCopyVisitor->OnCopyActor(&other, this);
        other.mCopyVisitor->depth++;

        if (!mPrototype && other.mPrototype)
            mPrototype = other.mPrototype;

        for (auto& child : other.mChildren)
        {
            child->mCopyVisitor = other.mCopyVisitor;

            AddChild(child->CloneAsRef<Actor>());

            child->mCopyVisitor = nullptr;
        }

        for (auto& component : other.mComponents)
        {
            auto newComponent = AddComponent(component->CloneAsRef<Component>());

            if (other.mCopyVisitor)
                other.mCopyVisitor->OnCopyComponent(component.Get(), newComponent.Get());
        }

        ActorRefResolver::ActorCreated(this);
        ActorRefResolver::UnlockResolving();

        other.CheckCopyVisitorFinalization();
    }

    Actor::Actor(RefCounter* refCounter, ActorTransform* transform, const AssetRef<ActorAsset>& prototype, ActorCreateMode mode /*= ActorCreateMode::Default*/) :
        Actor(refCounter, transform, *prototype->GetActor(), mode)
    {}

    Actor::Actor(RefCounter* refCounter, ActorTransform* transform, Vector<Ref<Component>> components, ActorCreateMode mode /*= ActorCreateMode::Default*/) :
        Actor(refCounter, transform, mode)
    {
        for (auto& comp : components)
            AddComponent(comp);
    }

    Actor::Actor(RefCounter* refCounter, ActorCreateMode mode /*= CreateMode::Default*/) :
        Actor(refCounter, mnew ActorTransform(), mode)
    {}

    Actor::Actor(RefCounter* refCounter, Vector<Ref<Component>> components, ActorCreateMode mode /*= ActorCreateMode::Default*/) :
        Actor(refCounter, mnew ActorTransform(), components, mode)
    {}

    Actor::Actor(RefCounter* refCounter, const AssetRef<ActorAsset>& prototype, ActorCreateMode mode /*= CreateMode::Default*/) :
        Actor(refCounter, mnew ActorTransform(*prototype->GetActor()->transform), prototype, mode)
    {}

    Actor::Actor(RefCounter* refCounter, const Actor& other, ActorCreateMode mode) :
        Actor(refCounter, mnew ActorTransform(*other.transform), other, mode)
    {}

    Actor::Actor(RefCounter* refCounter, const Actor& other) :
        Actor(refCounter, other, ActorCreateMode::Default)
    {}

    Actor::~Actor()
    {
        SetPrototype(nullptr);

        RemoveAllChildren();
        RemoveAllComponents();

        Scene::OnActorDestroy(WeakRef(this));

        delete transform;
    }

    Actor& Actor::operator=(const Actor& other)
    {
        RemoveAllChildren();
        RemoveAllComponents();

        ActorRefResolver::LockResolving();

        if (other.mCopyVisitor)
            other.mCopyVisitor->OnCopyActor(&other, this);

        if (other.mIsAsset)
            SetPrototype(AssetRef<ActorAsset>(other.GetAssetID()));

        ISceneDrawable::operator=(other);

        mName = other.mName;
        mEnabled = other.mEnabled;
        mResEnabled = mEnabled;
        mResEnabledInHierarchy = mEnabled;
        mSceneLayer = other.mSceneLayer;
        transform->CopyFrom(*other.transform);
        mAssetId = other.mAssetId;
        mPrototypeLink = other.mPrototypeLink.Lock();

        if (!other.mCopyVisitor)
            other.mCopyVisitor = mmake<SourceToTargetMapCloneVisitor>();

        other.mCopyVisitor->OnCopyActor(&other, this);
        other.mCopyVisitor->depth++;

        if (!mPrototype && other.mPrototype)
        {
            mPrototype = other.mPrototype;

#if IS_EDITOR
            Scene::LinkActorToPrototypesHierarchy(Ref(this), mPrototype);
#endif
        }

        for (auto& child : other.mChildren)
        {
            child->mCopyVisitor = other.mCopyVisitor;

            AddChild(child->CloneAsRef<Actor>());

            child->mCopyVisitor = nullptr;
        }

        for (auto& component : other.mComponents)
        {
            auto newComponent = AddComponent(component->CloneAsRef<Component>());

            if (other.mCopyVisitor)
                other.mCopyVisitor->OnCopyComponent(component.Get(), newComponent.Get());

            if (mPrototypeLink)
                newComponent->mPrototypeLink = component->mPrototypeLink;
        }

        Scene::OnActorCreated(Ref(this));

        ActorRefResolver::ActorCreated(this);
        ActorRefResolver::UnlockResolving();

        other.CheckCopyVisitorFinalization();
        UpdateResEnabledInHierarchy();

#if IS_EDITOR
        OnChanged();
#endif

        return *this;
    }

    void Actor::CheckCopyVisitorFinalization() const
    {
        if (mCopyVisitor)
        {
            mCopyVisitor->depth--;

            if (mCopyVisitor->depth == 0)
            {
                mCopyVisitor->Finalize();
                mCopyVisitor = nullptr;
            }
        }
    }

    void Actor::Destroy()
    {
        if (Scene::IsSingletonInitialzed())
            o2Scene.DestroyActor(Ref(this));
        else
            o2Debug.LogError("Cant destroy actor in scene, because scene is not exists. Delete actor manually");
    }

    void Actor::Draw()
    {
        PROFILE_SAMPLE_FUNC();

        OnDraw();

        for (auto& component : mComponents)
            component->OnDraw();

        ISceneDrawable::Draw();
    }

    void Actor::Update(float dt)
    {
        PROFILE_SAMPLE_FUNC();

        if (transform->IsDirty())
        {
            for (auto& child : mChildren)
                child->transform->SetDirty(true);

            UpdateSelfTransform();
        }

        OnUpdate(dt);

        for (auto& comp : mComponents)
            comp->OnUpdate(dt);
    }

    void Actor::FixedUpdate(float dt)
    {
        PROFILE_SAMPLE_FUNC();

        OnFixedUpdate(dt);

        for (auto& comp : mComponents)
            comp->OnFixedUpdate(dt);
    }

    void Actor::UpdateChildren(float dt)
    {
        PROFILE_SAMPLE_FUNC();

        for (auto& child : mChildren)
            child->Update(dt);

        for (auto& child : mChildren)
            child->UpdateChildren(dt);
    }

    void Actor::FixedUpdateChildren(float dt)
    {
        PROFILE_SAMPLE_FUNC();

        for (auto& child : mChildren)
            child->FixedUpdate(dt);

        for (auto& child : mChildren)
            child->FixedUpdateChildren(dt);
    }

    void Actor::UpdateTransform()
    {
        UpdateSelfTransform();
        UpdateChildrenTransforms();
    }

    void Actor::UpdateSelfTransform()
    {
        transform->Update();
        
        for (auto& comp : mComponents)
            comp->OnTransformUpdated();
    }

    void Actor::UpdateChildrenTransforms()
    {
        for (auto& child : mChildren)
            child->UpdateSelfTransform();

        for (auto& child : mChildren)
            child->UpdateChildrenTransforms();
    }

    void Actor::SetName(const String& name)
    {
        mName = name;

#if IS_EDITOR
        OnNameChanged();
#endif
    }

    const String& Actor::GetName() const
    {
        return mName;
    }

    SceneUID Actor::GetID() const
    {
        return mId;
    }

    void Actor::SetID(SceneUID id)
    {
        auto prevId = mId;
        mId = id;

        Scene::OnActorIdChanged(Ref(this), prevId);
        ActorRefResolver::OnActorIdChanged(this, prevId);
    }

    UID Actor::GetAssetID() const
    {
        return mAssetId;
    }

    bool Actor::IsAsset() const
    {
        if (mParent)
            return mIsAsset || mParent.Lock()->IsAsset();

        return mIsAsset;
    }

    void Actor::GenerateNewID(bool withChildren /*= true*/)
    {
        SetID(Math::Random());

        if (withChildren)
        {
            for (auto& child : mChildren)
                child->GenerateNewID(withChildren);
        }
    }

    void Actor::AddToScene()
    {
        if (mIsOnScene)
            return;

        mIsOnScene = true;;

        Scene::OnAddActorToScene(Ref(this));

        for (auto& child : mChildren)
            child->AddToScene();
    }

    void Actor::RemoveFromScene(bool keepEditorObjects /*= false*/)
    {
        if (!mIsOnScene)
            return;

        mIsOnScene = false;

        Scene::OnRemoveActorFromScene(Ref(this), keepEditorObjects);

        for (auto& child : mChildren)
            child->RemoveFromScene();
    }

    bool Actor::IsOnScene() const
    {
        return mIsOnScene;
    }

    void Actor::SetEnabled(bool enabled)
    {
        if (mEnabled == enabled)
            return;

        mEnabled = enabled;
        UpdateResEnabled();

#if IS_EDITOR
        onEnableChanged(mEnabled);

        if (mIsOnScene)
            o2Scene.onEnableChanged(Ref(this));

        OnChanged();
#endif
    }

    void Actor::Enable()
    {
        SetEnabled(true);
    }

    void Actor::Disable()
    {
        SetEnabled(false);
    }

    bool Actor::IsEnabled() const
    {
        return mEnabled;
    }

    bool Actor::IsResEnabled() const
    {
        return mResEnabled;
    }

    bool Actor::IsEnabledInHierarchy() const
    {
        return mResEnabledInHierarchy;
    }

    void Actor::SetIndexInSiblings(int index)
    {
        if (mParent)
        {
            auto parent = mParent.Lock();
            int lastIdx = parent->mChildren.IndexOf(Ref(this));
            parent->mChildren.Insert(Ref(this), index);

            if (index <= lastIdx)
                lastIdx++;

            parent->mChildren.RemoveAt(lastIdx);

            parent->OnChildrenChanged();
        }
        else
        {
            if (mIsOnScene)
            {
                int lastIdx = o2Scene.mRootActors.IndexOf(Ref(this));
                o2Scene.mRootActors.Insert(Ref(this), index);

                if (index <= lastIdx)
                    lastIdx++;

                o2Scene.mRootActors.RemoveAt(lastIdx);
            }
        }
    }

    void Actor::SetParent(const Ref<Actor>& actor, bool worldPositionStays /*= true*/, int idx /*= -1*/)
    {
        if ((actor && actor->mParent == this) || actor == this || mParent == actor)
            return;

        auto thisRef = Ref(this);

        // Store initial transform
        Basis lastParentBasis = transform->GetWorldBasis();
        auto oldParent = mParent;

        // Remove from parent list or root actors
        if (mParent)
            mParent.Lock()->RemoveChild(Ref(this), false);
        else if (mIsOnScene && Scene::IsSingletonInitialzed())
            o2Scene.mRootActors.Remove(thisRef);

        // Change parent
        mParent = actor;
        transform->mData->parentInvTransformActualFrame = 0;

        // Restore in new parent children list or root actors
        if (mParent)
        {
            auto parent = mParent.Lock();
            if (idx < 0)
                parent->mChildren.Add(thisRef);
            else
                parent->mChildren.Insert(thisRef, idx);

            parent->OnChildAdded(thisRef);
        }
        else if (mIsOnScene && Scene::IsSingletonInitialzed())
        {
            if (idx < 0)
                o2Scene.mRootActors.Add(thisRef);
            else
                o2Scene.mRootActors.Insert(thisRef, idx);
        }

        // Restore transform if needed
        if (worldPositionStays)
            transform->SetWorldBasis(lastParentBasis);
        else
            transform->SetDirty();

        // Call events only when actor was initialized
        if (mState == State::Default)
        {
            // Update enabled state in hierarchy
            UpdateResEnabledInHierarchy();

            // Checks if actor should be added or removed from scene
            if (mParent && mParent.Lock()->mIsOnScene != mIsOnScene)
            {
                if (mParent.Lock()->mIsOnScene && !mIsOnScene)
                    AddToScene();
                else if (!mParent.Lock()->mIsOnScene && mIsOnScene)
                    RemoveFromScene();
            }
        }
        else if (mState == State::Initializing && mParent && mParent.Lock()->mState == State::Initializing)
        {
            Scene::OnNewActorParented(Ref(this));
        }

        // Call parent update events
        OnParentChanged(oldParent.Lock());
    }

    const WeakRef<Actor>& Actor::GetParent() const
    {
        return mParent;
    }

    Ref<Actor> Actor::AddChild(const Ref<Actor>& actor)
    {
        Assert(actor, "Actor is null");

        actor->SetParent(Ref(this), false);
        return actor;
    }

    Ref<Actor> Actor::AddChild(const Ref<Actor>& actor, int index)
    {
        Assert(actor, "Actor is null");

        actor->SetParent(Ref(this), false, index);
        return actor;
    }

    void Actor::AddChildren(const Vector<Ref<Actor>>& actors)
    {
        for (auto& actor : actors)
            AddChild(actor);
    }

    Ref<Actor> Actor::GetChild(const String& path) const
    {
        int delPos = path.Find("/");
        String pathPart = path.SubStr(0, delPos);

        if (pathPart == "..")
        {
            if (mParent)
            {
                if (delPos == -1)
                    return mParent.Lock();
                else
                    return mParent.Lock()->GetChild(path.SubStr(delPos + 1));
            }

            return nullptr;
        }

        for (auto& child : mChildren)
        {
            if (child->mName == pathPart)
            {
                if (delPos == -1)
                    return child;
                else
                    return child->GetChild(path.SubStr(delPos + 1));
            }
        }

        return nullptr;
    }

    Ref<Actor> Actor::FindChild(const String& name) const
    {
        for (auto& child : mChildren)
        {
            if (child->mName == name)
                return child;

            if (auto res = child->FindChild(name))
                return res;
        }

        return nullptr;
    }

    Ref<Actor> Actor::FindChild(const Function<bool(const Ref<Actor>& child)>& pred) const
    {
        for (auto& child : mChildren)
        {
            if (pred(child))
                return child;

            if (auto res = child->FindChild(pred))
                return res;
        }

        return nullptr;
    }

    const Vector<Ref<Actor>>& Actor::GetChildren() const
    {
        return mChildren;
    }

    void Actor::GetAllChildrenActors(Vector<Ref<Actor>>& actors)
    {
        actors.Add(mChildren);

        for (auto& child : mChildren)
            child->GetAllChildrenActors(actors);
    }

    void Actor::RemoveChild(const Ref<Actor>& actor, bool withEvent /*= true*/)
    {
        auto oldParent = actor->mParent.Lock();

        actor->mParent = nullptr;
        mChildren.RemoveFirst([&](auto& x) { return x == actor; });

        if (withEvent)
            actor->OnParentChanged(oldParent);

        OnChildRemoved(actor);

        if (actor->mState != State::Destroyed)
        {
            actor->transform->SetDirty();
            actor->UpdateResEnabledInHierarchy();
        }
    }

    void Actor::RemoveAllChildren()
    {
        for (auto& child : mChildren)
        {
            child->mParent = nullptr;

            OnChildRemoved(child);
            child->OnParentChanged(Ref(this));
        }

        mChildren.Clear();
    }

    Ref<Actor> Actor::FindActorById(SceneUID id)
    {
        if (mId == id)
            return Ref(this);

        for (auto& child : mChildren)
        {
            if (auto res = child->FindActorById(id))
                return res;
        }

        return nullptr;
    }

    Ref<Component> Actor::AddComponent(const Ref<Component>& component)
    {
        component->SetOwnerActor(Ref(this));
        mComponents.Add(component);

        if (mIsOnScene && mState == State::Default)
            component->AddToScene();

        OnComponentAdded(component);

        component->OnTransformUpdated();

#if IS_EDITOR
        OnChanged();
#endif

        return component;
    }

    void Actor::RemoveComponent(const Ref<Component>& component)
    {
        if (mIsOnScene)
            component->RemoveFromScene();

        OnComponentRemoving(component);

        mComponents.Remove(component);
        component->mOwner = nullptr;

#if IS_EDITOR
        OnChanged();
#endif
    }

    void Actor::RemoveAllComponents()
    {
        auto components = mComponents;
        mComponents.Clear();

        for (auto& component : components)
        {
            if (mIsOnScene)
                component->RemoveFromScene();

            OnComponentRemoving(component);
        }

#if IS_EDITOR
        OnChanged();
#endif
    }

    Ref<Component> Actor::GetComponent(const String& typeName)
    {
        for (auto& comp : mComponents)
        {
            if (comp->GetType().GetName() == typeName)
                return comp;
        }

        return nullptr;
    }

    Ref<Component> Actor::GetComponent(const Type* type)
    {
        for (auto& comp : mComponents)
        {
            if (comp->GetType().IsBasedOn(*type))
                return comp;
        }

        return nullptr;
    }

    Ref<Component> Actor::GetComponent(SceneUID id)
    {
        for (auto& comp : mComponents)
        {
            if (comp->GetID() == id)
                return comp;
        }

        return nullptr;
    }

#if IS_SCRIPTING_SUPPORTED
    Ref<Component> Actor::GetComponent(const ScriptValue& typeValue)
    {
        auto proto = typeValue.GetPrototype();
        auto protoYpe = proto.GetValueType();
        if (proto.IsObject())
        {
            auto typeProp = proto.GetProperty("type");
            if (typeProp.IsObjectContainer())
            {
                const Type* type = typeProp.GetValue<const Type*>();
                for (auto& component : mComponents)
                {
                    if (component->GetType().IsBasedOn(*type))
                        return component;
                }
            }
        }
        else
        {
            for (auto& component : mComponents)
            {
                if (auto scriptComponent = DynamicCast<ScriptableComponent>(component))
                {
                    if (scriptComponent->GetClass() == typeValue)
                        return component;
                }
            }
        }

        return nullptr;
    }

    ActorTransform* Actor::GetTransform()
    {
        return transform;
    }
#endif

    const Vector<Ref<Component>>& Actor::GetComponents() const
    {
        return mComponents;
    }

    void Actor::SetDefaultCreationMode(ActorCreateMode mode)
    {
        if (mode == ActorCreateMode::Default)
            mode = ActorCreateMode::InScene;

        mDefaultCreationMode = mode;
    }

    ActorCreateMode Actor::GetDefaultCreationMode()
    {
        return mDefaultCreationMode;
    }

    bool Actor::IsModeOnScene(ActorCreateMode mode)
    {
        return mode == ActorCreateMode::InScene || (mode == ActorCreateMode::Default && mDefaultCreationMode != ActorCreateMode::NotInScene);
    }

    void Actor::OnAddToScene()
    {
        ISceneDrawable::OnAddToScene();

        for (auto& comp : mComponents)
            comp->AddToScene();
    }

    void Actor::OnRemoveFromScene()
    {
        ISceneDrawable::OnRemoveFromScene();

        for (auto& comp : mComponents)
            comp->RemoveFromScene();
    }

    void Actor::OnInitialized()
    {
        mState = State::Default;

        for (auto& comp : mComponents)
            comp->OnInitialized();
    }

    void Actor::OnStart()
    {
        for (auto& comp : mComponents)
            comp->OnStart();
    }

    void Actor::OnDestroy()
    {}

    void Actor::OnDraw()
    {}

    void Actor::OnUpdate(float dt)
    {}

    void Actor::OnFixedUpdate(float dt)
    {}

    void Actor::OnEnabled()
    {
        ISceneDrawable::OnEnabled();
    }

    void Actor::OnDisabled()
    {
        ISceneDrawable::OnDisabled();
    }

    void Actor::OnTransformUpdated()
    {
        for (auto& comp : mComponents)
            comp->OnTransformUpdated();
    }

    void Actor::OnChildAdded(const Ref<Actor>& child)
    {
        for (auto& comp : mComponents)
            comp->OnChildAdded(child);

        OnChildrenChanged();
    }

    void Actor::OnChildRemoved(const Ref<Actor>& child)
    {
        for (auto& comp : mComponents)
            comp->OnChildRemoved(child);

        OnChildrenChanged();
    }

    void Actor::OnComponentAdded(const Ref<Component>& component)
    {
        for (auto& comp : mComponents)
            comp->OnComponentAdded(component);
    }

    void Actor::OnComponentRemoving(const Ref<Component>& component)
    {
        for (auto& comp : mComponents)
            comp->OnComponentRemoving(component);
    }

#if !IS_EDITOR
    void Actor::OnChildrenChanged()
    {
        for (auto& comp : mComponents)
            comp->OnChildrenChanged();
    }

    void Actor::OnParentChanged(const Ref<Actor>& oldParent) 
    {
        ISceneDrawable::OnDrawbleParentChanged();

        for (auto& comp : mComponents)
            comp->OnParentChanged(oldParent);
    }
#endif

    void Actor::UpdateResEnabled(bool withChildren /*= true*/)
    {
        mResEnabled = mEnabled;
        UpdateResEnabledInHierarchy(withChildren);
    }

    void Actor::UpdateResEnabledInHierarchy(bool withChildren /*= true*/)
    {
        bool lastResEnabledInHierarchy = mResEnabledInHierarchy;

        if (mParent)
            mResEnabledInHierarchy = mResEnabled && mParent.Lock()->mResEnabledInHierarchy;
        else
            mResEnabledInHierarchy = mResEnabled;

        if (lastResEnabledInHierarchy != mResEnabledInHierarchy)
        {
            if (mResEnabledInHierarchy)
                OnEnabled();
            else
                OnDisabled();

            for (auto& comp : mComponents)
                comp->UpdateEnabledInHierarchy();

#if IS_EDITOR
            if (mIsOnScene)
                o2Scene.onEnableChanged(Ref(this));

            OnChanged();
#endif
        }

        if (withChildren)
        {
            for (auto& child : mChildren)
                child->UpdateResEnabledInHierarchy();
        }
    }

    void Actor::SerializeBasicOverride(DataValue& node) const
    {
        if (mPrototypeLink)
            SerializeWithProto(node);
        else
            SerializeRaw(node);

        OnSerialize(node);
    }

    void Actor::DeserializeBasicOverride(const DataValue& node)
    {
        if (node.FindMember("PrototypeLink") || node.FindMember("Prototype"))
            DeserializeWithProto(node);
        else
            DeserializeRaw(node);

        OnDeserialized(node);
    }

    void Actor::SerializeRaw(DataValue& node) const
    {
        SerializeBasic(node);

        node["Id"] = mId;
        node["Transform"].Set(*transform);

        if (!mChildren.IsEmpty())
        {
            auto& childsNode = node.AddMember("Children");
            for (auto& child : mChildren)
            {
                auto& childNode = childsNode.AddElement();
                childNode.AddMember("Type") = child->GetType().GetName();
                child->Serialize(childNode.AddMember("Data"));
            }
        }

        if (!mComponents.IsEmpty())
        {
            auto& componentsNode = node.AddMember("Components");
            for (auto& component : mComponents)
            {
                auto& componentNode = componentsNode.AddElement();
                componentNode.AddMember("Type") = component->GetType().GetName();
                component->Serialize(componentNode.AddMember("Data"));
            }
        }
    }

    void Actor::DeserializeRaw(const DataValue& node)
    {
        DeserializeBasic(node);

        ActorRefResolver::Instance().LockResolving();
        if (ActorRefResolver::Instance().mLockDepth == 0)
            ActorRefResolver::Instance().ActorCreated(this);

        SetID(node.GetMember("Id"));
        node.GetMember("Transform").Get(*transform);

        RemoveAllComponents();

        if (auto componentsNode = node.FindMember("Components"))
        {
            if (componentsNode->IsArray())
            {
                for (auto& componentNode : *componentsNode)
                {
                    String componentType = componentNode.GetMember("Type");
                    auto component = DynamicCast<Component>(o2Reflection.CreateTypeSampleRef(componentType));
                    if (component)
                    {
                        component->Deserialize(componentNode.GetMember("Data"));
                        AddComponent(component);
                    }
                    else
                        o2Debug.LogError("Can't create component by type %s", componentType.Data());

                }
            }
        }

        RemoveAllChildren();

        if (auto childsNode = node.FindMember("Children"))
        {
            if (childsNode->IsArray())
            {
                for (auto& childNode : *childsNode)
                {
                    const DataValue* typeNode = childNode.FindMember("Type");
                    const DataValue* dataValue = childNode.FindMember("Data");
                    if (typeNode && dataValue)
                    {
                        const ObjectType* type = dynamic_cast<const ObjectType*>(o2Reflection.GetType(*typeNode));
                        if (type)
                        {
                            auto child = DynamicCast<Actor>(type->CreateSampleRef());
                            child->Deserialize(*dataValue);
                            o2Scene.mRootActors.Remove(child);
                            AddChild(child);
                        }
                    }
                }
            }
        }

        ActorRefResolver::Instance().UnlockResolving();
        ActorRefResolver::Instance().ResolveRefs();
    }

    void Actor::SerializeWithProto(DataValue& node) const
    {
        const Actor* proto = mPrototypeLink.Lock().Get();

        // Prototype data
        if (mPrototype)
            node["Prototype"] = mPrototype;

        if (mPrototypeLink)
            node["PrototypeLink"] = proto->GetID();

        // Basic data
        node["Id"] = mId;

        // Transform data
        auto& transformNode = node.AddMember("Transform");
        transform->SerializeDelta(transformNode, *proto->transform);

        if (transformNode.IsEmpty() || transformNode.IsNull())
            node.RemoveMember("Transform");

        SerializeDelta(node, *mPrototypeLink.Lock().Get());

        // Children data
        if (!mChildren.IsEmpty())
        {
            auto& childsNode = node.AddMember("Children");
            childsNode.SetArray();
            for (auto& child : mChildren)
            {
                auto& childNode = childsNode.AddElement();
                childNode.AddMember("Type") = child->GetType().GetName();
                child->Serialize(childNode.AddMember("Data"));
            }
        }

        // Components data
        if (!mComponents.IsEmpty())
        {
            auto& componentsNode = node.AddMember("Components");
            componentsNode.SetArray();
            for (auto& component : mComponents)
            {
                auto& compNode = componentsNode.AddElement();
                compNode.AddMember("Type") = component->GetType().GetName();

                auto& dataValue = compNode.AddMember("Data");
                if (auto componentProtoLink = component->mPrototypeLink)
                {
                    dataValue["PrototypeLink"] = componentProtoLink.Lock()->mId;
                    dataValue.SetDelta(*component, *component->mPrototypeLink.Lock());
                }
                else
                    component->Serialize(dataValue);
            }
        }
    }

    void Actor::DeserializeWithProto(const DataValue& node)
    {
        RemoveAllChildren();
        RemoveAllComponents();

        ActorRefResolver::Instance().LockResolving();
        ActorRefResolver::Instance().ActorCreated(this);

        if (auto prototypeNode = node.FindMember("Prototype"))
            SetPrototype(*prototypeNode);

        if (auto prototypeLinkNode = node.FindMember("PrototypeLink"))
        {
            SceneUID id = *prototypeLinkNode;
            auto parent = mParent.Lock();
            while (parent && parent->mPrototypeLink)
            {
                bool found = false;

                Actor* protoLink = parent->mPrototypeLink.Lock().Get();
                while (protoLink)
                {
                    if (auto fnd = protoLink->FindActorById(id))
                    {
                        mPrototypeLink = fnd;
                        found = true;
                        break;
                    }

                    protoLink = protoLink->mPrototypeLink.Lock().Get();
                }

                if (found)
                    break;

                parent = parent->mParent.Lock();
            }
        }

        if (mPrototypeLink)
            DeserializeDelta(node, *mPrototypeLink.Lock().Get());
        else
            DeserializeBasic(node);

        SetID(node.GetMember("Id"));

        if (!mPrototypeLink)
        {
            ActorRefResolver::Instance().UnlockResolving();
            ActorRefResolver::Instance().ResolveRefs();
            return;
        }

        const Actor* proto = mPrototypeLink.Lock().Get();

        if (!mCopyVisitor)
            mCopyVisitor = mmake<SourceToTargetMapCloneVisitor>();

        mCopyVisitor->depth++;
        mCopyVisitor->OnCopyActor(proto, this);

        // Transform data
        if (auto transformNode = node.FindMember("Transform"))
            transform->DeserializeDelta(*transformNode, *proto->transform);
        else
            transform->CopyFrom(*proto->transform);

        // children
        auto childrenNode = node.FindMember("Children");
        if (childrenNode && childrenNode->IsArray())
        {
            for (auto& childNode : *childrenNode)
            {
                const DataValue* typeNode = childNode.FindMember("Type");
                const DataValue* dataValue = childNode.FindMember("Data");
                if (typeNode && dataValue)
                {
                    const ObjectType* type = dynamic_cast<const ObjectType*>(o2Reflection.GetType(*typeNode));
                    if (type)
                    {
                        auto child = DynamicCast<Actor>(type->CreateSampleRef());
                        child->mParent = Ref(this);
                        child->mCopyVisitor = mCopyVisitor;
                        child->Deserialize(*dataValue);
                        child->mParent = nullptr;
                        AddChild(child);
                    }
                }
            }
        }

        // components
        auto componentsNode = node.FindMember("Components");
        if (componentsNode && componentsNode->IsArray())
        {
            for (auto& componentNode : *componentsNode)
            {
                String type = componentNode["Type"];
                auto newComponent = DynamicCast<Component>(o2Reflection.CreateTypeSampleRef(type));
                if (!newComponent)
                {
                    o2Debug.LogError("Can't create component with type:" + type);
                    continue;
                }

                auto& componentDataValue = componentNode["Data"];

                if (auto prototypeLinkNode = componentDataValue.FindMember("PrototypeLink"))
                {
                    SceneUID id = *prototypeLinkNode;
                    if (mPrototypeLink)
                    {
                        for (auto& protoLinkComponent : mPrototypeLink.Lock()->mComponents)
                        {
                            if (protoLinkComponent->mId == id)
                            {
                                newComponent->mPrototypeLink = protoLinkComponent;
                                break;
                            }
                        }
                    }
                }

                if (!newComponent->mPrototypeLink)
                    newComponent->Deserialize(componentDataValue);
                else
                {
                    componentDataValue.GetDelta(*newComponent, *newComponent->mPrototypeLink.Lock());
                    mCopyVisitor->OnCopyComponent(newComponent->mPrototypeLink.Lock().Get(), newComponent.Get());
                }

                AddComponent(newComponent);
            }
        }

        transform->SetDirty();

        ActorRefResolver::Instance().UnlockResolving();
        ActorRefResolver::Instance().ResolveRefs();

        if (mCopyVisitor)
        {
            mCopyVisitor->depth--;

            if (mCopyVisitor->depth == 0)
            {
                mCopyVisitor->Finalize();
                mCopyVisitor = nullptr;
            }
        }
    }

    AssetRef<ActorAsset> Actor::GetPrototype() const
    {
        if (mPrototype)
            return mPrototype;

        if (mPrototypeLink && mParent)
            return mParent.Lock()->GetPrototype();

        return AssetRef<ActorAsset>();
    }

    AssetRef<ActorAsset> Actor::GetPrototypeDirectly() const
    {
        return mPrototype;
    }

    void Actor::SetPrototype(AssetRef<ActorAsset> asset)
    {
#if IS_EDITOR
        if (Scene::IsSingletonInitialzed())
            o2Scene.OnActorPrototypeBroken(this);
#endif

        mPrototype = asset;

        if (asset)
            mPrototypeLink = asset->GetActor();
        else
            mPrototypeLink = nullptr;

#if IS_EDITOR
        if (mPrototype)
            Scene::LinkActorToPrototypesHierarchy(Ref(this), mPrototype);
#endif
    }

    Ref<Actor> Actor::GetPrototypeLink() const
    {
        return mPrototypeLink.Lock();
    }

    void Actor::SetLayer(const Ref<SceneLayer>& layer)
    {
        mSceneLayer = layer;
        ISceneDrawable::SetDrawingDepthInheritFromParent(false); // Reregister inside
    }

    void Actor::SetLayer(const String& name)
    {
        SetLayer(o2Scene.GetLayer(name));
    }

    const Ref<SceneLayer>& Actor::GetLayer() const
    {
        return mSceneLayer;
    }

    Ref<SceneLayer> Actor::GetSceneDrawableSceneLayer() const
    {
        if (mSceneLayer)
            return mSceneLayer;

        return o2Scene.GetDefaultLayer();
    }

    Ref<ISceneDrawable> Actor::GetParentDrawable()
    {
        return DynamicCast<ISceneDrawable>(mParent.Lock());
    }

    int Actor::GetIndexInParentDrawable() const
    {
        if (mParent)
            return mParent.Lock()->mChildren.IndexOf(Ref(const_cast<Actor*>(this)));

        if (Scene::IsSingletonInitialzed() && mIsOnScene)
            return o2Scene.mRootActors.IndexOf(Ref(const_cast<Actor*>(this)));

        return 0;
    }

    Map<String, Ref<Actor>> Actor::GetAllChilds()
    {
        Map<String, Ref<Actor>> res;
        for (auto& child : mChildren)
            res.Add(child->GetName(), child);

        return res;
    }

    Map<String, Ref<Component>> Actor::GetAllComponents()
    {
        Map<String, Ref<Component>> res;
        for (auto& child : mComponents)
            res.Add(child->GetType().GetName(), child);

        return res;
    }

    void Actor::OnBeforeDestroy()
    {
        if (mResEnabledInHierarchy)
            OnDisabled();

        OnDestroy();

        mState = State::Destroyed;
    }

    String Actor::GetCreateMenuCategory()
    {
        return "";
    }

    String Actor::GetCreateMenuGroup()
    {
        return "";
    }

    void Actor::SourceToTargetMapCloneVisitor::OnCopyActor(const Actor* source, Actor* target)
    {
        sourceToTargetActors[source] = target;
    }

    void Actor::SourceToTargetMapCloneVisitor::OnCopyComponent(const Component* source, Component* target)
    {
        sourceToTargetComponents[source] = target;
    }

    void Actor::SourceToTargetMapCloneVisitor::Finalize()
    {
        ActorRefResolver::RemapReferences(sourceToTargetActors, sourceToTargetComponents);
    }

    void Actor::InstantiatePrototypeCloneVisitor::OnCopyActor(const Actor* source, Actor* target)
    {
        SourceToTargetMapCloneVisitor::OnCopyActor(source, target);
        target->mPrototypeLink = Ref(const_cast<Actor*>(source));
    }

    void Actor::InstantiatePrototypeCloneVisitor::OnCopyComponent(const Component* source, Component* target)
    {
        SourceToTargetMapCloneVisitor::OnCopyComponent(source, target);
        target->mPrototypeLink = Ref(const_cast<Component*>(source));
    }
}
// --- META ---

ENUM_META(o2::Actor::State)
{
    ENUM_ENTRY(Default);
    ENUM_ENTRY(Destroyed);
    ENUM_ENTRY(Destroying);
    ENUM_ENTRY(Initializing);
}
END_ENUM_META;

DECLARE_CLASS(o2::Actor, o2__Actor);
// --- END META ---
