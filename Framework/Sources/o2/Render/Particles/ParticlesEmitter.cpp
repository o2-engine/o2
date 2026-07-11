#include "o2/stdafx.h"
#include "ParticlesEmitter.h"

#include "o2/Render/Mesh.h"
#include "o2/Render/Particles/ParticlesEffects.h"
#include "o2/Render/Particles/ParticlesEmitterShapes.h"

namespace o2
{
    namespace
    {
        // Returns uniformly distributed random direction inside a cone around the axis
        Vec3F RandomDirectionInCone(const Vec3F& axis, float halfAngle)
        {
            float cosHalfAngle = Math::Cos(halfAngle);
            float z = Math::Random(cosHalfAngle, 1.0f);
            float around = Math::Random(0.0f, Math::PI()*2.0f);
            float planarRadius = Math::Sqrt(Math::Max(0.0f, 1.0f - z*z));
            Vec3F local(planarRadius*Math::Cos(around), planarRadius*Math::Sin(around), z);

            Vec3F axisNorm = axis.Normalized();
            Vec3F ortho = Math::Abs(axisNorm.z) < 0.99f ? Vec3F(0, 0, 1) : Vec3F(1, 0, 0);
            Vec3F right = ortho.Cross(axisNorm).Normalized();
            Vec3F up = axisNorm.Cross(right);

            return right*local.x + up*local.y + axisNorm*local.z;
        }
    }

    ParticlesEmitter::ParticlesEmitter() :
        IRectDrawable(), mShape(mmake<CircleParticlesEmitterShape>())
    {
        mLastTransform = mTransform.ToBasis();
        UpdateDuration();
    }

    ParticlesEmitter::~ParticlesEmitter()
    {}

    ParticlesEmitter::ParticlesEmitter(const ParticlesEmitter& other) :
        IRectDrawable(other), IAnimation(other), mParticlesSource(other.mParticlesSource->CloneAsRef<ParticleSource>()),
        mShape(other.mShape->CloneAsRef<ParticlesEmitterShape>()),
        mParticlesNumLimit(other.mParticlesNumLimit), mEmitParticlesFromShell(other.mEmitParticlesFromShell),
        mEmittingCoefficient(other.mEmittingCoefficient), mIsParticlesRelative(other.mIsParticlesRelative),
        mIs3D(other.mIs3D),
        mParticlesLifetime(other.mParticlesLifetime), mEmitParticlesPerSecond(other.mEmitParticlesPerSecond),
        mInitialAngle(other.mInitialAngle), mInitialAngleRange(other.mInitialAngleRange),
        mInitialSize(other.mInitialSize), mInitialSizeRange(other.mInitialSizeRange),
        mInitialSpeed(other.mInitialSpeed), mInitialSpeedRangle(other.mInitialSpeedRangle),
        mInitialMoveDirection(other.mInitialMoveDirection), mInitialMoveDirectionRange(other.mInitialMoveDirectionRange),
        mInitialMoveDirection3D(other.mInitialMoveDirection3D)
    {
        for (auto& effect : other.mEffects)
            AddEffect(effect->CloneAsRef<ParticlesEffect>());

		mLastTransform = mTransform.ToBasis();
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
        mIs3D = other.mIs3D;

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
        mInitialMoveDirection3D = other.mInitialMoveDirection3D;

        mLastTransform = mTransform.ToBasis();

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

    void ParticlesEmitter::OnMaterialChanged()
    {
        if (mParticlesContainer)
            mParticlesContainer->SetMaterial(GetMaterial());
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

                Basis3D emissionBasis;
                if (mIs3D)
                    emissionBasis = mEmission3DBasis;
                else
                    emissionBasis = Basis3D(mTransform.ToBasis());

                particle->position = mShape->GetEmittinPoint(emissionBasis, mEmitParticlesFromShell);
                particle->angle = initialAngle + Math::Random(-halfAngleRange, halfAngleRange);

                float randomSize = mInitialSize + Math::Random(-halfSizeRange, halfSizeRange);
                float randomWidthScale = mInitialWidthScale + Math::Random(-halfWidthScaleRange, halfWidthScaleRange);
                particle->size = Vec2F(randomSize, randomSize*randomWidthScale);

                float randomSpeed = mInitialSpeed + Math::Random(-halfSpeedRange, halfSpeedRange);
                if (mIs3D)
                    particle->velocity = RandomDirectionInCone(mInitialMoveDirection3D, halfDirRange)*randomSpeed;
                else
                {
                    Vec2F direction = Vec2F::Rotated(initialMoveDirection + Math::Random(-halfDirRange, halfDirRange));
                    particle->velocity = Vec3F(direction.x, direction.y, 0.0f)*randomSpeed;
                }

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
        if (!mIsParticlesRelative || mIs3D)
            return;

        Basis change = mLastTransform.Inverted()*mTransform.ToBasis();
        for (auto& particle : mParticles)
        {
            Vec2F position = change.Transform(Vec2F(particle.position.x, particle.position.y));
            particle.position = Vec3F(position.x, position.y, particle.position.z);
        }

        mLastTransform = mTransform.ToBasis();
    }

    void ParticlesEmitter::Play()
    {
        IAnimation::Play();

        mEmitTimeBuffer = 0.0f;
        mPrewarmTimeout = mPrewarmTime;
#if IS_EDITOR
        mBakedFrames.Clear();
#endif
    }

    void ParticlesEmitter::Stop()
    {
        IAnimation::Stop();
    }

    void ParticlesEmitter::SetDuration(float duration)
    {
        mParticlesLifetime = duration - mEmissionDuration;

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

    void ParticlesEmitter::SetIs3D(bool is3D)
    {
        mIs3D = is3D;
        OnChanged();
    }

    bool ParticlesEmitter::Is3D() const
    {
        return mIs3D;
    }

    void ParticlesEmitter::Set3DBasis(const Basis3D& basis)
    {
        if (basis == mLast3DBasis)
            return;

        if (mIs3D && mIsParticlesRelative)
        {
            // A degenerate basis has no inverse, so the relative change is undefined
            // and particles keep their world positions
            float det = mLast3DBasis.xv.Dot(mLast3DBasis.yv.Cross(mLast3DBasis.zv));
            if (Math::Abs(det) > FLT_EPSILON)
            {
                Basis3D change = mLast3DBasis.Inverted()*basis;
                for (auto& particle : mParticles)
                    particle.position = change.Transform(particle.position);
            }
        }

        mEmission3DBasis = basis;
        mLast3DBasis = basis;
    }

    const Basis3D& ParticlesEmitter::Get3DBasis() const
    {
        return mEmission3DBasis;
    }

    bool ParticlesEmitter::GetParticlesBounds(o2::AABB& bounds) const
    {
        bool hasAlive = false;
        Vec3F boundsMin(FLT_MAX, FLT_MAX, FLT_MAX);
        Vec3F boundsMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (auto& particle : mParticles)
        {
            if (!particle.alive)
                continue;

            float halfSize = Math::Max(particle.size.x, particle.size.y)*0.5f;
            boundsMin.x = Math::Min(boundsMin.x, particle.position.x - halfSize);
            boundsMin.y = Math::Min(boundsMin.y, particle.position.y - halfSize);
            boundsMin.z = Math::Min(boundsMin.z, particle.position.z - halfSize);
            boundsMax.x = Math::Max(boundsMax.x, particle.position.x + halfSize);
            boundsMax.y = Math::Max(boundsMax.y, particle.position.y + halfSize);
            boundsMax.z = Math::Max(boundsMax.z, particle.position.z + halfSize);

            hasAlive = true;
        }

        if (hasAlive)
            bounds = o2::AABB(boundsMin, boundsMax);

        return hasAlive;
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

    void ParticlesEmitter::SetEmitParticlesMoveDirection3D(const Vec3F& direction)
    {
        mInitialMoveDirection3D = direction;
        OnChanged();
    }

    const Vec3F& ParticlesEmitter::GetEmitParticlesMoveDirection3D() const
    {
        return mInitialMoveDirection3D;
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
