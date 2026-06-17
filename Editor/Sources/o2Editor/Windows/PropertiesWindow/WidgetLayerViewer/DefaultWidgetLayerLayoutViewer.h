#pragma once

#include "o2Editor/Windows/PropertiesWindow/WidgetLayerViewer/IWidgetLayerLayoutViewer.h"

namespace Editor
{
    FORWARD_CLASS_REF(IPropertyField);
    FORWARD_CLASS_REF(Vec2FProperty);
    FORWARD_CLASS_REF(FloatProperty);

    // --------------------------------------------
    // Default editor widget layer transform viewer
    // --------------------------------------------
    class DefaultWidgetLayerLayoutViewer : public IWidgetLayerLayoutViewer
    {
    public:
        // Default constructor. Initializes data widget
        DefaultWidgetLayerLayoutViewer();

        // Virtual destructor
        ~DefaultWidgetLayerLayoutViewer();

        // Sets target actors
        void SetTargetLayers(const Vector<WidgetLayer*>& layers) override;

        // Updates properties values
        void Refresh() override;

        IOBJECT(DefaultWidgetLayerLayoutViewer);

    protected:
        Vector<WidgetLayer*> mLayers;

        Ref<Vec2FProperty> mPositionProperty;
        Ref<Vec2FProperty> mSizeProperty;
        Ref<Vec2FProperty> mAnchorRightTopProperty;
        Ref<Vec2FProperty> mAnchorLeftBottomProperty;
        Ref<Vec2FProperty> mOffsetRightTopProperty;
        Ref<Vec2FProperty> mOffsetLeftBottomProperty;

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
    };
}
// --- META ---

CLASS_BASES_META(Editor::DefaultWidgetLayerLayoutViewer)
{
    BASE_CLASS(Editor::IWidgetLayerLayoutViewer);
}
END_META;
CLASS_FIELDS_META(Editor::DefaultWidgetLayerLayoutViewer)
{
    FIELD().PROTECTED().NAME(mLayers);
    FIELD().PROTECTED().NAME(mPositionProperty);
    FIELD().PROTECTED().NAME(mSizeProperty);
    FIELD().PROTECTED().NAME(mAnchorRightTopProperty);
    FIELD().PROTECTED().NAME(mAnchorLeftBottomProperty);
    FIELD().PROTECTED().NAME(mOffsetRightTopProperty);
    FIELD().PROTECTED().NAME(mOffsetLeftBottomProperty);
}
END_META;
CLASS_METHODS_META(Editor::DefaultWidgetLayerLayoutViewer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetLayers, const Vector<WidgetLayer*>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChanged, const Ref<IPropertyField>&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChangeCompleted, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
}
END_META;
// --- END META ---
