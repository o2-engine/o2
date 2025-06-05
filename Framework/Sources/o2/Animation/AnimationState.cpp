#include "o2/stdafx.h"
#include "AnimationState.h"

#include "o2/Scene/Components/AnimationComponent.h"

namespace o2
{
    FORWARD_REF_IMPL(AnimationComponent);

    AnimationState::AnimationState(const String& name):
        IAnimationState(name)
    {
        player->onTrackPlayerAdded = [&](auto track) { OnTrackPlayerAdded(track); };
        player->onTrackPlayerRemove = [&](auto track) { OnTrackPlayerRemove(track); };
    }

    void AnimationState::Update(float dt)
    {
        if (mAnimation)
            player->Update(dt);
    }

    IAnimation& AnimationState::GetPlayer()
    {
        return *player;
    }

	float AnimationState::GetDuration() const
	{
		return mAnimation ? mAnimation->animation->GetDuration() : 0.0f;
	}

	void AnimationState::SetWeight(float weight)
    {
        mWeight = weight;
    }

    float AnimationState::GetWeight() const
    {
        return mWeight;
    }

    void AnimationState::SetLooped(bool looped)
    {
        Loop loop = looped ? Loop::Repeat : Loop::None;

        player->SetLoop(loop);

        if (mAnimation)
			mAnimation->animation->SetLoop(loop);
    }

    bool AnimationState::IsLooped() const
    {
        return player->GetLoop() == Loop::Repeat;
    }

    void AnimationState::SetAnimation(const AssetRef<AnimationAsset>& animationAsset)
    {
        mAnimation = animationAsset;
        player->SetClip(mAnimation ? mAnimation->animation : nullptr);
    }

    const AssetRef<AnimationAsset>& AnimationState::GetAnimation() const
    {
        return mAnimation;
    }

    void AnimationState::Register(const Ref<AnimationComponent>& owner)
    {
        IAnimationState::Register(owner);

        player->SetTarget(mOwner.Lock()->GetActor().Get());
        player->SetPlaying(autoPlay);
        player->mAnimationState = Ref(this);

        for (auto& trackPlayer : player->mTrackPlayers)
            trackPlayer->RegMixer(Ref(this), trackPlayer->GetTrack()->path);
    }

    void AnimationState::Unregister()
    {
        auto owner = mOwner.Lock();

        for (auto& trackPlayer : player->mTrackPlayers)
            owner->UnregTrack(trackPlayer, trackPlayer->GetTrack()->path);
    }

    void AnimationState::OnAnimationChanged()
    {
        player->SetClip(mAnimation ? mAnimation->animation : nullptr);
    }

    void AnimationState::OnTrackPlayerAdded(const Ref<IAnimationTrack::IPlayer>& trackPlayer)
    {
        if (mOwner)
            mOwner.Lock()->OnStateAnimationTrackAdded(Ref(this), trackPlayer);
    }

    void AnimationState::OnTrackPlayerRemove(const Ref<IAnimationTrack::IPlayer>& trackPlayer)
    {
        if (mOwner)
            mOwner.Lock()->OnStateAnimationTrackRemoved(Ref(this), trackPlayer);
    }

    void AnimationState::OnDeserialized(const DataValue& node)
    {
        player->SetClip(mAnimation ? mAnimation->animation : nullptr);
    }

    IAnimationState::IAnimationState(const String& name) :
        name(name)
    {}

    void IAnimationState::Update(float dt)
    {}

    IAnimation& IAnimationState::GetPlayer()
    {
        static IAnimation empty;
        return empty;
    }

	float IAnimationState::GetDuration() const
	{
		return 0.0f;
	}

	void IAnimationState::SetWeight(float weight)
    {}

    float IAnimationState::GetWeight() const
    {
        return 1.0f;
    }

    void IAnimationState::SetLooped(bool looped)
    {}

    bool IAnimationState::IsLooped() const
    {
        return false;
    }

    void IAnimationState::Register(const Ref<AnimationComponent>& owner)
    {
        mOwner = owner;
    }

    void IAnimationState::Unregister()
    {}
}
// --- META ---

DECLARE_CLASS(o2::IAnimationState, o2__IAnimationState);

DECLARE_CLASS(o2::AnimationState, o2__AnimationState);
// --- END META ---
