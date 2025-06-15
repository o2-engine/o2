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

        Vector<IObject*> mTargets; // Viewing targets

    protected:
        // Sets target objects
        void SetTargets(const Vector<IObject*>& targets) override;

        // Enable viewer event function
        void OnPropertiesEnabled() override;

        // Disable viewer event function
        void OnPropertiesDisabled() override;
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
    FIELD().PROTECTED().NAME(mTargets);
}
END_META;
CLASS_METHODS_META(Editor::DefaultPropertiesViewer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PROTECTED().SIGNATURE(void, SetTargets, const Vector<IObject*>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
}
END_META;
// --- END META ---
