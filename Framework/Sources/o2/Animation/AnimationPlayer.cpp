#include "o2/stdafx.h"
#include "AnimationPlayer.h"

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/AnimationState.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    FORWARD_REF_IMPL(AnimationClip);
    FORWARD_REF_IMPL(AnimationState);

    AnimationPlayer::AnimationPlayer(RefCounter* refCounter, IObject* target /*= nullptr*/, const Ref<AnimationClip>& clip /*= nullptr*/):
        IAnimation(refCounter), mTarget(target), mClip(clip)
    {
        SetTarget(target);
        SetClip(clip);
    }

    AnimationPlayer::~AnimationPlayer()
    {
        mTrackPlayers.Clear();

        auto tmp = mClip;
        SetClip(nullptr);
    }

    void AnimationPlayer::SetTarget(IObject* target, bool errors /*= true*/)
    {
        mTarget = target;
        BindTracks(errors);
    }

    IObject* AnimationPlayer::GetTarget() const
    {
        return mTarget;
    }

    void AnimationPlayer::SetClip(const Ref<AnimationClip>& clip)
    {
        if (mClip)
        {
            mClip->onTrackAdded -= THIS_FUNC(OnClipTrackAdded);
            mClip->onTrackRemove -= THIS_FUNC(OnClipTrackRemove);
            mClip->onDurationChange -= THIS_FUNC(OnClipDurationChanged);
        }

        mClip = clip;

        if (mClip)
        {
            mClip->onTrackAdded += THIS_FUNC(OnClipTrackAdded);
            mClip->onTrackRemove += THIS_FUNC(OnClipTrackRemove);
            mClip->onDurationChange += THIS_FUNC(OnClipDurationChanged);

			mLoop = mClip->mLoop;
        }

        BindTracks(true);
    }

    const Ref<AnimationClip>& AnimationPlayer::GetClip() const
    {
        return mClip;
    }

    const Vector<Ref<IAnimationTrack::IPlayer>>& AnimationPlayer::GetTrackPlayers() const
    {
        return mTrackPlayers;
    }

    void AnimationPlayer::BindTracks(bool errors)
    {
        for (auto& player : mTrackPlayers)
            onTrackPlayerRemove(player);

		mTrackPlayers.Clear();

        if (!mClip)
            return;

		mLoop = mClip->mLoop;
		mDuration = mClip->GetDuration();
		mBeginTime = 0.0f;
		mEndTime = mDuration;

        if (!mTarget)
            return;

        const ObjectType* type = dynamic_cast<const ObjectType*>(&mTarget->GetType());
        void* castedTarget = type->DynamicCastFromIObject(mTarget);

        for (auto& track : mClip->mTracks)
            BindTrack(type, castedTarget, track, errors);
    }

    void AnimationPlayer::BindTrack(const ObjectType* type, void* castedTarget, const Ref<IAnimationTrack>& track, bool errors)
    {
        const FieldInfo* fieldInfo = nullptr;
        auto targetPtr = type->GetFieldPtr(castedTarget, track->path, fieldInfo);

        if (!fieldInfo)
        {
            if (errors)
                o2Debug.LogWarning("Can't find '" + track->path + "' for animating");
        }
        else
        {
            auto trackPlayer = track->CreatePlayer();
            trackPlayer->SetTrack(track);
            trackPlayer->mOwnerPlayer = WeakRef(this);

            targetPtr = trackPlayer->AdjustTargetType(targetPtr, *fieldInfo->GetType());

            if (fieldInfo->GetType()->GetUsage() == Type::Usage::Property)
                trackPlayer->SetTargetProxy(fieldInfo->GetType()->GetValueProxy(targetPtr));
            else
                trackPlayer->SetTargetVoid(targetPtr);

            mTrackPlayers.Add(trackPlayer);

            onTrackPlayerAdded(trackPlayer);
        }
    }

    void AnimationPlayer::OnClipTrackAdded(const Ref<IAnimationTrack>& track)
    {
        if (!mTarget)
            return;

        const ObjectType* type = dynamic_cast<const ObjectType*>(&mTarget->GetType());
        void* castedTarget = type->DynamicCastFromIObject(mTarget);

        BindTrack(type, castedTarget, track, false);
    }

    void AnimationPlayer::OnClipTrackRemove(const Ref<IAnimationTrack>& track)
    {
        mTrackPlayers.RemoveFirst([track, this](auto& x) { return x->GetTrack() == track; onTrackPlayerRemove(x); });
    }

    void AnimationPlayer::OnClipDurationChanged(float duration)
    {
        mDuration = duration;
        mEndTime = duration;
    }

	void AnimationPlayer::OnPlay()
	{
        for (auto& track : mTrackPlayers)
			track->OnPlay();
	}

	void AnimationPlayer::Evaluate()
    {
        for (auto& trackPlayer : mTrackPlayers)
            trackPlayer->ForceSetTime(mInDurationTime, mDuration);
    }
}
// --- META ---

DECLARE_CLASS(o2::AnimationPlayer, o2__AnimationPlayer);
// --- END META ---
