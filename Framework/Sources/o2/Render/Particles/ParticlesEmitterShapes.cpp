#include "o2/stdafx.h"
#include "ParticlesEmitterShapes.h"

#include "o2/Render/Particles/ParticlesEmitter.h"

namespace o2
{
    Vec2F ParticlesEmitterShape::GetEmittinPoint(const Basis& transform, bool fromShell)
    {
        return Vec2F();
    }

    void ParticlesEmitterShape::OnChanged()
    {
#if IS_EDITOR
        if (mEmitter)
            mEmitter.Lock()->InvalidateBakedFrames();
#endif
    }

    Vec2F CircleParticlesEmitterShape::GetEmittinPoint(const Basis& transform, bool fromShell)
    {
        if (fromShell)
        {
            Vec2F localPoint = Vec2F::Rotated(Math::Random(0.0f, Math::PI()*2.0f))*0.5f + Vec2F(0.5f, 0.5f);
            return localPoint*transform;
        }
        else
        {
            Vec2F localPoint = Vec2F::Rotated(Math::Random(0.0f, Math::PI()*2.0f))*Math::Random(0.0f, 0.5f) + Vec2F(0.5f, 0.5f);
            return localPoint*transform;
        }
    }

    Vec2F SquareParticlesEmitterShape::GetEmittinPoint(const Basis& transform, bool fromShell)
    {
        if (fromShell)
        {
            Vec2F localPoint = Vec2F(Math::Random(0.0f, 1.0f), Math::Random(0.0f, 1.0f));

            if (Math::Random(0, 100) > 50)
                localPoint.x = Math::Round(localPoint.x);
            else
                localPoint.y = Math::Round(localPoint.y);

            return localPoint*transform;
        }
        else
        {
            Vec2F localPoint = Vec2F(Math::Random(0.0f, 1.0f), Math::Random(0.0f, 1.0f));
            return localPoint*transform;
        }
    }
}
// --- META ---

DECLARE_CLASS(o2::ParticlesEmitterShape, o2__ParticlesEmitterShape);

DECLARE_CLASS(o2::CircleParticlesEmitterShape, o2__CircleParticlesEmitterShape);

DECLARE_CLASS(o2::SquareParticlesEmitterShape, o2__SquareParticlesEmitterShape);
// --- END META ---
