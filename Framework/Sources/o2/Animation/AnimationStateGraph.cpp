#include "o2/stdafx.h"
#include "AnimationStateGraph.h"

#include "o2/Assets/Types/AnimationStateGraphAsset.h"

namespace o2
{
    void AnimationGraphState::Animation::OnChanged()
    {
        if (auto state = mState.Lock())
            state->OnChanged();

        onChanged();
    }

    void AnimationGraphState::Animation::SetName(const String& name)
    {
        mName = name;
        OnChanged();
    }

    const String& AnimationGraphState::Animation::GetName() const
    {
        return mName;
    }

    void AnimationGraphState::Animation::SetWeight(float weight)
    {
        mWeight = weight;   
        OnChanged();
    }

    float AnimationGraphState::Animation::GetWeight() const
    {
        return mWeight;
    }

    void AnimationGraphState::Animation::SetState(const Ref<AnimationGraphState>& state)
    {
        mState = state;
    }

    void AnimationGraphTransition::SetDestinationState(const Ref<AnimationGraphState>& state)
    {
        mDestinationState = state->GetUID();
        mDestinationStateRef = state;
        OnChanged();
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

	void AnimationGraphTransition::OnChanged()
	{
		if (auto sourceState = mSourceStateRef.Lock())
		{
			if (auto graph = sourceState->GetGraph())
				graph->SetDirty();
		}

        onChanged();
	}

	const String& AnimationGraphState::GetName() const
	{
        if (mAnimations.IsEmpty())
            return String::empty;

		return mAnimations[0]->GetName();
	}

	UID AnimationGraphState::GetUID() const
    {
        return mUID;
    }

    Ref<AnimationGraphState::Animation> AnimationGraphState::GetAnimation(const String& name)
    {
        for (auto& animation : mAnimations)
        {
            if (animation->GetName() == name)
                return animation;
        }

        return nullptr;
    }

    Ref<AnimationGraphState::Animation> AnimationGraphState::AddAnimation(const String& name)
    {
        auto animation = mmake<Animation>();
        animation->SetName(name);
        animation->SetState(Ref(this));
        mAnimations.Add(animation);
        
        OnChanged();

        return animation;
    }

    void AnimationGraphState::RemoveAnimation(const String& name)
    {
        RemoveAnimation(GetAnimation(name));
    }

    void AnimationGraphState::RemoveAnimation(const Ref<Animation>& animation)
    {
		mAnimations.Remove(animation);
		OnChanged();
    }

	void AnimationGraphState::SetAnimations(const Vector<Ref<Animation>>& animations)
	{
		mAnimations = animations;
        for (auto& animation : mAnimations)
            animation->SetState(Ref(this));

		OnChanged();
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
		mTransitions.Add(transition);

		OnChanged();

        return transition;
    }

    void AnimationGraphState::RemoveTransition(const Ref<AnimationGraphTransition>& transition)
    {
		mTransitions.Remove(transition);
		OnChanged();
    }

    const Vector<Ref<AnimationGraphTransition>>& AnimationGraphState::GetTransitions() const
    {
        return mTransitions;
    }

	Ref<AnimationStateGraphAsset> AnimationGraphState::GetGraph() const
	{
		return mGraph.Lock();
	}

	void AnimationGraphState::SetPosition(const Vec2F& position)
	{
		mPosition = position;
		OnChanged();
	}

	Vec2F AnimationGraphState::GetPosition() const
	{
		return mPosition;
	}

	void AnimationGraphState::ReinitStatesAndTransitions()
    {
        for (auto& animation : mAnimations)
        {
            if (animation)
                animation->SetState(Ref(this));
        }

        for (auto& transition : mTransitions)
        {
            if (transition)
                transition->SetState(Ref(this));
        }
    }

    void AnimationGraphState::SetGraph(const Ref<AnimationStateGraphAsset>& graph)
    {
        mGraph = graph;

        ReinitStatesAndTransitions();
    }

	void AnimationGraphState::OnChanged()
	{
		if (auto graph = mGraph.Lock())
			graph->SetDirty();

        onChanged();
	}

}
// --- META ---

DECLARE_CLASS(o2::AnimationGraphTransition, o2__AnimationGraphTransition);

DECLARE_CLASS(o2::AnimationGraphState, o2__AnimationGraphState);

DECLARE_CLASS(o2::AnimationGraphState::Animation, o2__AnimationGraphState__Animation);
// --- END META ---
