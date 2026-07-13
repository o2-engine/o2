#pragma once

#include "o2/Utils/Math/Vector3.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"

// Sound system access macros
#define o2Sounds o2::SoundSystem::Instance()

struct ma_context;
struct ma_engine;

namespace o2
{
    FORWARD_CLASS_REF(LogStream);

    class SoundListener;
    class SoundPlayer;

    // ---------------------------------------------------------------------------------
    // Sound system. Manages audio device, mixing and spatial listener placed at camera
    // ---------------------------------------------------------------------------------
    class SoundSystem : public Singleton<SoundSystem>
    {
    public:
        // Default constructor. Initializes audio engine, in headless mode uses null backend
        SoundSystem(RefCounter* refCounter);

        // Destructor. Releases all players backends and audio engine
        ~SoundSystem();

        // Updates listener from current render camera and players scrub state
        void Update(float dt);

        // Sets master volume
        void SetVolume(float volume);

        // Returns master volume
        float GetVolume() const;

        // Sets spatial listener position
        void SetListenerPosition(const Vec3F& position);

        // Returns spatial listener position
        Vec3F GetListenerPosition() const;

        // Sets spatial listener orientation
        void SetListenerOrientation(const Vec3F& forward, const Vec3F& up);

        // Registers sound player for scrub playback tracking
        void RegisterPlayer(SoundPlayer* player);

        // Unregisters sound player
        void UnregisterPlayer(SoundPlayer* player);

        // Registers listener object; the first listening one drives the listener point
        void RegisterListener(SoundListener* listener);

        // Unregisters listener object
        void UnregisterListener(SoundListener* listener);

        // Returns listener object driving the listener point, null when camera is used
        Ref<SoundListener> GetActiveListener() const;

        // Returns true if audio engine initialized successfully
        bool IsReady() const;

        // Returns backend audio engine
        ma_engine* GetEngine() const;

    protected:
        Ref<LogStream> mLog; // Sound log stream

        ma_context* mContext = nullptr; // Backend context, created for null backend fallback
        ma_engine*  mEngine = nullptr;  // Backend audio engine

        bool mReady = false; // True if audio engine initialized successfully

        float mVolume = 1.0f; // Master volume

        Vector<WeakRef<SoundPlayer>>   mPlayers;   // All registered sound players
        Vector<WeakRef<SoundListener>> mListeners; // All registered listener objects

    protected:
        // Places listener at current render camera
        void UpdateListenerFromCamera();
    };
}
