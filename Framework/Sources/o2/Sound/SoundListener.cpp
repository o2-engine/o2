#include "o2/stdafx.h"
#include "SoundListener.h"

#include "o2/Sound/SoundSystem.h"

namespace o2
{
    SoundListener::SoundListener():
        SoundListener(nullptr)
    {}

    SoundListener::SoundListener(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    SoundListener::SoundListener(const SoundListener& other):
        SoundListener(nullptr, other)
    {}

    SoundListener::SoundListener(RefCounter* refCounter, const SoundListener& other):
        RefCounterable(refCounter), mPosition(other.mPosition), mForward(other.mForward), mUp(other.mUp)
    {}

    SoundListener::~SoundListener()
    {
        if (SoundSystem::IsSingletonInitialzed())
            o2Sounds.UnregisterListener(this);
    }

    void SoundListener::PostRefConstruct()
    {
        if (SoundSystem::IsSingletonInitialzed())
            o2Sounds.RegisterListener(this);
    }

    SoundListener& SoundListener::operator=(const SoundListener& other)
    {
        mPosition = other.mPosition;
        mForward = other.mForward;
        mUp = other.mUp;
        return *this;
    }

    void SoundListener::SetPosition(const Vec3F& position)
    {
        mPosition = position;
    }

    Vec3F SoundListener::GetPosition() const
    {
        return mPosition;
    }

    void SoundListener::SetOrientation(const Vec3F& forward, const Vec3F& up)
    {
        mForward = forward;
        mUp = up;
    }

    Vec3F SoundListener::GetForward() const
    {
        return mForward;
    }

    Vec3F SoundListener::GetUp() const
    {
        return mUp;
    }

    bool SoundListener::IsListening() const
    {
        return true;
    }

    bool SoundListener::IsActiveListener() const
    {
        return SoundSystem::IsSingletonInitialzed() && o2Sounds.GetActiveListener().Get() == this;
    }
}
// --- META ---

DECLARE_CLASS(o2::SoundListener, o2__SoundListener);
// --- END META ---
