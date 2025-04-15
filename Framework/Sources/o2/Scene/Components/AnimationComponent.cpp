#include "o2/stdafx.h"
#include "AnimationComponent.h"

#include "o2/Animation/Tracks/AnimationTrack.h"

namespace o2
{
    AnimationComponent::AnimationComponent()
    {}

    AnimationComponent::AnimationComponent(RefCounter* refCounter, const AnimationComponent& other):
        Component(refCounter, other)
    {
        for (auto& state : other.mStates)
            AddState(state->CloneAsRef<AnimationState>());
    }

    AnimationComponent::~AnimationComponent()
    {
        RemoveAllStates();
    }

    AnimationComponent& AnimationComponent::operator=(const AnimationComponent& other)
    {
        RemoveAllStates();

        for (auto& state : other.mStates)
            AddState(state->CloneAsRef<AnimationState>());

        return *this;
    }

    void AnimationComponent::OnUpdate(float dt)
    {
        if (mInEditMode)
            return;

        for (auto& state : mStates)
            state->Update(dt);

        for (auto& val : mValues)
            val->Update();

        if (mBlend.time > 0)
            mBlend.Update(dt);
    }

    Ref<IAnimationState> AnimationComponent::AddState(const Ref<IAnimationState>& state)
    {
        state->Register(Ref(this));
        mStates.Add(state);

        return state;
    }

    Ref<IAnimationState> AnimationComponent::AddState(const String& name, const Ref<AnimationClip>& animation, const AnimationMask& mask,
                                                     float weight)
    {
        Ref<AnimationState> res = mmake<AnimationState>(name);
        res->mAnimation = mmake<AnimationAsset>(animation);
        res->mask = mask;
        res->mWeight = weight;
        return AddState(res);
    }

    Ref<IAnimationState> AnimationComponent::AddState(const String& name)
    {
        Ref<AnimationState> res = mmake<AnimationState>(name);
        return AddState(res);
    }

    void AnimationComponent::RemoveState(const Ref<IAnimationState>& state)
    {
        state->Unregister();
        mStates.Remove(state);
    }

    void AnimationComponent::RemoveState(const String& name)
    {
        RemoveState(GetState(name));
    }

    void AnimationComponent::RemoveAllStates()
    {
        mStates.Clear();
        mValues.Clear();
    }

    Ref<IAnimationState> AnimationComponent::GetState(const String& name)
    {
        for (auto& state : mStates)
        {
            if (state->name == name)
                return state;
        }

        return nullptr;
    }

    const Vector<Ref<IAnimationState>>& AnimationComponent::GetStates() const
    {
        return mStates;
    }

    Map<String, Ref<IAnimationState>> AnimationComponent::GetAllStates() const
    {
        Map<String, Ref<IAnimationState>> result;

        for (auto& state : mStates)
            result[state->name] = state;

        return result;
    }

    Vector<String> AnimationComponent::GetStatesNames() const
    {
        Vector<String> result;
        
        for (auto& state : mStates)
            result.Add(state->name);
            
        return result;
    }

    Ref<IAnimationState> AnimationComponent::Play(const Ref<AnimationClip>& animation, const String& name)
    {
        auto state = AddState(name, animation, AnimationMask(), 1.0f);
        state->GetPlayer().Play();
        return state;
    }

    Ref<IAnimationState> AnimationComponent::Play(const Ref<AnimationClip>& animation)
    {
        auto state = AddState("unknown", animation, AnimationMask(), 1.0f);
        state->GetPlayer().Play();
        return state;
    }

    Ref<IAnimationState> AnimationComponent::Play(const String& name)
    {
        auto state = GetState(name);
        if (!state)
        {
            o2Debug.LogWarning("Can't play animation: " + name);
            return nullptr;
        }

        state->GetPlayer().Play();

        return state;
    }

    Ref<IAnimationState> AnimationComponent::BlendTo(const Ref<AnimationClip>& animation, const String& name, float duration /*= 1.0f*/)
    {
        auto state = AddState(name, animation, AnimationMask(), 1.0f);
        return BlendTo(state, duration);
    }

    Ref<IAnimationState> AnimationComponent::BlendTo(const Ref<AnimationClip>& animation, float duration /*= 1.0f*/)
    {
        auto state = AddState("unknown", animation, AnimationMask(), 1.0f);
        return BlendTo(state, duration);
    }

    Ref<IAnimationState> AnimationComponent::BlendTo(const String& name, float duration /*= 1.0f*/)
    {
        auto state = GetState(name);
        if (!state)
        {
            o2Debug.LogWarning("Can't blend animation: " + name);
            return nullptr;
        }
        return BlendTo(state, duration);
    }

    Ref<IAnimationState> AnimationComponent::BlendTo(const Ref<IAnimationState>& state, float duration /*= 1.0f*/)
    {
        mBlend.blendOffStates.Clear();

        for (auto& state : mStates)
        {
            if (state->GetPlayer().IsPlaying())
                mBlend.blendOffStates.Add(state);
        }

        mBlend.blendOnState = state;
        mBlend.duration = duration;
        mBlend.time = duration;

        state->GetPlayer().Play();

        return state;
    }

    void AnimationComponent::Stop(const String& animationName)
    {
        Ref<IAnimationState> state = GetState(animationName);
        if (!state)
        {
            o2Debug.LogWarning("Can't stop animation: " + animationName);
            return;
        }

        state->GetPlayer().Stop();
    }

    void AnimationComponent::StopAll()
    {
        for (auto& state : mStates)
            state->GetPlayer().Stop();

        mBlend.time = -1;
    }

    String AnimationComponent::GetName()
    {
        return "Animation";
    }

    String AnimationComponent::GetCategory()
    {
        return "Animation";
    }

    String AnimationComponent::GetIcon()
    {
        return "ui/UI4_animation_component.png";
    }

    void AnimationComponent::BeginAnimationEdit()
    {
        mInEditMode = true;
    }

    void AnimationComponent::EndAnimationEdit()
    {
        mInEditMode = false;
    }

    void AnimationComponent::OnStart()
    {
        for (auto& state : mStates)
        {
            if (state->autoPlay)
                state->GetPlayer().Play();
        }
    }

    void AnimationComponent::OnInitialized()
    {
        ReattachAnimationStates();
        Component::OnInitialized();
    }

    void AnimationComponent::RegSubTrack(const Ref<AnimationSubTrack::Player>& player, const String& path,
                                         const Ref<AnimationState>& state)
    {
        RegTrack<void, AnimationSubTrack, SubTrackMixer>(player, path, state);
    }

    void AnimationComponent::UnregTrack(const Ref<IAnimationTrack::IPlayer>& player, const String& path)
    {
        for (auto& val : mValues)
        {
            if (val->path == path)
            {
                val->RemoveTrack(player.Get());

                if (val->IsEmpty())
                    mValues.Remove(val);

                return;
            }
        }
    }

    void AnimationComponent::OnStateAnimationTrackAdded(const Ref<AnimationState>& state, const Ref<IAnimationTrack::IPlayer>& player)
    {
        player->RegMixer(state, player->GetTrack()->path);
    }

    void AnimationComponent::OnStateAnimationTrackRemoved(const Ref<AnimationState>& state, const Ref<IAnimationTrack::IPlayer>& player)
    {
        UnregTrack(player, player->GetTrack()->path);
    }

    void AnimationComponent::ReattachAnimationStates()
    {
        auto statesCopy = mStates;
        for (auto& state : statesCopy)
        {
            if (state && !state->mOwner)
            {
                mStates.Remove(state);
                AddState(state);
            }
        }
    }

    void AnimationComponent::BlendState::Update(float dt)
    {
        time -= dt;
        float coef = Math::Max(0.0f, time) / duration;

        for (auto& state : blendOffStates)
            state->SetWeight(coef);

        blendOnState->SetWeight(1.0f - coef);
    }

    template<>
    void AnimationComponent::TrackMixer<int>::Update()
    {
        AnimationState* firstValueState = tracks[0].first;
        AnimationTrack<int>::Player* firstValue = tracks[0].second;

        float weightsSum = firstValueState->mWeight*firstValueState->mask.GetNodeWeight(path);
        float valueSum = (float)firstValue->GetValue();

        for (int i = 1; i < tracks.Count(); i++)
        {
            AnimationState* valueState = tracks[i].first;
            AnimationTrack<int>::Player* value = tracks[i].second;

            weightsSum += valueState->mWeight*valueState->mask.GetNodeWeight(path);
            valueSum += (float)value->GetValue();
        }

        int resValue = Math::RoundToInt(valueSum / weightsSum);
        target->SetValue(resValue);
    }

    template<>
    void AnimationComponent::TrackMixer<bool>::Update()
    {
        AnimationState* firstValueState = tracks[0].first;
        AnimationTrack<bool>::Player* firstValue = tracks[0].second;

        float weightsSum = firstValueState->mWeight*firstValueState->mask.GetNodeWeight(path);
        float valueSum = firstValue->GetValue() ? 1.0f : 0.0f;

        for (int i = 1; i < tracks.Count(); i++)
        {
            AnimationState* valueState = tracks[i].first;
            AnimationTrack<bool>::Player* value = tracks[i].second;

            weightsSum += valueState->mWeight*valueState->mask.GetNodeWeight(path);
            valueSum += value->GetValue() ? 1.0f : 0.0f;
        }

        bool resValue = (valueSum / weightsSum) > 0.5f;
        target->SetValue(resValue);
    }

    AnimationComponent::SubTrackMixer::~SubTrackMixer()
    {}

    void AnimationComponent::SubTrackMixer::Update()
    {
    }

    void AnimationComponent::SubTrackMixer::RemoveTrack(IAnimationTrack::IPlayer* track)
    {
        tracks.RemoveAll([&](const auto& x) { return x.second == track; });
    }

    bool AnimationComponent::SubTrackMixer::IsEmpty() const
    {
        return tracks.IsEmpty();
    }

}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::AnimationComponent>);
// --- META ---

DECLARE_CLASS(o2::AnimationComponent, o2__AnimationComponent);
// --- END META ---
