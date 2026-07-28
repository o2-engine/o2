#pragma once

#include "Box2D/Dynamics/Joints/b2Joint.h"
#include "o2/Scene/ActorLinkRef.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Physics/RigidBody.h"

namespace o2
{
    // ------------------------------
    // Basic 2D physics joint (Box2D)
    // ------------------------------
    // Connects two RigidBody actors. Concrete joints build the Box2D joint def in CreateJoint().
    // The joint is created in OnStart() (both bodies exist by then) and retried in OnUpdate() until
    // both referenced bodies have their Box2D body, then destroyed on removal.
    class IJoint: public Component
    {
    public:
        PROPERTIES(IJoint);
        PROPERTY(LinkRef<RigidBody>, bodyA, SetBodyA, GetBodyA);              // First connected body property
        PROPERTY(LinkRef<RigidBody>, bodyB, SetBodyB, GetBodyB);              // Second connected body property
        PROPERTY(bool, collideConnected, SetCollideConnected, GetCollideConnected); // Do connected bodies collide property

    public:
        // Default constructor
        IJoint();

        // Copy-constructor
        IJoint(const IJoint& other);

        // Destructor
        ~IJoint();

        // Copy operator
        IJoint& operator=(const IJoint& other);

        // Sets first connected body
        void SetBodyA(const LinkRef<RigidBody>& body);

        // Returns first connected body
        const LinkRef<RigidBody>& GetBodyA() const;

        // Sets second connected body
        void SetBodyB(const LinkRef<RigidBody>& body);

        // Returns second connected body
        const LinkRef<RigidBody>& GetBodyB() const;

        // Sets whether the connected bodies collide with each other
        void SetCollideConnected(bool value);

        // Returns whether the connected bodies collide with each other
        bool GetCollideConnected() const;

        // Is component visible in create menu
        static bool IsAvailableFromCreateMenu();

        SERIALIZABLE(IJoint);

    protected:
        LinkRef<RigidBody> mBodyA; // First connected body @SERIALIZABLE
        LinkRef<RigidBody> mBodyB; // Second connected body @SERIALIZABLE

        bool mCollideConnected = false; // Whether the connected bodies collide with each other @SERIALIZABLE

        b2Joint* mJoint = nullptr;      // Box2D joint handle
        b2Body*  mJointBodyA = nullptr; // Body A the joint was created with (to avoid double-free after a body dies)
        b2Body*  mJointBodyB = nullptr; // Body B the joint was created with

    protected:
        // Builds and creates the concrete Box2D joint from both bodies; returns the joint or null
        virtual b2Joint* CreateJoint(b2Body* bodyA, b2Body* bodyB);

        // Fills the shared base def fields (bodyA/bodyB/collideConnected)
        void SetupBaseDef(b2JointDef& def, b2Body* bodyA, b2Body* bodyB);

        // Creates the joint when both referenced bodies exist; no-op otherwise
        void RebuildJoint();

        // Destroys the joint if present (only when both bodies are still alive, else Box2D already freed it)
        void RemoveJoint();

        // Returns the joint world anchor (this actor's world position), scaled to physics units
        Vec2F GetPhysicsAnchor() const;

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

CLASS_BASES_META(o2::IJoint)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::IJoint)
{
    FIELD().PUBLIC().NAME(bodyA);
    FIELD().PUBLIC().NAME(bodyB);
    FIELD().PUBLIC().NAME(collideConnected);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mBodyA);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mBodyB);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mCollideConnected);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mJoint);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mJointBodyA);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mJointBodyB);
}
END_META;
CLASS_METHODS_META(o2::IJoint)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const IJoint&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBodyA, const LinkRef<RigidBody>&);
    FUNCTION().PUBLIC().SIGNATURE(const LinkRef<RigidBody>&, GetBodyA);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBodyB, const LinkRef<RigidBody>&);
    FUNCTION().PUBLIC().SIGNATURE(const LinkRef<RigidBody>&, GetBodyB);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCollideConnected, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetCollideConnected);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
    FUNCTION().PROTECTED().SIGNATURE(b2Joint*, CreateJoint, b2Body*, b2Body*);
    FUNCTION().PROTECTED().SIGNATURE(void, SetupBaseDef, b2JointDef&, b2Body*, b2Body*);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildJoint);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveJoint);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, GetPhysicsAnchor);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStart);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemoveFromScene);
#if  IS_EDITOR
    FUNCTION().PROTECTED().SIGNATURE(void, OnDrawGizmos);
#endif
}
END_META;
// --- END META ---
