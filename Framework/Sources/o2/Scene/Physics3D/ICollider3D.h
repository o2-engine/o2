#pragma once

#include "box3d/box3d.h"
#include "o2/Scene/Component.h"

namespace o2
{
    class RigidBody3D;

    // -----------------------------------
    // Basic 3D physics collider interface
    // -----------------------------------
    class ICollider3D: public Component
    {
    public:
        PROPERTIES(ICollider3D);
        PROPERTY(float, friction, SetFriction, GetFriction);          // Friction property
        PROPERTY(float, density, SetDensity, GetDensity);             // Density property
        PROPERTY(float, restitution, SetRestitution, GetRestitution); // Restitution property
        PROPERTY(bool, isSensor, SetIsSensor, IsSensor);              // Is sensor property

    public:
        // Default constructor
        ICollider3D();

        // Copy-constructor
        ICollider3D(const ICollider3D& other);

        // Destructor
        ~ICollider3D();

        // Copy operator
        ICollider3D& operator=(const ICollider3D& other);

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

        // Sets is sensor
        void SetIsSensor(bool value);

        // Returns is collider a sensor
        bool IsSensor() const;

        // Is component visible in create menu
        static bool IsAvailableFromCreateMenu();

        SERIALIZABLE(ICollider3D);

    protected:
        float mFriction = 0.3f;    // Friction coefficient @SERIALIZABLE
        float mDensity = 1.0f;     // Density @SERIALIZABLE
        float mRestitution = 0.0f; // Restitution @SERIALIZABLE

        bool mIsSensor = false; // Is collider a sensor @SERIALIZABLE

        b3ShapeId    mShapeId;                 // Box3D shape handle (valid while mHasShape); zero-inited in ctor
        bool         mHasShape = false;        // True while a shape exists
        RigidBody3D* mRigidBodyComp = nullptr; // Owning rigid body

    protected:
        // Creates the concrete Box3D shape on the body using the collider-relative transform
        virtual b3ShapeId CreateShape(b3BodyId body, const b3ShapeDef& def, const b3Transform& relative, float invScale);

        // Adds the shape to the body
        void AddToRigidBody(RigidBody3D* body);

        // Removes the shape from the body
        void RemoveFromRigidBody();

        // Called by the body when it is destroyed; clears the shape handle without touching Box3D
        void OnBodyDestroyed();

        // Searches for a rigid body up the actor hierarchy
        Ref<RigidBody3D> FindRigidBody() const;

        // Recreates the shape after a change
        void OnShapeChanged();

        // Builds a Box3D shape def from this collider's material properties
        b3ShapeDef MakeShapeDef();

        // Computes this collider's transform relative to the body, position scaled to physics units
        b3Transform GetRelativeTransform(RigidBody3D* body, float invScale) const;

        // Called when transformation was changed
        void OnTransformUpdated() override;

        // Called when actor was included to scene
        void OnAddToScene() override;

        // Called when actor was excluded from scene
        void OnRemoveFromScene() override;

        friend class PhysicsWorld3D;
        friend class RigidBody3D;
    };
}
// --- META ---

CLASS_BASES_META(o2::ICollider3D)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::ICollider3D)
{
    FIELD().PUBLIC().NAME(friction);
    FIELD().PUBLIC().NAME(density);
    FIELD().PUBLIC().NAME(restitution);
    FIELD().PUBLIC().NAME(isSensor);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.3f).NAME(mFriction);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mDensity);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mRestitution);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mIsSensor);
    FIELD().PROTECTED().NAME(mShapeId);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHasShape);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mRigidBodyComp);
}
END_META;
CLASS_METHODS_META(o2::ICollider3D)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const ICollider3D&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFriction, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetFriction);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDensity, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDensity);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRestitution, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetRestitution);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIsSensor, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSensor);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
    FUNCTION().PROTECTED().SIGNATURE(b3ShapeId, CreateShape, b3BodyId, const b3ShapeDef&, const b3Transform&, float);
    FUNCTION().PROTECTED().SIGNATURE(void, AddToRigidBody, RigidBody3D*);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveFromRigidBody);
    FUNCTION().PROTECTED().SIGNATURE(void, OnBodyDestroyed);
    FUNCTION().PROTECTED().SIGNATURE(Ref<RigidBody3D>, FindRigidBody);
    FUNCTION().PROTECTED().SIGNATURE(void, OnShapeChanged);
    FUNCTION().PROTECTED().SIGNATURE(b3ShapeDef, MakeShapeDef);
    FUNCTION().PROTECTED().SIGNATURE(b3Transform, GetRelativeTransform, RigidBody3D*, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddToScene);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemoveFromScene);
}
END_META;
// --- END META ---
