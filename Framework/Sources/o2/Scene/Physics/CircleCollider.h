#pragma once
#include "o2/Scene/Physics/ICollider.h"
#include "Box2D/Collision/Shapes/b2CircleShape.h"

namespace o2
{
    // -----------------------
    // Circle physics collider
    // -----------------------
    class CircleCollider: public ICollider
    {
    public:
        PROPERTIES(CircleCollider);
        PROPERTY(float, radius, SetRadius, GetRadius);           // Collider radius property
        PROPERTY(bool, fitByActor, SetFitByActor, IsFitByActor); // Fit by actor's size property

    public:
        // Default constructor
        CircleCollider();

        // Copy-constructor
        CircleCollider(const CircleCollider& other);

        // Copy operator
        CircleCollider& operator=(const CircleCollider& other);

        // Sets collider radius
        void SetRadius(float radius);

        // Returns collider radius
        float GetRadius() const;

        // Sets collider fit to actor's size
        void SetFitByActor(bool fit);

        // Is collider fits to actor's size
        bool IsFitByActor() const;

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Is component visible in create menu
        static bool IsAvailableFromCreateMenu();

#if IS_EDITOR
        // Called when component added from editor; fits collider by actor size
        void OnAddedFromEditor() override;
#endif

        SERIALIZABLE(CircleCollider);
        CLONEABLE_REF(CircleCollider);

    private:
        float mRadius = 1.0f; // Size of collider @SERIALIZABLE

        bool mFitByActor = true; // Is box size is fitting by actor's size @SERIALIZABLE

        b2CircleShape mShape; // Physics shape

    private:
        // Returns shape with relative position and angle
        b2Shape* GetShape(const Basis& transform) override;

        // Called when transformation was changed; fits size by actor size when required
        void OnTransformUpdated() override;

#if IS_EDITOR
        // Draws collider circle wireframe
        void OnDrawGizmos() override;
#endif
    };
}
// --- META ---

CLASS_BASES_META(o2::CircleCollider)
{
    BASE_CLASS(o2::ICollider);
}
END_META;
CLASS_FIELDS_META(o2::CircleCollider)
{
    FIELD().PUBLIC().NAME(radius);
    FIELD().PUBLIC().NAME(fitByActor);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mRadius);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mFitByActor);
    FIELD().PRIVATE().NAME(mShape);
}
END_META;
CLASS_METHODS_META(o2::CircleCollider)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const CircleCollider&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRadius, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetRadius);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFitByActor, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFitByActor);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
#if  IS_EDITOR
    FUNCTION().PUBLIC().SIGNATURE(void, OnAddedFromEditor);
#endif
    FUNCTION().PRIVATE().SIGNATURE(b2Shape*, GetShape, const Basis&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnTransformUpdated);
#if  IS_EDITOR
    FUNCTION().PRIVATE().SIGNATURE(void, OnDrawGizmos);
#endif
}
END_META;
// --- END META ---
