#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Particles/ParticlesEffects.h"
#include "o2/Render/Particles/ParticlesEmitter.h"
#include "o2/Render/Particles/ParticlesEmitterShapes.h"
#include "o2/Render/Material.h"

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

// Effects edited in the editor must refresh the emitter that owns them, and clones must not share
// curves with their source (editing an instance edited the prototype and notified its effect instead)
TEST(ParticlesEffects, AddEffectLinksTheEmitter)
{
    auto emitter = mmake<ParticlesEmitter>();
    auto effect = mmake<ParticlesColorEffect>();
    emitter->AddEffect(effect);
    EXPECT_EQ(effect->GetEmitter(), emitter);

    emitter->RemoveEffect(effect);
    EXPECT_EQ(effect->GetEmitter(), nullptr);
}

TEST(ParticlesEffects, CloneDeepCopiesCurvesAndLinksItsOwnEmitter)
{
    auto emitter = mmake<ParticlesEmitter>();
    auto color = mmake<ParticlesColorEffect>();
    color->colorGradient->InsertKey(0.5f, Color4::Red());
    emitter->AddEffect(color);
    emitter->AddEffect(mmake<ParticlesSizeEffect>());

    auto clone = mmake<ParticlesEmitter>(*emitter);
    ASSERT_EQ(clone->GetEffects().Count(), 2);

    auto clonedColor = DynamicCast<ParticlesColorEffect>(clone->GetEffects()[0]);
    ASSERT_TRUE(clonedColor);
    EXPECT_NE(clonedColor, color);
    EXPECT_NE(clonedColor->colorGradient, color->colorGradient);
    EXPECT_EQ(clonedColor->colorGradient->GetKeys().Count(), color->colorGradient->GetKeys().Count());
    EXPECT_EQ(clonedColor->GetEmitter(), clone);

    // editing the clone's gradient leaves the source untouched
    int sourceKeys = color->colorGradient->GetKeys().Count();
    clonedColor->colorGradient->InsertKey(0.25f, Color4::Blue());
    EXPECT_EQ(color->colorGradient->GetKeys().Count(), sourceKeys);
}

TEST(ParticlesEffects, DestroyedEffectLeavesNoDelegateOnItsGradient)
{
    auto effect = mmake<ParticlesColorEffect>();
    auto gradient = effect->colorGradient;
    EXPECT_FALSE(gradient->onKeysChanged.IsEmpty());

    effect = nullptr;
    EXPECT_TRUE(gradient->onKeysChanged.IsEmpty());
    gradient->onKeysChanged();
}

#if IS_EDITOR
// A gradient edit re-bakes the frames of a sub-controlled emitter at once, no scrub needed
TEST(ParticlesEffects, GradientEditRebakesParticlesImmediately)
{
    auto emitter = mmake<ParticlesEmitter>();
    emitter->SetShape(mmake<CircleParticlesEmitterShape>());
    emitter->SetEmissionDuration(1.0f);
    emitter->SetParticlesLifetime(2.0f);
    emitter->SetParticlesPerSecond(100.0f);
    emitter->SetMaxParticles(200);
    emitter->SetLoop(Loop::None);
    emitter->SetSubControlled(true);
    emitter->Stop();

    auto color = mmake<ParticlesColorEffect>();
    color->colorGradient->InsertKey(0.0f, Color4::White());
    color->colorGradient->InsertKey(1.0f, Color4::White());
    emitter->AddEffect(color);

    emitter->SetTime(0.5f);
    ASSERT_GT(emitter->GetParticlesCount(), 0);
    for (auto& particle : emitter->GetParticles())
    {
        if (particle.alive)
            EXPECT_EQ(particle.color.r, 255);
    }

    // paint everything blue: the visible frame must follow without SetTime
    color->colorGradient->RemoveAllKeys();
    color->colorGradient->InsertKey(0.0f, Color4::Blue());
    color->colorGradient->InsertKey(1.0f, Color4::Blue());

    ASSERT_GT(emitter->GetParticlesCount(), 0);
    for (auto& particle : emitter->GetParticles())
    {
        if (particle.alive)
        {
            EXPECT_EQ(particle.color.r, 0);
            EXPECT_EQ(particle.color.b, 255);
        }
    }
}
#endif

// Streaks: a particle turns along its velocity and its x side grows with speed
TEST(ParticlesEffects, VelocityStretchAlignsAndElongatesParticles)
{
    auto emitter = mmake<ParticlesEmitter>();
    emitter->SetShape(mmake<CircleParticlesEmitterShape>());
    emitter->SetEmissionDuration(1.0f);
    emitter->SetParticlesLifetime(2.0f);
    emitter->SetParticlesPerSecond(100.0f);
    emitter->SetMaxParticles(50);
    emitter->SetInitialSpeed(300.0f);
    emitter->SetInitialSpeedRange(0.0f);
    emitter->SetInitialSize(0.5f);
    emitter->SetInitialSizeRange(0.0f);
    emitter->SetEmitParticlesMoveDirection(90.0f);
    emitter->SetEmitParticlesMoveDirectionRange(0.0f);

    auto stretch = mmake<ParticlesVelocityStretchEffect>();
    stretch->SetStretch(0.005f);
    stretch->SetMaxStretch(4.0f);
    emitter->AddEffect(stretch);

    emitter->Play();
    emitter->Update(0.1f);

    int checked = 0;
    for (auto& particle : emitter->GetParticles())
    {
        if (!particle.alive)
            continue;

        checked++;
        EXPECT_NEAR(particle.angle, Math::PI()*0.5f, 0.05f) << "the streak points up, along the velocity";
        EXPECT_NEAR(particle.size.x/particle.size.y, 1.0f + 0.005f*300.0f, 0.05f);
    }
    EXPECT_GT(checked, 0);

    // the bound caps runaway speeds
    stretch->SetMaxStretch(1.5f);
    emitter->Update(0.05f);
    for (auto& particle : emitter->GetParticles())
    {
        if (particle.alive)
            EXPECT_NEAR(particle.size.x/particle.size.y, 1.5f, 0.05f);
    }
}

// Prototype instances are copies: every emission parameter must survive the copy constructor
TEST(ParticlesEffects, CloneKeepsEmissionParameters)
{
    auto emitter = mmake<ParticlesEmitter>();
    emitter->SetEmissionDuration(0.72f);
    emitter->SetParticlesLifetime(0.46f);
    emitter->SetPrewarmTime(0.3f);
    emitter->SetInitialWidthScale(1.5f);
    emitter->SetInitialWidthScaleRange(0.25f);
    emitter->SetInitialAngleSpeed(90.0f);
    emitter->SetInitialAngleSpeedRange(30.0f);
    emitter->SetParticlesPerSecond(260.0f);

    auto clone = emitter->CloneAsRef<ParticlesEmitter>();
    EXPECT_FLOAT_EQ(clone->GetEmissionDuration(), 0.72f);
    EXPECT_FLOAT_EQ(clone->GetParticlesLifetime(), 0.46f);
    EXPECT_FLOAT_EQ(clone->GetPrewarmTime(), 0.3f);
    EXPECT_FLOAT_EQ(clone->GetInitialWidthScale(), 1.5f);
    EXPECT_FLOAT_EQ(clone->GetInitialWidthScaleRange(), 0.25f);
    EXPECT_FLOAT_EQ(clone->GetInitialAngleSpeed(), 90.0f);
    EXPECT_FLOAT_EQ(clone->GetInitialAngleSpeedRange(), 30.0f);
    EXPECT_FLOAT_EQ(clone->GetDuration(), emitter->GetDuration());
}

namespace
{
    struct MaterialProbeContainer: public ParticlesContainer
    {
        Ref<Material> material;

        void SetMaterial(const Ref<Material>& value) override { material = value; }
        void Update(Vector<Particle>& particles, int maxParticles) override {}
        void Draw() override {}
    };

    struct MaterialProbeSource: public ParticleSource
    {
        Ref<MaterialProbeContainer> container;

        Ref<ParticlesContainer> CreateContainer() override
        {
            container = mmake<MaterialProbeContainer>();
            return container;
        }
    };
}

// A container made later (new source, deserialization) draws with the emitter's material
TEST(ParticlesEmitter, NewContainerTakesTheEmitterMaterial)
{
    auto emitter = mmake<ParticlesEmitter>();
    auto material = mmake<Material>();
    material->SetBlendMode(BlendMode::Add);
    emitter->SetMaterial(material);

    auto source = mmake<MaterialProbeSource>();
    emitter->SetParticlesSource(source);

    ASSERT_TRUE(source->container);
    EXPECT_EQ(source->container->material, material);
}
