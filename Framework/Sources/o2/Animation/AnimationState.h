#pragma once

#include "o2/Animation/AnimationMask.h"
#include "o2/Animation/AnimationPlayer.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Assets/Types/AnimationAsset.h"
#include "o2/Utils/Basic/ICloneable.h"
#include "o2/Utils/Editor/Attributes/InvokeOnChangeAttribute.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    FORWARD_CLASS_REF(AnimationComponent);

    // -----------------------------------------------------
    // Animation state interface. Can be updated and blended
    // -----------------------------------------------------
    class IAnimationState: public ISerializable, public RefCounterable, public ICloneableRef
    {
    public:
        PROPERTIES(IAnimationState);
        PROPERTY(float, weight, SetWeight, GetWeight); // State weight

    public:
        String name;            // State name @SERIALIZABLE
        bool   autoPlay = true; // True, if state should be played automatically @SERIALIZABLE

    public:
        // Default constructor
        IAnimationState() = default;
        
		// Copy-constructor
		IAnimationState(const IAnimationState& other);

        // Constructor with name
        IAnimationState(const String& name);

        // Updates state
        virtual void Update(float dt);

        // Returns player
        virtual IAnimation& GetPlayer();

		// Returns animation duration
        virtual float GetDuration() const;

        // Sets state weight (0...1) of blending
        virtual void SetWeight(float weight);

        // Returns state weight (0...1) of blending
        virtual float GetWeight() const;

        // Sets looped state
        virtual void SetLooped(bool looped);

        // Returns looped state
        virtual bool IsLooped() const;

        SERIALIZABLE(IAnimationState);
        CLONEABLE_REF(IAnimationState);

    protected:
        WeakRef<AnimationComponent> mOwner; // Animation state owner component

    protected:
        // Registers animation in state
        virtual void Register(const Ref<AnimationComponent>& owner);

        // Removes animation state from component
        virtual void Unregister();

        friend class AnimationComponent;
    };

    // ---------------
    // Animation state
    // ---------------
    class AnimationState: public IAnimationState
    {
    public:
        AnimationMask        mask;                              // Animation mask @SERIALIZABLE
        Ref<AnimationPlayer> player = mmake<AnimationPlayer>(); // Animation player

    public:
        // Default constructor
        AnimationState() = default;

        // Copy-constructor
        AnimationState(const AnimationState& other);

        // Constructor with name
        AnimationState(const String& name);

        // Updates state
        void Update(float dt) override;

        // Returns player
        IAnimation& GetPlayer() override;

		// Returns animation duration
		float GetDuration() const override;

        // Sets state weight
        void SetWeight(float weight) override;

        // Returns state weight
        float GetWeight() const override;

        // Sets looped state
        void SetLooped(bool looped) override;

        // Returns looped state
        bool IsLooped() const override;

        // Sets animation
        void SetAnimation(const AssetRef<AnimationAsset>& animationAsset);

        // Returns animation
        const AssetRef<AnimationAsset>& GetAnimation() const;

        SERIALIZABLE(AnimationState);
        CLONEABLE_REF(AnimationState);

    protected:
        AssetRef<AnimationAsset> mAnimation; // Animation @SERIALIZABLE @EDITOR_PROPERTY @INVOKE_ON_CHANGE(OnAnimationChanged)

        float mWeight = 1.0f; // State weight @SERIALIZABLE @EDITOR_PROPERTY

    protected:
        // Registers animation in state
        void Register(const Ref<AnimationComponent>& owner) override;

        // Removes animation state from component
        void Unregister() override;

        // Called when animation changed from editor
        void OnAnimationChanged();

        // Called when player has added new track
        void OnTrackPlayerAdded(const Ref<IAnimationTrack::IPlayer>& trackPlayer);

        // Called when player is removing track
        void OnTrackPlayerRemove(const Ref<IAnimationTrack::IPlayer>& trackPlayer);

        // It is called when object was deserialized; sets animation into player
        void OnDeserialized(const DataValue& node) override;

        friend class AnimationComponent;
        friend class AnimationClip;

        template<typename _type>
        friend class AnimationTrack;

        friend class AnimationSubTrack;
    };
}
// --- META ---

CLASS_BASES_META(o2::IAnimationState)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::IAnimationState)
{
    FIELD().PUBLIC().NAME(weight);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(name);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(autoPlay);
    FIELD().PROTECTED().NAME(mOwner);
}
END_META;
CLASS_METHODS_META(o2::IAnimationState)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const IAnimationState&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(IAnimation&, GetPlayer);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWeight, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetWeight);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLooped, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLooped);
    FUNCTION().PROTECTED().SIGNATURE(void, Register, const Ref<AnimationComponent>&);
    FUNCTION().PROTECTED().SIGNATURE(void, Unregister);
}
END_META;

CLASS_BASES_META(o2::AnimationState)
{
    BASE_CLASS(o2::IAnimationState);
}
END_META;
CLASS_FIELDS_META(o2::AnimationState)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mask);
    FIELD().PUBLIC().DEFAULT_VALUE(mmake<AnimationPlayer>()).NAME(player);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().INVOKE_ON_CHANGE_ATTRIBUTE(OnAnimationChanged).SERIALIZABLE_ATTRIBUTE().NAME(mAnimation);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mWeight);
}
END_META;
CLASS_METHODS_META(o2::AnimationState)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const AnimationState&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(IAnimation&, GetPlayer);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDuration);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWeight, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetWeight);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLooped, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLooped);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAnimation, const AssetRef<AnimationAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<AnimationAsset>&, GetAnimation);
    FUNCTION().PROTECTED().SIGNATURE(void, Register, const Ref<AnimationComponent>&);
    FUNCTION().PROTECTED().SIGNATURE(void, Unregister);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAnimationChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTrackPlayerAdded, const Ref<IAnimationTrack::IPlayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTrackPlayerRemove, const Ref<IAnimationTrack::IPlayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
}
END_META;
// --- END META ---
