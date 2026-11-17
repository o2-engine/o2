#pragma once
#include "o2/Scene/Component.h"
#include "Box2D/Collision/Shapes/b2Shape.h"
#include "Box2D/Dynamics/b2Fixture.h"

namespace o2
{
    class RigidBody;

    // --------------------------------
    // Basic physics collider interface
    // --------------------------------
    class ICollider: public Component
    {
    public:
        PROPERTIES(ICollider);
        PROPERTY(float, friction, SetFriction, GetFriction);          // Friction property
        PROPERTY(float, density, SetDensity, GetDensity);             // Density property
        PROPERTY(float, restitution, SetRestitution, GetRestitution); // Restitution property
        PROPERTY(String, layer, SetLayer, GetLayer);                  // Layer name property
        PROPERTY(bool, isSensor, SetIsSensor, IsSensor);              // Is sensor property

    public:
        // Default constructor
        ICollider();

        // Copy-constructor
        ICollider(const ICollider& other);

        // Destructor
        ~ICollider();

        // Copy operator
        ICollider& operator=(const ICollider& other);

        // Sets friction coefficient
        void SetFriction(float value);

        // Returns friction coefficient
        float GetFriction() const;

        // Sets density
        void SetDensity(float value);

        // Returns density
        float GetDensity() const;

        // Sets restitution
        void SetRestitution(float value);

        // Returns restitution
        float GetRestitution() const;

        // Sets layer
        void SetLayer(const String& layer);

        // Returns layer name
        const String& GetLayer() const;

        // Sets is sensor
        void SetIsSensor(bool value);

        // Returns is collider sensor
        bool IsSensor() const;

        // Is component visible in create menu
        static bool IsAvailableFromCreateMenu();

        // Returns name of component icon
        static String GetIcon();

        SERIALIZABLE(ICollider);

    protected:
        float mFriction = 0.3f;    // Friction coefficient @SERIALIZABLE
        float mDensity = 1.0f;     // Density @SERIALIZABLE 
        float mRestitution = 0.0f; // Restitution @SERIALIZABLE

        String mLayer = String("Default"); // Layer name @SERIALIZABLE

        bool mIsSensor = false; // Is collider sensor @SERIALIZABLE

        b2Fixture* mFixture = nullptr;
        RigidBody* mRigidBodyComp = nullptr;

    protected:
        // Adds fixture with shape to body
        void AddToRigidBody(RigidBody* body);

        // Removes fixture from body
        void RemoveFromRigidBody();

        // Searches rigid body in parent hierarchy
        Ref<RigidBody> FindRigidBody() const;

        // Called when shape has changed, updates fixture with new shape
        void OnShapeChanged();

        // Returns shape with relative position and angle
        virtual b2Shape* GetShape(const Basis& transform);

        // Called when transformation was changed 
        void OnTransformUpdated() override;

        // Called when actor was included to scene
        void OnAddToScene() override;

        // Called when actor was excluded from scene
        void OnRemoveFromScene() override;

        friend class PhysicsWorld;
        friend class RigidBody;
    };
}
// --- META ---

CLASS_BASES_META(o2::ICollider)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::ICollider)
{
    FIELD().PUBLIC().NAME(friction);
    FIELD().PUBLIC().NAME(density);
    FIELD().PUBLIC().NAME(restitution);
    FIELD().PUBLIC().NAME(layer);
    FIELD().PUBLIC().NAME(isSensor);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.3f).NAME(mFriction);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mDensity);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mRestitution);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(String("Default")).NAME(mLayer);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mIsSensor);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mFixture);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mRigidBodyComp);
}
END_META;
CLASS_METHODS_META(o2::ICollider)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const ICollider&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFriction, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetFriction);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDensity, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDensity);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRestitution, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetRestitution);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLayer, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const String&, GetLayer);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIsSensor, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSensor);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetIcon);
    FUNCTION().PROTECTED().SIGNATURE(void, AddToRigidBody, RigidBody*);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveFromRigidBody);
    FUNCTION().PROTECTED().SIGNATURE(Ref<RigidBody>, FindRigidBody);
    FUNCTION().PROTECTED().SIGNATURE(void, OnShapeChanged);
    FUNCTION().PROTECTED().SIGNATURE(b2Shape*, GetShape, const Basis&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddToScene);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemoveFromScene);
}
END_META;
// --- END META ---
