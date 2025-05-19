#include "o2/stdafx.h"
#include "SpineComponent.h"

namespace o2
{
    SpineComponent::SpineComponent()
    {}

    SpineComponent::SpineComponent(const SpineComponent& other)
    {}

    SpineComponent::~SpineComponent()
    {}

    void SpineComponent::SetSpineAsset(const AssetRef<SpineAsset>& spineAsset)
    {
        mSpineAsset = spineAsset;
        LoadSpine();
        CreateAnimationStates();
    }

    const AssetRef<SpineAsset>& SpineComponent::GetSpineAsset() const
    {
        return mSpineAsset;
    }

    String SpineComponent::GetName()
    {
        return "Spine";
    }

    String SpineComponent::GetCategory()
    {
        return "Animation";
    }

    String SpineComponent::GetIcon()
    {
        return "ui/UI4_animation_component.png";
    }

    SpineComponent& SpineComponent::operator=(const SpineComponent& other)
    {
        AnimationComponent::operator=(other);

        mSpineAsset = other.mSpineAsset;
        LoadSpine();

        return *this;
    }

    void SpineComponent::LoadSpine()
    {
        if (!mSpineAsset)
            return;

        mSpineRenderer = mmake<Spine>(mSpineAsset);
    }

    void SpineComponent::CreateAnimationStates()
    {
        auto names = mSpineRenderer->GetAnimationNames();
        for (auto& name : names)
        {
            auto state = AddState(mmake<AnimationState>(name));
            state->autoPlay = mStates.Count() == 1;
        }
    }

    void SpineComponent::OnInitialized()
    {
        LoadSpine();
        AnimationComponent::OnInitialized();
    }

    void SpineComponent::OnTransformUpdated()
    {
        AnimationComponent::OnTransformUpdated();

        if (mSpineRenderer)
            mSpineRenderer->transform = mOwner.Lock()->transform->worldNonSizedBasis;
    }

    void SpineComponent::OnUpdate(float dt)
    {
        AnimationComponent::OnUpdate(dt);

        if (mSpineRenderer)
            mSpineRenderer->Update(dt);
    }

    void SpineComponent::OnDraw()
    {
        AnimationComponent::OnDraw();

        if (mSpineRenderer)
            mSpineRenderer->Draw();
    }

    SpineComponent::AnimationState::AnimationState(const String& name):
        IAnimationState(name)
    {}

    void SpineComponent::AnimationState::Update(float dt)
    {
        if (mTrack)
            mTrack->Update(dt);
    }

    IAnimation& SpineComponent::AnimationState::GetPlayer()
    {
        if (mTrack)
            return *mTrack;

        return IAnimationState::GetPlayer();
    }

	float SpineComponent::AnimationState::GetDuration() const
	{
		if (mTrack)
			return mTrack->GetDuration();

        return 0.0f;
	}

	void SpineComponent::AnimationState::SetWeight(float weight)
    {
        mWeight = weight;

        if (mTrack)
            mTrack->SetWeight(weight);
    }

    float SpineComponent::AnimationState::GetWeight() const
    {
        return mWeight;
    }

    void SpineComponent::AnimationState::SetLooped(bool looped)
    {
        mLooped = looped;
    
        if (mTrack)
            mTrack->SetLoop(looped ? Loop::Repeat : Loop::None);
    }

    bool SpineComponent::AnimationState::IsLooped() const
    {
        return mLooped;
    }

    void SpineComponent::AnimationState::Register(const Ref<AnimationComponent>& owner)
    {
        IAnimationState::Register(owner);

        if (auto spineComponent = DynamicCast<SpineComponent>(owner))
        {
            if (spineComponent->mSpineRenderer)
            {
                mTrack = spineComponent->mSpineRenderer->GetTrack(name);
                mTrack->SetLoop(looped ? Loop::Repeat : Loop::None);
                mTrack->SetWeight(mWeight);
            }
        }
    }

    void SpineComponent::AnimationState::Unregister()
    {
        IAnimationState::Unregister();

        mTrack = nullptr;
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::SpineComponent>);
// --- META ---

DECLARE_CLASS(o2::SpineComponent, o2__SpineComponent);

DECLARE_CLASS(o2::SpineComponent::AnimationState, o2__SpineComponent__AnimationState);
// --- END META ---
