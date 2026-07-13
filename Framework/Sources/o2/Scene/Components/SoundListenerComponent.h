#pragma once

#include "o2/Scene/Component.h"
#include "o2/Sound/SoundListener.h"

namespace o2
{
    // ---------------------------------------------------------------------------------
    // Sound listener component. Places the spatial audio listener at the actor position
    // and orientation; without an active listener on scene it follows the render camera
    // ---------------------------------------------------------------------------------
    class SoundListenerComponent: public Component, public SoundListener
    {
    public:
        // Default constructor
        SoundListenerComponent();

        // Copy-constructor
        SoundListenerComponent(const SoundListenerComponent& other);

        // Destructor
        ~SoundListenerComponent();

        // Assign operator
        SoundListenerComponent& operator=(const SoundListenerComponent& other);

        // Returns true when enabled and owner actor is on scene
        bool IsListening() const override;

        // Returns name of component
        static String GetName();

        // Returns category of component
        static String GetCategory();

        // Returns name of component icon
        static String GetIcon();

        // Dynamic cast to RefCounterable via Component
        static Ref<RefCounterable> CastToRefCounterable(const Ref<SoundListenerComponent>& ref);

        SERIALIZABLE(SoundListenerComponent);
        CLONEABLE_REF(SoundListenerComponent);
        REF_COUNTERABLE_IMPL(Component, SoundListener);

    protected:
        // Called on actor start; syncs listener pose with actor transform
        void OnStart() override;

        // Called when actor's transform was changed; syncs listener pose with actor transform
        void OnTransformUpdated() override;

        // Updates listener position and orientation from actor world transform
        void UpdatePoseFromTransform();
    };
}
// --- META ---

CLASS_BASES_META(o2::SoundListenerComponent)
{
    BASE_CLASS(o2::Component);
    BASE_CLASS(o2::SoundListener);
}
END_META;
CLASS_FIELDS_META(o2::SoundListenerComponent)
{
}
END_META;
CLASS_METHODS_META(o2::SoundListenerComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const SoundListenerComponent&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsListening);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<SoundListenerComponent>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStart);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdatePoseFromTransform);
}
END_META;
// --- END META ---
