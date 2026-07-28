#include "o2/stdafx.h"
#include "Actor.h"

#include "o2/Scene/ActorRefResolver.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Editor/ActorDifferences.h"
#include "o2/Utils/Editor/Attributes/PrototypeDeltaSearchAttribute.h"
#include "o2/Utils/Serialization/Serializable.h"

#if IS_EDITOR

namespace o2
{
    void Actor::MakePrototypeCloneVisitor::OnCopyActor(const Actor* source, Actor* target)
    {
        SourceToTargetMapCloneVisitor::OnCopyActor(source, target);
        target->mPrototypeLink = source->mPrototypeLink;
        const_cast<Actor*>(source)->mPrototypeLink = Ref(target);
    }

    void Actor::MakePrototypeCloneVisitor::OnCopyComponent(const Component* source, Component* target)
    {
        SourceToTargetMapCloneVisitor::OnCopyComponent(source, target);
        target->mPrototypeLink = source->mPrototypeLink;
        const_cast<Component*>(source)->mPrototypeLink = Ref(target);
    }

    void Actor::SetProtytypeDummy(AssetRef<ActorAsset> asset)
    {}

    void Actor::BreakPrototypeLink()
    {
        if (!mPrototype && !mPrototypeLink)
            return;

        SetPrototype(AssetRef<ActorAsset>());

        // remove from cache

        for (auto& child : mChildren)
            child->BreakPrototypeLink();

        for (auto& component : mComponents)
            component->mPrototypeLink = nullptr;

        OnChanged();
    }

    void Actor::RevertToPrototype()
    {
        if (!mPrototypeLink)
            return;

        Vector<Ref<Actor>> separatedActors;
        SeparateActors(separatedActors);

        for (auto it = separatedActors.Begin(); it != separatedActors.End();)
        {
            if ((*it)->mPrototypeLink)
                ++it;
            else
                it = separatedActors.Remove(it);
        }

        Vector<Actor**> actorPointersFields;
        Vector<Component**> componentPointersFields;
        Map<const Actor*, Actor*> actorsMap;
        Map<const Component*, Component*> componentsMap;
        Vector<ISerializable*> serializableObjects;

        ProcessReverting(this, mPrototypeLink.Lock().Get(), separatedActors, actorPointersFields, componentPointersFields, actorsMap,
                         componentsMap, serializableObjects);

        FixComponentFieldsPointers(actorPointersFields, componentPointersFields, actorsMap, componentsMap);

        UpdateResEnabledInHierarchy();
        transform->SetDirty();

        OnChanged();
    }

    AssetRef<ActorAsset> Actor::MakePrototype()
    {
        mCopyVisitor = mmake<MakePrototypeCloneVisitor>();
        auto prototype = CloneAsRef<Actor>();

        AssetRef<ActorAsset> prototypeAsset = AssetRef<ActorAsset>::CreateAsset(prototype);
        SetPrototype(prototypeAsset);

        prototype->UpdateResEnabledInHierarchy();
        prototype->UpdateTransform();
        prototype->OnChanged();

        return prototypeAsset;
    }

    void Actor::SeparateActors(Vector<Ref<Actor>>& separatedActors)
    {
        for (auto& child : mChildren)
        {
            child->mParent = nullptr;
            separatedActors.Add(child);

            child->SeparateActors(separatedActors);
        }

        mChildren.Clear();
    }

    void Actor::ApplyChangesToPrototype()
    {
        if (!mPrototype)
            return;

        // Get difference between this actor and prototype first
        auto diffs = ActorDifferences::GetDifference(Ref(this), mPrototype->GetActor());

        // Get applying actors infos
        ApplyActorInfo prototypeApplyInfo(mPrototype->GetActor());
        ApplyActorInfo thisApplyInfo(Ref(this));
        Vector<ApplyActorInfo> linkedActorsApplyInfos;

        auto& linkedActors = o2Scene.mPrototypeLinksCache[mPrototype];
        linkedActorsApplyInfos.Reserve(linkedActors.Count());
        linkedActors.ForEach([&](auto x) { if (x != this) linkedActorsApplyInfos.Add(ApplyActorInfo(x.Lock())); });
    
        // Apply differences
        diffs.newChildren.ForEach([&](auto d) { d->Apply(thisApplyInfo, prototypeApplyInfo, linkedActorsApplyInfos); });
        diffs.movedChildren.ForEach([&](auto d) { d->Apply(thisApplyInfo, prototypeApplyInfo, linkedActorsApplyInfos); });
        diffs.newComponents.ForEach([&](auto d) { d->Apply(thisApplyInfo, prototypeApplyInfo, linkedActorsApplyInfos); });
        diffs.changedActorFields.ForEach([&](auto d) { d->Apply(thisApplyInfo, prototypeApplyInfo, linkedActorsApplyInfos); });
        diffs.changedComponentFields.ForEach([&](auto d) { d->Apply(thisApplyInfo, prototypeApplyInfo, linkedActorsApplyInfos); });
        diffs.removedComponents.ForEach([&](auto d) { d->Apply(thisApplyInfo, prototypeApplyInfo, linkedActorsApplyInfos); });
        diffs.removedChildren.ForEach([&](auto d) { d->Apply(thisApplyInfo, prototypeApplyInfo, linkedActorsApplyInfos); });

        // Invoke changed callback for actors and save assets
        for (auto& info : linkedActorsApplyInfos)
        {
            info.actor->OnChanged();
            info.actor->UpdateTransform();

            if (info.actor->IsAsset())
            {
                AssetRef<ActorAsset> asset(info.actor->GetAssetID());
                if (info.actor != asset->GetActor())
                    asset->SetActor(info.actor);

                asset->Save();
            }
        }

        // Save prototype
        mPrototype->GetActor()->UpdateTransform();
        mPrototype->GetActor()->OnChanged();
        mPrototype->Save();

        OnChanged();
    }

    void Actor::GetDifferences(ActorDifferences& differences) const
    {
        Vector<const FieldInfo*> stack;
        stack.Add(GetType().GetField("transform"));

        auto createActorChangedFieldDiff = [&](const String& fieldPath) {
            auto diff = mmake<ActorDifferences::ChangedObjectField>();
            diff->prototypeLink = mPrototypeLink.Lock();
            diff->path = fieldPath;
            differences.changedActorFields.Add(diff);
        };

        ActorDifferences::GetObjectDifferences(createActorChangedFieldDiff, stack, transform->GetType(), transform, mPrototypeLink.Lock()->transform);
    }

    void Actor::BeginMakePrototype() const
    {
        mCopyVisitor = mmake<MakePrototypeCloneVisitor>();
    }

    void Actor::BeginInstantiatePrototype() const
    {
        mCopyVisitor = mmake<InstantiatePrototypeCloneVisitor>();
    }

    void Actor::DrawGizmos()
    {
        OnDrawGizmos();
    }

    bool Actor::IsLinkedToActor(const Ref<Actor>& actor) const
    {
        if (mPrototypeLink)
        {
            auto t = mPrototypeLink.Lock().Get();
            while (t)
            {
                if (t == actor.Get())
                    return true;

                t = t->mPrototypeLink.Lock().Get();
            }
        }

        return false;
    }

    Ref<Actor> Actor::FindLinkedActor(const Ref<Actor>& linkActor)
    {
        if (GetPrototypeLink() == linkActor)
            return Ref(this);

        for (auto& child : mChildren)
        {
            if (auto res = child->FindLinkedActor(linkActor))
                return res;
        }

        return nullptr;
    }

    bool Actor::IsSupportsLocking() const
    {
        return true;
    }

    void Actor::SetLocked(bool locked)
    {
        mLocked = locked;
        UpdateLocking();

        OnLockChanged();
    }

    void Actor::Lock()
    {
        SetLocked(true);
    }

    void Actor::Unlock()
    {
        SetLocked(false);
    }

    bool Actor::IsLocked() const
    {
        return mLocked;
    }

    bool Actor::IsLockedInHierarchy() const
    {
        return mResLocked;
    }

    void Actor::UpdateLocking()
    {
        bool lastResLocked = mResLocked;

        if (mParent)
            mResLocked = mLocked || mParent.Lock()->mResLocked;
        else
            mResLocked = mLocked;

        if (lastResLocked != mResLocked)
            OnLockChanged();

        for (auto& child : mChildren)
            child->UpdateLocking();
    }

    bool Actor::IsSupportsTransforming() const
    {
        return true;
    }

    Basis Actor::GetTransform() const
    {
        return transform->GetWorldBasis();
    }

    void Actor::SetTransform(const Basis& transform)
    {
        this->transform->SetWorldBasis(transform);
    }

    bool Actor::IsSupportsPivot() const
    {
        return true;
    }

    void Actor::SetPivot(const Vec2F& pivot)
    {
        transform->SetWorldPivot(pivot);
    }

    Vec2F Actor::GetPivot() const
    {
        return transform->GetWorldPivot();
    }

    void Actor::AddEditableChild(const Ref<SceneEditableObject>& object, int idx /*= -1*/)
    {
        if (auto actor = DynamicCast<Actor>(object))
        {
            if (idx >= 0)
                AddChild(actor, idx);
            else
                AddChild(actor);
        }
    }

    bool Actor::IsSupportsDisabling() const
    {
        return true;
    }

    Vector<Ref<SceneEditableObject>> Actor::GetEditableChildren() const
    {
        Vector<Ref<SceneEditableObject>> res;
        for (auto& child : mChildren) {
            res.Add(DynamicCast<SceneEditableObject>(child));
        }

        return res;
    }

    void Actor::SetEditableParent(const Ref<SceneEditableObject>& object, int idx /*= -1*/)
    {
        SetParent(DynamicCast<Actor>(object), false, idx);
    }

    Ref<SceneEditableObject> Actor::GetEditableOwner()
    {
        return Ref(this);
    }

    Ref<SceneEditableObject> Actor::GetEditableLink() const
    {
        return Ref(const_cast<Actor*>(mPrototypeLink.Lock().Get()));
    }

    Ref<SceneEditableObject> Actor::GetEditableParent() const
    {
        return mParent.Lock();
    }

    void Actor::OnChanged()
    {
        onChanged();

        if (Scene::IsSingletonInitialzed() && IsOnScene() && mState != State::Destroyed && mState != State::Destroying)
            o2Scene.OnObjectChanged(Ref(this));
    }

    void Actor::OnLockChanged()
    {
        onLockChanged(mLocked);
        onChanged();

        if (Scene::IsSingletonInitialzed() && IsOnScene())
        {
            o2Scene.OnObjectChanged(Ref(this));
            o2Scene.onLockChanged(Ref(this));
        }
    }

    void Actor::OnNameChanged()
    {
        onNameChanged();
        onChanged();

        if (Scene::IsSingletonInitialzed() && IsOnScene())
        {
            o2Scene.OnObjectChanged(Ref(this));
            o2Scene.onNameChanged(Ref(this));
        }
    }

    void Actor::OnEditableParentChanged(const Ref<SceneEditableObject>& oldParent)
    {
        OnParentChanged(DynamicCast<Actor>(oldParent));
    }

    void Actor::OnChildrenChanged()
    {
        for (auto& comp : mComponents)
            comp->OnChildrenChanged();

        onChildHierarchyChanged();
        onChanged();

        if (Scene::IsSingletonInitialzed() && IsOnScene())
        {
            o2Scene.OnObjectChanged(Ref(this));
            o2Scene.onChildrenHierarchyChanged(Ref(this));
        }
    }

    void Actor::OnParentChanged(const Ref<Actor>& oldParent)
    {
        ISceneDrawable::OnDrawbleParentChanged();

        for (auto& comp : mComponents)
            comp->OnParentChanged(oldParent);

        onParentChanged(oldParent);
        onChanged();

        if (Scene::IsSingletonInitialzed() && IsOnScene())
        {
            o2Scene.OnObjectChanged(Ref(this));
            o2Scene.OnObjectChanged(oldParent);
            o2Scene.OnObjectChanged(mParent.Lock());

            o2Scene.onChildrenHierarchyChanged(oldParent);
            o2Scene.onChildrenHierarchyChanged(mParent.Lock());
        }
    }

    void Actor::ProcessReverting(Actor* dest, const Actor* source, const Vector<Ref<Actor>>& separatedActors,
                                 Vector<Actor**>& actorsPointers, Vector<Component**>& componentsPointers,
                                 Map<const Actor*, Actor*>& actorsMap,
                                 Map<const Component*, Component*>& componentsMap,
                                 Vector<ISerializable*>& serializableObjects)
    {
        dest->mName = source->mName;
        dest->mEnabled = source->mEnabled;
        *dest->transform = *source->transform;
        dest->mAssetId = source->mAssetId;

        actorsMap.Add(source, dest);

        for (auto& child : source->mChildren)
        {
            Ref<Actor> newChild;

            newChild = separatedActors.FindOrDefault([&](const Ref<Actor>& x) { return x->GetPrototypeLink() == child; });

            if (!newChild)
                newChild = mmake<Actor>(dest->IsOnScene() ? ActorCreateMode::InScene : ActorCreateMode::NotInScene);

            dest->AddChild(newChild);

            ProcessReverting(newChild.Get(), child.Get(), separatedActors, actorsPointers, componentsPointers, actorsMap,
                             componentsMap, serializableObjects);
        }

        for (auto it = dest->mComponents.Begin(); it != dest->mComponents.End();)
        {
            if (!(*it)->mPrototypeLink)
            {
                (*it)->mOwner = nullptr;
                it = dest->mComponents.Remove(it);
            }
            else ++it;
        }

        for (auto& component : source->mComponents)
        {
            auto matchingComponent = dest->mComponents.FindOrDefault([&](const Ref<Component>& x) { return x->GetPrototypeLink() == component; });
            if (matchingComponent)
            {
                Vector<const FieldInfo*> fields;
                GetComponentFields(matchingComponent, fields);

                CopyFields(fields, const_cast<Component*>(component.Get()), matchingComponent.Get(), actorsPointers, componentsPointers,
                           serializableObjects);

                continue;
            }

            auto newComponent = dest->AddComponent(CloneAsRef<Component>());
            componentsMap.Add(component.Get(), newComponent.Get());

            CollectFixingFields(newComponent.Get(), componentsPointers, actorsPointers);
        }
    }

    void Actor::CopyFields(Vector<const FieldInfo*>& fields, IObject* source, IObject* dest, Vector<Actor**>& actorsPointers,
                           Vector<Component**>& componentsPointers, Vector<ISerializable*>& serializableObjects)
    {
        for (auto& field : fields)
        {
            if (!field->HasAttribute<SerializableAttribute>())
                continue;

            if (*field->GetType() == TypeOf(Actor*))
            {
                Actor* sourceValue = field->GetValue<Actor*>(source);
                Actor** destValuePtr = (Actor**)(field->GetValuePtrStrong(dest));

                *destValuePtr = sourceValue;
                actorsPointers.Add(destValuePtr);
            }
            else if (*field->GetType() == TypeOf(Component*))
            {
                Component* sourceValue = field->GetValue<Component*>(source);
                Component** destValuePtr = (Component**)(field->GetValuePtrStrong(dest));

                *destValuePtr = sourceValue;
                componentsPointers.Add(destValuePtr);
            }
            else if (field->GetType()->IsBasedOn(TypeOf(AssetRef<Asset>)))
            {
                field->CopyValue(dest, source);
            }
            else if (field->GetType()->IsBasedOn(TypeOf(IObject)))
            {
                if (field->GetType()->IsBasedOn(TypeOf(ISerializable)))
                    serializableObjects.Add((ISerializable*)field->GetValuePtr(dest));

                auto fields = field->GetType()->GetFieldsWithBaseClasses();
                CopyFields(fields, (IObject*)field->GetValuePtr(source),
                           (IObject*)field->GetValuePtr(dest),
                           actorsPointers, componentsPointers, serializableObjects);
            }
            else field->CopyValue(dest, source);
        }
    }

    void Actor::CollectFixingFields(Component* newComponent, Vector<Component**>& componentsPointers,
                                    Vector<Actor**>& actorsPointers)
    {
        Vector<const FieldInfo*> fields;
        GetComponentFields(Ref(newComponent), fields);

        for (auto& field : fields)
        {
            if (field->GetType()->GetUsage() == Type::Usage::Pointer)
            {
                const PointerType* fieldType = (const PointerType*)field->GetType();

                if (fieldType->GetBaseType()->IsBasedOn(TypeOf(Component)))
                    componentsPointers.Add((Component**)(field->GetValuePtrStrong(newComponent)));

                if (*fieldType == TypeOf(Actor*))
                    actorsPointers.Add((Actor**)(field->GetValuePtrStrong(newComponent)));
            }
        }
    }

    void Actor::FixComponentFieldsPointers(const Vector<Actor**>& actorsPointers,
                                           const Vector<Component**>& componentsPointers,
                                           const Map<const Actor*, Actor*>& actorsMap,
                                           const Map<const Component*, Component*>& componentsMap)
    {
        for (auto& actorPtr : actorsPointers)
        {
            Actor* newActorPtr;
            if (actorsMap.TryGetValue(*actorPtr, newActorPtr))
                *actorPtr = newActorPtr;
        }

        for (auto& componentPtr : componentsPointers)
        {
            Component* newComponentPtr;
            if (componentsMap.TryGetValue(*componentPtr, newComponentPtr))
                *componentPtr = newComponentPtr;
        }
    }

    void Actor::GetComponentFields(const Ref<Component>& component, Vector<const FieldInfo*>& fields)
    {
        struct helper
        {
            static void GetFields(const Type* type, Vector<const FieldInfo*>& fields)
            {
                fields.Add(type->GetFields().Convert<const FieldInfo*>([](const auto& x) { return &x; }));

                for (auto& baseType : type->GetBaseTypes())
                {
                    if (*baseType.type != TypeOf(Component))
                        GetFields(baseType.type, fields);
                }
            }
        };

        helper::GetFields(&component->GetType(), fields);
    }

    void Actor::CopyActorChangedFields(Actor* source, Actor* changed, Actor* dest, Vector<Actor*>& allDestChilds,
                                       bool withTransform)
    {
        if (changed->mParent && changed->mParent.Lock()->mPrototypeLink)
        {
            auto changedParent = changed->mParent.Lock();
            auto destParent = dest->mParent.Lock();
            auto sourceParent = source->mParent.Lock();

            if (!changedParent->IsLinkedToActor(sourceParent) && dest->mParent &&
                destParent->IsLinkedToActor(sourceParent))
            {
                Actor* newParent = allDestChilds.FindOrDefault([&](Actor* x) { return x->IsLinkedToActor(Ref(changedParent->mPrototypeLink.Lock().Get())); });
                dest->SetParent(Ref(newParent));
            }
        }

        if (source->mName != changed->mName && dest->mName == source->mName)
            dest->mName = changed->mName;

        if (source->mEnabled != changed->mEnabled && dest->mEnabled == source->mEnabled)
            dest->mEnabled = changed->mEnabled;

        if (source->mLocked != changed->mLocked && dest->mLocked == source->mLocked)
            dest->mLocked = changed->mLocked;

        // transform
        if (withTransform)
        {
            if (source->transform->mPosition != changed->transform->mPosition &&
                dest->transform->mPosition == source->transform->mPosition)
            {
                dest->transform->mPosition = changed->transform->mPosition;
            }

            if (source->transform->mScale != changed->transform->mScale &&
                dest->transform->mScale == source->transform->mScale)
            {
                dest->transform->mScale = changed->transform->mScale;
            }

            if (source->transform->mSize != changed->transform->mSize &&
                dest->transform->mSize == source->transform->mSize)
            {
                dest->transform->mSize = changed->transform->mSize;
            }

            if (source->transform->mPivot != changed->transform->mPivot &&
                dest->transform->mPivot == source->transform->mPivot)
            {
                dest->transform->mPivot = changed->transform->mPivot;
            }

            if (!Math::Equals(source->transform->mEulerAngles, changed->transform->mEulerAngles) &&
                Math::Equals(dest->transform->mEulerAngles, source->transform->mEulerAngles))
            {
                dest->transform->mEulerAngles = changed->transform->mEulerAngles;
            }

            if (!Math::Equals(source->transform->mShear, changed->transform->mShear) &&
                Math::Equals(dest->transform->mShear, source->transform->mShear))
            {
                dest->transform->mShear = changed->transform->mShear;
            }
        }
    }
}

#endif //IS_EDITOR
