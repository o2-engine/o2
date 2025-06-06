#pragma once

#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/Editor/EditableAnimation.h"
#include "o2/Animation/Tracks/AnimationSubTrack.h"
#include "o2/Animation/Tracks/AnimationTrack.h"
#include "o2/Scene/Component.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Editor/Attributes/DefaultTypeAttribute.h"
#include "o2/Utils/Editor/Attributes/DontDeleteAttribute.h"
#include "o2/Utils/Editor/Attributes/InvokeOnChangeAttribute.h"

namespace o2
{
    class IAnimationState;
    class AnimationMask;
    
    // -------------------
    // Animation component
    // -------------------
    class AnimationComponent: public Component, public IEditableAnimation
    {
    public:
        PROPERTIES(AnimationComponent);
        ACCESSOR(Ref<IAnimationState>, state, String, GetState, GetAllStates); // Animation states accessor by name

    public:
        // Default constructor @SCRIPTABLE
        AnimationComponent();

        // Copy-constructor
        AnimationComponent(RefCounter* refCounter, const AnimationComponent& other);

        // Destructor
        ~AnimationComponent();

        // Copy-operator
        AnimationComponent& operator=(const AnimationComponent& other);

        // Updates animations, blendings and assigning blended values
        void OnUpdate(float dt) override;

        // Adds new animation state and returns him
        Ref<IAnimationState> AddState(const Ref<IAnimationState>& state);

        // Adds new animation state and returns him
        Ref<IAnimationState> AddState(const String& name, const Ref<AnimationClip>& animation,
                                     const AnimationMask& mask, float weight);

        // Adds new animation state and returns him
        Ref<IAnimationState> AddState(const String& name);

        // Removes animation state
        void RemoveState(const Ref<IAnimationState>& state);

        // Removes animation state by name
        void RemoveState(const String& name);

        // Removes all animation states @SCRIPTABLE
        void RemoveAllStates();

        // Returns state with specified name. Returns nullptr if can't find state with specified name
        Ref<IAnimationState> GetState(const String& name);

        // Returns all states array
        const Vector<Ref<IAnimationState>>& GetStates() const;

        // Returns all states
        Map<String, Ref<IAnimationState>> GetAllStates() const;

        // Returns vector of available states names @SCRIPTABLE
        Vector<String> GetStatesNames() const;

        // Creates new state and plays him
        Ref<IAnimationState> Play(const Ref<AnimationClip>& animation, const String& name);

        // Creates new state and plays him
        Ref<IAnimationState> Play(const Ref<AnimationClip>& animation);

        // Searches animation state with name and plays him @SCRIPTABLE
        Ref<IAnimationState> Play(const String& name);

        // Creates new state, and blends animation with duration
        Ref<IAnimationState> BlendTo(const Ref<AnimationClip>& animation, const String& name, float duration = 1.0f);

        // Creates new state, and blends animation with duration
        Ref<IAnimationState> BlendTo(const Ref<AnimationClip>& animation, float duration = 1.0f);

        // Creates new state, and blends animation with duration @SCRIPTABLE
        Ref<IAnimationState> BlendTo(const String& name, float duration = 1.0f);

        // Plays state and blends animation with duration
        Ref<IAnimationState> BlendTo(const Ref<IAnimationState>& state, float duration = 1.0f);

        // Stops animation with name @SCRIPTABLE
        void Stop(const String& animationName);

        // Stops all states @SCRIPTABLE
        void StopAll();

        // Called when animation started to edit. It means that animation must be deactivated
        void BeginAnimationEdit() override;

        // Called when animation finished editing. ANimation must be reactivated
        void EndAnimationEdit() override;

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Returns name of component icon
        static String GetIcon();

        SERIALIZABLE(AnimationComponent);
        CLONEABLE_REF(AnimationComponent);

    public:
        // -------------------------------
        // Value assigning agent interface
        // -------------------------------
        struct ITrackMixer: public RefCounterable
        {
            // Value path
            String path;

        public:
            virtual ~ITrackMixer() {}

            // Updates value and blend
            virtual void Update() = 0;

            // Removes Animation track from agent
            virtual void RemoveTrack(IAnimationTrack::IPlayer* track) = 0;

            // Returns is agent hasn't no values
            virtual bool IsEmpty() const = 0;
        };

        // ------------------------------
        // Template value assigning agent
        // ------------------------------
        template<typename _type>
        struct TrackMixer: public ITrackMixer
        {
        public:
            Vector<Pair<AnimationState*, typename AnimationTrack<_type>::Player*>> tracks; // Animation tracks associated with animation states
            
            Ref<IValueProxy<_type>> target; // Target value proxy

			_type defaultValue = _type(); // Default value of target, used to mix single value track

        public:
            // Destructor
            ~TrackMixer();

            // Updates value and blend
            void Update() override;

            // Removes Animation track from agent
            void RemoveTrack(IAnimationTrack::IPlayer* track) override;

            // Returns is agent hasn't no values
            bool IsEmpty() const override;
        };

        // ------------------------------
        // Template value assigning agent
        // ------------------------------
        struct SubTrackMixer : public ITrackMixer
        {
        public:
            Vector<Pair<AnimationState*, typename AnimationSubTrack::Player*>> tracks; // Animation tracks associated with animation states

        public:
            // Destructor
            ~SubTrackMixer();

            // Updates value and blend
            void Update() override;

            // Removes Animation track from agent
            void RemoveTrack(IAnimationTrack::IPlayer* track) override;

            // Returns is agent hasn't no values
            bool IsEmpty() const override;
        };

        // ----------------------
        // Blend state parameters
        // ----------------------
        struct BlendState
        {
            Vector<Ref<IAnimationState>> blendOffStates; // Turning off states
            Ref<IAnimationState>         blendOnState;   // Turning on state

            float duration;     // Blending duration
            float time = -1.0f; // Current blending remaining time

        public:
            // Updates work weight by time
            void Update(float dt);
        };

    protected:
        Vector<Ref<IAnimationState>> mStates; // Animation states array @SERIALIZABLE @EDITOR_PROPERTY @DEFAULT_TYPE(o2::AnimationState) @INVOKE_ON_CHANGE(ReattachAnimationStates) @DONT_DELETE
        Vector<Ref<ITrackMixer>>     mValues; // Assigning value agents

        BlendState mBlend;  // Current blend parameters

        bool mInEditMode = false; // True when some state animation is editing now, disables update

    protected:
        // Called when component started, checks states auto play
        void OnStart() override;

        // Called when actor initialized, reattaches animation states
        void OnInitialized() override;

        // Registers track by path and state
        template<typename _valueType, typename _trackType, typename _mixerType>
        void RegTrack(const Ref<typename _trackType::Player>& player, const String& path, const Ref<AnimationState>& state);

        // Registers value track by path and state
        template<typename _valueType>
        void RegValueTrack(const Ref<typename AnimationTrack<_valueType>::Player>& player, const String& path, const Ref<AnimationState>& state);

        // Registers sub track by path and state
        void RegSubTrack(const Ref<AnimationSubTrack::Player>& player, const String& path, const Ref<AnimationState>& state);

        // Removes Animation track from agent by path
        void UnregTrack(const Ref<IAnimationTrack::IPlayer>& player, const String& path);

        // Called when new track added in animation state, registers track player in mixer
        void OnStateAnimationTrackAdded(const Ref<AnimationState>& state, const Ref<IAnimationTrack::IPlayer>& player);

        // Called when track is removing from animation state, unregisters track player from mixer
        void OnStateAnimationTrackRemoved(const Ref<AnimationState>& state, const Ref<IAnimationTrack::IPlayer>& player);

        // Reattaches animation states to component, used to start and update animations
        virtual void ReattachAnimationStates();

        REF_COUNTERABLE_IMPL(Component);

        friend class AnimationClip;
        friend class AnimationState;
        friend class IAnimationTrack;

        template<typename _type>
        friend class AnimationTrack;

        friend class AnimationSubTrack;
    };
}

#include "o2/Animation/AnimationState.h"

namespace o2
{
    template<typename _valueType, typename _trackType, typename _mixerType>
    void AnimationComponent::RegTrack(const Ref<typename _trackType::Player>& player, const String& path, const Ref<AnimationState>& state)
    {
        for (auto& val : mValues)
        {
            if (val->path == path)
            {
                auto agent = DynamicCast<_mixerType>(val);

                if (!agent)
                {
                    o2Debug.LogWarning("Different track types at: " + path);
                    return;
                }

                agent->tracks.Add({ state.Get(), player.Get() });
                return;
            }
        }

        auto newAgent = mmake<_mixerType>();
        mValues.Add(newAgent);
        newAgent->path = path;
        newAgent->tracks.Add({ state.Get(), player.Get() });

        const FieldInfo* fieldInfo = nullptr;
        auto owner = mOwner.Lock();
        auto fieldPtr = owner.Get()->GetType().GetFieldPtr(owner.Get(), path, fieldInfo);

        if (!fieldInfo)
        {
            o2Debug.LogWarning("Can't animate value " + path + ": can't find field");
            return;
        }

        if constexpr (!std::is_same<_valueType, void>::value)
        {
            newAgent->target = DynamicCast<IValueProxy<_valueType>>(fieldInfo->GetType()->GetValueProxy(fieldPtr));
            newAgent->defaultValue = newAgent->target->GetValue();
        }
    }

    template<typename _valueType>
    void AnimationComponent::RegValueTrack(const Ref<typename AnimationTrack<_valueType>::Player>& player, const String& path, const Ref<AnimationState>& state)
    {
        RegTrack<_valueType, AnimationTrack<_valueType>, TrackMixer<_valueType>>(player, path, state);
    }

    template<typename _type>
    AnimationComponent::TrackMixer<_type>::~TrackMixer()
    {}

    template<typename _type>
    bool AnimationComponent::TrackMixer<_type>::IsEmpty() const
    {
        return tracks.IsEmpty();
    }

    template<typename _type>
    void AnimationComponent::TrackMixer<_type>::RemoveTrack(IAnimationTrack::IPlayer* value)
    {
        tracks.RemoveAll([&](const auto& x) { return x.second == value; });
    }

    template<>
    void AnimationComponent::TrackMixer<bool>::Update();

    template<>
    void AnimationComponent::TrackMixer<int>::Update();

    template<typename _type>
    void AnimationComponent::TrackMixer<_type>::Update()
    {
        auto firstValueState = tracks[0].first;
        auto firstValue = tracks[0].second;

        float weightsSum = firstValueState->mWeight*firstValueState->mask.GetNodeWeight(path);
        _type valueSum = firstValue->GetValue()*firstValueState->mWeight;

        if (tracks.Count() > 1)
        {
            for (int i = 1; i < tracks.Count(); i++)
            {
                auto valueState = tracks[i].first;
                auto value = tracks[i].second;

                weightsSum += valueState->mWeight*valueState->mask.GetNodeWeight(path);
                valueSum += value->GetValue()*valueState->mWeight;
            }
        }
        else
        {
            float defaultWeight = 1.0f - weightsSum;
            valueSum += defaultValue*defaultWeight;
            weightsSum += defaultWeight;
        }

        _type resValue = valueSum / weightsSum;
        target->SetValue(resValue);
    }
    
    template<typename _type>
    void AnimationTrack<_type>::Player::RegMixer(const Ref<AnimationState>& state, const String& path)
    {
        state->mOwner.Lock()->RegValueTrack<_type>(Ref(this), path, state);
    }
}
// --- META ---

CLASS_BASES_META(o2::AnimationComponent)
{
    BASE_CLASS(o2::Component);
    BASE_CLASS(o2::IEditableAnimation);
}
END_META;
CLASS_FIELDS_META(o2::AnimationComponent)
{
    FIELD().PUBLIC().NAME(state);
    FIELD().PROTECTED().DEFAULT_TYPE_ATTRIBUTE(o2::AnimationState).DONT_DELETE_ATTRIBUTE().EDITOR_PROPERTY_ATTRIBUTE().INVOKE_ON_CHANGE_ATTRIBUTE(ReattachAnimationStates).SERIALIZABLE_ATTRIBUTE().NAME(mStates);
    FIELD().PROTECTED().NAME(mValues);
    FIELD().PROTECTED().NAME(mBlend);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mInEditMode);
}
END_META;
CLASS_METHODS_META(o2::AnimationComponent)
{

    typedef Map<String, Ref<IAnimationState>> _tmp1;

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const AnimationComponent&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationState>, AddState, const Ref<IAnimationState>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationState>, AddState, const String&, const Ref<AnimationClip>&, const AnimationMask&, float);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationState>, AddState, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveState, const Ref<IAnimationState>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveState, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, RemoveAllStates);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationState>, GetState, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<IAnimationState>>&, GetStates);
    FUNCTION().PUBLIC().SIGNATURE(_tmp1, GetAllStates);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vector<String>, GetStatesNames);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationState>, Play, const Ref<AnimationClip>&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationState>, Play, const Ref<AnimationClip>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<IAnimationState>, Play, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationState>, BlendTo, const Ref<AnimationClip>&, const String&, float);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationState>, BlendTo, const Ref<AnimationClip>&, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Ref<IAnimationState>, BlendTo, const String&, float);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IAnimationState>, BlendTo, const Ref<IAnimationState>&, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Stop, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, StopAll);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginAnimationEdit);
    FUNCTION().PUBLIC().SIGNATURE(void, EndAnimationEdit);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetIcon);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStart);
    FUNCTION().PROTECTED().SIGNATURE(void, OnInitialized);
    FUNCTION().PROTECTED().SIGNATURE(void, RegSubTrack, const Ref<AnimationSubTrack::Player>&, const String&, const Ref<AnimationState>&);
    FUNCTION().PROTECTED().SIGNATURE(void, UnregTrack, const Ref<IAnimationTrack::IPlayer>&, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateAnimationTrackAdded, const Ref<AnimationState>&, const Ref<IAnimationTrack::IPlayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateAnimationTrackRemoved, const Ref<AnimationState>&, const Ref<IAnimationTrack::IPlayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, ReattachAnimationStates);
}
END_META;
// --- END META ---
