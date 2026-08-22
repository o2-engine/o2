#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Particles/ParticlesEffects.h"
#include "o2/Render/Particles/ParticlesEmitter.h"
#include "o2/Render/Particles/ParticlesEmitterShapes.h"

using namespace o2;

// Damping slows particles down like air drag: velocity shrinks each update,
// clamped at zero for extreme coefficients
TEST(ParticlesEffects, DampingSlowsParticlesDown)
{
    auto emitter = mmake<ParticlesEmitter>();
    emitter->SetShape(mmake<CircleParticlesEmitterShape>());
    emitter->SetParticlesPerSecond(100.0f);
    emitter->SetMaxParticles(20);
    emitter->SetParticlesLifetime(10.0f);
    emitter->SetInitialSpeed(200.0f);
    emitter->SetInitialSpeedRange(0.0f);

    auto damping = mmake<ParticlesDampingEffect>();
    damping->SetDamping(2.0f);
    emitter->AddEffect(damping);

    emitter->Play();
    emitter->Update(0.1f);

    auto& particles = damping->GetParticlesDirect(emitter.Get());
    ASSERT_GT(particles.Count(), 0);

    float speedBefore = particles[0].velocity.Length();
    ASSERT_GT(speedBefore, 0.0f);

    emitter->Update(0.1f);
    float speedAfter = particles[0].velocity.Length();
    EXPECT_LT(speedAfter, speedBefore);
}

TEST(ParticlesEffects, GravityAcceleratesParticlesDown)
{
    auto emitter = mmake<ParticlesEmitter>();
    emitter->SetShape(mmake<CircleParticlesEmitterShape>());
    emitter->SetParticlesPerSecond(100.0f);
    emitter->SetMaxParticles(20);
    emitter->SetParticlesLifetime(10.0f);
    emitter->SetInitialSpeed(0.0f);
    emitter->SetInitialSpeedRange(0.0f);

    auto gravity = mmake<ParticlesGravityEffect>();
    gravity->SetGravity(Vec3F(0, -500, 0));
    emitter->AddEffect(gravity);

    emitter->Play();
    emitter->Update(0.1f);
    emitter->Update(0.1f);

    auto& particles = gravity->GetParticlesDirect(emitter.Get());
    ASSERT_GT(particles.Count(), 0);
    EXPECT_LT(particles[0].velocity.y, 0.0f);
}
