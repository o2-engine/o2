#include "o2/stdafx.h"
#include "SoundComponent.h"

#include "o2/Scene/Actor.h"

namespace o2
{
    SoundComponent::SoundComponent()
    {}

    SoundComponent::SoundComponent(const SoundComponent& other):
        Component(other), SoundPlayer(other)
    {}

    SoundComponent::~SoundComponent()
    {}

    SoundComponent& SoundComponent::operator=(const SoundComponent& other)
    {
        Component::operator=(other);
        SoundPlayer::operator=(other);
        return *this;
    }

    void SoundComponent::OnUpdate(float dt)
    {
        SoundPlayer::Update(dt);
    }

    String SoundComponent::GetName()
    {
        return "Sound";
    }

    String SoundComponent::GetCategory()
    {
        return "Sound";
    }

    String SoundComponent::GetIcon()
    {
        return "ui/UI4_animation_component.png";
    }

    Ref<o2::RefCounterable> SoundComponent::CastToRefCounterable(const Ref<SoundComponent>& ref)
    {
        return DynamicCast<Component>(ref);
    }

    void SoundComponent::OnTransformUpdated()
    {
        SetPosition(mOwner.Lock()->transform->GetWorldPosition());
    }

    void SoundComponent::OnRemoveFromScene()
    {
        Component::OnRemoveFromScene();
        StopBackend();
    }

    void SoundComponent::OnDisabled()
    {
        Component::OnDisabled();
        StopBackend();
    }

    void SoundComponent::OnSerialize(DataValue& node) const
    {
        Component::OnSerialize(node);
        SoundPlayer::OnSerialize(node);
    }

    void SoundComponent::OnDeserialized(const DataValue& node)
    {
        Component::OnDeserialized(node);
        SoundPlayer::OnDeserialized(node);
    }

    void SoundComponent::OnSerializeDelta(DataValue& node, const IObject& origin) const
    {
        Component::OnSerializeDelta(node, origin);
        SoundPlayer::OnSerializeDelta(node, origin);
    }

    void SoundComponent::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        Component::OnDeserializedDelta(node, origin);
        SoundPlayer::OnDeserializedDelta(node, origin);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::SoundComponent>);
// --- META ---

DECLARE_CLASS(o2::SoundComponent, o2__SoundComponent);
// --- END META ---
