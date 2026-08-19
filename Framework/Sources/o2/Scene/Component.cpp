#include "o2/stdafx.h"
#include "Component.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/ActorRefResolver.h"
#include "o2/Scene/Scene.h"

namespace o2
{
    Component::Component() :
        Component(nullptr)
    {}

    Component::Component(RefCounter* refCounter, const Component& other) :
        RefCounterable(refCounter), mEnabled(other.mEnabled), mEnabledInHierarchy(false), mId(Math::Random())
    {
        ActorRefResolver::ComponentCreated(this);
    }

    Component::Component(RefCounter* refCounter):
        RefCounterable(refCounter), mId(Math::Random())
    {
        ActorRefResolver::ComponentCreated(this);
    }

    Component::Component(const Component& other):
        Component(other.GetRefCounter(), other)
    {}

    Component::~Component()
    {}

    Component& Component::operator=(const Component& other)
    {
        mEnabled = other.mEnabled;
        UpdateEnabledInHierarchy();

#if IS_EDITOR
        if (mOwner)
            mOwner.Lock()->OnChanged();
#endif

        return *this;
    }

    SceneUID Component::GetID() const
    {
        return mId;
    }

    void Component::GenerateNewID()
    {
        mId = Math::Random();
    }

    void Component::SetEnabled(bool active)
    {
        if (mEnabled == active)
            return;

        mEnabled = active;
        UpdateEnabledInHierarchy();

#if IS_EDITOR
        if (mOwner)
            mOwner.Lock()->OnChanged();
#endif
    }

    void Component::Enable()
    {
        SetEnabled(true);
    }

    void Component::Disable()
    {
        SetEnabled(false);
    }

    bool Component::IsEnabled() const
    {
        return mEnabled;
    }

    bool Component::IsEnabledInHierarchy() const
    {
        return mEnabledInHierarchy;
    }

    const WeakRef<Component>& Component::GetPrototypeLink() const
    {
        return mPrototypeLink;
    }

    bool Component::IsLinkedToComponent(const Ref<Component>& component) const
    {
        if (mPrototypeLink)
        {
            auto t = mPrototypeLink;
            while (t)
            {
                if (t == component)
                    return true;

                t = t.Lock()->mPrototypeLink;
            }
        }

        return false;
    }

    Ref<Actor> Component::GetActor() const
    {
        return mOwner.Lock();
    }

    SceneDrawableCategory Component::GetSceneDrawableCategory() const
    {
        return SceneDrawableCategory::Scene2D;
    }

    void Component::Draw()
    {
        OnDraw();
    }

    bool Component::Get3DDrawableBounds(AABB& bounds)
    {
        return false;
    }

    bool Component::Get3DDrawableLocalBounds(AABB& bounds)
    {
        return false;
    }

    bool Component::Is3DDrawableTransparent() const
    {
        return false;
    }

    String Component::GetName()
    {
        return String();
    }

    String Component::GetCategory()
    {
        return "";
    }

    String Component::GetIcon()
    {
        return "ui/UI4_component_icon.png";
    }

    bool Component::IsAvailableFromCreateMenu()
    {
        return true;
    }

    void Component::UpdateEnabledInHierarchy()
    {
        bool lastEnabledInHierarchy = mEnabledInHierarchy;

        if (mOwner)
            mEnabledInHierarchy = mEnabled && mOwner.Lock()->mResEnabledInHierarchy;
        else
            mEnabledInHierarchy = mEnabled;

        if (lastEnabledInHierarchy != mEnabledInHierarchy)
        {
            if (mEnabledInHierarchy)
                OnEnabled();
            else
                OnDisabled();

#if IS_EDITOR
            if (mOwner)
                mOwner.Lock()->OnChanged();
#endif
        }
    }

    void Component::OnSerialize(DataValue& node) const
    {
        node.AddMember("mId") = mId;
    }

    void Component::OnDeserialized(const DataValue& node)
    {
        auto prevId = mId;
        mId = node.GetMember("mId");
        ActorRefResolver::OnComponentIdChanged(this, prevId);
    }

    void Component::OnSerializeDelta(DataValue& node, const IObject& origin) const
    {
        OnSerialize(node);
    }

    void Component::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        OnDeserialized(node);
    }

    void Component::AddToScene()
    {
        o2Scene.OnComponentAdded(this);

        if (GetSceneDrawableCategory() == SceneDrawableCategory::Scene3D)
            o2Scene.OnDrawable3DComponentAdded(this);

        OnAddToScene();
    }

    void Component::RemoveFromScene()
    {
        o2Scene.OnComponentRemoved(this);

        if (GetSceneDrawableCategory() == SceneDrawableCategory::Scene3D)
            o2Scene.OnDrawable3DComponentRemoved(this);

        OnRemoveFromScene();
    }

#if IS_EDITOR
    void Component::DrawGizmos()
    {
        OnDrawGizmos();
    }
#endif

    void Component::SetOwnerActor(const Ref<Actor>& actor)
    {
        if (mOwner == actor)
            return;

        if (mOwner)
            mOwner.Lock()->RemoveComponent(Ref(this));

        mOwner = actor;

        OnParentChanged(nullptr);
    }
}
// --- META ---

DECLARE_CLASS(o2::Component, o2__Component);
// --- END META ---
