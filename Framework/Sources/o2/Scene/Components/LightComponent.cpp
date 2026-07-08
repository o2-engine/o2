#include "o2/stdafx.h"
#include "LightComponent.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"

namespace o2
{
    LightComponent::LightComponent()
    {}

    LightComponent::LightComponent(const LightComponent& other):
        Component(other), mLightType(other.mLightType), mColor(other.mColor), mIntensity(other.mIntensity),
        mRange(other.mRange)
    {}

    LightComponent::~LightComponent()
    {}

    LightComponent& LightComponent::operator=(const LightComponent& other)
    {
        Component::operator=(other);

        mLightType = other.mLightType;
        mColor = other.mColor;
        mIntensity = other.mIntensity;
        mRange = other.mRange;

        return *this;
    }

    void LightComponent::SetLightType(Type type)
    {
        mLightType = type;
    }

    LightComponent::Type LightComponent::GetLightType() const
    {
        return mLightType;
    }

    void LightComponent::SetColor(const Color4& color)
    {
        mColor = color;
    }

    const Color4& LightComponent::GetColor() const
    {
        return mColor;
    }

    void LightComponent::SetIntensity(float intensity)
    {
        mIntensity = intensity;
    }

    float LightComponent::GetIntensity() const
    {
        return mIntensity;
    }

    void LightComponent::SetRange(float range)
    {
        mRange = range;
    }

    float LightComponent::GetRange() const
    {
        return mRange;
    }

    Vec3F LightComponent::GetWorldDirection() const
    {
        if (auto owner = mOwner.Lock())
            return owner->transform->GetWorldTransform3D().TransformDirection(Vec3F(0.0f, 0.0f, -1.0f)).Normalized();

        return Vec3F(0.0f, 0.0f, -1.0f);
    }

    Quat LightComponent::GetWorldRotation() const
    {
        if (auto owner = mOwner.Lock())
        {
            Vec3F position, scale;
            Quat rotation;
            owner->transform->GetWorldTransform3D().Decompose(position, rotation, scale);
            return rotation;
        }

        return Quat();
    }

    Vec3F LightComponent::GetWorldPosition() const
    {
        if (auto owner = mOwner.Lock())
        {
            Vec3F position, scale;
            Quat rotation;
            owner->transform->GetWorldTransform3D().Decompose(position, rotation, scale);
            return position;
        }

        return Vec3F();
    }

    String LightComponent::GetName()
    {
        return "Light";
    }

    String LightComponent::GetCategory()
    {
        return "Render";
    }

    String LightComponent::GetIcon()
    {
        return "ui/UI4_image_component.png";
    }

    void LightComponent::OnAddToScene()
    {
        o2Scene.OnLightAddedOnScene(this);
    }

    void LightComponent::OnRemoveFromScene()
    {
        o2Scene.OnLightRemovedScene(this);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::LightComponent>);
// --- META ---

ENUM_META(o2::LightComponent::Type, o2__LightComponent__Type)
{
    ENUM_ENTRY(Directional);
    ENUM_ENTRY(Point);
}
END_ENUM_META;

DECLARE_CLASS(o2::LightComponent, o2__LightComponent);
// --- END META ---
