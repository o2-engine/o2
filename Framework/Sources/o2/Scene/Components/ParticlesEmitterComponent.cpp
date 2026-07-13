#include "o2/stdafx.h"
#include "ParticlesEmitterComponent.h"

#include "o2/Scene/Actor.h"

namespace o2
{

    ParticlesEmitterComponent::ParticlesEmitterComponent()
    {}

    ParticlesEmitterComponent::ParticlesEmitterComponent(const ParticlesEmitterComponent& other):
        Component(other), ParticlesEmitter(other)
    {}

    ParticlesEmitterComponent::~ParticlesEmitterComponent()
    {}

    ParticlesEmitterComponent& ParticlesEmitterComponent::operator=(const ParticlesEmitterComponent& other)
    {
        Component::operator=(other);
        ParticlesEmitter::operator=(other);
        return *this;
    }

    void ParticlesEmitterComponent::OnDraw()
    {
        ParticlesEmitter::Draw();
    }

    void ParticlesEmitterComponent::OnUpdate(float dt)
    {
        ParticlesEmitter::Update(dt);
    }

    bool ParticlesEmitterComponent::IsUnderPoint(const Vec2F& point)
    {
        return ParticlesEmitter::IsUnderPoint(point);
    }

    String ParticlesEmitterComponent::GetName()
    {
        return "Particles emitter";
    }

    String ParticlesEmitterComponent::GetCategory()
    {
        return "Render";
    }

    String ParticlesEmitterComponent::GetIcon()
    {
        return "ui/UI4_emitter_component.png";
    }

    Ref<o2::RefCounterable> ParticlesEmitterComponent::CastToRefCounterable(const Ref<ParticlesEmitterComponent>& ref)
    {
        return DynamicCast<Component>(ref);
    }

    SceneDrawableCategory ParticlesEmitterComponent::GetSceneDrawableCategory() const
    {
        return Is3D() ? SceneDrawableCategory::Scene3D : SceneDrawableCategory::Scene2D;
    }

    bool ParticlesEmitterComponent::Get3DDrawableBounds(o2::AABB& bounds)
    {
        if (!Is3D())
            return false;

        return GetParticlesBounds(bounds);
    }

    bool ParticlesEmitterComponent::Is3DDrawableTransparent() const
    {
        return true;
    }

    void ParticlesEmitterComponent::OnTransformUpdated()
    {
        auto transform = mOwner.Lock()->transform;
        basis = transform->GetWorldBasis();
        Set3DBasis(transform->GetWorldBasis3D());
    }

    void ParticlesEmitterComponent::OnSerialize(DataValue& node) const
    {
        Component::OnSerialize(node);
        ParticlesEmitter::OnSerialize(node);
    }

    void ParticlesEmitterComponent::OnDeserialized(const DataValue& node)
    {
        Component::OnDeserialized(node);
        ParticlesEmitter::OnDeserialized(node);
    }

    void ParticlesEmitterComponent::OnSerializeDelta(DataValue& node, const IObject& origin) const
    {
        Component::OnSerializeDelta(node, origin);
        ParticlesEmitter::OnSerializeDelta(node, origin);
    }

    void ParticlesEmitterComponent::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        Component::OnDeserializedDelta(node, origin);
        ParticlesEmitter::OnDeserializedDelta(node, origin);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::ParticlesEmitterComponent>);
// --- META ---

DECLARE_CLASS(o2::ParticlesEmitterComponent, o2__ParticlesEmitterComponent);
// --- END META ---
