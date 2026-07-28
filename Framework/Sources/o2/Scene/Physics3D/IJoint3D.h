#pragma once

#include "box3d/box3d.h"
#include "o2/Scene/ActorLinkRef.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Physics3D/RigidBody3D.h"

namespace o2
{
    // -------------------------------
    // Basic 3D physics joint (Box3D)
    // -------------------------------
    // Connects two RigidBody3D actors. Concrete joints build the Box3D joint def in CreateJoint().
    // The joint is created in OnStart() and retried in OnUpdate() until both bodies exist, then
    // destroyed on removal. The joint frame on each body is derived from this actor's world transform.
    class IJoint3D: public Component
    {
    public:
        PROPERTIES(IJoint3D);
        PROPERTY(LinkRef<RigidBody3D>, bodyA, SetBodyA, GetBodyA);            // First connected body property
        PROPERTY(LinkRef<RigidBody3D>, bodyB, SetBodyB, GetBodyB);            // Second connected body property
        PROPERTY(bool, collideConnected, SetCollideConnected, GetCollideConnected); // Do connected bodies collide property

    public:
        // Default constructor
        IJoint3D();

        // Copy-constructor
        IJoint3D(const IJoint3D& other);

        // Destructor
        ~IJoint3D();

        // Copy operator
        IJoint3D& operator=(const IJoint3D& other);

        // Sets first connected body
        void SetBodyA(const LinkRef<RigidBody3D>& body);

        // Returns first connected body
        const LinkRef<RigidBody3D>& GetBodyA() const;

        // Sets second connected body
        void SetBodyB(const LinkRef<RigidBody3D>& body);

        // Returns second connected body
        const LinkRef<RigidBody3D>& GetBodyB() const;

        // Sets whether the connected bodies collide with each other
        void SetCollideConnected(bool value);

        // Returns whether the connected bodies collide with each other
        bool GetCollideConnected() const;

        // Is component visible in create menu
        static bool IsAvailableFromCreateMenu();

        SERIALIZABLE(IJoint3D);

    protected:
        LinkRef<RigidBody3D> mBodyA; // First connected body @SERIALIZABLE
        LinkRef<RigidBody3D> mBodyB; // Second connected body @SERIALIZABLE

        bool mCollideConnected = false; // Whether the connected bodies collide with each other @SERIALIZABLE

        b3JointId mJointId;         // Box3D joint handle; zero-inited in ctor
        bool      mHasJoint = false; // True while a Box3D joint exists

    protected:
        // Builds and creates the concrete Box3D joint from both bodies; returns the joint id
        virtual b3JointId CreateJoint(RigidBody3D* bodyA, RigidBody3D* bodyB);

        // Fills the shared base def (bodies, local frames from this actor's transform, collideConnected)
        void SetupBaseDef(b3JointDef& def, RigidBody3D* bodyA, RigidBody3D* bodyB);

        // Computes this actor's world frame expressed in the given body's local frame, in physics units
        b3Transform ComputeLocalFrame(RigidBody3D* body) const;

        // Creates the joint when both referenced bodies exist; no-op otherwise
        void RebuildJoint();

        // Destroys the joint if present
        void RemoveJoint();

        // Called on scene start; creates the joint
        void OnStart() override;

        // Retries joint creation until both bodies are ready
        void OnUpdate(float dt) override;

        // Called when removed from scene; destroys the joint
        void OnRemoveFromScene() override;

#if IS_EDITOR
        // Draws links from the joint anchor to both connected bodies
        void OnDrawGizmos() override;
#endif
    };
}
// --- META ---

CLASS_BASES_META(o2::IJoint3D)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::IJoint3D)
{
    FIELD().PUBLIC().NAME(bodyA);
    FIELD().PUBLIC().NAME(bodyB);
    FIELD().PUBLIC().NAME(collideConnected);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mBodyA);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mBodyB);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mCollideConnected);
    FIELD().PROTECTED().NAME(mJointId);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHasJoint);
}
END_META;
CLASS_METHODS_META(o2::IJoint3D)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const IJoint3D&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBodyA, const LinkRef<RigidBody3D>&);
    FUNCTION().PUBLIC().SIGNATURE(const LinkRef<RigidBody3D>&, GetBodyA);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBodyB, const LinkRef<RigidBody3D>&);
    FUNCTION().PUBLIC().SIGNATURE(const LinkRef<RigidBody3D>&, GetBodyB);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCollideConnected, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetCollideConnected);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
    FUNCTION().PROTECTED().SIGNATURE(b3JointId, CreateJoint, RigidBody3D*, RigidBody3D*);
    FUNCTION().PROTECTED().SIGNATURE(void, SetupBaseDef, b3JointDef&, RigidBody3D*, RigidBody3D*);
    FUNCTION().PROTECTED().SIGNATURE(b3Transform, ComputeLocalFrame, RigidBody3D*);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildJoint);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveJoint);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStart);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemoveFromScene);
#if  IS_EDITOR
    FUNCTION().PROTECTED().SIGNATURE(void, OnDrawGizmos);
#endif
}
END_META;
// --- END META ---
