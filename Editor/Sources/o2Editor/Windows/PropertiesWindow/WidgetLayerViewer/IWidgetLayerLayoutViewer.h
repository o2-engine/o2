#pragma once

#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Reflection/Type.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Properties/IPropertyField.h"

using namespace o2;

namespace o2
{
    class Widget;
    class WidgetLayer;
}

namespace Editor
{
    FORWARD_CLASS_REF(SpoilerWithHead);

    // -------------------------------------------
    // Editor widget layer layout viewer interface
    // -------------------------------------------
    class IWidgetLayerLayoutViewer : public IObject, virtual public RefCounterable
    {
    public:
        typedef Function<void(const Ref<IPropertyField>& field, bool byUser)> OnPropertyChangedFunc;
        typedef Function<void(const String& path, const Vector<DataDocument>& before, 
                               const Vector<DataDocument>& after)> OnPropertyChangeCompletedFunc;

        OnPropertyChangedFunc         onPropertyChanged;         // Called when property changed
        OnPropertyChangeCompletedFunc onPropertyChangeCompleted; // Called when property change completed

    public:
        // Default constructor. Initializes data widget
        IWidgetLayerLayoutViewer();

        // Virtual destructor
        virtual ~IWidgetLayerLayoutViewer();

        // Sets target actors
        virtual void SetTargetLayers(const Vector<WidgetLayer*>& layers) {}

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

        IOBJECT(IWidgetLayerLayoutViewer);

    protected:
        Ref<SpoilerWithHead> mSpoiler;

        bool mPropertiesEnabled = false; // Is viewer enabled 

    protected:
        // Enable viewer event function
        virtual void OnPropertiesEnabled() {}

        // Disable viewer event function
        virtual void OnPropertiesDisabled() {}

        // Called when some property changed
        virtual void OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser);

        // Called when some property change completed
        virtual void OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after);

        friend class WidgetLayerViewer;
    };
}
// --- META ---

CLASS_BASES_META(Editor::IWidgetLayerLayoutViewer)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(Editor::IWidgetLayerLayoutViewer)
{
    FIELD().PUBLIC().NAME(onPropertyChanged);
    FIELD().PUBLIC().NAME(onPropertyChangeCompleted);
    FIELD().PROTECTED().NAME(mSpoiler);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPropertiesEnabled);
}
END_META;
CLASS_METHODS_META(Editor::IWidgetLayerLayoutViewer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetLayers, const Vector<WidgetLayer*>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, GetWidget);
    FUNCTION().PUBLIC().SIGNATURE(void, Expand);
    FUNCTION().PUBLIC().SIGNATURE(void, Collapse);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPropertiesEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChanged, const Ref<IPropertyField>&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChangeCompleted, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
}
END_META;
// --- END META ---
