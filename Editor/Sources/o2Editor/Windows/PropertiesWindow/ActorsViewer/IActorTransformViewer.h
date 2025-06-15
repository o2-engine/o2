#pragma once

#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Reflection/Type.h"
#include "o2/Utils/Types/Containers/Vector.h"

using namespace o2;

namespace o2
{
    class Actor;
    class Component;
    class Widget;
}

namespace Editor
{
    FORWARD_CLASS_REF(SpoilerWithHead);

    // ---------------------------------------
    // Editor actor transform viewer interface
    // ---------------------------------------
    class IActorTransformViewer: public IObject, virtual public RefCounterable
    {
    public:
        // Default constructor. Initializes data widget
        IActorTransformViewer();

        // Virtual destructor
        virtual ~IActorTransformViewer();

        // Sets target actors
        virtual void SetTargetActors(const Vector<Actor*>& actors) {}

        // Returns data widget
        virtual Ref<Widget> GetWidget() const;

        // Expands view
        void Expand();

        // Collapse view
        void Collapse();

        // Updates properties values
        virtual void Refresh();

        // Sets viewer enabled
        void SetPropertiesEnabled(bool enabled);

        // Returns is viewer enabled
        bool IsPropertiesEnabled() const;

        IOBJECT(IActorTransformViewer);

    protected:
        Ref<SpoilerWithHead> mSpoiler;

        bool mPropertiesEnabled = false; // Is viewer enabled 

    protected:
        // Enable viewer event function
        virtual void OnPropertiesEnabled() {}

        // Disable viewer event function
        virtual void OnPropertiesDisabled() {}

        friend class ActorViewer;
    };
}
// --- META ---

CLASS_BASES_META(Editor::IActorTransformViewer)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(Editor::IActorTransformViewer)
{
    FIELD().PROTECTED().NAME(mSpoiler);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPropertiesEnabled);
}
END_META;
CLASS_METHODS_META(Editor::IActorTransformViewer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetActors, const Vector<Actor*>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, GetWidget);
    FUNCTION().PUBLIC().SIGNATURE(void, Expand);
    FUNCTION().PUBLIC().SIGNATURE(void, Collapse);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPropertiesEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
}
END_META;
// --- END META ---
