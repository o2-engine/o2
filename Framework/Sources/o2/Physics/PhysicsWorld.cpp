#include "o2/stdafx.h"
#include "PhysicsWorld.h"

#include "Box2D/Dynamics/b2Body.h"
#include "o2/Config/ProjectConfig.h"
#include "o2/Scene/Physics/ICollider.h"
#include "o2/Scene/Physics/RigidBody.h"
#include "o2/Render/Render.h"

namespace o2
{
    DECLARE_SINGLETON(PhysicsWorld);

    PhysicsWorld::PhysicsWorld(RefCounter* refCounter):
        Singleton<PhysicsWorld>(refCounter), mWorld(Vec2F())
    {
        auto debugDraw = mnew PhysicsDebugDraw();
        mWorld.SetDebugDraw(debugDraw);
        debugDraw->SetFlags(b2Draw::e_shapeBit | b2Draw::e_aabbBit | b2Draw::e_pairBit | b2Draw::e_centerOfMassBit | b2Draw::e_jointBit);

        mPrevPhysicsScale = o2Config.physics.scale;
    }

    void PhysicsWorld::PreUpdate()
    {
        CheckPhysicsScale();

        mIsUpdatingPhysicsNow = true;

        mWorld.SetGravity(o2Config.physics.gravity);

        float invScale = 1.0f/o2Config.physics.scale;

        for (b2Body* body = mWorld.GetBodyList(); body; body = body->GetNext())
        {
            auto rigidBody = (RigidBody*)body->GetUserData();
            auto transform = rigidBody->transform;

            body->SetTransform(transform->GetWorldPosition2D()*invScale, transform->GetWorldAngle());
        }
    }

    void PhysicsWorld::Update(float dt)
    {
        mWorld.Step(dt, o2Config.physics.velocityIterations, o2Config.physics.positionIterations);
    }

    void PhysicsWorld::PostUpdate()
    {
        float scale = o2Config.physics.scale;
        for (b2Body* body = mWorld.GetBodyList(); body; body = body->GetNext())
        {
            auto rigidBody = (RigidBody*)body->GetUserData();
            auto transform = rigidBody->transform;

            transform->SetWorldPosition2D(Vec2F(body->GetPosition())*scale);
            transform->SetWorldAngle(body->GetAngle());
        }

        mIsUpdatingPhysicsNow = false;
    }

    void PhysicsWorld::DrawDebug()
    {
        mWorld.DrawDebugData();
    }

    bool PhysicsWorld::IsUpdatingPhysicsNow() const
    {
        return mIsUpdatingPhysicsNow;
    }

    void PhysicsWorld::CheckPhysicsScale()
    {
        if (Math::Equals(mPrevPhysicsScale, o2Config.physics.scale))
            return;

        float scale = o2Config.physics.scale;
        float invScale = 1.0f/scale;
        for (b2Body* body = mWorld.GetBodyList(); body; body = body->GetNext())
        {
            auto rigidBody = (RigidBody*)body->GetUserData();
            auto transform = rigidBody->transform;

            body->SetTransform(transform->GetWorldPosition2D()*invScale, transform->GetWorldAngle());

            auto colliders = rigidBody->mColliders;
            for (auto& collider : colliders)
                collider.Lock()->OnShapeChanged();
        }

        mPrevPhysicsScale = scale;
    }

    void PhysicsDebugDraw::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color)
    {
        float scale = o2Config.physics.scale;
        o2Render.DrawAACircle(Vec2F(center)*scale, radius*scale, Color4(color.r, color.g, color.b, o2Config.physics.debugDrawAlpha));
    }

    void PhysicsDebugDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
    {
        float scale = o2Config.physics.scale;

        Vector<Vec2F> points; 
        points.Resize(vertexCount + 1);
        for (int i = 0; i < vertexCount; i++)
            points[i] = Vec2F(vertices[i])*scale;

        points.Last() = Vec2F(vertices[0])*scale;

        o2Render.DrawAALine(points, Color4(color.r, color.g, color.b, o2Config.physics.debugDrawAlpha));
    }

    void PhysicsDebugDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
    {
        float scale = o2Config.physics.scale;
        o2Render.DrawAALine(Vec2F(p1)*scale, Vec2F(p2)*scale, Color4(color.r, color.g, color.b, o2Config.physics.debugDrawAlpha));
    }

    void PhysicsDebugDraw::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
    {
        float scale = o2Config.physics.scale;
        o2Render.DrawFilledCircle(Vec2F(center)*scale, radius*scale, Color4(color.r, color.g, color.b, o2Config.physics.debugDrawAlpha));
    }

    void PhysicsDebugDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
    {
        float scale = o2Config.physics.scale;

        Vector<Vec2F> points;
        points.Resize(vertexCount);
        for (int i = 0; i < vertexCount; i++)
            points[i] = Vec2F(vertices[i])*scale;

        o2Render.DrawFilledPolygon(points, Color4(color.r, color.g, color.b, o2Config.physics.debugDrawAlpha));
    }

    void PhysicsDebugDraw::DrawTransform(const b2Transform& xf)
    {
        float scale = o2Config.physics.scale;
        o2Render.DrawBasis(Basis(Vec2F(xf.p)*scale, Vec2F(xf.q.GetXAxis())*scale, Vec2F(xf.q.GetYAxis())*scale));
    }

}
