#include "o2/stdafx.h"
#include "AnimationStateGraphAsset.h"

namespace o2
{
    AnimationStateGraphAsset::AnimationStateGraphAsset()
    {}

    AnimationStateGraphAsset::AnimationStateGraphAsset(const AnimationStateGraphAsset& other)
    {
        RemoveAllStates();

        for (auto& state : other.mStates)
        {
            auto newState = state->CloneAsRef<AnimationGraphState>();
            AddState(newState);
        }
    }

    AnimationStateGraphAsset::~AnimationStateGraphAsset()
    {
        RemoveAllStates();
    }

    AnimationStateGraphAsset& AnimationStateGraphAsset::operator=(const AnimationStateGraphAsset& other)
    {
        RemoveAllStates();

        Asset::operator=(other);

        for (auto& state : other.mStates)
        {
            auto newState = state->CloneAsRef<AnimationGraphState>();
            AddState(newState);
        }

        return *this;
    }

    void AnimationStateGraphAsset::SetInitialState(const String& name)
    {
        mInitialState = name;
    }

    const String& AnimationStateGraphAsset::GetInitialState() const
    {
        return mInitialState;
    }

    void AnimationStateGraphAsset::AddState(const Ref<AnimationGraphState>& state)
    {
        mStates.Add(state);
        state->SetGraph(Ref(this));
    }

    Ref<AnimationGraphState> AnimationStateGraphAsset::AddState(const String& name, const Vector<String>& animations)
    {
        auto state = mmake<AnimationGraphState>();

        state->name = name;

        for (auto& animation : animations)
            state->AddAnimation(animation);

        AddState(state);
        return state;
    }

    void AnimationStateGraphAsset::RemoveState(const Ref<AnimationGraphState>& state)
    {
        mStates.Remove(state);
    }

    void AnimationStateGraphAsset::RemoveState(const String& name)
    {
        RemoveState(GetState(name));
    }

    void AnimationStateGraphAsset::RemoveAllStates()
    {
        mStates.Clear();
    }

    Ref<AnimationGraphState> AnimationStateGraphAsset::GetState(const String& name)
    {
        for (auto& state : mStates)
        {
            if (state->name == name)
                return state;
        }

        return nullptr;
    }

    Ref<AnimationGraphState> AnimationStateGraphAsset::GetState(const UID& uid)
    {
        for (auto& state : mStates)
        {
            if (state->GetUID() == uid)
                return state;
        }

        return nullptr;
    }

    const Vector<Ref<AnimationGraphState>>& AnimationStateGraphAsset::GetStates() const
    {
        return mStates;
    }

    Vector<Ref<AnimationGraphTransition>> AnimationStateGraphAsset::CalculatePath(const Ref<AnimationGraphState>& from, 
                                                                                      const Ref<AnimationGraphState>& to)
    {
        Vector<Ref<AnimationGraphTransition>> path;

        if (!from || !to)
            return path;

        Map<Ref<AnimationGraphState>, Ref<AnimationGraphTransition>> visited;
        Vector<Ref<AnimationGraphState>> current = { from };

        while (!current.empty())
        {
            Vector<Ref<AnimationGraphState>> next;

            for (auto& state : current)
            {
                if (state == to)
                {
                    auto currentState = state;
                    while (currentState != from)
                    {
                        auto passedStateTransition = visited[currentState];
                        path.Add(passedStateTransition);
                        currentState = passedStateTransition->GetSourceState();
                    }

					path.Reverse();
                    return path;
                }

                for (auto& transition : state->GetTransitions())
                {
                    auto destinationState = transition->GetDestinationState();
                    if (!visited.ContainsKey(destinationState))
                    {
                        visited[destinationState] = transition;
                        next.Add(destinationState);
                    }
                }
            }

            current = next;
        }

        return path;
    }

    Vector<String> AnimationStateGraphAsset::GetFileExtensions()
    {
        return { "asg" };
    }

    void AnimationStateGraphAsset::OnDeserialized(const DataValue& node)
    {
        Asset::OnDeserialized(node);

        for (auto& state : mStates)
            state->SetGraph(Ref(this));
    }

}

DECLARE_TEMPLATE_CLASS(o2::AssetWithDefaultMeta<o2::AnimationStateGraphAsset>);
DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::AnimationStateGraphAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AnimationStateGraphAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::AssetWithDefaultMeta<o2::AnimationStateGraphAsset>>);
// --- META ---

DECLARE_CLASS(o2::AnimationStateGraphAsset, o2__AnimationStateGraphAsset);
// --- END META ---
