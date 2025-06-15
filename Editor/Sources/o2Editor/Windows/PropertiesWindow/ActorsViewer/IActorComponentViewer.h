#pragma once

#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Reflection/Type.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Properties/PropertiesContext.h"

using namespace o2;

namespace o2
{
    class Actor;
    class Button;
    class Component;
    class Widget;
}

namespace Editor
{
    FORWARD_CLASS_REF(SpoilerWithHead);

    // ---------------------------------------
    // Editor actor component viewer interface
    // ---------------------------------------
    class IActorComponentViewer: public IObject, virtual public RefCounterable
    {
    public:
        // Default constructor. Initializes data widget
        IActorComponentViewer();

        // Virtual destructor
        virtual ~IActorComponentViewer();

        // Sets target actors
        virtual void SetTargetComponents(const Vector<Ref<Component>>& components);

        // Returns viewing component type 
        virtual const Type* GetComponentType() const { return nullptr; }

        // Returns data widget
        virtual Ref<Widget> GetWidget() const;

        // Expands view
        void Expand();

        // Collapse view
        void Collapse();

        // Updates all component values
        virtual void Refresh();

        // Sets viewer enabled
        void SetPropertiesEnabled(bool enabled);

        // Returns is viewer enabled
        bool IsPropertiesEnabled() const;

        IOBJECT(IActorComponentViewer);

    protected:
        Vector<Ref<Component>> mTargetComponents; // Target components

        Ref<SpoilerWithHead> mSpoiler;      // Component's spoiler
        Ref<Button>          mRemoveButton; // Remove component button

        bool mPropertiesEnabled = false; // Is viewer enabled 

    protected:
        // Removes target components
        void RemoveTargetComponents();

        // Enable viewer event function
        virtual void OnPropertiesEnabled() {}

        // Disable viewer event function
        virtual void OnPropertiesDisabled() {}

        friend class ActorViewer;
    };
}
// --- META ---

CLASS_BASES_META(Editor::IActorComponentViewer)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(Editor::IActorComponentViewer)
{
    FIELD().PROTECTED().NAME(mTargetComponents);
    FIELD().PROTECTED().NAME(mSpoiler);
    FIELD().PROTECTED().NAME(mRemoveButton);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPropertiesEnabled);
}
END_META;
CLASS_METHODS_META(Editor::IActorComponentViewer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetComponents, const Vector<Ref<Component>>&);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetComponentType);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, GetWidget);
    FUNCTION().PUBLIC().SIGNATURE(void, Expand);
    FUNCTION().PUBLIC().SIGNATURE(void, Collapse);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPropertiesEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveTargetComponents);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
}
END_META;
// --- END META ---
