#pragma once

#include "o2Editor/Windows/PropertiesWindow/IPropertiesViewer.h"

using namespace o2;

namespace Editor
{
    FORWARD_CLASS_REF(ObjectViewer);

    // -------------------------
    // Default properties viewer
    // -------------------------
    class DefaultPropertiesViewer : public IPropertiesViewer
    {
    public:
        DefaultPropertiesViewer();

        // Virtual destructor
        ~DefaultPropertiesViewer();

        // Updates properties values
        void Refresh() override;

        IOBJECT(DefaultPropertiesViewer);

    protected:
        Ref<ObjectViewer> mViewer; // Object viewer

    protected:
        // Sets target objects
        void SetTargets(const Vector<IObject*>& targets) override;

        // Enable viewer event function
        void OnPropertiesEnabled() override;

        // Disable viewer event function
        void OnPropertiesDisabled() override;

        // Called when some property changed
        void OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser) override;

        // Called when some property change completed
        void OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after);
    };

}
// --- META ---

CLASS_BASES_META(Editor::DefaultPropertiesViewer)
{
    BASE_CLASS(Editor::IPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::DefaultPropertiesViewer)
{
    FIELD().PROTECTED().NAME(mViewer);
}
END_META;
CLASS_METHODS_META(Editor::DefaultPropertiesViewer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PROTECTED().SIGNATURE(void, SetTargets, const Vector<IObject*>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChanged, const Ref<IPropertyField>&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChangeCompleted, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
}
END_META;
// --- END META ---
