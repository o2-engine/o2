#pragma once

#include "o2Editor/Properties/IPropertyField.h"
#include "o2Editor/Properties/PropertiesContext.h"
#include "o2Editor/Windows/PropertiesWindow/WidgetLayerViewer/IWidgetLayerPropertiesViewer.h"

namespace o2
{
    FORWARD_CLASS_REF(Button);
}

namespace Editor
{
    FORWARD_CLASS_REF(IObjectPropertiesViewer);

    // --------------------------------------
    // Default widget layer properties viewer
    // -------------------------------
    class DefaultWidgetLayerPropertiesViewer : public IWidgetLayerPropertiesViewer
    {
    public:
        // Default constructor. Initializes data widget
        DefaultWidgetLayerPropertiesViewer();

        // Virtual destructor
        ~DefaultWidgetLayerPropertiesViewer();

        // Sets target actors
        void SetTargetLayers(const Vector<WidgetLayer*>& layers) override;

        // Returns viewing layer drawable type 
        const Type* GetDrawableType() const override;

        // Updates all actor values
        void Refresh() override;

        // Returns is there no properties
        bool IsEmpty() const override;

        IOBJECT(DefaultWidgetLayerPropertiesViewer);

    protected:
        Vector<WidgetLayer*> mLayers;                 // Target widget layers
        const Type*          mDrawableType = nullptr; // Target drawable type

        Ref<IObjectPropertiesViewer> mViewer;        // Properties viewer
        Ref<Button>                  mFitSizeButton; // Fit size of layer by drawable size

    protected:
        // Enable viewer event function
        void OnPropertiesEnabled() override;

        // Disable viewer event function
        void OnPropertiesDisabled() override;

        // Called when some property changed
        void OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser) override;

        // Called when some property change completed
        void OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after) override;

        // Fits layer size by drawable size, Called when mFitSizeButton were pressed
        void FitLayerByDrawable();
    };
}
// --- META ---

CLASS_BASES_META(Editor::DefaultWidgetLayerPropertiesViewer)
{
    BASE_CLASS(Editor::IWidgetLayerPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::DefaultWidgetLayerPropertiesViewer)
{
    FIELD().PROTECTED().NAME(mLayers);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mDrawableType);
    FIELD().PROTECTED().NAME(mViewer);
    FIELD().PROTECTED().NAME(mFitSizeButton);
}
END_META;
CLASS_METHODS_META(Editor::DefaultWidgetLayerPropertiesViewer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetLayers, const Vector<WidgetLayer*>&);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetDrawableType);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEmpty);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChanged, const Ref<IPropertyField>&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChangeCompleted, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
    FUNCTION().PROTECTED().SIGNATURE(void, FitLayerByDrawable);
}
END_META;
// --- END META ---
