#include "o2/stdafx.h"
#include "SoundListenerComponent.h"

#include "o2/Scene/Actor.h"

namespace o2
{
    SoundListenerComponent::SoundListenerComponent()
    {}

    SoundListenerComponent::SoundListenerComponent(const SoundListenerComponent& other):
        Component(other), SoundListener(other)
    {}

    SoundListenerComponent::~SoundListenerComponent()
    {}

    SoundListenerComponent& SoundListenerComponent::operator=(const SoundListenerComponent& other)
    {
        Component::operator=(other);
        SoundListener::operator=(other);
        return *this;
    }

    bool SoundListenerComponent::IsListening() const
    {
        if (!IsEnabledInHierarchy())
            return false;

        auto owner = GetActor();
        return owner && owner->IsOnScene();
    }

    String SoundListenerComponent::GetName()
    {
        return "Sound listener";
    }

    String SoundListenerComponent::GetCategory()
    {
        return "Sound";
    }

    String SoundListenerComponent::GetIcon()
    {
        return "ui/UI4_animation_component.png";
    }

    Ref<o2::RefCounterable> SoundListenerComponent::CastToRefCounterable(const Ref<SoundListenerComponent>& ref)
    {
        return DynamicCast<Component>(ref);
    }

    void SoundListenerComponent::OnStart()
    {
        UpdatePoseFromTransform();
    }

    void SoundListenerComponent::OnTransformUpdated()
    {
        UpdatePoseFromTransform();
    }

    void SoundListenerComponent::UpdatePoseFromTransform()
    {
        auto transform = mOwner.Lock()->transform;
        Basis3D basis = transform->GetWorldBasis3D();

        SetPosition(transform->GetWorldPosition());
        SetOrientation(basis.zv.Normalized()*-1.0f, basis.yv.Normalized());
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::SoundListenerComponent>);
// --- META ---

DECLARE_CLASS(o2::SoundListenerComponent, o2__SoundListenerComponent);
// --- END META ---
