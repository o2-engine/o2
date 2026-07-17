#pragma once

#include "o2/Render/Particles/Particle.h"
#include "o2/Utils/Math/ColorGradient.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    class ParticlesEmitter;

    // -------------------------------
    // Particles effect base interface
    // -------------------------------
    class ParticlesEffect: public ISerializable, public RefCounterable, public ICloneableRef
    {
    public:
        // Called when particle is emitted, used to initialize effect data
        virtual void OnParticleEmitted(Particle& particle) {}

        // Called when particle is died, used to cleanup effect data
        virtual void OnParticleDied(Particle& particle) {}

        // Called each frame to update effect data
        virtual void Update(float dt, ParticlesEmitter* emitter);

        // Get particles directly from emitter
        Vector<Particle>& GetParticlesDirect(ParticlesEmitter* emitter);

        // Returns owning emitter    
        Ref<ParticlesEmitter> GetEmitter() const;

        SERIALIZABLE(ParticlesEffect);
        CLONEABLE_REF(ParticlesEffect);

    protected:
        WeakRef<ParticlesEmitter> mEmitter; // Owning emitter

    protected:
        // Called  when particle effect parameters are changed, used to invalidate baked frames
        void OnChanged();

        friend class ParticlesEmitter;
    };

    // ------------------------
    // Particles gravity effect
    // ------------------------
    class ParticlesGravityEffect : public ParticlesEffect
    {
    public:
        PROPERTIES(ParticlesGravityEffect);
        PROPERTY(Vec3F, gravity, SetGravity, GetGravity);

    public:
        // Default constructor @SCRIPTABLE
        ParticlesGravityEffect() {}

        // Set gravity vector; z is used by 3D emitters @SCRIPTABLE
        void SetGravity(const Vec3F& gravity) { mGravity = gravity; OnChanged(); }

        // Get gravity vector
        const Vec3F& GetGravity() const { return mGravity; }

        // Update particles velocity with gravity vector
        void Update(float dt, ParticlesEmitter* emitter) override;

        SERIALIZABLE(ParticlesGravityEffect);
        CLONEABLE_REF(ParticlesGravityEffect);

    protected:
        Vec3F mGravity; // Vector of gravity @SERIALIZABLE
    };

    // --------------------------------
    // Particles color over time effect
    // --------------------------------
    class ParticlesColorEffect : public ParticlesEffect
    {
    public:
        Ref<ColorGradient> colorGradient; // Color gradient @SERIALIZABLE @SCRIPTABLE

    public:
        // Default constructor @SCRIPTABLE
        ParticlesColorEffect();

        // Called when particle is emitted, used to initialize effect data
        void OnParticleEmitted(Particle& particle) override;

        // Update particles color over time
        void Update(float dt, ParticlesEmitter* emitter) override;

        SERIALIZABLE(ParticlesColorEffect);
        CLONEABLE_REF(ParticlesColorEffect);

    private:
        struct ParticleColorData
        {
            int cacheKey = 0;

            bool operator==(const ParticleColorData& other) const
            {
                return cacheKey == other.cacheKey;
            }
        };

        Vector<ParticleColorData> mColorData; // Color data buffer

    private:
        // Check if color data buffer size is enough
        void CheckDataBufferSize(int particlesCount);

        // Called when deserialization is done, used to subscribe to color gradient changes
        void OnDeserialized(const DataValue& node) override;
    };

    // ---------------------------------------------------
    // Particles random color between two over time effect
    // ---------------------------------------------------
    class ParticlesRandomColorEffect : public ParticlesEffect
    {
    public:
        Ref<ColorGradient> colorGradientA; // Color gradient A @SERIALIZABLE
        Ref<ColorGradient> colorGradientB; // Color gradient B @SERIALIZABLE

    public:
        // Default constructor
        ParticlesRandomColorEffect();

        // Called when particle is emitted, used to initialize effect data
        void OnParticleEmitted(Particle& particle) override;

        // Update particles color between two gradients over time
        void Update(float dt, ParticlesEmitter* emitter) override;

        SERIALIZABLE(ParticlesRandomColorEffect);
        CLONEABLE_REF(ParticlesRandomColorEffect);

    private:
        struct ParticleColorData
        {
            int cacheKeyA = 0;
            int cacheKeyB = 0;

            float coef = 0.0f;

            bool operator==(const ParticleColorData& other) const
            {
                return cacheKeyA == other.cacheKeyA && cacheKeyB == other.cacheKeyB && coef == other.coef;
            }
        };

        Vector<ParticleColorData> mColorData; // Color data buffer

    private:
        void CheckDataBufferSize(int particlesCount);

        // Called when deserialization is done, used to subscribe to color gradient changes
        void OnDeserialized(const DataValue& node) override;
    };

    // -------------------------------
    // Particles size over time effect
    // -------------------------------
    class ParticlesSizeEffect : public ParticlesEffect
    {
    public:
        Ref<Curve> curve; // Size curve @SERIALIZABLE

    public:
        // Default constructor
        ParticlesSizeEffect();

        // Called when particle is emitted, used to initialize effect data
        void OnParticleEmitted(Particle& particle) override;

        // Update particles size over time
        void Update(float dt, ParticlesEmitter* emitter) override;

        SERIALIZABLE(ParticlesSizeEffect);
        CLONEABLE_REF(ParticlesSizeEffect);

    private:
        struct ParticleData
        {
            Vec2F initialSize;

            int cacheKey = 0;
            int cacheKeyApprox = 0;

            float randomCoef = 0.0f;

            bool operator==(const ParticleData& other) const
            {
                return cacheKey == other.cacheKey;
            }
        };

        Vector<ParticleData> mData; // Particles data buffer

    private:
        void CheckDataBufferSize(int particlesCount);

        // Called when deserialization is done, used to subscribe to size curve changes
        void OnDeserialized(const DataValue& node) override;
    };

    // -------------------------------
    // Particles angle over time effect
    // -------------------------------
    class ParticlesAngleEffect : public ParticlesEffect
    {
    public:
        Ref<Curve> curve; // Angle curve @SERIALIZABLE

    public:
        // Default constructor
        ParticlesAngleEffect();

        // Called when particle is emitted, used to initialize effect data
        void OnParticleEmitted(Particle& particle) override;

        // Update particles size over time
        void Update(float dt, ParticlesEmitter* emitter) override;

        SERIALIZABLE(ParticlesAngleEffect);
        CLONEABLE_REF(ParticlesAngleEffect);

    private:
        struct ParticleData
        {
            float initialAngle = 0.0f;

            int cacheKey = 0;
            int cacheKeyApprox = 0;

            float randomCoef = 0.0f;

            bool operator==(const ParticleData& other) const
            {
                return cacheKey == other.cacheKey;
            }
        };

        Vector<ParticleData> mData; // Particles data buffer

    private:
        void CheckDataBufferSize(int particlesCount);

        // Called when deserialization is done, used to subscribe to size curve changes
        void OnDeserialized(const DataValue& node) override;
    };

    // --------------------------------------
    // Particles angle speed over time effect
    // --------------------------------------
    class ParticlesAngleSpeedEffect : public ParticlesEffect
    {
    public:
        Ref<Curve> curve; // Angle speed curve @SERIALIZABLE

    public:
        // Default constructor
        ParticlesAngleSpeedEffect();

        // Called when particle is emitted, used to initialize effect data
        void OnParticleEmitted(Particle& particle) override;

        // Update particles size over time
        void Update(float dt, ParticlesEmitter* emitter) override;

        SERIALIZABLE(ParticlesAngleSpeedEffect);
        CLONEABLE_REF(ParticlesAngleSpeedEffect);

    private:
        struct ParticleData
        {
            float initialSpeed = 0.0f;

            int cacheKey = 0;
            int cacheKeyApprox = 0;

            float randomCoef = 0.0f;

            bool operator==(const ParticleData& other) const
            {
                return cacheKey == other.cacheKey;
            }
        };

        Vector<ParticleData> mData; // Particles data buffer

    private:
        void CheckDataBufferSize(int particlesCount);

        // Called when deserialization is done, used to subscribe to size curve changes
        void OnDeserialized(const DataValue& node) override;
    };

    // --------------------------------------
    // Particles velocity over time effect
    // --------------------------------------
    class ParticlesVelocityEffect : public ParticlesEffect
    {
    public:
        Ref<Curve> XCurve; // Velocity by X curve @SERIALIZABLE
        Ref<Curve> YCurve; // Velocity by X curve @SERIALIZABLE

    public:
        // Default constructor
        ParticlesVelocityEffect();

        // Called when particle is emitted, used to initialize effect data
        void OnParticleEmitted(Particle& particle) override;

        // Update particles size over time
        void Update(float dt, ParticlesEmitter* emitter) override;

        SERIALIZABLE(ParticlesVelocityEffect);
        CLONEABLE_REF(ParticlesVelocityEffect);

    private:
        struct ParticleData
        {
            Vec3F initialVelocity;

            int cacheXKey = 0;
            int cacheXKeyApprox = 0;
            float randomXCoef = 0.0f;

            int cacheYKey = 0;
            int cacheYKeyApprox = 0;
            float randomYCoef = 0.0f;

            bool operator==(const ParticleData& other) const
            {
                return cacheXKey == other.cacheXKey && cacheYKey == other.cacheYKey;
            }
        };

        Vector<ParticleData> mData; // Particles data buffer

    private:
        void CheckDataBufferSize(int particlesCount);

        // Called when deserialization is done, used to subscribe to size curve changes
        void OnDeserialized(const DataValue& node) override;
    };

    // ---------------------------------------------
    // Particles position over time on spline effect
    // ---------------------------------------------
    class ParticlesSplineEffect : public ParticlesEffect
    {
    public:
        Ref<Curve>  timeCurve; // Time movement curve @SERIALIZABLE
        Ref<Spline> spline;    // Trajectory spline @SERIALIZABLE

    public:
        // Default constructor
        ParticlesSplineEffect();

        // Called when particle is emitted, used to initialize effect data
        void OnParticleEmitted(Particle& particle) override;

        // Update particles size over time
        void Update(float dt, ParticlesEmitter* emitter) override;

        SERIALIZABLE(ParticlesSplineEffect);
        CLONEABLE_REF(ParticlesSplineEffect);

    private:
        struct ParticleData
        {
            Vec3F initialPosition;

            int timeCacheKey = 0;
            int timeCacheKeyApprox = 0;
            float timeRandomCoef = 0.0f;

            int splineCacheKey = 0;
            int splineCacheKeyApprox = 0;
            float splineRandomCoef = 0.0f;

            bool operator==(const ParticleData& other) const
            {
                return timeCacheKey == other.timeCacheKey && splineCacheKey == other.splineCacheKey;
            }
        };

        Vector<ParticleData> mData; // Particles data buffer

    private:
        void CheckDataBufferSize(int particlesCount);

        // Called when deserialization is done, used to subscribe to size curve changes
        void OnDeserialized(const DataValue& node) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::ParticlesEffect)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::ParticlesEffect)
{
    FIELD().PROTECTED().NAME(mEmitter);
}
END_META;
CLASS_METHODS_META(o2::ParticlesEffect)
{

    FUNCTION().PUBLIC().SIGNATURE(void, OnParticleEmitted, Particle&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnParticleDied, Particle&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float, ParticlesEmitter*);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Particle>&, GetParticlesDirect, ParticlesEmitter*);
    FUNCTION().PUBLIC().SIGNATURE(Ref<ParticlesEmitter>, GetEmitter);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChanged);
}
END_META;

CLASS_BASES_META(o2::ParticlesGravityEffect)
{
    BASE_CLASS(o2::ParticlesEffect);
}
END_META;
CLASS_FIELDS_META(o2::ParticlesGravityEffect)
{
    FIELD().PUBLIC().NAME(gravity);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mGravity);
}
END_META;
CLASS_METHODS_META(o2::ParticlesGravityEffect)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR();
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetGravity, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(const Vec3F&, GetGravity);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float, ParticlesEmitter*);
}
END_META;

CLASS_BASES_META(o2::ParticlesColorEffect)
{
    BASE_CLASS(o2::ParticlesEffect);
}
END_META;
CLASS_FIELDS_META(o2::ParticlesColorEffect)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(colorGradient);
    FIELD().PRIVATE().NAME(mColorData);
}
END_META;
CLASS_METHODS_META(o2::ParticlesColorEffect)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, OnParticleEmitted, Particle&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float, ParticlesEmitter*);
    FUNCTION().PRIVATE().SIGNATURE(void, CheckDataBufferSize, int);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;

CLASS_BASES_META(o2::ParticlesRandomColorEffect)
{
    BASE_CLASS(o2::ParticlesEffect);
}
END_META;
CLASS_FIELDS_META(o2::ParticlesRandomColorEffect)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(colorGradientA);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(colorGradientB);
    FIELD().PRIVATE().NAME(mColorData);
}
END_META;
CLASS_METHODS_META(o2::ParticlesRandomColorEffect)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, OnParticleEmitted, Particle&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float, ParticlesEmitter*);
    FUNCTION().PRIVATE().SIGNATURE(void, CheckDataBufferSize, int);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;

CLASS_BASES_META(o2::ParticlesSizeEffect)
{
    BASE_CLASS(o2::ParticlesEffect);
}
END_META;
CLASS_FIELDS_META(o2::ParticlesSizeEffect)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(curve);
    FIELD().PRIVATE().NAME(mData);
}
END_META;
CLASS_METHODS_META(o2::ParticlesSizeEffect)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, OnParticleEmitted, Particle&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float, ParticlesEmitter*);
    FUNCTION().PRIVATE().SIGNATURE(void, CheckDataBufferSize, int);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;

CLASS_BASES_META(o2::ParticlesAngleEffect)
{
    BASE_CLASS(o2::ParticlesEffect);
}
END_META;
CLASS_FIELDS_META(o2::ParticlesAngleEffect)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(curve);
    FIELD().PRIVATE().NAME(mData);
}
END_META;
CLASS_METHODS_META(o2::ParticlesAngleEffect)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, OnParticleEmitted, Particle&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float, ParticlesEmitter*);
    FUNCTION().PRIVATE().SIGNATURE(void, CheckDataBufferSize, int);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;

CLASS_BASES_META(o2::ParticlesAngleSpeedEffect)
{
    BASE_CLASS(o2::ParticlesEffect);
}
END_META;
CLASS_FIELDS_META(o2::ParticlesAngleSpeedEffect)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(curve);
    FIELD().PRIVATE().NAME(mData);
}
END_META;
CLASS_METHODS_META(o2::ParticlesAngleSpeedEffect)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, OnParticleEmitted, Particle&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float, ParticlesEmitter*);
    FUNCTION().PRIVATE().SIGNATURE(void, CheckDataBufferSize, int);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;

CLASS_BASES_META(o2::ParticlesVelocityEffect)
{
    BASE_CLASS(o2::ParticlesEffect);
}
END_META;
CLASS_FIELDS_META(o2::ParticlesVelocityEffect)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(XCurve);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(YCurve);
    FIELD().PRIVATE().NAME(mData);
}
END_META;
CLASS_METHODS_META(o2::ParticlesVelocityEffect)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, OnParticleEmitted, Particle&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float, ParticlesEmitter*);
    FUNCTION().PRIVATE().SIGNATURE(void, CheckDataBufferSize, int);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;

CLASS_BASES_META(o2::ParticlesSplineEffect)
{
    BASE_CLASS(o2::ParticlesEffect);
}
END_META;
CLASS_FIELDS_META(o2::ParticlesSplineEffect)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(timeCurve);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(spline);
    FIELD().PRIVATE().NAME(mData);
}
END_META;
CLASS_METHODS_META(o2::ParticlesSplineEffect)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, OnParticleEmitted, Particle&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float, ParticlesEmitter*);
    FUNCTION().PRIVATE().SIGNATURE(void, CheckDataBufferSize, int);
    FUNCTION().PRIVATE().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;
// --- END META ---
