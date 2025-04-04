#pragma once

#include "o2/Animation/AnimationState.h"
#include "o2/Scene/Component.h"
#include "o2/Utils/Basic/ICloneable.h"
#include "o2/Utils/Math/Curve.h"

namespace o2
{    
    class AnimationGraphState;
    FORWARD_CLASS_REF(AnimationStateGraphAsset);

    // -------------------------
    // Transition between states
    // -------------------------
    class AnimationGraphTransition : public ISerializable, public RefCounterable, public ICloneableRef
    {
    public:
        float duration = 0.0f;          // Duration of transition @SERIALIZABLE
        float beginTimeRange = 0.0f;    // Begin time range of transition @SERIALIZABLE
        float endTimeRange = 1.0f;      // End time range of transition @SERIALIZABLE
        bool  backwardAvailable = true; // Is backward transition available @SERIALIZABLE

        Ref<Curve> curve; // Curve of transition @SERIALIZABLE

        PROPERTIES(AnimationGraphTransition);
        PROPERTY(Ref<AnimationGraphState>, destinationState, SetDestinationState, GetDestinationState); // Destination state property

    public:
        // Sets destination state
        void SetDestinationState(const Ref<AnimationGraphState>& state);

        // Returns destination state
        Ref<AnimationGraphState> GetDestinationState() const;

        // Returns source state
        Ref<AnimationGraphState> GetSourceState() const;

        SERIALIZABLE(AnimationGraphTransition);
        CLONEABLE_REF(AnimationGraphTransition);

    protected:
        UID mDestinationState; // To state UID @SERIALIZABLE

        WeakRef<AnimationGraphState> mSourceStateRef;      // From state reference (owner)
        WeakRef<AnimationGraphState> mDestinationStateRef; // To state reference

    protected:
        // Sets state and initializes references
        void SetState(const Ref<AnimationGraphState>& state);

        friend class AnimationGraphState;
    };

    // ---------------------------------------------------------------
    // Animation state. Can contain multiple animations and blend them
    // ---------------------------------------------------------------
    class AnimationGraphState : public ISerializable, public RefCounterable, public ICloneableRef
    {
    public:
        // ---------------------------------------------------------------
        // Single animation from state. Contains animation name and weight
        // ---------------------------------------------------------------
        struct Animation : public ISerializable, public RefCounterable
        {
            String name;          // Name of animation @SERIALIZABLE
            float  weight = 1.0f; // Weight of animation @SERIALIZABLE

            SERIALIZABLE(Animation);;
        };

    public:
        String name; // Name of state @SERIALIZABLE

    public:
        // Returns UID of state
        UID GetUID() const;

        // Returns animation by name
        Ref<Animation> GetAnimation(const String& name);

        // Adds animation to state
        Ref<Animation> AddAnimation(const String& name);

        // Removes animation from state
        void RemoveAnimation(const String& name);

        // Removes animation from state
        void RemoveAnimation(const Ref<Animation>& animation);

        // Returns animations
        const Vector<Ref<Animation>>& GetAnimations() const;

        // Adds transition to state
        Ref<AnimationGraphTransition> AddTransition(const Ref<AnimationGraphState>& destinationState);

        // Removes transition from state
        void RemoveTransition(const Ref<AnimationGraphTransition>& transition);

        // Returns transitions
        const Vector<Ref<AnimationGraphTransition>>& GetTransitions() const;

		// Sets position of state in graph
        void SetPosition(const Vec2F& position);

		// Returns position of state in graph
        Vec2F GetPosition() const;

        SERIALIZABLE(AnimationGraphState);
        CLONEABLE_REF(AnimationGraphState);

    protected:
        UID mUID; // UID of state @SERIALIZABLE

        Vector<Ref<Animation>>  mAnimations;  // Animations @SERIALIZABLE @EDITOR_PROPERTY @INVOKE_ON_CHANGE(ReinitAnimations)
        Vector<Ref<AnimationGraphTransition>> mTransitions; // Transitions @SERIALIZABLE @EDITOR_PROPERTY @INVOKE_ON_CHANGE(ReinitTransitions)

        WeakRef<AnimationStateGraphAsset> mGraph; // Graph reference

		Vec2F mPosition; // Position of state in graph @SERIALIZABLE @EDITOR_PROPERTY
            
    protected:
        // Reinitializes transitions
        void ReinitTransitions();

        // Sets graph and initializes references
        void SetGraph(const Ref<AnimationStateGraphAsset>& graph);

        friend class AnimationGraphTransition;
        friend class AnimationStateGraphAsset;
    };
}
// --- META ---

CLASS_BASES_META(o2::AnimationGraphTransition)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::AnimationGraphTransition)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(duration);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(beginTimeRange);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(endTimeRange);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(backwardAvailable);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(curve);
    FIELD().PUBLIC().NAME(destinationState);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mDestinationState);
    FIELD().PROTECTED().NAME(mSourceStateRef);
    FIELD().PROTECTED().NAME(mDestinationStateRef);
}
END_META;
CLASS_METHODS_META(o2::AnimationGraphTransition)
{

    FUNCTION().PUBLIC().SIGNATURE(void, SetDestinationState, const Ref<AnimationGraphState>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<AnimationGraphState>, GetDestinationState);
    FUNCTION().PUBLIC().SIGNATURE(Ref<AnimationGraphState>, GetSourceState);
    FUNCTION().PROTECTED().SIGNATURE(void, SetState, const Ref<AnimationGraphState>&);
}
END_META;

CLASS_BASES_META(o2::AnimationGraphState)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::AnimationGraphState)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(name);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mUID);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().INVOKE_ON_CHANGE_ATTRIBUTE(ReinitAnimations).SERIALIZABLE_ATTRIBUTE().NAME(mAnimations);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().INVOKE_ON_CHANGE_ATTRIBUTE(ReinitTransitions).SERIALIZABLE_ATTRIBUTE().NAME(mTransitions);
    FIELD().PROTECTED().NAME(mGraph);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(mPosition);
}
END_META;
CLASS_METHODS_META(o2::AnimationGraphState)
{

    FUNCTION().PUBLIC().SIGNATURE(UID, GetUID);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Animation>, GetAnimation, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Animation>, AddAnimation, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAnimation, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAnimation, const Ref<Animation>&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<Animation>>&, GetAnimations);
    FUNCTION().PUBLIC().SIGNATURE(Ref<AnimationGraphTransition>, AddTransition, const Ref<AnimationGraphState>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveTransition, const Ref<AnimationGraphTransition>&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<AnimationGraphTransition>>&, GetTransitions);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPosition, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetPosition);
    FUNCTION().PROTECTED().SIGNATURE(void, ReinitTransitions);
    FUNCTION().PROTECTED().SIGNATURE(void, SetGraph, const Ref<AnimationStateGraphAsset>&);
}
END_META;

CLASS_BASES_META(o2::AnimationGraphState::Animation)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(o2::AnimationGraphState::Animation)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(name);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(weight);
}
END_META;
CLASS_METHODS_META(o2::AnimationGraphState::Animation)
{
}
END_META;
// --- END META ---
