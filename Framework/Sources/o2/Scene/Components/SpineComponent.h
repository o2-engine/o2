#pragma once

#include "o2/Render/Spine/Spine.h"
#include "o2/Scene/Components/AnimationComponent.h"

namespace o2
{
    // --------------------------------
    // EsotericSoftware spine component
    // --------------------------------
    class SpineComponent : public AnimationComponent
    {
    public:
        // ---------------------
        // Spine animation state
        // ---------------------
        class AnimationState : public IAnimationState
        {
        public:
            PROPERTIES(AnimationState);
            PROPERTY(bool, looped, SetLooped, IsLooped); // State looped

        public:
            // Default constructor
            AnimationState() = default;

            // Constructor
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

            // Sets state looped
            void SetLooped(bool looped) override;

            // Returns state looped
            bool IsLooped() const override;

            SERIALIZABLE(AnimationState);
            CLONEABLE_REF(AnimationState);

        private:
            Ref<Spine::Track> mTrack; // Spine track @EDITOR_PROPERTY

            bool mLooped = false; // State looped @SERIALIZABLE
            float mWeight = 1.0f; // State weight @SERIALIZABLE @RANGE(0, 1)

        protected:
            // Registers animation in state
            void Register(const Ref<AnimationComponent>& owner);

            // Removes animation state from component
            void Unregister();
        };

    public:
        PROPERTIES(SpineComponent);
        PROPERTY(AssetRef<SpineAsset>, spineAsset, SetSpineAsset, GetSpineAsset); // Spine asset @EDITOR_PROPERTY

    public:
        // Default constructor
        SpineComponent();

        // Copy-constructor
        SpineComponent(const SpineComponent& other);

        // Copy-operator
        SpineComponent& operator=(const SpineComponent& other);

        // Destructor
        ~SpineComponent();

        // Sets spine asset and loads spine
        void SetSpineAsset(const AssetRef<SpineAsset>& spineAsset);

        // Returns spine asset
        const AssetRef<SpineAsset>& GetSpineAsset() const;

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Returns name of component icon
        static String GetIcon();

        SERIALIZABLE(SpineComponent);
        CLONEABLE_REF(SpineComponent);

    protected:
        AssetRef<SpineAsset> mSpineAsset; // Spine asset @SERIALIZABLE

        Ref<Spine> mSpineRenderer; // Spine renderer

    protected:
        // Loads spine from asset
        void LoadSpine();

        // Creates animation states from spine
        void CreateAnimationStates();

        // Called when actor and scene is initialized
        void OnInitialized() override;

        // Called when transformation was updated. Updates spine root transform
        void OnTransformUpdated() override;

        // Called on update with frame dt, updates spine animation
        void OnUpdate(float dt) override;

        // Called when actor is drawing, draws spine animation
        void OnDraw() override;
    };
}
// --- META ---

CLASS_BASES_META(o2::SpineComponent)
{
    BASE_CLASS(o2::AnimationComponent);
}
END_META;
CLASS_FIELDS_META(o2::SpineComponent)
{
    FIELD().PUBLIC().EDITOR_PROPERTY_ATTRIBUTE().NAME(spineAsset);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSpineAsset);
    FIELD().PROTECTED().NAME(mSpineRenderer);
}
END_META;
CLASS_METHODS_META(o2::SpineComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const SpineComponent&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSpineAsset, const AssetRef<SpineAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<SpineAsset>&, GetSpineAsset);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetIcon);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadSpine);
    FUNCTION().PROTECTED().SIGNATURE(void, CreateAnimationStates);
    FUNCTION().PROTECTED().SIGNATURE(void, OnInitialized);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraw);
}
END_META;

CLASS_BASES_META(o2::SpineComponent::AnimationState)
{
    BASE_CLASS(o2::IAnimationState);
}
END_META;
CLASS_FIELDS_META(o2::SpineComponent::AnimationState)
{
    FIELD().PUBLIC().NAME(looped);
    FIELD().PRIVATE().EDITOR_PROPERTY_ATTRIBUTE().NAME(mTrack);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mLooped);
    FIELD().PRIVATE().RANGE_ATTRIBUTE(0, 1).SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mWeight);
}
END_META;
CLASS_METHODS_META(o2::SpineComponent::AnimationState)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
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
// --- END META ---
