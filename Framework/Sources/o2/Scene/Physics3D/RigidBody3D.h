#pragma once

#include "box3d/box3d.h"
#include "o2/Scene/Actor.h"

namespace o2
{
    class ICollider3D;

    // ---------------------
    // Physics 3D rigid body
    // ---------------------
    class RigidBody3D: public Actor
    {
    public:
        enum class Type { Dynamic, Static, Kinematic };

        PROPERTIES(RigidBody3D);
        PROPERTY(Type, bodyType, SetBodyType, GetBodyType);                       // Body type property
        PROPERTY(Vec3F, linearVelocity, SetLinearVelocity, GetLinearVelocity);    // Linear velocity property, in m/s
        PROPERTY(Vec3F, angularVelocity, SetAngularVelocity, GetAngularVelocity); // Angular velocity property, in rad/s
        PROPERTY(float, linearDamping, SetLinearDamping, GetLinearDamping);       // Linear damping property
        PROPERTY(float, angularDamping, SetAngularDamping, GetAngularDamping);    // Angular damping property
        PROPERTY(float, gravityScale, SetGravityScale, GetGravityScale);          // Gravity scale property
        PROPERTY(bool, isBullet, SetIsBullet, IsBullet);                          // Continuous collision detection property
        PROPERTY(bool, isSleeping, SetIsSleeping, IsSleeping);                    // Is body sleeping property
        PROPERTY(bool, isFixedRotation, SetIsFixedRotation, IsFixedRotation);     // Locks all rotation property

    public:
        // Default constructor
        explicit RigidBody3D(RefCounter* refCounter);

        // Copy-constructor
        RigidBody3D(RefCounter* refCounter, const RigidBody3D& other);

        // Copy-operator
        RigidBody3D& operator=(const RigidBody3D& other);

        // Destructor
        ~RigidBody3D();

        // Sets body type
        void SetBodyType(Type type);

        // Returns body type
        Type GetBodyType() const;

        // Sets linear velocity in m/s
        void SetLinearVelocity(const Vec3F& velocity);

        // Returns linear velocity in m/s
        Vec3F GetLinearVelocity() const;

        // Sets angular velocity in rad/s
        void SetAngularVelocity(const Vec3F& velocity);

        // Returns angular velocity in rad/s
        Vec3F GetAngularVelocity() const;

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

        // Sets all-rotation lock
        void SetIsFixedRotation(bool isFixedRotation);

        // Returns is body rotation locked
        bool IsFixedRotation() const;

        // Returns the Box3D body handle
        b3BodyId GetBodyId() const;

        SERIALIZABLE(RigidBody3D);
        CLONEABLE_REF(RigidBody3D);

    protected:
        b3BodyId mBodyId;          // Box3D physics body handle (valid while mHasBody); zero-inited in ctor
        bool     mHasBody = false; // True while a Box3D body exists

        Vec3F mLastSyncPos; // Last pose exchanged with the actor, for external-move detection
        Quat  mLastSyncRot; // Stored in the actor's own reported space so the body quaternion stays authoritative

        Type mBodyType = Type::Dynamic; // Type of body @SERIALIZABLE

        float mLinearDamping = 0.0f;    // Linear damping @SERIALIZABLE
        float mAngularDamping = 0.05f;  // Angular damping @SERIALIZABLE
        float mGravityScale = 1.0f;     // Gravity scale @SERIALIZABLE
        bool  mIsBullet = false;        // Is using continuous collision detection @SERIALIZABLE
        bool  mIsFixedRotation = false; // Locks all rotation @SERIALIZABLE

        Vector<WeakRef<ICollider3D>> mColliders; // Attached colliders list

    protected:
        // Called when enabled, turns on the rigid body
        void OnEnabled() override;

        // Called when disabled, turns off the rigid body
        void OnDisabled() override;

        // Called when actor is added to scene; creates the rigid body
        void OnAddToScene() override;

        // Called when actor is removed from scene; destroys the rigid body
        void OnRemoveFromScene() override;

        // Creates the Box3D body and registers in the physics world
        void CreateBody();

        // Removes the Box3D body
        void RemoveBody();

        // Adds a collider to the body
        void AddCollider(ICollider3D* collider);

        // Removes a collider from the body
        void RemoveCollider(ICollider3D* collider);

        // Pushes the actor pose into the body when the actor was moved externally
        void SyncActorToBody(float invScale);

        // Writes the simulated body pose back into the actor
        void SyncBodyToActor(float scale);

        // Converts a body type to the Box3D enum
        static b3BodyType GetBox3DBodyType(Type type);

        friend class ICollider3D;
        friend class PhysicsWorld3D;
    };
}
// --- META ---

PRE_ENUM_META(o2::RigidBody3D::Type);

CLASS_BASES_META(o2::RigidBody3D)
{
    BASE_CLASS(o2::Actor);
}
END_META;
CLASS_FIELDS_META(o2::RigidBody3D)
{
    FIELD().PUBLIC().NAME(bodyType);
    FIELD().PUBLIC().NAME(linearVelocity);
    FIELD().PUBLIC().NAME(angularVelocity);
    FIELD().PUBLIC().NAME(linearDamping);
    FIELD().PUBLIC().NAME(angularDamping);
    FIELD().PUBLIC().NAME(gravityScale);
    FIELD().PUBLIC().NAME(isBullet);
    FIELD().PUBLIC().NAME(isSleeping);
    FIELD().PUBLIC().NAME(isFixedRotation);
    FIELD().PROTECTED().NAME(mBodyId);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHasBody);
    FIELD().PROTECTED().NAME(mLastSyncPos);
    FIELD().PROTECTED().NAME(mLastSyncRot);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Type::Dynamic).NAME(mBodyType);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mLinearDamping);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.05f).NAME(mAngularDamping);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mGravityScale);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mIsBullet);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mIsFixedRotation);
    FIELD().PROTECTED().NAME(mColliders);
}
END_META;
CLASS_METHODS_META(o2::RigidBody3D)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const RigidBody3D&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBodyType, Type);
    FUNCTION().PUBLIC().SIGNATURE(Type, GetBodyType);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLinearVelocity, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetLinearVelocity);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAngularVelocity, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetAngularVelocity);
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
    FUNCTION().PUBLIC().SIGNATURE(b3BodyId, GetBodyId);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddToScene);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemoveFromScene);
    FUNCTION().PROTECTED().SIGNATURE(void, CreateBody);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveBody);
    FUNCTION().PROTECTED().SIGNATURE(void, AddCollider, ICollider3D*);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveCollider, ICollider3D*);
    FUNCTION().PROTECTED().SIGNATURE(void, SyncActorToBody, float);
    FUNCTION().PROTECTED().SIGNATURE(void, SyncBodyToActor, float);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(b3BodyType, GetBox3DBodyType, Type);
}
END_META;
// --- END META ---
