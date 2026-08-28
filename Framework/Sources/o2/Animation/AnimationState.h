#pragma once

#include "o2/Animation/AnimationMask.h"
#include "o2/Animation/AnimationPlayer.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Assets/Types/AnimationAsset.h"
#include "o2/Utils/Basic/ICloneable.h"
#include "o2/Utils/Editor/AssetEditablePreview.h"
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
        PROPERTY(float, weight, SetWeight, GetWeight); // State weight @RANGE(0.0f, 1.0f)

    public:
        String name;            // State name @SERIALIZABLE
        bool   autoPlay = true; // True, if state should be played automatically @SERIALIZABLE @SCRIPTABLE

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
        // Sets state weight @SCRIPTABLE
        virtual void SetWeight(float weight);

        // Returns state weight (0...1) of blending
        // Returns state weight @SCRIPTABLE
        virtual float GetWeight() const;

        // Sets looped state
        virtual void SetLooped(bool looped);

        // Returns looped state
        virtual bool IsLooped() const;

        // Returns true while the state is previewed by the editor and driven by its player
        virtual bool IsInEditMode() const;

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
    class AnimationState: public IAnimationState, public AnimationAssetEditablePreview
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

        // Returns true while the state is previewed by the editor
        bool IsInEditMode() const override;

        // Sets animation @SCRIPTABLE
        void SetAnimation(const AssetRef<AnimationAsset>& animationAsset);

        // Returns animation
        const AssetRef<AnimationAsset>& GetAnimation() const;

		// Returns ref counter
		RefCounter* GetRefCounter() const override;

        SERIALIZABLE(AnimationState);
        CLONEABLE_REF(AnimationState);

    protected:
        AssetRef<AnimationAsset> mAnimation; // Animation @SERIALIZABLE @EDITOR_PROPERTY @INVOKE_ON_CHANGE(OnAnimationChanged)

		float mWeight = 1.0f; // State weight @SERIALIZABLE

		bool mInEditMode = false; // True, if animation is in edit mode (previewing)

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

		// Called when animation started to edit. It means that animation must be deactivated
		void BeginPreview() override;

		// Called when animation finished editing. Animation must be reactivated
		void EndPreview() override;

		// Returns actor that is being previewed
		Ref<Actor> GetPreviewActor() const override;

		// Returns animation player for previewing
        Ref<IAnimation> GetPreviewPlayer() const override;

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
    FIELD().PUBLIC().RANGE_ATTRIBUTE(0.0f, 1.0f).NAME(weight);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(name);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(autoPlay);
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
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetWeight, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetWeight);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLooped, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLooped);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsInEditMode);
    FUNCTION().PROTECTED().SIGNATURE(void, Register, const Ref<AnimationComponent>&);
    FUNCTION().PROTECTED().SIGNATURE(void, Unregister);
}
END_META;

CLASS_BASES_META(o2::AnimationState)
{
    BASE_CLASS(o2::IAnimationState);
    BASE_CLASS(o2::AnimationAssetEditablePreview);
}
END_META;
CLASS_FIELDS_META(o2::AnimationState)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(mask);
    FIELD().PUBLIC().DEFAULT_VALUE(mmake<AnimationPlayer>()).NAME(player);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().INVOKE_ON_CHANGE_ATTRIBUTE(OnAnimationChanged).SERIALIZABLE_ATTRIBUTE().NAME(mAnimation);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mWeight);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mInEditMode);
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
    FUNCTION().PUBLIC().SIGNATURE(bool, IsInEditMode);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetAnimation, const AssetRef<AnimationAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<AnimationAsset>&, GetAnimation);
    FUNCTION().PUBLIC().SIGNATURE(RefCounter*, GetRefCounter);
    FUNCTION().PROTECTED().SIGNATURE(void, Register, const Ref<AnimationComponent>&);
    FUNCTION().PROTECTED().SIGNATURE(void, Unregister);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAnimationChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTrackPlayerAdded, const Ref<IAnimationTrack::IPlayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTrackPlayerRemove, const Ref<IAnimationTrack::IPlayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, BeginPreview);
    FUNCTION().PROTECTED().SIGNATURE(void, EndPreview);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Actor>, GetPreviewActor);
    FUNCTION().PROTECTED().SIGNATURE(Ref<IAnimation>, GetPreviewPlayer);
}
END_META;
// --- END META ---
