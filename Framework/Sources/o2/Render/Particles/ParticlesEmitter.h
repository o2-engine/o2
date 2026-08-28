#pragma once

#include "o2/Animation/IAnimation.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Particles/Particle.h"
#include "o2/Render/Particles/ParticlesContainer.h"
#include "o2/Render/Particles/ParticlesEffects.h"
#include "o2/Render/Particles/ParticlesEmitterShapes.h"
#include "o2/Render/IRectDrawable.h"
#include "o2/Utils/Editor/Attributes/GroupAttribute.h"
#include "o2/Utils/Editor/Attributes/NameAttribute.h"
#include "o2/Utils/Editor/Attributes/RangeAttribute.h"
#include "o2/Utils/Math/AABB.h"
#include "o2/Utils/Math/Basis3D.h"
#include "o2/Utils/Math/ColorGradient.h"
#include "o2/Utils/Math/Curve.h"

namespace o2
{
    class Mesh;

    // ------------------------------------------------------
    // Particles emitter. Emits, updates and manage particles
    // ------------------------------------------------------
    class ParticlesEmitter: public IRectDrawable, public IAnimation
    {
    public:
        PROPERTIES(ParticlesEmitter);

        PROPERTY(float, duration, SetDuration, GetDuration); // Animation duration property @RANGE(0, 5)

        PROPERTY(Ref<ParticleSource>, particlesSource, SetParticlesSource, GetParticlesSource); // Particles source property @EXPANDED_BY_DEFAULT @DONT_DELETE @DEFAULT_TYPE(o2::SingleSpriteParticleSource) @GROUP("Emission")

        PROPERTY(Ref<ParticlesEmitterShape>, shape, SetShape, GetShape);                    // Emitting shape property @EXPANDED_BY_DEFAULT @DONT_DELETE @DEFAULT_TYPE(o2::CircleParticlesEmitterShape) @GROUP("Emission")
        PROPERTY(bool, emitFromShell, SetParticlesEmitFromShell, IsParticlesEmitFromShell); // Emitting particles from shell of shape property @GROUP("Emission")
                
        PROPERTY(float, particlesPerSecond, SetParticlesPerSecond, GetParticlesPerSecond); // Amount of particles emitting in one second property @RANGE(0, 100) @GROUP("Emission")
        PROPERTY(int, maxParticles, SetMaxParticles, GetMaxParticles);                     // Number of maximum particles in emitter property @GROUP("Emission")
        PROPERTY(float, emissionDuration, SetEmissionDuration, GetEmissionDuration);       // Emitting particles duration in seconds property @RANGE(0, 10) @GROUP("Emission")
        PROPERTY(float, particlesLifetime, SetParticlesLifetime, GetParticlesLifetime);    // Particles lifetime in seconds property @RANGE(0, 10) @GROUP("Emission")

        PROPERTY(bool, particlesRelative, SetParticlesRelativity, IsParticlesRelative); // Is particles relative to emitter @GROUP("Emission")

    public:

        PROPERTY(bool, is3D, SetIs3D, Is3D); // Is emitter working in 3D mode: emits in 3D space, draws camera facing billboards @GROUP("Emission")

        PROPERTY(float, emittingCoefficient, SetEmittingCoef, GetEmittingCoef); // Particles emitting coefficient property (0...1) @RANGE(0, 1) @GROUP("Emission")

        PROPERTY(float, prewarmTime, SetPrewarmTime, GetPrewarmTime); // Emitting particles prewarm time property @RANGE(0, 1) @GROUP("Emission")
        
        PROPERTY(float, initialAngle, SetInitialAngle, GetInitialAngle);                // Emitting particle angle property in degrees @RANGE(0, 360) @GROUP("Initial parameters") @NAME("Angle")
        PROPERTY(float, initialAngleRange, SetInitialAngleRange, GetInitialAngleRange); // Emitting particle angle range property in degrees @RANGE(0, 360) @GROUP("Initial parameters") @NAME("Angle range")
        
        PROPERTY(float, initialSize, SetInitialSize, GetInitialSize);                // Emitting particle size property @RANGE(0, 3) @GROUP("Initial parameters") @NAME("Size")
        PROPERTY(float, initialSizeRange, SetInitialSizeRange, GetInitialSizeRange); // Emitting particle size range property @RANGE(0, 2) @GROUP("Initial parameters") @NAME("Size range")

        PROPERTY(float, initialWidthScale, SetInitialWidthScale, GetInitialWidthScale);                // Emitting particle width scale property @RANGE(0, 3) @GROUP("Initial parameters") @NAME("Width scale")
        PROPERTY(float, initialWidthScaleRange, SetInitialWidthScaleRange, GetInitialWidthScaleRange); // Emitting particle width scale range property @RANGE(0, 2) @GROUP("Initial parameters") @NAME("Width scale range")
        
        PROPERTY(float, initialSpeed, SetInitialSpeed, GetInitialSpeed);                // Emitting particle angle speed property in degrees/sec @RANGE(0, 360) @GROUP("Initial parameters") @NAME("Speed")
        PROPERTY(float, initialSpeedRange, SetInitialSpeedRange, GetInitialSpeedRange); // Emitting particle angle speed range in degrees/sec @RANGE(0, 360) @GROUP("Initial parameters") @NAME("Speed range")
        
        PROPERTY(float, initialAngleSpeed, SetInitialAngleSpeed, GetInitialAngleSpeed);                // Emitting particle speed property @RANGE(0, 720) @GROUP("Initial parameters") @NAME("Angle speed")
        PROPERTY(float, initialAngleSpeedRange, SetInitialAngleSpeedRange, GetInitialAngleSpeedRange); // Emitting particle speed range @RANGE(0, 720) @GROUP("Initial parameters") @NAME("Angle speed range")
       
        PROPERTY(float, moveDirection, SetEmitParticlesMoveDirection, GetEmitParticlesMoveDirection);                // Emitting particle moving direction in degrees @RANGE(0, 360) @GROUP("Initial parameters")
        PROPERTY(float, moveDirectionRange, SetEmitParticlesMoveDirectionRange, GetEmitParticlesMoveDirectionRange); // Emitting particle moving direction range in degrees property @RANGE(0, 360) @GROUP("Initial parameters")

        PROPERTY(Vec3F, moveDirection3D, SetEmitParticlesMoveDirection3D, GetEmitParticlesMoveDirection3D); // Emitting particle moving direction in 3D mode; moveDirectionRange works as cone angle @GROUP("Initial parameters")

    public:
        // Default constructor
        ParticlesEmitter();

        // Copy-constructor
        ParticlesEmitter(const ParticlesEmitter& other);

        // Destructor
        ~ParticlesEmitter();

        // Copy-operator
        ParticlesEmitter& operator=(const ParticlesEmitter& other);

        // DRaws particles
        void Draw() override;

        // Updates particles
        void Update(float dt);

        // Starts playing @SCRIPTABLE
        void Play();

        // Stops playing @SCRIPTABLE
        void Stop();

        // Sets duration of emitter (emission duration + particles lifetime)
        void SetDuration(float duration);

        // Returns duration (emission duration + particles lifetime)
        float GetDuration() const override;

        // Sets time of emitter
        void SetTime(float time) override;

        // Sets particles source @SCRIPTABLE
        void SetParticlesSource(const Ref<ParticleSource>& source);

        // Returns particles source
        const Ref<ParticleSource>& GetParticlesSource() const;

        // Sets particles emitting coefficient (0...1)
        void SetEmittingCoef(float coef);

        // Returns particles emitting coefficient
        float GetEmittingCoef() const;

        // Sets emitting shape @SCRIPTABLE
        void SetShape(const Ref<ParticlesEmitterShape>& shape);

        // Return emitting shape
        const Ref<ParticlesEmitterShape>& GetShape() const;

        // Adds effect @SCRIPTABLE
        void AddEffect(const Ref<ParticlesEffect>& effect);

        // Links effects and shape to this emitter, called by mmake after construction
        void PostRefConstruct();

#if IS_EDITOR
        // Returns number of baked frames (editor scrub cache)
        int GetBakedFramesCount() const;
#endif

        // Adds effect
        template<typename _type, typename ... _args>
        void AddEffect(_args ... args);

        // Returns list of effects
        const Vector<Ref<ParticlesEffect>>& GetEffects() const;

        // Removes effect
        void RemoveEffect(const Ref<ParticlesEffect>& effect);

        // Removes all effects
        void RemoveAllEffects();

        // Set particles limit number @SCRIPTABLE
        void SetMaxParticles(int count);

        // Returns particles limit number @SCRIPTABLE
        int GetMaxParticles() const;

        // Returns current particles count
        int GetParticlesCount() const;

        // Returns has alive particles
        bool IsAliveParticles() const;

        // Returns particles list
        const Vector<Particle>& GetParticles() const;

        // Sets particles relativity
        void SetParticlesRelativity(bool relative);

        // Is particles relative to emitter
        bool IsParticlesRelative() const;

        // Sets 3D mode: particles are emitted in 3D space and drawn as camera facing billboards
        void SetIs3D(bool is3D);

        // Returns is emitter in 3D mode
        bool Is3D() const;

        // Sets world emission basis for 3D mode; the shape emits in its unit volume
        void Set3DBasis(const Basis3D& basis);

        // Returns world emission basis for 3D mode
        const Basis3D& Get3DBasis() const;

        // Returns world bounds of alive particles; false when there are none
        bool GetParticlesBounds(o2::AABB& bounds) const;

        // Sets particles emit from shell
        void SetParticlesEmitFromShell(bool fromShell);

        // Is particles emit from shell
        bool IsParticlesEmitFromShell() const;

        // Sets emitting particles duration in seconds @SCRIPTABLE
        void SetEmissionDuration(float duration);

        // Returns emitting particles duration in seconds
        float GetEmissionDuration() const;

        // Sets particles lifetime in seconds @SCRIPTABLE
        void SetParticlesLifetime(float lifetime);

        // Returns particles lifetime in seconds
        float GetParticlesLifetime() const;

        // Sets number of particles emitting per second @SCRIPTABLE
        void SetParticlesPerSecond(float numParticles);

        // Returns number of particles emitting per second @SCRIPTABLE
        float GetParticlesPerSecond() const;

        // Sets emitting particles prewarm time @SCRIPTABLE
        void SetPrewarmTime(float time);

        // Returns emitting particles prewarm time
        float GetPrewarmTime() const;

        // Sets emitting particles rotation angle in degrees
        void SetInitialAngle(float angle);

        // Returns emitting particles rotation angle in degrees
        float GetInitialAngle() const;

        // Sets emitting particles rotation angle range in degrees
        void SetInitialAngleRange(float range);

        // Returns emitting particles rotation angle range in degrees
        float GetInitialAngleRange() const;

        // Sets emitting particles size @SCRIPTABLE
        void SetInitialSize(float size);

        // Returns emitting particles size
        float GetInitialSize() const;

        // Sets emitting particles size range @SCRIPTABLE
        void SetInitialSizeRange(float range);

        // Returns emitting particles size range
        float GetInitialSizeRange() const;

        // Sets emitting particles width scale
        void SetInitialWidthScale(float scale);

        // Returns emitting particles width scale
        float GetInitialWidthScale() const;

        // Sets emitting particles width scale range
        void SetInitialWidthScaleRange(float scaleRange);

        // Returns emitting particles width scale range
        float GetInitialWidthScaleRange() const;

        // Sets emitting particles angle speed in degrees/sec @SCRIPTABLE
        void SetInitialAngleSpeed(float speed);

        // Returns emitting particles angle speed in degrees/sec
        float GetInitialAngleSpeed() const;

        // Sets emitting particles angle speed range in degrees/sec @SCRIPTABLE
        void SetInitialAngleSpeedRange(float speedRange);

        // Returns emitting particles angle speed range in degrees/sec
        float GetInitialAngleSpeedRange() const;

        // Sets emitting particles speed @SCRIPTABLE
        void SetInitialSpeed(float speed);

        // Returns emitting particles speed
        float GetInitialSpeed() const;

        // Sets emitting particles speed range @SCRIPTABLE
        void SetInitialSpeedRange(float speedRange);

        // Returns emitting particles speed range
        float GetInitialSpeedRange() const;

        // Sets emitting particles moving direction angle in degrees @SCRIPTABLE
        void SetEmitParticlesMoveDirection(float direction);

        // Returns emitting particles moving direction angle in degrees
        float GetEmitParticlesMoveDirection() const;

        // Sets emitting particles moving direction angle range in degrees @SCRIPTABLE
        void SetEmitParticlesMoveDirectionRange(float directionRange);

        // Returns emitting particles moving direction angle range in degrees
        float GetEmitParticlesMoveDirectionRange() const;

        // Sets emitting particles moving direction for 3D mode
        void SetEmitParticlesMoveDirection3D(const Vec3F& direction);

        // Returns emitting particles moving direction for 3D mode
        const Vec3F& GetEmitParticlesMoveDirection3D() const;

        // Dynamic cast to RefCounterable via IAnimation
        static Ref<RefCounterable> CastToRefCounterable(const Ref<ParticlesEmitter>& ref);

        SERIALIZABLE(ParticlesEmitter);
        CLONEABLE_REF(ParticlesEmitter);

        REF_COUNTERABLE_IMPL(IRectDrawable, IAnimation);

    protected:
        Ref<ParticleSource> mParticlesSource = mmake<SingleSpriteParticleSource>(); // Source of particles @SERIALIZABLE 

        Ref<ParticlesContainer> mParticlesContainer; // Particles container, stores and updates particles, created from particles source

        Ref<ParticlesEmitterShape> mShape = nullptr; // Particles emitting shape @SERIALIZABLE 

        bool mEmitParticlesFromShell = false; // Emitting particles from shell of shape @SERIALIZABLE

        Vector<Ref<ParticlesEffect>> mEffects; // Particles effect @SERIALIZABLE @EDITOR_PROPERTY @EXPANDED_BY_DEFAULT @DONT_DELETE @INVOKE_ON_CHANGED(OnEffectsListChanged)
                                                                         
        int mParticlesNumLimit = 100; // Max available visible particles @SERIALIZABLE
                                       
        float mEmittingCoefficient = 1.0f; // Emitting particles number coefficient (0...1) @SERIALIZABLE
        bool  mIsParticlesRelative = true; // Is particles relative to emitter or global @SERIALIZABLE

        bool mIs3D = false; // Is emitter working in 3D mode @SERIALIZABLE

        float mEmissionDuration = 0.1f; // Emission duration in seconds @SERIALIZABLE
                                                                  
        float mParticlesLifetime = 0.5f;      // Particles lifetime in seconds @SERIALIZABLE
        float mParticlesLifetimeRange = 0.0f; // Particles lifetime range in seconds @SERIALIZABLE
        float mEmitParticlesPerSecond = 10;   // Number of particles emitting in one second @SERIALIZABLE

        float mPrewarmTime = 0.0f;    // Particles prewarm time (updates particles before start) @SERIALIZABLE
        float mPrewarmTimeout = 0.0f; // Particles prewarm timeout. Starts from prewarm time and decreases to 0
                                                          
        float mInitialAngle = 0;          // Emitting particles angle in degrees @SERIALIZABLE
        float mInitialAngleRange = 45.0f; // Emitting particles angle in degrees randomize range @SERIALIZABLE
                                                        
        float mInitialSize = 1.0f;      // Emitting particles size @SERIALIZABLE
        float mInitialSizeRange = 0.3f; // Emitting particles size randomize range @SERIALIZABLE

        float mInitialWidthScale = 1.0f;  // Emitting particles width scale @SERIALIZABLE
        float mInitialWidthScaleRange = 0; // Emitting particles width scale randomize range @SERIALIZABLE
                                                        
        float mInitialSpeed = 10;      // Emitting particles speed @SERIALIZABLE
        float mInitialSpeedRangle = 5; // Emitting particles speed range @SERIALIZABLE
                                                        
        float mInitialMoveDirection = 0;          // Emitting particles direction in degrees @SERIALIZABLE
        float mInitialMoveDirectionRange = 45.0f; // Emitting particles direction range in degrees @SERIALIZABLE

        Vec3F mInitialMoveDirection3D = Vec3F(0, 0, 1); // Emitting particles direction in 3D mode @SERIALIZABLE
              
        float mInitialAngleSpeed = 0;      // Emitting particles angle speed in degrees/sec
        float mInitialAngleSpeedRange = 0; // Emitting particles angle speed range in degrees/sec

        float            mEmitTimeBuffer = 0;    // Emitting next particle time buffer
        Vector<Particle> mParticles;             // Working particles
        Vector<int>      mDeadParticles;         // Dead particles indexes
        int              mNumAliveParticles = 0; // Count of current alive particles
        Basis            mLastTransform;         // Last transformation

        Basis3D mEmission3DBasis; // World emission basis for 3D mode, set by owning component
        Basis3D mLast3DBasis;     // Last 3D emission basis, for relative particles transformation

    protected:
        // Called when blend mode was changed
        void OnMaterialChanged() override;

        // Beginning serialization callback
        void OnSerialize(DataValue& node) const override;

        // Completion deserialization callback, initializes particles container
        void OnDeserialized(const DataValue& node) override;

        // Beginning serialization delta callback
        void OnSerializeDelta(DataValue& node, const IObject& origin) const override;

        // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;

		// Recalculates particles duration from emission duration and particles lifetime
        void UpdateDuration();

        // Checks is particles container initialized
        void CreateParticlesContainer();

        // Emits particles hen updating
        void UpdateEmitting(float dt);

        // Applies and updates particles effects
        void UpdateEffects(float dt);

        // Updates particles
        void UpdateParticles(float dt);
        
        // Called when basis was changed, updates particles positions from last transform
        void BasisChanged() override;

        // Called when effects list changed, updates emitter reference in effects
        void OnEffectsListChanged();

        // Called when something changed, invalidates baked frames when particles paused
        void OnChanged();

        friend class ParticlesEffect;
        friend class ParticlesEmitterShape;

#if IS_EDITOR
    public:
        // Sets particles pause for editor
        void SetParticlesPause(bool paused);

    protected:
        struct BakedFrame
        {
            Vector<Particle> particles;             // Baked particles frame
            Vector<int>      deadParticles;         // Dead particles indexes
            int              numAliveParticles = 0; // Count of current alive particles
            float            emitTimeBuffer = 0;    // Emitting next particle time buffer
            Basis            transform;             // Emitter transform the frame was baked with

            bool operator==(const BakedFrame& other) const { return false; }
        };

    protected:
        static int mBakedFPS; // Baked particles frames per second

        Vector<BakedFrame> mBakedFrames; // Baked particles frames for editor
        bool mIsBaking = false;          // Frames are being (re)baked now: invalidation requests are ignored

        size_t mRandomSeed = 0; // Random seed for particles emitting

        bool mIsUpdating = false;      // Is updating particles now. Used to detect separated Evaluate calls when time changed
        bool mParticlesPaused = false; // Is particles paused for editor

    protected:
        // Invalidates baked frames
        void InvalidateBakedFrames();

        // Called when updated or time changed, updates baked particles for editor
        void Evaluate() override;

        // Returns baked frame index by time
        int GetBakedFrameIndex(float time) const;

        // Checks that baked enough frames
        void CheckBakedFrames(int maxFrameIdx);

        // Restores baked frame by index
        void RestoreBakedFrame(int frameIdx);

        // Editor scrub: puts the owning actor where the clip has it at the baked frame time
        virtual void OnEditorBakeFrame(float time);
#endif
    };

    template<typename _type, typename ... _args>
    void ParticlesEmitter::AddEffect(_args ... args)
    {
        AddEffect(mmake<_type>(args ...));
    }
}
// --- META ---

CLASS_BASES_META(o2::ParticlesEmitter)
{
    BASE_CLASS(o2::IRectDrawable);
    BASE_CLASS(o2::IAnimation);
}
END_META;
CLASS_FIELDS_META(o2::ParticlesEmitter)
{
    FIELD().PUBLIC().RANGE_ATTRIBUTE(0, 5).NAME(duration);
    FIELD().PUBLIC().DEFAULT_TYPE_ATTRIBUTE(o2::SingleSpriteParticleSource).DONT_DELETE_ATTRIBUTE().EXPANDED_BY_DEFAULT_ATTRIBUTE().GROUP_ATTRIBUTE("Emission").NAME(particlesSource);
    FIELD().PUBLIC().DEFAULT_TYPE_ATTRIBUTE(o2::CircleParticlesEmitterShape).DONT_DELETE_ATTRIBUTE().EXPANDED_BY_DEFAULT_ATTRIBUTE().GROUP_ATTRIBUTE("Emission").NAME(shape);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Emission").NAME(emitFromShell);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Emission").RANGE_ATTRIBUTE(0, 100).NAME(particlesPerSecond);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Emission").NAME(maxParticles);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Emission").RANGE_ATTRIBUTE(0, 10).NAME(emissionDuration);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Emission").RANGE_ATTRIBUTE(0, 10).NAME(particlesLifetime);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Emission").NAME(particlesRelative);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Emission").NAME(is3D);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Emission").RANGE_ATTRIBUTE(0, 1).NAME(emittingCoefficient);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Emission").RANGE_ATTRIBUTE(0, 1).NAME(prewarmTime);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Initial parameters").NAME_ATTRIBUTE("Angle").RANGE_ATTRIBUTE(0, 360).NAME(initialAngle);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Initial parameters").NAME_ATTRIBUTE("Angle range").RANGE_ATTRIBUTE(0, 360).NAME(initialAngleRange);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Initial parameters").NAME_ATTRIBUTE("Size").RANGE_ATTRIBUTE(0, 3).NAME(initialSize);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Initial parameters").NAME_ATTRIBUTE("Size range").RANGE_ATTRIBUTE(0, 2).NAME(initialSizeRange);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Initial parameters").NAME_ATTRIBUTE("Width scale").RANGE_ATTRIBUTE(0, 3).NAME(initialWidthScale);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Initial parameters").NAME_ATTRIBUTE("Width scale range").RANGE_ATTRIBUTE(0, 2).NAME(initialWidthScaleRange);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Initial parameters").NAME_ATTRIBUTE("Speed").RANGE_ATTRIBUTE(0, 360).NAME(initialSpeed);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Initial parameters").NAME_ATTRIBUTE("Speed range").RANGE_ATTRIBUTE(0, 360).NAME(initialSpeedRange);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Initial parameters").NAME_ATTRIBUTE("Angle speed").RANGE_ATTRIBUTE(0, 720).NAME(initialAngleSpeed);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Initial parameters").NAME_ATTRIBUTE("Angle speed range").RANGE_ATTRIBUTE(0, 720).NAME(initialAngleSpeedRange);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Initial parameters").RANGE_ATTRIBUTE(0, 360).NAME(moveDirection);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Initial parameters").RANGE_ATTRIBUTE(0, 360).NAME(moveDirectionRange);
    FIELD().PUBLIC().GROUP_ATTRIBUTE("Initial parameters").NAME(moveDirection3D);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(mmake<SingleSpriteParticleSource>()).NAME(mParticlesSource);
    FIELD().PROTECTED().NAME(mParticlesContainer);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(nullptr).NAME(mShape);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mEmitParticlesFromShell);
    FIELD().PROTECTED().DONT_DELETE_ATTRIBUTE().EDITOR_PROPERTY_ATTRIBUTE().EXPANDED_BY_DEFAULT_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mEffects);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(100).NAME(mParticlesNumLimit);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mEmittingCoefficient);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mIsParticlesRelative);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mIs3D);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.1f).NAME(mEmissionDuration);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.5f).NAME(mParticlesLifetime);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mParticlesLifetimeRange);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(10).NAME(mEmitParticlesPerSecond);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mPrewarmTime);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mPrewarmTimeout);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(mInitialAngle);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(45.0f).NAME(mInitialAngleRange);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mInitialSize);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.3f).NAME(mInitialSizeRange);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mInitialWidthScale);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(mInitialWidthScaleRange);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(10).NAME(mInitialSpeed);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(5).NAME(mInitialSpeedRangle);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(mInitialMoveDirection);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(45.0f).NAME(mInitialMoveDirectionRange);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Vec3F(0, 0, 1)).NAME(mInitialMoveDirection3D);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mInitialAngleSpeed);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mInitialAngleSpeedRange);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mEmitTimeBuffer);
    FIELD().PROTECTED().NAME(mParticles);
    FIELD().PROTECTED().NAME(mDeadParticles);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mNumAliveParticles);
    FIELD().PROTECTED().NAME(mLastTransform);
    FIELD().PROTECTED().NAME(mEmission3DBasis);
    FIELD().PROTECTED().NAME(mLast3DBasis);
#if  IS_EDITOR
    FIELD().PROTECTED().NAME(mBakedFrames);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsBaking);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mRandomSeed);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsUpdating);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mParticlesPaused);
#endif
}
END_META;
CLASS_METHODS_META(o2::ParticlesEmitter)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const ParticlesEmitter&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Play);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Stop);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDuration, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTime, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetParticlesSource, const Ref<ParticleSource>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ParticleSource>&, GetParticlesSource);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEmittingCoef, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetEmittingCoef);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetShape, const Ref<ParticlesEmitterShape>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ParticlesEmitterShape>&, GetShape);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, AddEffect, const Ref<ParticlesEffect>&);
    FUNCTION().PUBLIC().SIGNATURE(void, PostRefConstruct);
#if  IS_EDITOR
    FUNCTION().PUBLIC().SIGNATURE(int, GetBakedFramesCount);
#endif
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<ParticlesEffect>>&, GetEffects);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveEffect, const Ref<ParticlesEffect>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAllEffects);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetMaxParticles, int);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(int, GetMaxParticles);
    FUNCTION().PUBLIC().SIGNATURE(int, GetParticlesCount);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsAliveParticles);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Particle>&, GetParticles);
    FUNCTION().PUBLIC().SIGNATURE(void, SetParticlesRelativity, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsParticlesRelative);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIs3D, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, Is3D);
    FUNCTION().PUBLIC().SIGNATURE(void, Set3DBasis, const Basis3D&);
    FUNCTION().PUBLIC().SIGNATURE(const Basis3D&, Get3DBasis);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetParticlesBounds, o2::AABB&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetParticlesEmitFromShell, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsParticlesEmitFromShell);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetEmissionDuration, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetEmissionDuration);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetParticlesLifetime, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetParticlesLifetime);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetParticlesPerSecond, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetParticlesPerSecond);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetPrewarmTime, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetPrewarmTime);
    FUNCTION().PUBLIC().SIGNATURE(void, SetInitialAngle, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetInitialAngle);
    FUNCTION().PUBLIC().SIGNATURE(void, SetInitialAngleRange, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetInitialAngleRange);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetInitialSize, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetInitialSize);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetInitialSizeRange, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetInitialSizeRange);
    FUNCTION().PUBLIC().SIGNATURE(void, SetInitialWidthScale, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetInitialWidthScale);
    FUNCTION().PUBLIC().SIGNATURE(void, SetInitialWidthScaleRange, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetInitialWidthScaleRange);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetInitialAngleSpeed, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetInitialAngleSpeed);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetInitialAngleSpeedRange, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetInitialAngleSpeedRange);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetInitialSpeed, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetInitialSpeed);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetInitialSpeedRange, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetInitialSpeedRange);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetEmitParticlesMoveDirection, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetEmitParticlesMoveDirection);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetEmitParticlesMoveDirectionRange, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetEmitParticlesMoveDirectionRange);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEmitParticlesMoveDirection3D, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(const Vec3F&, GetEmitParticlesMoveDirection3D);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<ParticlesEmitter>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMaterialChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerializeDelta, DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateDuration);
    FUNCTION().PROTECTED().SIGNATURE(void, CreateParticlesContainer);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateEmitting, float);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateEffects, float);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateParticles, float);
    FUNCTION().PROTECTED().SIGNATURE(void, BasisChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEffectsListChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChanged);
#if  IS_EDITOR
    FUNCTION().PUBLIC().SIGNATURE(void, SetParticlesPause, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, InvalidateBakedFrames);
    FUNCTION().PROTECTED().SIGNATURE(void, Evaluate);
    FUNCTION().PROTECTED().SIGNATURE(int, GetBakedFrameIndex, float);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckBakedFrames, int);
    FUNCTION().PROTECTED().SIGNATURE(void, RestoreBakedFrame, int);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEditorBakeFrame, float);
#endif
}
END_META;
// --- END META ---
