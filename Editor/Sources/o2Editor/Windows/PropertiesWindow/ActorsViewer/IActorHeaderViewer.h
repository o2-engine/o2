#pragma once

#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Types/Containers/Vector.h"

using namespace o2;

namespace o2
{
    class Actor;
    class Widget;
}

namespace Editor
{
    // ------------------------------------
    // Editor actor header viewer interface
    // ------------------------------------
    class IActorHeaderViewer: public IObject, virtual public RefCounterable
    {
    public:
        // Virtual destructor
        virtual ~IActorHeaderViewer() {}

        // Sets target actors
        virtual void SetTargetActors(const Vector<Actor*>& actors) {}

        // Returns data widget
        virtual Ref<Widget> GetWidget() const { return nullptr; }

        // Updates properties values
        virtual void Refresh() {}

        // Sets viewer enabled
        void SetPropertiesEnabled(bool enabled);

        // Returns is viewer enabled
        bool IsPropertiesEnabled() const;

        IOBJECT(IActorHeaderViewer);

    protected:
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

CLASS_BASES_META(Editor::IActorHeaderViewer)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(Editor::IActorHeaderViewer)
{
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPropertiesEnabled);
}
END_META;
CLASS_METHODS_META(Editor::IActorHeaderViewer)
{

    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetActors, const Vector<Actor*>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, GetWidget);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPropertiesEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
}
END_META;
// --- END META ---
