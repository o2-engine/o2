#include "o2/stdafx.h"
#include "SoundSystem.h"

#include "o2/Integration.h"
#include "o2/Render/Render.h"
#include "o2/Sound/SoundListener.h"
#include "o2/Sound/SoundPlayer.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"

#include "miniaudio.h"

namespace o2
{
    FORWARD_REF_IMPL(LogStream);

    DECLARE_SINGLETON(SoundSystem);

    SoundSystem::SoundSystem(RefCounter* refCounter):
        Singleton<SoundSystem>(refCounter)
    {
        mLog = mmake<LogStream>("Sound");
        o2Debug.GetLog()->BindStream(mLog);

        // Null backend keeps the whole sound API working without an audio device (tests, CI)
        if (Integration::IsHeadless())
        {
            mContext = mnew ma_context();
            ma_backend backends[] = { ma_backend_null };
            if (ma_context_init(backends, 1, nullptr, mContext) != MA_SUCCESS)
            {
                delete mContext;
                mContext = nullptr;
            }
        }

        mEngine = mnew ma_engine();

        ma_engine_config config = ma_engine_config_init();
        config.pContext = mContext;

        ma_result result = ma_engine_init(&config, mEngine);
        if (result != MA_SUCCESS && !mContext)
        {
            mLog->Error("Failed to initialize audio device (error " + (String)(int)result + "), falling back to null backend");

            mContext = mnew ma_context();
            ma_backend backends[] = { ma_backend_null };
            if (ma_context_init(backends, 1, nullptr, mContext) == MA_SUCCESS)
            {
                config.pContext = mContext;
                result = ma_engine_init(&config, mEngine);
            }
            else
            {
                delete mContext;
                mContext = nullptr;
            }
        }

        if (result != MA_SUCCESS)
        {
            mLog->Error("Failed to initialize audio engine, sounds are disabled (error " + (String)(int)result + ")");

            delete mEngine;
            mEngine = nullptr;
            return;
        }

        ma_engine_listener_set_position(mEngine, 0, 0.0f, 0.0f, 0.0f);
        ma_engine_listener_set_direction(mEngine, 0, 0.0f, 0.0f, -1.0f);
        ma_engine_listener_set_world_up(mEngine, 0, 0.0f, 1.0f, 0.0f);

        mReady = true;
        mLog->Out("Initialized");
    }

    SoundSystem::~SoundSystem()
    {
        for (auto& player : mPlayers)
        {
            if (auto playerRef = player.Lock())
                playerRef->ReleaseBackend();
        }

        if (mEngine)
        {
            ma_engine_uninit(mEngine);
            delete mEngine;
        }

        if (mContext)
        {
            ma_context_uninit(mContext);
            delete mContext;
        }
    }

    void SoundSystem::Update(float dt)
    {
        if (!mReady)
            return;

        if (auto listener = GetActiveListener())
        {
            SetListenerPosition(listener->GetPosition());
            SetListenerOrientation(listener->GetForward(), listener->GetUp());
        }
        else
            UpdateListenerFromCamera();

        for (auto& player : mPlayers)
        {
            if (auto playerRef = player.Lock())
                playerRef->UpdateScrubWatchdog(dt);
        }
    }

    void SoundSystem::SetVolume(float volume)
    {
        mVolume = volume;

        if (mReady)
            ma_engine_set_volume(mEngine, volume);
    }

    float SoundSystem::GetVolume() const
    {
        return mVolume;
    }

    void SoundSystem::SetListenerPosition(const Vec3F& position)
    {
        if (mReady)
            ma_engine_listener_set_position(mEngine, 0, position.x, position.y, position.z);
    }

    Vec3F SoundSystem::GetListenerPosition() const
    {
        if (!mReady)
            return Vec3F();

        ma_vec3f position = ma_engine_listener_get_position(mEngine, 0);
        return Vec3F(position.x, position.y, position.z);
    }

    void SoundSystem::SetListenerOrientation(const Vec3F& forward, const Vec3F& up)
    {
        if (!mReady)
            return;

        ma_engine_listener_set_direction(mEngine, 0, forward.x, forward.y, forward.z);
        ma_engine_listener_set_world_up(mEngine, 0, up.x, up.y, up.z);
    }

    bool SoundSystem::IsReady() const
    {
        return mReady;
    }

    ma_engine* SoundSystem::GetEngine() const
    {
        return mEngine;
    }

    void SoundSystem::RegisterPlayer(SoundPlayer* player)
    {
        mPlayers.Add(WeakRef<SoundPlayer>(player));
    }

    void SoundSystem::UnregisterPlayer(SoundPlayer* player)
    {
        mPlayers.RemoveAll([=](auto& x) { return x == player; });
    }

    void SoundSystem::RegisterListener(SoundListener* listener)
    {
        mListeners.Add(WeakRef<SoundListener>(listener));
    }

    void SoundSystem::UnregisterListener(SoundListener* listener)
    {
        mListeners.RemoveAll([=](auto& x) { return x == listener; });
    }

    Ref<SoundListener> SoundSystem::GetActiveListener() const
    {
        for (auto& listener : mListeners)
        {
            if (auto listenerRef = listener.Lock())
            {
                if (listenerRef->IsListening())
                    return listenerRef;
            }
        }

        return nullptr;
    }

    void SoundSystem::UpdateListenerFromCamera()
    {
        if (!Render::IsSingletonInitialzed())
            return;

        Camera camera = o2Render.GetCamera();
        Quat rotation = camera.GetRotation();

        SetListenerPosition(camera.GetPosition());
        SetListenerOrientation(rotation*Vec3F(0.0f, 0.0f, -1.0f), rotation*Vec3F(0.0f, 1.0f, 0.0f));
    }
}
