#pragma once

#include "o2/Animation/IAnimation.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Assets/Types/SoundAsset.h"
#include "o2/Utils/Basic/ICloneable.h"
#include "o2/Utils/Math/Vector3.h"

struct ma_decoder;
struct ma_sound;

namespace o2
{
    // ---------------------------------------------------------------------------------
    // Sound player. Plays sound asset as animation, can be sequenced and scrubbed in
    // the animation editor. Supports 2D/3D spatialization relative to the camera
    // ---------------------------------------------------------------------------------
    class SoundPlayer: public IAnimation, virtual public ICloneableRef
    {
    public:
        PROPERTIES(SoundPlayer);
        PROPERTY(AssetRef<SoundAsset>, sound, SetSound, GetSound); // Sound asset property
        PROPERTY(float, volume, SetVolume, GetVolume);             // Volume property, 1.0 is default
        PROPERTY(float, pitch, SetPitch, GetPitch);                // Pitch property, 1.0 is default
        PROPERTY(bool, spatial, SetSpatial, IsSpatial);            // Spatialization enable property

    public:
        // Default constructor
        SoundPlayer();

        // Default constructor with ref counter
        explicit SoundPlayer(RefCounter* refCounter);

        // Copy-constructor
        SoundPlayer(const SoundPlayer& other);

        // Copy-constructor with ref counter
        SoundPlayer(RefCounter* refCounter, const SoundPlayer& other);

        // Destructor
        ~SoundPlayer();

        // Assign operator
        SoundPlayer& operator=(const SoundPlayer& other);

        // Called by mmake after reference counter initialization; registers player in the sound system
        void PostRefConstruct();

        // Sets sound asset
        void SetSound(const AssetRef<SoundAsset>& sound);

        // Returns sound asset
        const AssetRef<SoundAsset>& GetSound() const;

        // Sets volume, 1.0 is default
        void SetVolume(float volume);

        // Returns volume
        float GetVolume() const;

        // Sets pitch, 1.0 is default
        void SetPitch(float pitch);

        // Returns pitch
        float GetPitch() const;

        // Enables spatialization: sound is positioned in 2D/3D relative to the camera listener
        void SetSpatial(bool spatial);

        // Returns true if spatialization enabled
        bool IsSpatial() const;

        // Sets spatial sound source position
        void SetPosition(const Vec3F& position);

        // Returns spatial sound source position
        Vec3F GetPosition() const;

        // Sets distance at which sound is heard at full volume
        void SetMinDistance(float distance);

        // Returns distance at which sound is heard at full volume
        float GetMinDistance() const;

        // Sets distance at which sound attenuation stops
        void SetMaxDistance(float distance);

        // Returns distance at which sound attenuation stops
        float GetMaxDistance() const;

        // Sets attenuation rolloff factor
        void SetRolloff(float rolloff);

        // Returns attenuation rolloff factor
        float GetRolloff() const;

        // Updates animation time and syncs backend playback
        void Update(float dt) override;

        // Returns true if backend sound is actually audible now
        bool IsBackendPlaying() const;

        // Returns sound duration
        float GetDuration() const override;

        SERIALIZABLE(SoundPlayer);
        CLONEABLE_REF(SoundPlayer);

    protected:
        AssetRef<SoundAsset> mSound; // Sound asset @SERIALIZABLE

        float mVolume = 1.0f;  // Volume @SERIALIZABLE
        float mPitch = 1.0f;   // Pitch @SERIALIZABLE
        bool  mSpatial = false; // Is spatialization enabled @SERIALIZABLE

        float mMinDistance = 100.0f;   // Full volume distance @SERIALIZABLE
        float mMaxDistance = 10000.0f; // Attenuation end distance @SERIALIZABLE
        float mRolloff = 1.0f;         // Attenuation rolloff factor @SERIALIZABLE

        Vec3F mPosition; // Spatial sound source position

        ma_decoder* mBackendDecoder = nullptr; // Backend decoder over asset data
        ma_sound*   mBackendSound = nullptr;   // Backend sound instance

        bool mIsUpdating = false; // True inside Update, to distinguish playback from scrubbing in Evaluate

        bool  mScrubPlaying = false; // True when backend is playing driven by external time sets
        float mScrubTimer = 0.0f;    // Time since last external time set, stops scrub playback on timeout

    protected:
        // Syncs backend playback with current time; when driven externally starts scrub preview playback
        void Evaluate() override;

        // Starts backend playback at current time
        void OnPlay() override;

        // Stops backend playback
        void OnStop() override;

        // Applies looping to backend
        void OnLoopChanged() override;

        // Completion deserialization callback; recreates backend sound
        void OnDeserialized(const DataValue& node) override;

        // Creates backend decoder and sound from asset data
        void CreateBackend();

        // Destroys backend decoder and sound
        void ReleaseBackend();

        // Applies volume, pitch, spatialization and attenuation to backend
        void ApplyBackendParams();

        // Updates duration from asset and resets bounds
        void UpdateDuration();

        // Returns backend playback cursor in seconds
        float GetBackendTime() const;

        // Seeks backend playback to time
        void SeekBackend(float time);

        // Starts backend playback if not playing
        void StartBackend();

        // Stops backend playback
        void StopBackend();

        // Stops scrub preview playback when external time sets have stopped coming
        void UpdateScrubWatchdog(float dt);

        friend class SoundSystem;
    };
}
// --- META ---

CLASS_BASES_META(o2::SoundPlayer)
{
    BASE_CLASS(o2::IAnimation);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::SoundPlayer)
{
    FIELD().PUBLIC().NAME(sound);
    FIELD().PUBLIC().NAME(volume);
    FIELD().PUBLIC().NAME(pitch);
    FIELD().PUBLIC().NAME(spatial);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSound);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mVolume);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mPitch);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mSpatial);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(100.0f).NAME(mMinDistance);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(10000.0f).NAME(mMaxDistance);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mRolloff);
    FIELD().PROTECTED().NAME(mPosition);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mBackendDecoder);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mBackendSound);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsUpdating);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mScrubPlaying);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mScrubTimer);
}
END_META;
CLASS_METHODS_META(o2::SoundPlayer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(const SoundPlayer&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const SoundPlayer&);
    FUNCTION().PUBLIC().SIGNATURE(void, PostRefConstruct);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSound, const AssetRef<SoundAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<SoundAsset>&, GetSound);
    FUNCTION().PUBLIC().SIGNATURE(void, SetVolume, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetVolume);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPitch, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetPitch);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSpatial, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSpatial);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPosition, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetPosition);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMinDistance, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetMinDistance);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaxDistance, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetMaxDistance);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRolloff, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetRolloff);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsBackendPlaying);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PROTECTED().SIGNATURE(void, Evaluate);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPlay);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStop);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLoopChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, CreateBackend);
    FUNCTION().PROTECTED().SIGNATURE(void, ReleaseBackend);
    FUNCTION().PROTECTED().SIGNATURE(void, ApplyBackendParams);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateDuration);
    FUNCTION().PROTECTED().SIGNATURE(float, GetBackendTime);
    FUNCTION().PROTECTED().SIGNATURE(void, SeekBackend, float);
    FUNCTION().PROTECTED().SIGNATURE(void, StartBackend);
    FUNCTION().PROTECTED().SIGNATURE(void, StopBackend);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateScrubWatchdog, float);
}
END_META;
// --- END META ---
