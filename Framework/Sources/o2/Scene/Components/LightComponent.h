#pragma once

#include "o2/Scene/Component.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Quaternion.h"
#include "o2/Utils/Math/Vector3.h"

namespace o2
{
    // --------------------------------------------------------------------------
    // Light source component. Direction and position are taken from owner actor
    // transform; registered in scene and consumed by lighting render passes
    // --------------------------------------------------------------------------
    class LightComponent: public Component
    {
    public:
        enum class Type { Directional, Point };

    public:
        PROPERTIES(LightComponent);
        PROPERTY(Type, lightType, SetLightType, GetLightType);       // Light type property
        PROPERTY(Color4, color, SetColor, GetColor);                 // Light color property
        PROPERTY(float, intensity, SetIntensity, GetIntensity);     // Light intensity property
        PROPERTY(float, range, SetRange, GetRange);                 // Point light range property

    public:
        // Default constructor
        LightComponent();

        // Copy-constructor
        LightComponent(const LightComponent& other);

        // Destructor
        ~LightComponent();

        // Assign operator
        LightComponent& operator=(const LightComponent& other);

        // Sets light type
        void SetLightType(Type type);

        // Returns light type
        Type GetLightType() const;

        // Sets light color
        void SetColor(const Color4& color);

        // Returns light color
        const Color4& GetColor() const;

        // Sets light intensity
        void SetIntensity(float intensity);

        // Returns light intensity
        float GetIntensity() const;

        // Sets point light range
        void SetRange(float range);

        // Returns point light range
        float GetRange() const;

        // Returns world space light direction (actor's forward, -Z)
        Vec3F GetWorldDirection() const;

        // Returns world space light rotation
        Quat GetWorldRotation() const;

        // Returns world space light position
        Vec3F GetWorldPosition() const;

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Returns name of component icon
        static String GetIcon();

        SERIALIZABLE(LightComponent);
        CLONEABLE_REF(LightComponent);

    protected:
        Type   mLightType = Type::Directional; // Light type @SERIALIZABLE
        Color4 mColor = Color4::White();       // Light color @SERIALIZABLE
        float  mIntensity = 1.0f;              // Light intensity @SERIALIZABLE
        float  mRange = 100.0f;                // Point light range @SERIALIZABLE

    protected:
        // Called when actor was included to scene, registers light
        void OnAddToScene() override;

        // Called when actor was excluded from scene, unregisters light
        void OnRemoveFromScene() override;
    };
}
// --- META ---

PRE_ENUM_META(o2::LightComponent::Type);

CLASS_BASES_META(o2::LightComponent)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::LightComponent)
{
    FIELD().PUBLIC().NAME(lightType);
    FIELD().PUBLIC().NAME(color);
    FIELD().PUBLIC().NAME(intensity);
    FIELD().PUBLIC().NAME(range);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Type::Directional).NAME(mLightType);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Color4::White()).NAME(mColor);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mIntensity);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(100.0f).NAME(mRange);
}
END_META;
CLASS_METHODS_META(o2::LightComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const LightComponent&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLightType, Type);
    FUNCTION().PUBLIC().SIGNATURE(Type, GetLightType);
    FUNCTION().PUBLIC().SIGNATURE(void, SetColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(const Color4&, GetColor);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIntensity, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetIntensity);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRange, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetRange);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetWorldDirection);
    FUNCTION().PUBLIC().SIGNATURE(Quat, GetWorldRotation);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetWorldPosition);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetIcon);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddToScene);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemoveFromScene);
}
END_META;
// --- END META ---
