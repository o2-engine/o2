#pragma once

#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Types/Containers/Vector.h"

using namespace o2;

namespace o2
{
    class Widget;
    class WidgetLayer;
}

namespace Editor
{
    // -------------------------------------------
    // Editor widget layer header viewer interface
    // -------------------------------------------
    class IWidgetLayerHeaderViewer : public IObject, virtual public RefCounterable
    {
    public:
        // Virtual destructor
        virtual ~IWidgetLayerHeaderViewer() {}

        // Sets target actors
        virtual void SetTargetLayers(const Vector<WidgetLayer*>& layers) {}

        // Returns data widget
        virtual Ref<Widget> GetWidget() const { return nullptr; }

        // Updates properties values
        virtual void Refresh() {}

        // Sets viewer enabled
        void SetPropertiesEnabled(bool enabled);

        // Returns is viewer enabled
        bool IsPropertiesEnabled() const;

        IOBJECT(IWidgetLayerHeaderViewer);

    protected:
        bool mPropertiesEnabled = false; // Is viewer enabled 

    protected:
        // Enable viewer event function
        virtual void OnPropertiesEnabled() {}

        // Disable viewer event function
        virtual void OnPropertiesDisabled() {}
    };
}
// --- META ---

CLASS_BASES_META(Editor::IWidgetLayerHeaderViewer)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(Editor::IWidgetLayerHeaderViewer)
{
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPropertiesEnabled);
}
END_META;
CLASS_METHODS_META(Editor::IWidgetLayerHeaderViewer)
{

    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetLayers, const Vector<WidgetLayer*>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, GetWidget);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPropertiesEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
}
END_META;
// --- END META ---
