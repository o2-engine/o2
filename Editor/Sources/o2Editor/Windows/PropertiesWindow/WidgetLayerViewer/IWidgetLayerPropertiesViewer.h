#pragma once

#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Reflection/Type.h"
#include "o2/Utils/Types/Containers/Vector.h"

using namespace o2;

namespace o2
{
    class Widget;
    class WidgetLayer;
}

namespace Editor
{
    FORWARD_CLASS_REF(SpoilerWithHead);

    // -----------------------------------------------
    // Editor widget layer properties viewer interface
    // -----------------------------------------------
    class IWidgetLayerPropertiesViewer : public IObject, virtual public RefCounterable
    {
    public:
        // Default constructor. Initializes data widget
        IWidgetLayerPropertiesViewer();

        // Virtual destructor
        virtual ~IWidgetLayerPropertiesViewer();

        // Sets target actors
        virtual void SetTargetLayers(const Vector<WidgetLayer*>& layers) {}

        // Returns viewing layer drawable type 
        virtual const Type* GetDrawableType() const { return nullptr; }

        // Returns data widget
        virtual Ref<Widget> GetWidget() const;

        // Expands view
        void Expand();

        // Collapse view
        void Collapse();

        // Updates all actor values
        virtual void Refresh();

        // Returns is there no properties
        virtual bool IsEmpty() const;

        // Sets viewer enabled
        void SetPropertiesEnabled(bool enabled);

        // Returns is viewer enabled
        bool IsPropertiesEnabled() const;

        IOBJECT(IWidgetLayerPropertiesViewer);

    protected:
        Ref<SpoilerWithHead> mSpoiler;

        bool mPropertiesEnabled = false; // Is viewer enabled 

    protected:
        // Enable viewer event function
        virtual void OnPropertiesEnabled() {}

        // Disable viewer event function
        virtual void OnPropertiesDisabled() {}
    };
}
// --- META ---

CLASS_BASES_META(Editor::IWidgetLayerPropertiesViewer)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(Editor::IWidgetLayerPropertiesViewer)
{
    FIELD().PROTECTED().NAME(mSpoiler);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPropertiesEnabled);
}
END_META;
CLASS_METHODS_META(Editor::IWidgetLayerPropertiesViewer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetLayers, const Vector<WidgetLayer*>&);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetDrawableType);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, GetWidget);
    FUNCTION().PUBLIC().SIGNATURE(void, Expand);
    FUNCTION().PUBLIC().SIGNATURE(void, Collapse);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEmpty);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPropertiesEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
}
END_META;
// --- END META ---
