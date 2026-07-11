#include "o2/stdafx.h"
#include "ParticlesEmitterShapes.h"

#include "o2/Render/Particles/ParticlesEmitter.h"

namespace o2
{
    Vec3F ParticlesEmitterShape::GetEmittinPoint(const Basis3D& transform, bool fromShell)
    {
        return Vec3F();
    }

    void ParticlesEmitterShape::OnChanged()
    {
#if IS_EDITOR
        if (mEmitter)
            mEmitter.Lock()->InvalidateBakedFrames();
#endif
    }

    Vec3F CircleParticlesEmitterShape::GetEmittinPoint(const Basis3D& transform, bool fromShell)
    {
        float radius = fromShell ? 0.5f : Math::Random(0.0f, 0.5f);
        Vec2F localPoint = Vec2F::Rotated(Math::Random(0.0f, Math::PI()*2.0f))*radius + Vec2F(0.5f, 0.5f);
        return transform.Transform(Vec3F(localPoint.x, localPoint.y, 0.0f));
    }

    Vec3F SquareParticlesEmitterShape::GetEmittinPoint(const Basis3D& transform, bool fromShell)
    {
        Vec2F localPoint = Vec2F(Math::Random(0.0f, 1.0f), Math::Random(0.0f, 1.0f));

        if (fromShell)
        {
            if (Math::Random(0, 100) > 50)
                localPoint.x = Math::Round(localPoint.x);
            else
                localPoint.y = Math::Round(localPoint.y);
        }

        return transform.Transform(Vec3F(localPoint.x, localPoint.y, 0.0f));
    }

    Vec3F SphereParticlesEmitterShape::GetEmittinPoint(const Basis3D& transform, bool fromShell)
    {
        float u = Math::Random(-1.0f, 1.0f);
        float around = Math::Random(0.0f, Math::PI()*2.0f);
        float planarRadius = Math::Sqrt(Math::Max(0.0f, 1.0f - u*u));
        Vec3F direction(planarRadius*Math::Cos(around), planarRadius*Math::Sin(around), u);

        float radius = fromShell ? 0.5f : 0.5f*Math::Pow(Math::Random(0.0f, 1.0f), 1.0f/3.0f);
        Vec3F localPoint = direction*radius + Vec3F(0.5f, 0.5f, 0.0f);
        return transform.Transform(localPoint);
    }
}
// --- META ---

DECLARE_CLASS(o2::ParticlesEmitterShape, o2__ParticlesEmitterShape);

DECLARE_CLASS(o2::CircleParticlesEmitterShape, o2__CircleParticlesEmitterShape);

DECLARE_CLASS(o2::SquareParticlesEmitterShape, o2__SquareParticlesEmitterShape);

DECLARE_CLASS(o2::SphereParticlesEmitterShape, o2__SphereParticlesEmitterShape);
// --- END META ---
