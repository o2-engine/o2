#include "o2/stdafx.h"
#include "ParticlesEffects.h"

#include "o2/Render/Particles/ParticlesEmitter.h"

namespace o2
{
    void ParticlesEffect::Update(float dt, ParticlesEmitter* emitter)
    {}

    Vector<Particle>& ParticlesEffect::GetParticlesDirect(ParticlesEmitter* emitter)
    {
        return emitter->mParticles;
    }

    Ref<ParticlesEmitter> ParticlesEffect::GetEmitter() const
    {
        return mEmitter.Lock();
    }

    void ParticlesEffect::OnChanged()
    {
#if IS_EDITOR
        if (auto emitter = mEmitter.Lock())
            emitter->InvalidateBakedFrames();
#endif
    }

    void ParticlesEffect::ListenKeysChanged(Function<void()>& keysEvent)
    {
        keysEvent -= MakeFunction(this, &ParticlesEffect::OnChanged);
        keysEvent += MakeFunction(this, &ParticlesEffect::OnChanged);
    }

    void ParticlesEffect::UnlistenKeysChanged(Function<void()>& keysEvent)
    {
        keysEvent -= MakeFunction(this, &ParticlesEffect::OnChanged);
    }

    void ParticlesGravityEffect::Update(float dt, ParticlesEmitter* emitter)
    {
        Vec3F v = mGravity*dt;
        for (auto& p : GetParticlesDirect(emitter))
            p.velocity += v;
    }

    void ParticlesDampingEffect::Update(float dt, ParticlesEmitter* emitter)
    {
        float factor = Math::Clamp01(1.0f - mDamping*dt);
        for (auto& p : GetParticlesDirect(emitter))
            p.velocity *= factor;
    }

    ParticlesColorEffect::ParticlesColorEffect()
    {
        colorGradient = mmake<ColorGradient>();
        ListenKeysChanged(colorGradient->onKeysChanged);
    }

    ParticlesColorEffect::ParticlesColorEffect(const ParticlesColorEffect& other):
        ParticlesEffect(other), colorGradient(mmake<ColorGradient>(*other.colorGradient))
    {
        ListenKeysChanged(colorGradient->onKeysChanged);
    }

    ParticlesColorEffect::~ParticlesColorEffect()
    {
        if (colorGradient)
            UnlistenKeysChanged(colorGradient->onKeysChanged);
    }

    void ParticlesColorEffect::OnParticleEmitted(Particle& particle)
    {
        int particleIndex = particle.index;

        CheckDataBufferSize(particleIndex);

        mColorData[particleIndex].cacheKey = 0;
    }

    void ParticlesColorEffect::Update(float dt, ParticlesEmitter* emitter)
    {
        if (!colorGradient)
            return;

        auto& particles = GetParticlesDirect(emitter);

        CheckDataBufferSize(particles.Count());

        for (auto& p : particles)
        {
            int particleIndex = p.index;

            auto& colorData = mColorData[particleIndex];
            float lifeTimeCoef = 1.0f - p.timeLeft / p.lifetime;
            //o2Debug.Log("lifeTimeCoef: %f (%f, %f), particle id: %i", lifeTimeCoef, p.timeLeft, p.lifetime, particleIndex);
            p.color = colorGradient->Evaluate(lifeTimeCoef, true, colorData.cacheKey);
        }
    }

    void ParticlesColorEffect::CheckDataBufferSize(int particlesCount)
    {
        for (int i = mColorData.size(); i <= particlesCount; i++)
            mColorData.Add(ParticleColorData());
    }

    void ParticlesColorEffect::OnDeserialized(const DataValue& node)
    {
        ListenKeysChanged(colorGradient->onKeysChanged);
    }

    ParticlesRandomColorEffect::ParticlesRandomColorEffect()
    {
        colorGradientA = mmake<ColorGradient>();
        colorGradientB = mmake<ColorGradient>();

        ListenKeysChanged(colorGradientA->onKeysChanged);
        ListenKeysChanged(colorGradientB->onKeysChanged);
    }

    ParticlesRandomColorEffect::ParticlesRandomColorEffect(const ParticlesRandomColorEffect& other):
        ParticlesEffect(other), colorGradientA(mmake<ColorGradient>(*other.colorGradientA)),
        colorGradientB(mmake<ColorGradient>(*other.colorGradientB))
    {
        ListenKeysChanged(colorGradientA->onKeysChanged);
        ListenKeysChanged(colorGradientB->onKeysChanged);
    }

    ParticlesRandomColorEffect::~ParticlesRandomColorEffect()
    {
        if (colorGradientA)
            UnlistenKeysChanged(colorGradientA->onKeysChanged);

        if (colorGradientB)
            UnlistenKeysChanged(colorGradientB->onKeysChanged);
    }

    void ParticlesRandomColorEffect::CheckDataBufferSize(int particlesCount)
    {
        for (int i = mColorData.size(); i <= particlesCount; i++)
            mColorData.Add(ParticleColorData());
    }

    void ParticlesRandomColorEffect::OnDeserialized(const DataValue& node)
    {
        ListenKeysChanged(colorGradientA->onKeysChanged);
        ListenKeysChanged(colorGradientB->onKeysChanged);
    }

    void ParticlesRandomColorEffect::OnParticleEmitted(Particle& particle)
    {
        int particleIndex = particle.index;

        CheckDataBufferSize(particleIndex);

        auto& colorData = mColorData[particleIndex];
        colorData.cacheKeyA = 0;
        colorData.cacheKeyB = 0;
        colorData.coef = Math::Random(0.0f, 1.0f);
    }

    void ParticlesRandomColorEffect::Update(float dt, ParticlesEmitter* emitter)
    {
        auto& particles = GetParticlesDirect(emitter);

        CheckDataBufferSize(particles.Count());

        for (auto& p : particles)
        {
            int particleIndex = p.index;

            auto& colorData = mColorData[particleIndex];
            auto lifeTimeCoef = 1.0f - p.timeLeft / p.lifetime;
            auto colorA = colorGradientA->Evaluate(lifeTimeCoef, true, colorData.cacheKeyA);
            auto colorB = colorGradientB->Evaluate(lifeTimeCoef, true, colorData.cacheKeyB);
            p.color = Math::Lerp(colorA, colorB, colorData.coef);
        }
    }

    ParticlesSizeEffect::ParticlesSizeEffect()
    {
        curve = mmake<Curve>(Curve::Linear(0.0f, 1.0f));
        ListenKeysChanged(curve->onKeysChanged);
    }

    ParticlesSizeEffect::ParticlesSizeEffect(const ParticlesSizeEffect& other):
        ParticlesEffect(other), curve(mmake<Curve>(*other.curve))
    {
        ListenKeysChanged(curve->onKeysChanged);
    }

    ParticlesSizeEffect::~ParticlesSizeEffect()
    {
        if (curve)
            UnlistenKeysChanged(curve->onKeysChanged);
    }

    void ParticlesSizeEffect::OnParticleEmitted(Particle& particle)
    {
        int particleIndex = particle.index;

        CheckDataBufferSize(particleIndex);

        mData[particleIndex].initialSize = particle.size;
        mData[particleIndex].cacheKey = 0;
        mData[particleIndex].cacheKeyApprox = 0;
        mData[particleIndex].randomCoef = Math::Random(0.0f, 1.0f);
    }

    void ParticlesSizeEffect::Update(float dt, ParticlesEmitter* emitter)
    {
        auto& particles = GetParticlesDirect(emitter);

        CheckDataBufferSize(particles.Count());

        for (auto& p : particles)
        {
            int particleIndex = p.index;

            auto& sizeData = mData[particleIndex];
            p.size = sizeData.initialSize*curve->Evaluate(1.0f - p.timeLeft/p.lifetime, sizeData.randomCoef, true, sizeData.cacheKey, sizeData.cacheKeyApprox);
        }
    }

    void ParticlesSizeEffect::CheckDataBufferSize(int particlesCount)
    {
        for (int i = mData.size(); i <= particlesCount; i++)
            mData.Add(ParticleData());
    }

    void ParticlesSizeEffect::OnDeserialized(const DataValue& node)
    {
        ListenKeysChanged(curve->onKeysChanged);
    }

    ParticlesAngleEffect::ParticlesAngleEffect()
    {
        curve = mmake<Curve>(Curve::Linear(0.0f, 360.0f));
        ListenKeysChanged(curve->onKeysChanged);
    }

    ParticlesAngleEffect::ParticlesAngleEffect(const ParticlesAngleEffect& other):
        ParticlesEffect(other), curve(mmake<Curve>(*other.curve))
    {
        ListenKeysChanged(curve->onKeysChanged);
    }

    ParticlesAngleEffect::~ParticlesAngleEffect()
    {
        if (curve)
            UnlistenKeysChanged(curve->onKeysChanged);
    }

    void ParticlesAngleEffect::OnParticleEmitted(Particle& particle)
    {
        int particleIndex = particle.index;

        CheckDataBufferSize(particleIndex);

        mData[particleIndex].initialAngle = particle.angle;
        mData[particleIndex].cacheKey = 0;
        mData[particleIndex].cacheKeyApprox = 0;
        mData[particleIndex].randomCoef = Math::Random(0.0f, 1.0f);
    }

    void ParticlesAngleEffect::Update(float dt, ParticlesEmitter* emitter)
    {
        auto& particles = GetParticlesDirect(emitter);

        CheckDataBufferSize(particles.Count());

        for (auto& p : particles)
        {
            int particleIndex = p.index;

            auto& data = mData[particleIndex];
            p.angle = data.initialAngle + curve->Evaluate(1.0f - p.timeLeft/p.lifetime, data.randomCoef, true, data.cacheKey, data.cacheKeyApprox);
        }
    }

    void ParticlesAngleEffect::CheckDataBufferSize(int particlesCount)
    {
        for (int i = mData.size(); i <= particlesCount; i++)
            mData.Add(ParticleData());
    }

    void ParticlesAngleEffect::OnDeserialized(const DataValue& node)
    {
        ListenKeysChanged(curve->onKeysChanged);
    }

    ParticlesAngleSpeedEffect::ParticlesAngleSpeedEffect()
    {
        curve = mmake<Curve>(Curve::Linear(0.0f, 360.0f));
        ListenKeysChanged(curve->onKeysChanged);
    }

    ParticlesAngleSpeedEffect::ParticlesAngleSpeedEffect(const ParticlesAngleSpeedEffect& other):
        ParticlesEffect(other), curve(mmake<Curve>(*other.curve))
    {
        ListenKeysChanged(curve->onKeysChanged);
    }

    ParticlesAngleSpeedEffect::~ParticlesAngleSpeedEffect()
    {
        if (curve)
            UnlistenKeysChanged(curve->onKeysChanged);
    }

    void ParticlesAngleSpeedEffect::OnParticleEmitted(Particle& particle)
    {
        int particleIndex = particle.index;

        CheckDataBufferSize(particleIndex);

        mData[particleIndex].initialSpeed = particle.angleSpeed;
        mData[particleIndex].cacheKey = 0;
        mData[particleIndex].cacheKeyApprox = 0;
        mData[particleIndex].randomCoef = Math::Random(0.0f, 1.0f);
    }

    void ParticlesAngleSpeedEffect::Update(float dt, ParticlesEmitter* emitter)
    {
        auto& particles = GetParticlesDirect(emitter);

        CheckDataBufferSize(particles.Count());

        for (auto& p : particles)
        {
            int particleIndex = p.index;

            auto& data = mData[particleIndex];
            p.angleSpeed = data.initialSpeed + curve->Evaluate(1.0f - p.timeLeft/p.lifetime, data.randomCoef, true, data.cacheKey, data.cacheKeyApprox);
        }
    }

    void ParticlesAngleSpeedEffect::CheckDataBufferSize(int particlesCount)
    {
        for (int i = mData.size(); i <= particlesCount; i++)
            mData.Add(ParticleData());
    }

    void ParticlesAngleSpeedEffect::OnDeserialized(const DataValue& node)
    {
        ListenKeysChanged(curve->onKeysChanged);
    }

    ParticlesVelocityEffect::ParticlesVelocityEffect()
    {
        XCurve = mmake<Curve>(Curve::Linear(0.0f, 10.0f));
        ListenKeysChanged(XCurve->onKeysChanged);

        YCurve = mmake<Curve>(Curve::Linear(0.0f, 10.0f));
        ListenKeysChanged(YCurve->onKeysChanged);
    }

    ParticlesVelocityEffect::ParticlesVelocityEffect(const ParticlesVelocityEffect& other):
        ParticlesEffect(other), XCurve(mmake<Curve>(*other.XCurve)), YCurve(mmake<Curve>(*other.YCurve))
    {
        ListenKeysChanged(XCurve->onKeysChanged);
        ListenKeysChanged(YCurve->onKeysChanged);
    }

    ParticlesVelocityEffect::~ParticlesVelocityEffect()
    {
        if (XCurve)
            UnlistenKeysChanged(XCurve->onKeysChanged);

        if (YCurve)
            UnlistenKeysChanged(YCurve->onKeysChanged);
    }

    void ParticlesVelocityEffect::OnParticleEmitted(Particle& particle)
    {
        int particleIndex = particle.index;

        CheckDataBufferSize(particleIndex);

        mData[particleIndex].initialVelocity = particle.velocity;

        mData[particleIndex].cacheXKey = 0;
        mData[particleIndex].cacheXKeyApprox = 0;
        mData[particleIndex].randomXCoef = Math::Random(0.0f, 1.0f);

        mData[particleIndex].cacheYKey = 0;
        mData[particleIndex].cacheYKeyApprox = 0;
        mData[particleIndex].randomYCoef = Math::Random(0.0f, 1.0f);
    }

    void ParticlesVelocityEffect::Update(float dt, ParticlesEmitter* emitter)
    {
        auto& particles = GetParticlesDirect(emitter);

        CheckDataBufferSize(particles.Count());

        for (auto& p : particles)
        {
            int particleIndex = p.index;

            auto& data = mData[particleIndex];
            float x = XCurve->Evaluate(1.0f - p.timeLeft/p.lifetime, data.randomXCoef, true, data.cacheXKey, data.cacheXKeyApprox);
            float y = YCurve->Evaluate(1.0f - p.timeLeft/p.lifetime, data.randomYCoef, true, data.cacheYKey, data.cacheYKeyApprox);

            p.velocity = data.initialVelocity + Vec3F(x, y, 0.0f);
        }
    }

    void ParticlesVelocityEffect::CheckDataBufferSize(int particlesCount)
    {
        for (int i = mData.size(); i <= particlesCount; i++)
            mData.Add(ParticleData());
    }

    void ParticlesVelocityEffect::OnDeserialized(const DataValue& node)
    {
        ListenKeysChanged(XCurve->onKeysChanged);
        ListenKeysChanged(YCurve->onKeysChanged);
    }

    ParticlesSplineEffect::ParticlesSplineEffect()
    {
        timeCurve = mmake<Curve>(Curve::EaseInOut());
        ListenKeysChanged(timeCurve->onKeysChanged);

        spline = mmake<Spline>(Vector<Vec2F>{ Vec2F(), Vec2F(100, 0) });
        ListenKeysChanged(spline->onKeysChanged);
    }

    ParticlesSplineEffect::ParticlesSplineEffect(const ParticlesSplineEffect& other):
        ParticlesEffect(other), timeCurve(mmake<Curve>(*other.timeCurve)), spline(mmake<Spline>(*other.spline))
    {
        ListenKeysChanged(timeCurve->onKeysChanged);
        ListenKeysChanged(spline->onKeysChanged);
    }

    ParticlesSplineEffect::~ParticlesSplineEffect()
    {
        if (timeCurve)
            UnlistenKeysChanged(timeCurve->onKeysChanged);

        if (spline)
            UnlistenKeysChanged(spline->onKeysChanged);
    }

    void ParticlesSplineEffect::OnParticleEmitted(Particle& particle)
    {
        int particleIndex = particle.index;

        CheckDataBufferSize(particleIndex);

        mData[particleIndex].initialPosition = particle.position;

        mData[particleIndex].timeCacheKey = 0;
        mData[particleIndex].timeCacheKeyApprox = 0;
        mData[particleIndex].timeRandomCoef = Math::Random(0.0f, 1.0f);

        mData[particleIndex].splineCacheKey = 0;
        mData[particleIndex].splineCacheKeyApprox = 0;
        mData[particleIndex].splineRandomCoef = Math::Random(0.0f, 1.0f);
    }

    void ParticlesSplineEffect::Update(float dt, ParticlesEmitter* emitter)
    {
        auto& particles = GetParticlesDirect(emitter);

        CheckDataBufferSize(particles.Count());

        float splineLength = spline->Length();

        for (auto& p : particles)
        {
            int particleIndex = p.index;

            auto& data = mData[particleIndex];
            float t = timeCurve->Evaluate(1.0f - p.timeLeft/p.lifetime, data.timeRandomCoef, true, data.timeCacheKey, data.timeCacheKeyApprox);

            Vec2F splineOffset = spline->Evaluate(t*splineLength, data.splineRandomCoef, true,
                                                  data.splineCacheKey, data.splineCacheKeyApprox);
            p.position = data.initialPosition + Vec3F(splineOffset.x, splineOffset.y, 0.0f);
        }
    }

    void ParticlesSplineEffect::CheckDataBufferSize(int particlesCount)
    {
        for (int i = mData.size(); i <= particlesCount; i++)
            mData.Add(ParticleData());
    }

    void ParticlesSplineEffect::OnDeserialized(const DataValue& node)
    {
        ListenKeysChanged(timeCurve->onKeysChanged);
        ListenKeysChanged(spline->onKeysChanged);
    }
}
// --- META ---

DECLARE_CLASS(o2::ParticlesEffect, o2__ParticlesEffect);

DECLARE_CLASS(o2::ParticlesGravityEffect, o2__ParticlesGravityEffect);

DECLARE_CLASS(o2::ParticlesDampingEffect, o2__ParticlesDampingEffect);

DECLARE_CLASS(o2::ParticlesColorEffect, o2__ParticlesColorEffect);

DECLARE_CLASS(o2::ParticlesRandomColorEffect, o2__ParticlesRandomColorEffect);

DECLARE_CLASS(o2::ParticlesSizeEffect, o2__ParticlesSizeEffect);

DECLARE_CLASS(o2::ParticlesAngleEffect, o2__ParticlesAngleEffect);

DECLARE_CLASS(o2::ParticlesAngleSpeedEffect, o2__ParticlesAngleSpeedEffect);

DECLARE_CLASS(o2::ParticlesVelocityEffect, o2__ParticlesVelocityEffect);

DECLARE_CLASS(o2::ParticlesSplineEffect, o2__ParticlesSplineEffect);
// --- END META ---
