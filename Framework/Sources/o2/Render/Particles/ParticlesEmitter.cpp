#include "o2/stdafx.h"
#include "ParticlesEmitter.h"

#include "o2/Render/Mesh.h"
#include "o2/Render/Particles/ParticlesEffects.h"
#include "o2/Render/Particles/ParticlesEmitterShapes.h"

namespace o2
{
    ParticlesEmitter::ParticlesEmitter() :
        IRectDrawable(), mShape(mmake<CircleParticlesEmitterShape>())
    {
        mLastTransform = mTransform;
        UpdateDuration();
    }

    ParticlesEmitter::~ParticlesEmitter()
    {}

    ParticlesEmitter::ParticlesEmitter(const ParticlesEmitter& other) :
        IRectDrawable(other), IAnimation(other), mParticlesSource(other.mParticlesSource->CloneAsRef<ParticleSource>()),
        mShape(other.mShape->CloneAsRef<ParticlesEmitterShape>()),
        mParticlesNumLimit(other.mParticlesNumLimit), mEmitParticlesFromShell(other.mEmitParticlesFromShell),
        mEmittingCoefficient(other.mEmittingCoefficient), mIsParticlesRelative(other.mIsParticlesRelative),
        mParticlesLifetime(other.mParticlesLifetime), mEmitParticlesPerSecond(other.mEmitParticlesPerSecond),
        mInitialAngle(other.mInitialAngle), mInitialAngleRange(other.mInitialAngleRange),
        mInitialSize(other.mInitialSize), mInitialSizeRange(other.mInitialSizeRange),
        mInitialSpeed(other.mInitialSpeed), mInitialSpeedRangle(other.mInitialSpeedRangle),
        mInitialMoveDirection(other.mInitialMoveDirection), mInitialMoveDirectionRange(other.mInitialMoveDirectionRange),
        emittingCoefficient(this), particlesRelative(this), maxParticles(this),
        particlesLifetime(this), particlesPerSecond(this), initialAngle(this), initialAngleRange(this),
        initialSize(this), initialSizeRange(this), initialSpeed(this), initialSpeedRange(this), initialAngleSpeed(this),
        initialAngleSpeedRange(this), moveDirection(this), moveDirectionRange(this),
        particlesSource(this), shape(this)
    {
        for (auto& effect : other.mEffects)
            AddEffect(effect->CloneAsRef<ParticlesEffect>());

		mLastTransform = mTransform;
		UpdateDuration();
    }

    ParticlesEmitter& ParticlesEmitter::operator=(const ParticlesEmitter& other)
    {
        RemoveAllEffects();
        mShape = nullptr;

        int idx = 0;
        for (auto& particle : mParticles)
        {
            if (particle.alive)
            {
                mDeadParticles.Add(idx);
                particle.alive = false;
            }

            idx++;
        }
        mNumAliveParticles = 0;

        IRectDrawable::operator=(other);
        IAnimation::operator=(other);

        mParticlesSource = other.mParticlesSource;
        CreateParticlesContainer();

        mShape = other.mShape->CloneAsRef<ParticlesEmitterShape>();

        for (auto& effect : other.mEffects)
            AddEffect(effect->CloneAsRef<ParticlesEffect>());

        mEmitParticlesFromShell = other.mEmitParticlesFromShell;

        mParticlesNumLimit = other.mParticlesNumLimit;

        mEmittingCoefficient = other.mEmittingCoefficient;
        mIsParticlesRelative = other.mIsParticlesRelative;

		UpdateDuration();

        mEmissionDuration = other.mEmissionDuration;
        mParticlesLifetime = other.mParticlesLifetime;
        mEmitParticlesPerSecond = other.mEmitParticlesPerSecond;

        mInitialAngle = other.mInitialAngle;
        mInitialAngleRange = other.mInitialAngleRange;

        mInitialSize = other.mInitialSize;
        mInitialSizeRange = other.mInitialSizeRange;

        mInitialSpeed = other.mInitialSpeed;
        mInitialSpeedRangle = other.mInitialSpeedRangle;

        mInitialMoveDirection = other.mInitialMoveDirection;
        mInitialMoveDirectionRange = other.mInitialMoveDirectionRange;

        mLastTransform = mTransform;

        OnChanged();

        return *this;
    }

    void ParticlesEmitter::Draw()
    {
        if (mParticlesContainer)
            mParticlesContainer->Draw();
    }

    void ParticlesEmitter::Update(float dt)
    {
        if (!mEnabled)
            return;

#if IS_EDITOR
        mIsUpdating = true;

        if (mRandomSeed == 0)
        {
            mRandomSeed = ::time(0);
            srand(mRandomSeed);
        }
#endif

        if (!mParticlesContainer)
            CreateParticlesContainer();

        if (!mSubControlled)
        {
#if IS_EDITOR
            if (!mParticlesPaused)
#endif
            {
                float prewardDt = 1.0f / 30.0f;
                while (mPrewarmTimeout > 0.0f)
                {
                    UpdateEmitting(prewardDt);
                    UpdateEffects(prewardDt);
                    UpdateParticles(prewardDt);
                    mPrewarmTimeout -= prewardDt;
                }

                UpdateEmitting(dt);
                UpdateEffects(dt);
                UpdateParticles(dt);
            }
        }

        IAnimation::Update(dt);

        mParticlesContainer->Update(mParticles, mParticlesNumLimit);

#if IS_EDITOR
        mIsUpdating = false;
#endif
    }

    void ParticlesEmitter::BlendModeChanged()
    {
        if (mParticlesContainer)
            mParticlesContainer->SetBlendMode(mBlendMode);
    }

    void ParticlesEmitter::OnSerialize(o2::DataValue& node) const
    {
        IRectDrawable::OnSerialize(node);
        IAnimation::OnSerialize(node);
    }

    void ParticlesEmitter::OnDeserialized(const DataValue& node)
    {
        CreateParticlesContainer();

        IRectDrawable::OnDeserialized(node);
        IAnimation::OnDeserialized(node);

		UpdateDuration();

        OnEffectsListChanged();
        OnChanged();
    }

    void ParticlesEmitter::OnSerializeDelta(DataValue& node, const IObject& origin) const
    {
        IRectDrawable::OnSerializeDelta(node, origin);
        IAnimation::OnSerializeDelta(node, origin);
    }

    void ParticlesEmitter::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        CreateParticlesContainer();
        IRectDrawable::OnDeserializedDelta(node, origin);
        IAnimation::OnDeserializedDelta(node, origin);
        OnEffectsListChanged();
        OnChanged();
    }

	void ParticlesEmitter::UpdateDuration()
	{
        if (mLoop == Loop::None)
            mDuration = mEmissionDuration + mParticlesLifetime;
        else if (mLoop == Loop::Repeat)
            mDuration = mEmissionDuration;

        ResetBounds();
	}

	void ParticlesEmitter::CreateParticlesContainer()
    {
        mParticlesContainer = mParticlesSource->CreateContainer();
        mParticlesContainer->emitter = this;
    }

    void ParticlesEmitter::UpdateEmitting(float dt)
    {
        if (!mPlaying)
            return;

        if (mLoop == Loop::None && mTime > mEmissionDuration)
            return;

        mEmitTimeBuffer += dt;

        float currentParticlesPerSecond = mEmitParticlesPerSecond*mEmittingCoefficient;
        float particlesDelay = 1.0f/currentParticlesPerSecond;

        float initialAngle = Math::Deg2rad(mInitialAngle);
        float halfAngleRange = Math::Deg2rad(mInitialAngleRange)*0.5f;

        float halfSizeRange = mInitialSizeRange*0.5f;
        float halfWidthScaleRange = mInitialWidthScaleRange*0.5f;

        float halfSpeedRange = mInitialSpeedRangle*0.5f;

        float initialMoveDirection = Math::Deg2rad(mInitialMoveDirection);
        float halfDirRange = Math::Deg2rad(mInitialMoveDirectionRange)*0.5f;

        float initialAngleSpeed = Math::Deg2rad(mInitialAngleSpeed);
        float halfAngleSpeedRange = Math::Deg2rad(mInitialAngleSpeedRange)*0.5f;

        float halfLifetimeRange = mParticlesLifetimeRange * 0.5f;

        while (mEmitTimeBuffer > particlesDelay)
        {
            if (mNumAliveParticles < mParticlesNumLimit)
            {
                Particle* particle;
                int particleIndex;

                // Allocate particle
                if (mDeadParticles.IsEmpty())
                {
                    mParticles.Add(Particle());
                    particle = &mParticles.Last();
                    particleIndex = mParticles.Count() - 1;
                }
                else
                {
                    particleIndex = mDeadParticles.PopBack();
                    particle = &mParticles[particleIndex];
                }

                // Initialize particle
                particle->index = particleIndex;

                particle->position = mShape->GetEmittinPoint(mTransform, mEmitParticlesFromShell);
                particle->angle = initialAngle + Math::Random(-halfAngleRange, halfAngleRange);

                float randomSize = mInitialSize + Math::Random(-halfSizeRange, halfSizeRange);
                float randomWidthScale = mInitialWidthScale + Math::Random(-halfWidthScaleRange, halfWidthScaleRange);
                particle->size = Vec2F(randomSize, randomSize*randomWidthScale);

                particle->velocity = Vec2F::Rotated(initialMoveDirection + Math::Random(-halfDirRange, halfDirRange))*
                    (mInitialSpeed + Math::Random(-halfSpeedRange, halfSpeedRange));

                particle->angleSpeed = initialAngleSpeed + Math::Random(-halfAngleSpeedRange, halfAngleSpeedRange);

                particle->color = Color4::White();

                particle->timeLeft = mParticlesLifetime + Math::Random(-halfLifetimeRange, halfLifetimeRange);
                particle->lifetime = particle->timeLeft;

                particle->alive = true;

                // Notify container and effects
                mParticlesContainer->OnParticleEmitted(*particle);

                for (auto& effect : mEffects)
                {
                    if (effect)
                        effect->OnParticleEmitted(*particle);
                }

                mNumAliveParticles++;
            }

            mEmitTimeBuffer -= particlesDelay;
        }
    }

    void ParticlesEmitter::UpdateEffects(float dt)
    {
        for (auto& effect : mEffects)
        {
            if (effect)
                effect->Update(dt, this);
        }
    }

    void ParticlesEmitter::UpdateParticles(float dt)
    {
        int idx = 0;
        for (auto& particle : mParticles)
        {
            if (!particle.alive)
            {
                idx++;
                continue;
            }

            particle.position += particle.velocity*dt;
            particle.angle += particle.angleSpeed*dt;

            particle.timeLeft -= dt;

            if (particle.timeLeft < 0)
            {
                particle.alive = false;

                mParticlesContainer->OnParticleDied(particle);

                for (auto& effect : mEffects)
                {
                    if (effect)
                        effect->OnParticleDied(particle);
                }

                mDeadParticles.Add(idx);
                mNumAliveParticles--;
            }

            idx++;
        }
    }

    void ParticlesEmitter::BasisChanged()
    {
        if (!mIsParticlesRelative)
            return;

        Basis change = mLastTransform.Inverted()*mTransform;
        for (auto& particle : mParticles)
            particle.position = change.Transform(particle.position);

        mLastTransform = mTransform;
    }

    void ParticlesEmitter::Play()
    {
        IAnimation::Play();

        mEmitTimeBuffer = 0.0f;
        mPrewarmTimeout = mPrewarmTime;
        mBakedFrames.Clear();
    }

    void ParticlesEmitter::Stop()
    {
        IAnimation::Stop();
    }

    void ParticlesEmitter::SetDuration(float duration)
    {
        if (mLoop == Loop::None)
            mParticlesLifetime = duration - mEmissionDuration;
        else if (mLoop == Loop::Repeat)
			mParticlesLifetime = duration;

		UpdateDuration();

        OnChanged();
    }

    float ParticlesEmitter::GetDuration() const
    {
        return mEmissionDuration + mParticlesLifetime;
    }

    void ParticlesEmitter::SetTime(float time)
    {
        mTime = time;
        UpdateTime();
    }

    void ParticlesEmitter::SetParticlesSource(const Ref<ParticleSource>& source)
    {
        mParticlesSource = source;
        CreateParticlesContainer();
        OnChanged();
    }

    const Ref<ParticleSource>& ParticlesEmitter::GetParticlesSource() const
    {
        return mParticlesSource;
    }

    void ParticlesEmitter::SetEmittingCoef(float coef)
    {
        mEmittingCoefficient = coef;
        OnChanged();
    }

    float ParticlesEmitter::GetEmittingCoef() const
    {
        return mEmittingCoefficient;
    }

    void ParticlesEmitter::SetShape(const Ref<ParticlesEmitterShape>& shape)
    {
        mShape = shape;
        OnChanged();
    }

    const Ref<ParticlesEmitterShape>& ParticlesEmitter::GetShape() const
    {
        return mShape;
    }

    void ParticlesEmitter::AddEffect(const Ref<ParticlesEffect>& effect)
    {
        mEffects.Add(effect);
        OnChanged();
    }

    const Vector<Ref<ParticlesEffect>>& ParticlesEmitter::GetEffects() const
    {
        return mEffects;
    }

    void ParticlesEmitter::RemoveEffect(const Ref<ParticlesEffect>& effect)
    {
        mEffects.Remove(effect);
        OnChanged();
    }

    void ParticlesEmitter::RemoveAllEffects()
    {
        mEffects.Clear();
        OnChanged();
    }

    void ParticlesEmitter::SetMaxParticles(int count)
    {
        mParticlesNumLimit = count;

        int idx = 0;
        while (mNumAliveParticles > mParticlesNumLimit)
        {
            if (mParticles[idx].alive)
            {
                mParticles[idx].alive = false;
                mDeadParticles.Add(idx);
                mNumAliveParticles--;
            }

            idx++;
        }

        OnChanged();
    }

    int ParticlesEmitter::GetMaxParticles() const
    {
        return mParticlesNumLimit;
    }

    int ParticlesEmitter::GetParticlesCount() const
    {
        return mNumAliveParticles;
    }

    bool ParticlesEmitter::IsAliveParticles() const
    {
        return mNumAliveParticles > 0;
    }

    const Vector<Particle>& ParticlesEmitter::GetParticles() const
    {
        return mParticles;
    }

    void ParticlesEmitter::SetParticlesRelativity(bool relative)
    {
        mIsParticlesRelative = relative;
        OnChanged();
    }

    bool ParticlesEmitter::IsParticlesRelative() const
    {
        return mIsParticlesRelative;
    }

    void ParticlesEmitter::SetParticlesEmitFromShell(bool fromShell)
    {
        mEmitParticlesFromShell = fromShell;
        OnChanged();
    }

    bool ParticlesEmitter::IsParticlesEmitFromShell() const
    {
        return mEmitParticlesFromShell;
    }

    void ParticlesEmitter::SetEmissionDuration(float duration)
    {
		mEmissionDuration = duration;
		UpdateDuration();
        OnChanged();
    }

    float ParticlesEmitter::GetEmissionDuration() const
    {
        return mEmissionDuration;
    }

    void ParticlesEmitter::SetParticlesLifetime(float lifetime)
    {
		mParticlesLifetime = lifetime;
		UpdateDuration();
        OnChanged();
    }

    float ParticlesEmitter::GetParticlesLifetime() const
    {
        return mParticlesLifetime;
    }

    void ParticlesEmitter::SetParticlesPerSecond(float numParticles)
    {
        mEmitParticlesPerSecond = numParticles;
        OnChanged();
    }

    float ParticlesEmitter::GetParticlesPerSecond() const
    {
        return mEmitParticlesPerSecond;
    }

    void ParticlesEmitter::SetPrewarmTime(float time)
    {
        mPrewarmTime = time;
        OnChanged();
    }

    float ParticlesEmitter::GetPrewarmTime() const
    {
        return mPrewarmTime;
    }

    void ParticlesEmitter::SetInitialAngle(float angle)
    {
        mInitialAngle = angle;
        OnChanged();
    }

    float ParticlesEmitter::GetInitialAngle() const
    {
        return mInitialAngle;
    }

    void ParticlesEmitter::SetInitialAngleRange(float range)
    {
        mInitialAngleRange = range;
        OnChanged();
    }

    float ParticlesEmitter::GetInitialAngleRange() const
    {
        return mInitialAngleRange;
    }

    void ParticlesEmitter::SetInitialSize(float size)
    {
        mInitialSize = size;
        OnChanged();
    }

    float ParticlesEmitter::GetInitialSize() const
    {
        return mInitialSize;
    }

    void ParticlesEmitter::SetInitialSizeRange(float range)
    {
        mInitialSizeRange = range;
        OnChanged();
    }

    float ParticlesEmitter::GetInitialSizeRange() const
    {
        return mInitialSizeRange;
    }

    void ParticlesEmitter::SetInitialWidthScale(float scale)
    {
        mInitialWidthScale = scale;
        OnChanged();
    }

    float ParticlesEmitter::GetInitialWidthScale() const
    {
        return mInitialWidthScale;
    }

    void ParticlesEmitter::SetInitialWidthScaleRange(float scaleRange)
    {
        mInitialWidthScaleRange = scaleRange;
        OnChanged();
    }

    float ParticlesEmitter::GetInitialWidthScaleRange() const
    {
        return mInitialWidthScaleRange;
    }

    void ParticlesEmitter::SetInitialAngleSpeed(float speed)
    {
        mInitialAngleSpeed = speed;
        OnChanged();
    }

    float ParticlesEmitter::GetInitialAngleSpeed() const
    {
        return mInitialAngleSpeed;
    }

    void ParticlesEmitter::SetInitialAngleSpeedRange(float speedRange)
    {
        mInitialAngleSpeedRange = speedRange;
        OnChanged();
    }

    float ParticlesEmitter::GetInitialAngleSpeedRange() const
    {
        return mInitialAngleSpeedRange;
    }

    void ParticlesEmitter::SetInitialSpeed(float speed)
    {
        mInitialSpeed = speed;
        OnChanged();
    }

    float ParticlesEmitter::GetInitialSpeed() const
    {
        return mInitialSpeed;
    }

    void ParticlesEmitter::SetInitialSpeedRange(float speedRange)
    {
        mInitialSpeedRangle = speedRange;
        OnChanged();
    }

    float ParticlesEmitter::GetInitialSpeedRange() const
    {
        return mInitialSpeedRangle;
    }

    void ParticlesEmitter::SetEmitParticlesMoveDirection(float direction)
    {
        mInitialMoveDirection = direction;
        OnChanged();
    }

    float ParticlesEmitter::GetEmitParticlesMoveDirection() const
    {
        return mInitialMoveDirection;
    }

    void ParticlesEmitter::SetEmitParticlesMoveDirectionRange(float directionRange)
    {
        mInitialMoveDirectionRange = directionRange;
        OnChanged();
    }

    float ParticlesEmitter::GetEmitParticlesMoveDirectionRange() const
    {
        return mInitialMoveDirectionRange;
    }

    Ref<RefCounterable> ParticlesEmitter::CastToRefCounterable(const Ref<ParticlesEmitter>& ref)
    {
        return DynamicCast<IAnimation>(ref);
    }

    void ParticlesEmitter::OnEffectsListChanged()
    {
        for (auto& effect : mEffects)
        {
            if (effect)
                effect->mEmitter = Ref(this);
        }

        if (mShape)
            mShape->mEmitter = Ref(this);
    }

    void ParticlesEmitter::OnChanged()
    {
#if IS_EDITOR
        InvalidateBakedFrames();
#endif
    }

#if IS_EDITOR
    int ParticlesEmitter::mBakedFPS = 60;

    void ParticlesEmitter::SetParticlesPause(bool paused)
    {
        mParticlesPaused = paused;
    }

    void ParticlesEmitter::InvalidateBakedFrames()
    {
        mBakedFrames.Clear();

        if (!mPlaying)
        {
            srand(mRandomSeed);

            if (mLoop == Loop::Repeat && GetRelativeTime() > 1.0f)
                mTime = Math::Mod(mTime, GetDuration());

            RestoreBakedFrame(GetBakedFrameIndex(mTime));
        }
    }

    void ParticlesEmitter::Evaluate()
    {
        if (mIsUpdating)
        {
            int frameIdx = GetBakedFrameIndex(mTime);

            if (mBakedFrames.Count() <= frameIdx)
                mBakedFrames.Resize(frameIdx + 1);

            mBakedFrames[frameIdx].particles = mParticles;
            mBakedFrames[frameIdx].deadParticles = mDeadParticles;
            mBakedFrames[frameIdx].numAliveParticles = mNumAliveParticles;
            mBakedFrames[frameIdx].emitTimeBuffer = mEmitTimeBuffer;

            //o2Debug.Log("Baked frame %i with %i particles, time: %f", frameIdx, mParticles.Count(), mTime);
        }
        else
        {
            RestoreBakedFrame(GetBakedFrameIndex(mTime));
        }
    }

    int ParticlesEmitter::GetBakedFrameIndex(float time) const
    {
        return Math::RoundToInt(time*mBakedFPS);
    }

    void ParticlesEmitter::CheckBakedFrames(int maxFrameIdx)
    {
        //o2Debug.Log("Baked frames count: %i, max frame index: %i", mBakedFrames.Count(), maxFrameIdx);

        if (mBakedFrames.Count() > maxFrameIdx || maxFrameIdx < 1)
            return;

        //o2Debug.Log("Baked frames count: %i, max frame index: %i", mBakedFrames.Count(), maxFrameIdx);

        int startIdx = Math::Max(0, mBakedFrames.Count() - 1);
        mBakedFrames.Resize(maxFrameIdx + 1);

        float prevTime = mTime;
        mTime = (float)startIdx/(float)mBakedFPS;

        // Reset particles to previous state
        auto prevParticles = mParticles;
        auto prevDeadParticles = mDeadParticles;
        auto prevNumAliveParticles = mNumAliveParticles;
        auto prevEmitTimeBuffer = mEmitTimeBuffer;
        auto prevSubControlled = mSubControlled;

        if (startIdx >= 0)
        {
            mParticles = mBakedFrames[startIdx].particles;
            mDeadParticles = mBakedFrames[startIdx].deadParticles;
            mNumAliveParticles = mBakedFrames[startIdx].numAliveParticles;
            mEmitTimeBuffer = mBakedFrames[startIdx].emitTimeBuffer;

//             o2Debug.Log("Setup particles: %i", mNumAliveParticles);
//             for (auto& p : mParticles)
//                 o2Debug.Log("   Particle: %i, %f, %f", p.index, p.timeLeft, p.lifetime);
        }
        else
        {
            mParticles.Clear();
            mDeadParticles.Clear();
            mNumAliveParticles = 0;
            mEmitTimeBuffer = 0.0f;
        }

        //o2Debug.Log("Start baked frame index: %i, time: %f", startIdx, mTime);

        // Prepare to update particles
        bool prevPlaying = mPlaying;
        mPlaying = true;

        bool prevPaused = mParticlesPaused;
        mParticlesPaused = false;

        if (startIdx == 0)
            mPrewarmTimeout = mPrewarmTime;

        mSubControlled = false;

        // Update and bake frames
        for (int i = startIdx; i <= maxFrameIdx; i++)
        {
            //o2Debug.Log("To bake frame: %i", i);
            Update(1.0f/(float)mBakedFPS);
        }

        // Restore previous state
        mTime = prevTime;
        mPlaying = prevPlaying;
        mParticlesPaused = prevPaused;
        mParticles = prevParticles;
        mDeadParticles = prevDeadParticles;
        mNumAliveParticles = prevNumAliveParticles;
        mEmitTimeBuffer = prevEmitTimeBuffer;
        mSubControlled = prevSubControlled;
    }

    void ParticlesEmitter::RestoreBakedFrame(int frameIdx)
    {
        //o2Debug.Log("\n------------------------\nRestore baked frame %i", frameIdx);

        CheckBakedFrames(frameIdx);

        if (frameIdx == 0)
        {
            mParticles.Clear();
            mDeadParticles.Clear();
            mNumAliveParticles = 0;
            mEmitTimeBuffer = 0.0f;
        }
        else
        {
            mParticles = mBakedFrames[frameIdx].particles;
            mDeadParticles = mBakedFrames[frameIdx].deadParticles;
            mNumAliveParticles = mBakedFrames[frameIdx].numAliveParticles;
            mEmitTimeBuffer = mBakedFrames[frameIdx].emitTimeBuffer;
        }
    }

#endif
}
// --- META ---

DECLARE_CLASS(o2::ParticlesEmitter, o2__ParticlesEmitter);
// --- END META ---
