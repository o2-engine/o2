#include "o2/stdafx.h"
#include "SoundPlayer.h"

#include "o2/Sound/SoundSystem.h"

#include "miniaudio.h"

namespace o2
{
    // Max desync between animation time and backend cursor before seeking
    static const float seekThreshold = 0.1f;

    // Scrub preview playback stops when external time sets don't come longer than this
    static const float scrubStopTimeout = 0.25f;

    SoundPlayer::SoundPlayer():
        SoundPlayer(nullptr)
    {}

    SoundPlayer::SoundPlayer(RefCounter* refCounter):
        IAnimation(refCounter)
    {}

    SoundPlayer::SoundPlayer(const SoundPlayer& other):
        SoundPlayer(nullptr, other)
    {}

    SoundPlayer::SoundPlayer(RefCounter* refCounter, const SoundPlayer& other):
        IAnimation(refCounter, other), mSound(other.mSound), mVolume(other.mVolume), mPitch(other.mPitch),
        mSpatial(other.mSpatial), mMinDistance(other.mMinDistance), mMaxDistance(other.mMaxDistance),
        mRolloff(other.mRolloff), mPosition(other.mPosition)
    {
        UpdateDuration();
    }

    SoundPlayer::~SoundPlayer()
    {
        ReleaseBackend();

        if (SoundSystem::IsSingletonInitialzed())
            o2Sounds.UnregisterPlayer(this);
    }

    void SoundPlayer::PostRefConstruct()
    {
        if (SoundSystem::IsSingletonInitialzed())
            o2Sounds.RegisterPlayer(this);
    }

    SoundPlayer& SoundPlayer::operator=(const SoundPlayer& other)
    {
        ReleaseBackend();

        IAnimation::operator=(other);

        mSound = other.mSound;
        mVolume = other.mVolume;
        mPitch = other.mPitch;
        mSpatial = other.mSpatial;
        mMinDistance = other.mMinDistance;
        mMaxDistance = other.mMaxDistance;
        mRolloff = other.mRolloff;
        mPosition = other.mPosition;

        UpdateDuration();

        return *this;
    }

    void SoundPlayer::SetSound(const AssetRef<SoundAsset>& sound)
    {
        Stop();
        ReleaseBackend();

        mSound = sound;

        UpdateDuration();
    }

    const AssetRef<SoundAsset>& SoundPlayer::GetSound() const
    {
        return mSound;
    }

    void SoundPlayer::SetVolume(float volume)
    {
        mVolume = volume;
        ApplyBackendParams();
    }

    float SoundPlayer::GetVolume() const
    {
        return mVolume;
    }

    void SoundPlayer::SetPitch(float pitch)
    {
        mPitch = pitch;
        ApplyBackendParams();
    }

    float SoundPlayer::GetPitch() const
    {
        return mPitch;
    }

    void SoundPlayer::SetSpatial(bool spatial)
    {
        mSpatial = spatial;
        ApplyBackendParams();
    }

    bool SoundPlayer::IsSpatial() const
    {
        return mSpatial;
    }

    void SoundPlayer::SetPosition(const Vec3F& position)
    {
        mPosition = position;

        if (mBackendSound)
            ma_sound_set_position(mBackendSound, position.x, position.y, position.z);
    }

    Vec3F SoundPlayer::GetPosition() const
    {
        return mPosition;
    }

    void SoundPlayer::SetMinDistance(float distance)
    {
        mMinDistance = distance;
        ApplyBackendParams();
    }

    float SoundPlayer::GetMinDistance() const
    {
        return mMinDistance;
    }

    void SoundPlayer::SetMaxDistance(float distance)
    {
        mMaxDistance = distance;
        ApplyBackendParams();
    }

    float SoundPlayer::GetMaxDistance() const
    {
        return mMaxDistance;
    }

    void SoundPlayer::SetRolloff(float rolloff)
    {
        mRolloff = rolloff;
        ApplyBackendParams();
    }

    float SoundPlayer::GetRolloff() const
    {
        return mRolloff;
    }

    void SoundPlayer::Update(float dt)
    {
        mIsUpdating = true;
        IAnimation::Update(dt);
        mIsUpdating = false;
    }

    float SoundPlayer::GetDuration() const
    {
        return mDuration;
    }

    bool SoundPlayer::IsBackendPlaying() const
    {
        return mBackendSound && ma_sound_is_playing(mBackendSound);
    }

    void SoundPlayer::Evaluate()
    {
        if (!mBackendSound)
            CreateBackend();

        if (!mBackendSound)
            return;

        float time = mInDurationTime;

        if (mIsUpdating)
        {
            if (!mPlaying)
                return;

            StartBackend();

            // Backend plays at its own rate when pitch or speed differ, syncing would cause crackle
            bool syncable = Math::Equals(mPitch, 1.0f) && Math::Equals(mSpeed, 1.0f) && mDirection > 0.0f;
            if (syncable && Math::Abs(GetBackendTime() - time) > seekThreshold)
                SeekBackend(time);
        }
        else
        {
            if (Math::Abs(GetBackendTime() - time) > seekThreshold)
                SeekBackend(time);

            // Driven externally (editor scrub or parent animation sub track) - start preview
            // playback, stopped by watchdog when time sets stop coming
            if (!mPlaying)
            {
                StartBackend();

                mScrubPlaying = true;
                mScrubTimer = 0.0f;
            }
        }
    }

    void SoundPlayer::OnPlay()
    {
        mScrubPlaying = false;

        if (!mBackendSound)
            CreateBackend();

        SeekBackend(mInDurationTime);
        StartBackend();
    }

    void SoundPlayer::OnStop()
    {
        mScrubPlaying = false;
        StopBackend();
    }

    void SoundPlayer::OnLoopChanged()
    {
        ApplyBackendParams();
    }

    void SoundPlayer::OnDeserialized(const DataValue& node)
    {
        ReleaseBackend();
        UpdateDuration();
    }

    void SoundPlayer::CreateBackend()
    {
        ReleaseBackend();

        if (!SoundSystem::IsSingletonInitialzed() || !o2Sounds.IsReady())
            return;

        if (!mSound || !mSound->GetData() || mSound->GetDataSize() == 0)
            return;

        mBackendDecoder = mnew ma_decoder();

        ma_decoder_config config = ma_decoder_config_init_default();
        if (ma_decoder_init_memory(mSound->GetData(), mSound->GetDataSize(), &config, mBackendDecoder) != MA_SUCCESS)
        {
            delete mBackendDecoder;
            mBackendDecoder = nullptr;
            return;
        }

        mBackendSound = mnew ma_sound();
        if (ma_sound_init_from_data_source(o2Sounds.GetEngine(), mBackendDecoder, 0, nullptr, mBackendSound) != MA_SUCCESS)
        {
            delete mBackendSound;
            mBackendSound = nullptr;

            ma_decoder_uninit(mBackendDecoder);
            delete mBackendDecoder;
            mBackendDecoder = nullptr;
            return;
        }

        ApplyBackendParams();
    }

    void SoundPlayer::ReleaseBackend()
    {
        if (mBackendSound)
        {
            ma_sound_uninit(mBackendSound);
            delete mBackendSound;
            mBackendSound = nullptr;
        }

        if (mBackendDecoder)
        {
            ma_decoder_uninit(mBackendDecoder);
            delete mBackendDecoder;
            mBackendDecoder = nullptr;
        }

        mScrubPlaying = false;
    }

    void SoundPlayer::ApplyBackendParams()
    {
        if (!mBackendSound)
            return;

        ma_sound_set_volume(mBackendSound, mVolume);
        ma_sound_set_pitch(mBackendSound, mPitch);
        ma_sound_set_looping(mBackendSound, mLoop == Loop::Repeat);

        ma_sound_set_spatialization_enabled(mBackendSound, mSpatial);
        ma_sound_set_positioning(mBackendSound, ma_positioning_absolute);
        ma_sound_set_position(mBackendSound, mPosition.x, mPosition.y, mPosition.z);
        ma_sound_set_attenuation_model(mBackendSound, ma_attenuation_model_inverse);
        ma_sound_set_min_distance(mBackendSound, mMinDistance);
        ma_sound_set_max_distance(mBackendSound, mMaxDistance);
        ma_sound_set_rolloff(mBackendSound, mRolloff);
    }

    void SoundPlayer::UpdateDuration()
    {
        mDuration = mSound ? mSound->GetDuration() : 0.0f;
        ResetBounds();
    }

    float SoundPlayer::GetBackendTime() const
    {
        if (!mBackendSound)
            return 0.0f;

        float cursor = 0.0f;
        ma_sound_get_cursor_in_seconds(mBackendSound, &cursor);
        return cursor;
    }

    void SoundPlayer::SeekBackend(float time)
    {
        if (!mBackendSound || !mBackendDecoder)
            return;

        ma_uint64 frame = (ma_uint64)((double)Math::Max(time, 0.0f)*(double)mBackendDecoder->outputSampleRate);
        ma_sound_seek_to_pcm_frame(mBackendSound, frame);
    }

    void SoundPlayer::StartBackend()
    {
        if (mBackendSound && !ma_sound_is_playing(mBackendSound))
            ma_sound_start(mBackendSound);
    }

    void SoundPlayer::StopBackend()
    {
        if (mBackendSound && ma_sound_is_playing(mBackendSound))
            ma_sound_stop(mBackendSound);
    }

    void SoundPlayer::UpdateScrubWatchdog(float dt)
    {
        if (!mScrubPlaying)
            return;

        if (mPlaying)
        {
            mScrubPlaying = false;
            return;
        }

        mScrubTimer += dt;
        if (mScrubTimer > scrubStopTimeout)
        {
            StopBackend();
            mScrubPlaying = false;
        }
    }
}
// --- META ---

DECLARE_CLASS(o2::SoundPlayer, o2__SoundPlayer);
// --- END META ---
