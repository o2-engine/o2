#include "o2/stdafx.h"
#include "AnimationState.h"

#include "o2/Scene/Components/AnimationComponent.h"

namespace o2
{
    FORWARD_REF_IMPL(AnimationComponent);

	IAnimationState::IAnimationState(const String& name) :
		name(name)
	{}

	IAnimationState::IAnimationState(const IAnimationState& other) :
		name(other.name), autoPlay(other.autoPlay)
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

	bool IAnimationState::IsInEditMode() const
	{
		return false;
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

    AnimationState::AnimationState(const String& name):
        IAnimationState(name)
    {
        player->onTrackPlayerAdded = [&](auto track) { OnTrackPlayerAdded(track); };
        player->onTrackPlayerRemove = [&](auto track) { OnTrackPlayerRemove(track); };
    }

	AnimationState::AnimationState(const AnimationState& other):
        IAnimationState(other.name), mAnimation(other.mAnimation), mWeight(other.mWeight)
    {
        player->onTrackPlayerAdded = [&](auto track) { OnTrackPlayerAdded(track); };
        player->onTrackPlayerRemove = [&](auto track) { OnTrackPlayerRemove(track); };
	}

	void AnimationState::Update(float dt)
    {
        if (mInEditMode)
			return;

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

	RefCounter* AnimationState::GetRefCounter() const
	{
        return IAnimationState::GetRefCounter();
	}

	void AnimationState::Register(const Ref<AnimationComponent>& owner)
	{
		IAnimationState::Register(owner);

        auto actor = mOwner.Lock()->GetActor();
        if (!actor)
            return;

		player->SetClip(mAnimation ? mAnimation->animation : nullptr);
        player->SetTarget(static_cast<IObject*>(static_cast<ActorBase*>(actor.Get())));
        player->mAnimationState = Ref(this);
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
        if (auto owner = mOwner.Lock())
            owner->OnStateAnimationTrackAdded(Ref(this), trackPlayer);
    }

    void AnimationState::OnTrackPlayerRemove(const Ref<IAnimationTrack::IPlayer>& trackPlayer)
    {
        if (auto owner = mOwner.Lock())
            owner->OnStateAnimationTrackRemoved(Ref(this), trackPlayer);
    }

    void AnimationState::OnDeserialized(const DataValue& node)
    {
        player->SetClip(mAnimation ? mAnimation->animation : nullptr);
	}

	bool AnimationState::IsInEditMode() const
	{
		return mInEditMode;
	}

	void AnimationState::BeginPreview()
	{
        mInEditMode = true;
	}

	void AnimationState::EndPreview()
	{
		mInEditMode = false;
	}

	Ref<Actor> AnimationState::GetPreviewActor() const
	{
        if (auto owner = mOwner.Lock())
            return owner->GetActor();
        
        return nullptr;
	}

	Ref<IAnimation> AnimationState::GetPreviewPlayer() const
	{
		return player;
	}

}
// --- META ---

DECLARE_CLASS(o2::IAnimationState, o2__IAnimationState);

DECLARE_CLASS(o2::AnimationState, o2__AnimationState);
// --- END META ---
