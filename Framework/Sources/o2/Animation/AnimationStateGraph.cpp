#include "o2/stdafx.h"
#include "AnimationStateGraph.h"

#include "o2/Assets/Types/AnimationStateGraphAsset.h"

namespace o2
{
    void AnimationGraphTransition::SetDestinationState(const Ref<AnimationGraphState>& state)
    {
        mDestinationState = state->GetUID();
        mDestinationStateRef = state;
    }

    Ref<AnimationGraphState> AnimationGraphTransition::GetDestinationState() const
    {
        return mDestinationStateRef.Lock();
    }

    Ref<AnimationGraphState> AnimationGraphTransition::GetSourceState() const
    {
        return mSourceStateRef.Lock();
    }

    void AnimationGraphTransition::SetState(const Ref<AnimationGraphState>& state)
    {
        mSourceStateRef = state;

        if (state && state->mGraph)
            mDestinationStateRef = state->mGraph.Lock()->GetState(mDestinationState);
    }

    UID AnimationGraphState::GetUID() const
    {
        return mUID;
    }

    Ref<AnimationGraphState::Animation> AnimationGraphState::GetAnimation(const String& name)
    {
        for (auto& animation : mAnimations)
        {
            if (animation->name == name)
                return animation;
        }

        return nullptr;
    }

    Ref<AnimationGraphState::Animation> AnimationGraphState::AddAnimation(const String& name)
    {
        auto animation = mmake<Animation>();
        animation->name = name;
        mAnimations.Add(animation);

        return animation;
    }

    void AnimationGraphState::RemoveAnimation(const String& name)
    {
        RemoveAnimation(GetAnimation(name));
    }

    void AnimationGraphState::RemoveAnimation(const Ref<Animation>& animation)
    {
        mAnimations.Remove(animation);
    }

    const Vector<Ref<AnimationGraphState::Animation>>& AnimationGraphState::GetAnimations() const
    {
        return mAnimations;
    }

    Ref<AnimationGraphTransition> AnimationGraphState::AddTransition(const Ref<AnimationGraphState>& destinationState)
    {
        auto transition = mmake<AnimationGraphTransition>();
        transition->SetDestinationState(destinationState);
        transition->SetState(Ref(this));
        return transition;
    }

    void AnimationGraphState::RemoveTransition(const Ref<AnimationGraphTransition>& transition)
    {
        mTransitions.Remove(transition);
    }

    const Vector<Ref<AnimationGraphTransition>>& AnimationGraphState::GetTransitions() const
    {
        return mTransitions;
    }

	void AnimationGraphState::SetPosition(const Vec2F& position)
	{
		mPosition = position;
	}

	Vec2F AnimationGraphState::GetPosition() const
	{
		return mPosition;
	}

	void AnimationGraphState::ReinitTransitions()
    {
        for (auto& transition : mTransitions)
        {
            if (transition)
                transition->SetState(Ref(this));
        }
    }

    void AnimationGraphState::SetGraph(const Ref<AnimationStateGraphAsset>& graph)
    {
        mGraph = graph;

        ReinitTransitions();
    }
}
// --- META ---

DECLARE_CLASS(o2::AnimationGraphTransition, o2__AnimationGraphTransition);

DECLARE_CLASS(o2::AnimationGraphState, o2__AnimationGraphState);

DECLARE_CLASS(o2::AnimationGraphState::Animation, o2__AnimationGraphState__Animation);
// --- END META ---
