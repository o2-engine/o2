#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Particles/ParticlesEffects.h"
#include "o2/Render/Particles/ParticlesEmitter.h"
#include "o2/Render/Particles/ParticlesEmitterShapes.h"
#include "o2/Scene/Components/ParticlesEmitterComponent.h"
#include "Scene/SceneTestHelpers.h"

using namespace o2;

namespace
{
    Ref<ParticlesEmitter> MakePlayingEmitter(bool is3D)
    {
        auto emitter = mmake<ParticlesEmitter>();
        emitter->SetIs3D(is3D);
        emitter->SetParticlesPerSecond(100.0f);
        emitter->SetMaxParticles(100);
        emitter->SetParticlesLifetime(10.0f);
        emitter->SetEmissionDuration(10.0f);
        emitter->SetLoop(Loop::Repeat);
        emitter->Play();
        return emitter;
    }

    int CountAlive(const Ref<ParticlesEmitter>& emitter)
    {
        int count = 0;
        for (auto& particle : emitter->GetParticles())
        {
            if (particle.alive)
                count++;
        }
        return count;
    }
}

TEST(ParticlesEmitter3D, EmitsVelocitiesAlongDirectionWithZeroCone)
{
    auto emitter = MakePlayingEmitter(true);
    emitter->SetEmitParticlesMoveDirection3D(Vec3F(0, 0, 1));
    emitter->SetEmitParticlesMoveDirectionRange(0.0f);
    emitter->SetInitialSpeed(100.0f);
    emitter->SetInitialSpeedRange(0.0f);

    emitter->Update(0.5f);

    ASSERT_GT(CountAlive(emitter), 0);
    for (auto& particle : emitter->GetParticles())
    {
        if (!particle.alive)
            continue;

        EXPECT_NEAR(particle.velocity.x, 0.0f, 0.001f);
        EXPECT_NEAR(particle.velocity.y, 0.0f, 0.001f);
        EXPECT_GT(particle.velocity.z, 0.0f);
    }
}

TEST(ParticlesEmitter3D, ConeSpreadStaysWithinHalfAngle)
{
    auto emitter = MakePlayingEmitter(true);
    emitter->SetEmitParticlesMoveDirection3D(Vec3F(0, 0, 1));
    emitter->SetEmitParticlesMoveDirectionRange(60.0f);
    emitter->SetInitialSpeed(100.0f);
    emitter->SetInitialSpeedRange(0.0f);

    emitter->Update(0.9f);

    ASSERT_GT(CountAlive(emitter), 10);
    float minCos = Math::Cos(Math::Deg2rad(30.0f)) - 0.001f;
    for (auto& particle : emitter->GetParticles())
    {
        if (!particle.alive)
            continue;

        Vec3F direction = particle.velocity.Normalized();
        EXPECT_GE(direction.z, minCos);
    }
}

// Shapes emit in the basis unit space centered at local (0.5, 0.5, 0), like the 2D rect
TEST(ParticlesEmitter3D, SphereShapeEmitsAroundBasisCenter)
{
    auto emitter = MakePlayingEmitter(true);
    emitter->SetShape(mmake<SphereParticlesEmitterShape>());
    emitter->SetInitialSpeed(0.0f);
    emitter->SetInitialSpeedRange(0.0f);

    Vec3F origin(100, 200, 300);
    float size = 40.0f;
    emitter->Set3DBasis(Basis3D(origin, Vec3F(size, 0, 0), Vec3F(0, size, 0), Vec3F(0, 0, size)));

    emitter->Update(0.9f);

    Vec3F center = origin + Vec3F(size*0.5f, size*0.5f, 0.0f);

    ASSERT_GT(CountAlive(emitter), 10);
    for (auto& particle : emitter->GetParticles())
    {
        if (!particle.alive)
            continue;

        EXPECT_LE((particle.position - center).Length(), size*0.5f + 0.001f);
    }
}

TEST(ParticlesEmitter3D, GravityEffectAccelerates3DVelocity)
{
    auto emitter = MakePlayingEmitter(true);
    emitter->SetInitialSpeed(0.0f);
    emitter->SetInitialSpeedRange(0.0f);

    auto gravityEffect = mmake<ParticlesGravityEffect>();
    gravityEffect->SetGravity(Vec3F(0, 0, -100));
    emitter->AddEffect(gravityEffect);

    emitter->Update(0.5f);
    emitter->Update(0.5f);

    ASSERT_GT(CountAlive(emitter), 0);
    for (auto& particle : emitter->GetParticles())
    {
        if (!particle.alive)
            continue;

        EXPECT_LT(particle.velocity.z, 0.0f);
    }
}

TEST(ParticlesEmitter3D, ParticlesBoundsCoverEmittedParticles)
{
    auto emitter = MakePlayingEmitter(true);
    emitter->SetInitialSpeed(0.0f);
    emitter->SetInitialSpeedRange(0.0f);

    Vec3F center(50, -20, 70);
    emitter->Set3DBasis(Basis3D(center, Vec3F(10, 0, 0), Vec3F(0, 10, 0), Vec3F(0, 0, 10)));

    AABB bounds;
    EXPECT_FALSE(emitter->GetParticlesBounds(bounds));

    emitter->Update(0.5f);

    ASSERT_TRUE(emitter->GetParticlesBounds(bounds));
    EXPECT_LE(bounds.min.x, center.x + 5.0f);
    EXPECT_GE(bounds.max.x, center.x - 5.0f);
    EXPECT_LE(bounds.min.z, center.z + 5.0f);
    EXPECT_GE(bounds.max.z, center.z - 5.0f);
}

TEST(ParticlesEmitter3D, TwoDimensionalModeKeepsZeroZ)
{
    auto emitter = MakePlayingEmitter(false);
    emitter->SetInitialSpeed(100.0f);
    emitter->SetInitialSpeedRange(50.0f);

    auto gravityEffect = mmake<ParticlesGravityEffect>();
    gravityEffect->SetGravity(Vec3F(0, -100, 0));
    emitter->AddEffect(gravityEffect);

    emitter->Update(0.5f);
    emitter->Update(0.5f);

    ASSERT_GT(CountAlive(emitter), 0);
    for (auto& particle : emitter->GetParticles())
    {
        if (!particle.alive)
            continue;

        EXPECT_EQ(particle.position.z, 0.0f);
        EXPECT_EQ(particle.velocity.z, 0.0f);
    }
}

TEST(ParticlesEmitter3D, ComponentCategoryFollowsMode)
{
    SceneCleanGuard guard;

    auto component = mmake<ParticlesEmitterComponent>();
    EXPECT_EQ(component->GetSceneDrawableCategory(), SceneDrawableCategory::Scene2D);
    EXPECT_TRUE(component->Is3DDrawableTransparent());

    component->SetIs3D(true);
    EXPECT_EQ(component->GetSceneDrawableCategory(), SceneDrawableCategory::Scene3D);
}

// The editor re-sends the same world basis on every transform refresh, particles must not move
TEST(ParticlesEmitter3D, SameBasisReassignKeepsParticlePositions)
{
    auto emitter = MakePlayingEmitter(true);
    emitter->SetInitialSpeed(100.0f);

    Basis3D basis(Vec3F(-30.0f, 40.0f, 130.0f), Vec3F(40, 0, 0), Vec3F(0, 40, 0), Vec3F(0, 0, 40));
    emitter->Set3DBasis(basis);
    emitter->Update(0.5f);

    ASSERT_GT(CountAlive(emitter), 0);
    auto before = emitter->GetParticles();

    emitter->Set3DBasis(basis);

    auto& after = emitter->GetParticles();
    for (int i = 0; i < before.Count(); i++)
    {
        if (!before[i].alive)
            continue;

        EXPECT_EQ(after[i].position.x, before[i].position.x);
        EXPECT_EQ(after[i].position.y, before[i].position.y);
        EXPECT_EQ(after[i].position.z, before[i].position.z);
    }
}

// A zero-size actor gives a degenerate basis, it has no inverse and must not collapse particles
TEST(ParticlesEmitter3D, DegenerateBasisReassignDoesNotCollapseParticles)
{
    auto emitter = MakePlayingEmitter(true);
    emitter->SetInitialSpeed(100.0f);
    emitter->SetInitialSpeedRange(0.0f);

    Basis3D degenerate(Vec3F(-30.0f, 40.0f, 130.0f), Vec3F(), Vec3F(), Vec3F());
    emitter->Set3DBasis(degenerate);
    emitter->Update(0.5f);
    emitter->Update(0.5f);

    ASSERT_GT(CountAlive(emitter), 0);
    auto before = emitter->GetParticles();

    emitter->Set3DBasis(degenerate);
    Basis3D moved = degenerate;
    moved.origin += Vec3F(1, 0, 0);
    emitter->Set3DBasis(moved);

    auto& after = emitter->GetParticles();
    for (int i = 0; i < before.Count(); i++)
    {
        if (!before[i].alive)
            continue;

        float shift = (after[i].position - before[i].position).Length();
        EXPECT_LT(shift, 2.0f) << "particle " << i << " collapsed to the emitter origin";
    }
}
