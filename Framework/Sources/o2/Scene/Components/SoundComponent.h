#pragma once

#include "o2/Scene/Component.h"
#include "o2/Sound/SoundPlayer.h"

namespace o2
{
    // -------------------------------------------------------------------------------
    // Sound component. Plays sound at actor position, animatable in animation editor
    // -------------------------------------------------------------------------------
    class SoundComponent: public Component, public SoundPlayer
    {
    public:
        // Default constructor
        SoundComponent();

        // Copy constructor
        SoundComponent(const SoundComponent& other);

        // Destructor
        ~SoundComponent();

        // Assign operator
        SoundComponent& operator=(const SoundComponent& other);

        // Updates component
        void OnUpdate(float dt) override;

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Returns name of component icon
        static String GetIcon();

        // Dynamic cast to RefCounterable via Component
        static Ref<RefCounterable> CastToRefCounterable(const Ref<SoundComponent>& ref);

        SERIALIZABLE(SoundComponent);
        CLONEABLE_REF(SoundComponent);
        REF_COUNTERABLE_IMPL(Component, SoundPlayer);

    protected:
        // Called when actor's transform was changed; moves spatial sound source
        void OnTransformUpdated() override;

        // Silences backend sound; playback resumes from animation state on next update in scene
        void OnRemoveFromScene() override;

        // Silences backend sound
        void OnDisabled() override;

        // Beginning serialization callback
        void OnSerialize(DataValue& node) const override;

        // Called when object was deserialized
        void OnDeserialized(const DataValue& node) override;

        // Beginning serialization delta callback
        void OnSerializeDelta(DataValue& node, const IObject& origin) const override;

        // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::SoundComponent)
{
    BASE_CLASS(o2::Component);
    BASE_CLASS(o2::SoundPlayer);
}
END_META;
CLASS_FIELDS_META(o2::SoundComponent)
{
}
END_META;
CLASS_METHODS_META(o2::SoundComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const SoundComponent&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<SoundComponent>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemoveFromScene);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerializeDelta, DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
}
END_META;
// --- END META ---
