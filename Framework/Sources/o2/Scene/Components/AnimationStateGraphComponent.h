#pragma once

#include "o2/Animation/AnimationState.h"
#include "o2/Scene/Component.h"
#include "o2/Utils/Basic/ICloneable.h"
#include "o2/Utils/Math/Curve.h"
#include "o2/Assets/Types/AnimationStateGraphAsset.h"

namespace o2
{    
    // ---------------------------------------------------------------------------------------
    // Animation state graph component. Contains animation states and transitions between them
    // ---------------------------------------------------------------------------------------
    class AnimationStateGraphComponent: public Component
    {
	public:
		// ------------------------------------------------------------
		// State player. Contains animation players and state reference
		// ------------------------------------------------------------
		struct StatePlayer: public RefCounterable
		{
		public:
			// Setup state player. Initializes animation players
			void Setup(const Ref<AnimationComponent>& animationComponent, const Ref<AnimationGraphState>& state,
					   const Ref<AnimationStateGraphComponent>& owner);

			// Play state
			void Play();

			// Stop state
			void Stop();

			// Set weight of state
			void SetWeight(float weight);

			// Returns time of state. Gets value from first animation player
			float GetTime() const;

			// Returns duration of state. Gets value from first animation player
			float GetDuration() const;

			// Returns players
            const Vector<Pair<Ref<AnimationGraphState::Animation>, Ref<IAnimationState>>>& GetPlayers() const;

			// Returns state
			const Ref<AnimationGraphState>& GetState() const;

			// Returns owner
			Ref<AnimationStateGraphComponent> GetOwner() const;

		private:
			Vector<Pair<Ref<AnimationGraphState::Animation>, Ref<IAnimationState>>> mPlayers; // Animation players

			Ref<AnimationGraphState> mState; // State reference

			WeakRef<AnimationStateGraphComponent> mOwner; // Owner reference
		};

    public:
        PROPERTIES(AnimationStateGraphComponent);
        PROPERTY(AssetRef<AnimationStateGraphAsset>, graph, SetGraph, GetGraph); // State graph asset property
        PROPERTY(String, State, GoToState, GetCurrentStateName);                 // Current state property

    public:
		Function<void(const Ref<StatePlayer>& player)> onStateStarted;  // Event, called when state started
		Function<void(const Ref<StatePlayer>& player)> onStateFinished; // Event, called when state finished

		Function<void(const Ref<AnimationGraphTransition>& transition)> onTransitionStarted;  // Event, called when transition started
		Function<void(const Ref<AnimationGraphTransition>& transition)> onTransitionFinished; // Event, called when transition finished
    	Function<void(const Ref<AnimationGraphTransition>& transition)> onTransitionCancelled; // Event, called when transition cancelled

		Function<void(const Vector<Ref<AnimationGraphTransition>>& path)> onTransitionsPlanned; // Event, called when transitions planned

    public:
        // Default constructor @SCRIPTABLE
        AnimationStateGraphComponent();

        // Copy-constructor
        AnimationStateGraphComponent(const AnimationStateGraphComponent& other);

        // Destructor
        ~AnimationStateGraphComponent() override;

        // Copy-operator
        AnimationStateGraphComponent& operator=(const AnimationStateGraphComponent& other);

        // Updates animations, blendings and transitions
        void OnUpdate(float dt) override;

        // Sets animation state graph asset
        void SetGraph(const AssetRef<AnimationStateGraphAsset>& graph);

        // Returns animation state graph asset
		const AssetRef<AnimationStateGraphAsset>& GetGraph() const;

		// Resets graph to initial state
		void Reset();

        // Starts transition to state
        void GoToState(const String& name);

        // Starts transition to state
        void GoToState(const Ref<AnimationGraphState>& state);

        // Forces play state without transition
        void ForcePlayState(const String& name);

        // Forces play state without transition
        void ForcePlayState(const Ref<AnimationGraphState>& state);

        // Immediately stops current transitions
        void StopTransition();

        // Returns current state
        const Ref<AnimationGraphState>& GetCurrentState() const;

		// Returns current state player
		const Ref<StatePlayer>& GetCurrentStatePlayer() const;

        // Returns current state name
        String GetCurrentStateName() const;

        // Returns next state
        const Ref<AnimationGraphState>& GetNextState() const;

		// Returns next state player
		const Ref<StatePlayer>& GetNextStatePlayer() const;

		// Returns next state name
		String GetNextStateName() const;

        // Returns current transition
        const Ref<AnimationGraphTransition>& GetCurrentTransition() const;

		// Returns current transition time
		float GetCurrentTransitionTime() const;

        // Returns next transitions
        const Vector<Ref<AnimationGraphTransition>>& GetNextTransitions() const;

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Returns name of component icon
        static String GetIcon();

        SERIALIZABLE(AnimationStateGraphComponent);
        REF_COUNTERABLE_IMPL(Component);

    protected:
        AssetRef<AnimationStateGraphAsset> mStateGraph; // State graph asset @SERIALIZABLE 

        WeakRef<AnimationComponent> mAnimationComponent; // Animation component reference

        Ref<AnimationGraphState> mCurrentState;       // Current state
        Ref<StatePlayer>         mCurrentStatePlayer; // Current state player

        Ref<AnimationGraphState> mNextState;       // Next state
        Ref<StatePlayer>         mNextStatePlayer; // Next state player

        Ref<AnimationGraphTransition> mCurrentTransition;            // Current transition
        float                         mCurrentTransitionTime = 0.0f; // Current transition time

        Vector<Ref<AnimationGraphTransition>> mNextTransitions;   // Next transitions

    protected:
        // Called when actor initialized, reattaches animation states
        void OnInitialized() override;

        // Checks if current transition is finished and starts next transition
        void CheckStartNextTransition();

        // Updates current transition
        void UpdateCurrentTransition(float dt);

        // Returns animation component. Stores reference to animation component
        Ref<AnimationComponent> GetAnimationComponent();
    };
}
// --- META ---

CLASS_BASES_META(o2::AnimationStateGraphComponent)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::AnimationStateGraphComponent)
{
    FIELD().PUBLIC().NAME(graph);
    FIELD().PUBLIC().NAME(State);
    FIELD().PUBLIC().NAME(onStateStarted);
    FIELD().PUBLIC().NAME(onStateFinished);
    FIELD().PUBLIC().NAME(onTransitionStarted);
    FIELD().PUBLIC().NAME(onTransitionFinished);
    FIELD().PUBLIC().NAME(onTransitionCancelled);
    FIELD().PUBLIC().NAME(onTransitionsPlanned);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mStateGraph);
    FIELD().PROTECTED().NAME(mAnimationComponent);
    FIELD().PROTECTED().NAME(mCurrentState);
    FIELD().PROTECTED().NAME(mCurrentStatePlayer);
    FIELD().PROTECTED().NAME(mNextState);
    FIELD().PROTECTED().NAME(mNextStatePlayer);
    FIELD().PROTECTED().NAME(mCurrentTransition);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mCurrentTransitionTime);
    FIELD().PROTECTED().NAME(mNextTransitions);
}
END_META;
CLASS_METHODS_META(o2::AnimationStateGraphComponent)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const AnimationStateGraphComponent&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetGraph, const AssetRef<AnimationStateGraphAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<AnimationStateGraphAsset>&, GetGraph);
    FUNCTION().PUBLIC().SIGNATURE(void, Reset);
    FUNCTION().PUBLIC().SIGNATURE(void, GoToState, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, GoToState, const Ref<AnimationGraphState>&);
    FUNCTION().PUBLIC().SIGNATURE(void, ForcePlayState, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, ForcePlayState, const Ref<AnimationGraphState>&);
    FUNCTION().PUBLIC().SIGNATURE(void, StopTransition);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<AnimationGraphState>&, GetCurrentState);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<StatePlayer>&, GetCurrentStatePlayer);
    FUNCTION().PUBLIC().SIGNATURE(String, GetCurrentStateName);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<AnimationGraphState>&, GetNextState);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<StatePlayer>&, GetNextStatePlayer);
    FUNCTION().PUBLIC().SIGNATURE(String, GetNextStateName);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<AnimationGraphTransition>&, GetCurrentTransition);
    FUNCTION().PUBLIC().SIGNATURE(float, GetCurrentTransitionTime);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<AnimationGraphTransition>>&, GetNextTransitions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetIcon);
    FUNCTION().PROTECTED().SIGNATURE(void, OnInitialized);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckStartNextTransition);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateCurrentTransition, float);
    FUNCTION().PROTECTED().SIGNATURE(Ref<AnimationComponent>, GetAnimationComponent);
}
END_META;
// --- END META ---
