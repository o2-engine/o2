#pragma once

#include "Box2D/Common/b2Draw.h"
#include "Box2D/Dynamics/b2World.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Ref.h"

// Render physics macros
#define o2Physics o2::PhysicsWorld::Instance()

namespace o2
{
    // -------------------
    // Box2D Physics world
    // -------------------
    class PhysicsWorld : public Singleton<PhysicsWorld>
    {
    public:
        // Default constructor
        PhysicsWorld(RefCounter* refCounter);

        // Synchronize physics bodies with actors
        void PreUpdate();

        // Updates physics world and sync bodies
        void Update(float dt);

        // Synchronize actors with bodies
        void PostUpdate();

        // Draws debug graphics
        void DrawDebug();

        // Returns True when PreUpdate has just called, until PostUpdate finished
        bool IsUpdatingPhysicsNow() const;

        // Creates a joint in the Box2D world
        b2Joint* CreateJoint(const b2JointDef& def);

        // Destroys a joint
        void DestroyJoint(b2Joint* joint);

    private:
        b2World mWorld;

        bool mIsUpdatingPhysicsNow = false; // True when PreUpdate has just called, until PostUpdate finished

        float mPrevPhysicsScale = 0.0f; // Previous physics scale

    private:
        // Checks phsyics scale config; updates bodies and colliders with new scale
        void CheckPhysicsScale();

        friend class RigidBody;
    }; 
    
    class PhysicsDebugDraw: public b2Draw
    {
    public:
        float alpha = 0.5f;

        void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) override;
        void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;
        void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) override;
        void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) override;
        void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;
        void DrawTransform(const b2Transform& xf) override;
    };
}
