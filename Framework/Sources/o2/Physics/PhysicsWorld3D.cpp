#include "o2/stdafx.h"
#include "PhysicsWorld3D.h"

#include "o2/Config/ProjectConfig.h"
#include "o2/Physics/Box3DConvert.h"
#include "o2/Scene/Physics3D/RigidBody3D.h"

namespace o2
{
    DECLARE_SINGLETON(PhysicsWorld3D);

    PhysicsWorld3D::PhysicsWorld3D(RefCounter* refCounter):
        Singleton<PhysicsWorld3D>(refCounter)
    {
        b3WorldDef def = b3DefaultWorldDef();
        def.gravity = ToBox3D(o2Config.physics3D.gravity);
        def.workerCount = 1; // serial stepping, no external task system

        mWorldId = b3CreateWorld(&def);
    }

    PhysicsWorld3D::~PhysicsWorld3D()
    {
        if (b3World_IsValid(mWorldId))
            b3DestroyWorld(mWorldId);
    }

    void PhysicsWorld3D::PreUpdate()
    {
        mIsUpdatingPhysicsNow = true;

        b3World_SetGravity(mWorldId, ToBox3D(o2Config.physics3D.gravity));

        float invScale = 1.0f/o2Config.physics3D.scale;
        for (auto body : mBodies)
            body->SyncActorToBody(invScale);
    }

    void PhysicsWorld3D::Update(float dt)
    {
        b3World_Step(mWorldId, dt, o2Config.physics3D.subStepCount);
    }

    void PhysicsWorld3D::PostUpdate()
    {
        float scale = o2Config.physics3D.scale;
        for (auto body : mBodies)
            body->SyncBodyToActor(scale);

        mIsUpdatingPhysicsNow = false;
    }

    bool PhysicsWorld3D::IsUpdatingPhysicsNow() const
    {
        return mIsUpdatingPhysicsNow;
    }

    b3WorldId PhysicsWorld3D::GetWorldId() const
    {
        return mWorldId;
    }

    void PhysicsWorld3D::Register(RigidBody3D* body)
    {
        if (!mBodies.Contains(body))
            mBodies.Add(body);
    }

    void PhysicsWorld3D::Unregister(RigidBody3D* body)
    {
        mBodies.RemoveFirst([&](auto x) { return x == body; });
    }
}
