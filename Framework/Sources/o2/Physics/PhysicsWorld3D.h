#pragma once

#include "box3d/box3d.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Containers/Vector.h"

// Physics 3D world access macro
#define o2Physics3D o2::PhysicsWorld3D::Instance()

namespace o2
{
    class RigidBody3D;

    // -------------------
    // Box3D physics world
    // -------------------
    class PhysicsWorld3D : public Singleton<PhysicsWorld3D>
    {
    public:
        // Default constructor, creates the Box3D world
        explicit PhysicsWorld3D(RefCounter* refCounter);

        // Destructor, destroys the Box3D world
        ~PhysicsWorld3D();

        // Synchronizes physics bodies with actors that were moved externally
        void PreUpdate();

        // Steps the physics world
        void Update(float dt);

        // Synchronizes actors with simulated bodies
        void PostUpdate();

        // Returns True when PreUpdate has just called, until PostUpdate finished
        bool IsUpdatingPhysicsNow() const;

        // Returns the Box3D world handle
        b3WorldId GetWorldId() const;

    private:
        b3WorldId mWorldId = {}; // Box3D world handle

        Vector<RigidBody3D*> mBodies; // Registered rigid bodies (box3d gives no body enumeration, so we track them)

        bool mIsUpdatingPhysicsNow = false; // True while stepping, between PreUpdate and PostUpdate

    private:
        // Registers a body for per-frame sync
        void Register(RigidBody3D* body);

        // Unregisters a body
        void Unregister(RigidBody3D* body);

        friend class RigidBody3D;
    };
}
