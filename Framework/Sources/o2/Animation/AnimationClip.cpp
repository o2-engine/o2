#include "o2/stdafx.h"
#include "AnimationClip.h"

#include "o2/Animation/Tracks/AnimationTrack.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    AnimationClip::AnimationClip(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    AnimationClip::AnimationClip(RefCounter* refCounter, const AnimationClip& other):
        RefCounterable(refCounter), loop(this), duration(this)
    {
        for (auto& track : other.mTracks)
        {
            Ref<IAnimationTrack> newTrack = track->CloneAsRef<IAnimationTrack>();
            mTracks.Add(newTrack);
            OnTrackAdded(newTrack);
        }

        mLoop = other.mLoop;

        RecalculateDuration();
    }

    AnimationClip::~AnimationClip()
    {
        Clear();
    }

    AnimationClip& AnimationClip::operator=(const AnimationClip& other)
    {
        Clear();

        for (auto& track : other.mTracks)
        {
            Ref<IAnimationTrack> newTrack = track->CloneAsRef<IAnimationTrack>();
            mTracks.Add(newTrack);
            OnTrackAdded(newTrack);
        }

        mLoop = other.mLoop;

        RecalculateDuration();

        return *this;
    }

    void AnimationClip::Clear()
    {
        for (auto& track : mTracks)
            track->onKeysChanged -= THIS_FUNC(OnTrackChanged);

        mTracks.Clear();
    }

    float AnimationClip::GetDuration() const
    {
        return mDuration;
    }

    void AnimationClip::SetLoop(Loop loop)
    {
        mLoop = loop;
    }

    Loop AnimationClip::GetLoop() const
    {
        return mLoop;
    }

    Vector<Ref<IAnimationTrack>>& AnimationClip::GetTracks()
    {
        return mTracks;
    }

    const Vector<Ref<IAnimationTrack>>& AnimationClip::GetTracks() const
    {
        return mTracks;
    }

    bool AnimationClip::ContainsTrack(const String& path) const
    {
        for (auto& track : mTracks)
        {
            if (track->path == path)
                return true;
        }

        return false;
    }

	Ref<IAnimationTrack> AnimationClip::GetTrack(const String& path)
	{
        for (auto& track : mTracks)
        {
            if (track->path == path)
                return track;
		}

		return nullptr;
	}

	Ref<IAnimationTrack> AnimationClip::AddTrack(const String& path, const Type& type)
    {
        const Type* animType = nullptr;

        if (type.IsBasedOn(TypeOf(IAnimation)))
            animType = &TypeOf(AnimationSubTrack);
        else
        {
            auto animTypeName = "o2::AnimationTrack<" + type.GetName() + ">";
            animType = dynamic_cast<const ObjectType*>(o2Reflection.GetType(animTypeName));
        }

        if (!animType)
        {
            o2Debug.LogWarning("Can't create Animation track: can't find animation type " + type.GetName());
            return nullptr;
        }

        auto track = DynamicCast<IAnimationTrack>(animType->CreateSampleRef());
        track->path = path;
        track->onKeysChanged += THIS_FUNC(OnTrackChanged);

        mTracks.Add(track);
        OnTrackAdded(track);

        return track;
    }

    void AnimationClip::RemoveTrack(const String& path)
    {
        for (auto& track : mTracks)
        {
            if (track->path == path)
            {
                onTrackRemove(track);

                mTracks.Remove(track);

                onChanged();
                return;
            }
        }
    }

    void AnimationClip::OnTrackChanged()
    {
        RecalculateDuration();

        onChanged();
    }

    void AnimationClip::RecalculateDuration()
    {
        float lastDuration = mDuration;
        mDuration = 0.0f;

        for (auto& track : mTracks)
            mDuration = Math::Max(mDuration, track->GetDuration());

        if (!Math::Equals(lastDuration, mDuration))
            onDurationChange(mDuration);
    }

    void AnimationClip::OnDeserialized(const DataValue& node)
    {
        for (auto& track : mTracks)
            track->onKeysChanged += THIS_FUNC(OnTrackChanged);

        OnTrackChanged();
    }

    void AnimationClip::OnTrackAdded(const Ref<IAnimationTrack>& track)
    {
        track->onKeysChanged += THIS_FUNC(OnTrackChanged);
        track->mOwnerClip = WeakRef(this);

        onTrackAdded(track);
        onChanged();
    }
}
// --- META ---

DECLARE_CLASS(o2::AnimationClip, o2__AnimationClip);
// --- END META ---
