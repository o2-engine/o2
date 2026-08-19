#pragma once

#include "o2/Scene/Actor.h"
#include "Box2D/Collision/Shapes/b2Shape.h"
#include "Box2D/Dynamics/b2Body.h"

namespace o2
{
    class ICollider;

    // ------------------
    // Physics rigid body
    // ------------------
    class RigidBody: public Actor
    {
    public:
        enum class Type { Dynamic, Static, Kinematic };

        PROPERTIES(RigidBody);
        PROPERTY(Type, bodyType, SetBodyType, GetBodyType);                       // Body type property
        PROPERTY(float, mass, SetMass, GetMass);                                  // Mass property, in kg
        PROPERTY(float, inertia, SetInertia, GetInertia);                         // Inertia property, in kg-m^2
        PROPERTY(Vec2F, linearVelocity, SetLinearVelocity, GetLinearVelocity);    // Linear velocity property, in m/s
        PROPERTY(float, angularVelocity, SetAngularVelocity, GetAngularVelocity); // Angular velocity property, in rad/s
        PROPERTY(float, linearDamping, SetLinearDamping, GetLinearDamping);       // Linear damping property
        PROPERTY(float, angularDamping, SetAngularDamping, GetAngularDamping);    // Angular damping property
        PROPERTY(float, gravityScale, SetGravityScale, GetGravityScale);          // Gravity scale property
        PROPERTY(bool, isBullet, SetIsBullet, IsBullet);                          // Is body using continuous collision detection property
        PROPERTY(bool, isSleeping, SetIsSleeping, IsSleeping);                    // Is body sleeping property
        PROPERTY(bool, isFixedRotation, SetIsFixedRotation, IsFixedRotation);     // Is fixed rotation property

    public:
        // Default constructor
        explicit RigidBody(RefCounter* refCounter);

        // Copy-constructor
        RigidBody(RefCounter* refCounter, const RigidBody& other);

        // Copy-operator
        RigidBody& operator=(const RigidBody& other);

        // Destructor
        ~RigidBody();

        // Sets body type
        void SetBodyType(Type type);

        // Returns body type
        Type GetBodyType() const;

        // Sets mass, in kilograms
        void SetMass(float mass);

        // Returns mass in kilograms
        float GetMass() const;

        // Sets body inertia in kg-m^2
        void SetInertia(float inertia);

        // Returns body inertia kg-m^2
        float GetInertia() const;

        // Sets linear velocity in m/s 
        void SetLinearVelocity(const Vec2F& velocity);

        // Returns linear velocity in m/s
        Vec2F GetLinearVelocity() const;

        // Sets angular velocity in rad/s
        void SetAngularVelocity(float velocity);

        // Returns angular velocity in rad/s
        float GetAngularVelocity() const;

        // Sets linear damping
        void SetLinearDamping(float damping);

        // Returns linear damping
        float GetLinearDamping() const;

        // Sets angular damping
        void SetAngularDamping(float damping);

        // Returns angular damping
        float GetAngularDamping() const;

        // Sets gravity scale
        void SetGravityScale(float scale);

        // Returns gravity scale
        float GetGravityScale() const;

        // Sets is body using continuous collision detection
        void SetIsBullet(bool isBullet);

        // Returns is body using continuous collision detection
        bool IsBullet() const;

        // Sets body is sleeping
        void SetIsSleeping(bool isSleeping);

        // Returns body is sleeping
        bool IsSleeping() const;

        // Sets is body fixed rotation
        void SetIsFixedRotation(bool isFixedRotation);

        // Returns is body fixed rotation
        bool IsFixedRotation() const;

        // Returns the Box2D body (null until created on scene)
        b2Body* GetBody() const;

        SERIALIZABLE(RigidBody);
        CLONEABLE_REF(RigidBody);

    protected:
        b2Body* mBody = nullptr; // Box 2d physics body

        Type mBodyType = Type::Dynamic; // Type of body @SERIALIZABLE

        float mMass = 1.0f;             // Mass @SERIALIZABLE
        float mInertia = 1.0f;          // Inertia @SERIALIZABLE
        float mLinearDamping = 0.0f;    // Linear damping @SERIALIZABLE
        float mAngularDamping = 0.0f;   // Angular damping @SERIALIZABLE
        float mGravityScale = 1.0f;     // Gravity scale @SERIALIZABLE
        bool  mIsBullet = false;        // Is using continuous collision detection @SERIALIZABLE
        bool  mIsFixedRotation = false; // Is fixed rotation @SERIALIZABLE

        Vector<WeakRef<ICollider>> mColliders; // Attached colliders list

    protected:
        // Called when enabled, turns on rigid body
        void OnEnabled() override;

        // Called when enabled, turns off rigid body
        void OnDisabled() override;

        // Called when actor has added to scene; creates rigid body
        void OnAddToScene() override;

        // Called when actor has removed from scene; destroys rigid body
        void OnRemoveFromScene() override;

        // Creates box2d body, registers in physics world
        void CreateBody();

        // Applies mass and inertia to the body, keeping the center of mass calculated from fixtures
        void ApplyMassData();

        // Removes body
        void RemoveBody();

        // Adds collider to body
        void AddCollider(ICollider* collider);

        // Removes collider from body
        void RemoveCollider(ICollider* collider);

        // Converts body type
        static b2BodyType GetBodyType(Type type);

        friend class ICollider;
        friend class PhysicsWorld;
    };
}
// --- META ---

PRE_ENUM_META(o2::RigidBody::Type);

CLASS_BASES_META(o2::RigidBody)
{
    BASE_CLASS(o2::Actor);
}
END_META;
CLASS_FIELDS_META(o2::RigidBody)
{
    FIELD().PUBLIC().NAME(bodyType);
    FIELD().PUBLIC().NAME(mass);
    FIELD().PUBLIC().NAME(inertia);
    FIELD().PUBLIC().NAME(linearVelocity);
    FIELD().PUBLIC().NAME(angularVelocity);
    FIELD().PUBLIC().NAME(linearDamping);
    FIELD().PUBLIC().NAME(angularDamping);
    FIELD().PUBLIC().NAME(gravityScale);
    FIELD().PUBLIC().NAME(isBullet);
    FIELD().PUBLIC().NAME(isSleeping);
    FIELD().PUBLIC().NAME(isFixedRotation);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mBody);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Type::Dynamic).NAME(mBodyType);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mMass);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mInertia);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mLinearDamping);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mAngularDamping);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mGravityScale);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mIsBullet);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mIsFixedRotation);
    FIELD().PROTECTED().NAME(mColliders);
}
END_META;
CLASS_METHODS_META(o2::RigidBody)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const RigidBody&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBodyType, Type);
    FUNCTION().PUBLIC().SIGNATURE(Type, GetBodyType);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMass, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetMass);
    FUNCTION().PUBLIC().SIGNATURE(void, SetInertia, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetInertia);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLinearVelocity, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetLinearVelocity);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAngularVelocity, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetAngularVelocity);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLinearDamping, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetLinearDamping);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAngularDamping, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetAngularDamping);
    FUNCTION().PUBLIC().SIGNATURE(void, SetGravityScale, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetGravityScale);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIsBullet, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsBullet);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIsSleeping, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSleeping);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIsFixedRotation, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFixedRotation);
    FUNCTION().PUBLIC().SIGNATURE(b2Body*, GetBody);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddToScene);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemoveFromScene);
    FUNCTION().PROTECTED().SIGNATURE(void, CreateBody);
    FUNCTION().PROTECTED().SIGNATURE(void, ApplyMassData);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveBody);
    FUNCTION().PROTECTED().SIGNATURE(void, AddCollider, ICollider*);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveCollider, ICollider*);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(b2BodyType, GetBodyType, Type);
}
END_META;
// --- END META ---
