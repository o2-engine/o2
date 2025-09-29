#include "o2Editor/stdafx.h"
#include "IWidgetLayerHeadViewer.h"

#include "o2/Scene/UI/Widget.h"

namespace Editor
{
    void IWidgetLayerHeaderViewer::SetPropertiesEnabled(bool enabled)
    {
        if (mPropertiesEnabled == enabled)
            return;

        mPropertiesEnabled = enabled;

        if (mPropertiesEnabled)
            OnPropertiesEnabled();
        else
            OnPropertiesDisabled();
    }

    bool IWidgetLayerHeaderViewer::IsPropertiesEnabled() const
    {
        return mPropertiesEnabled;
    }

    void IWidgetLayerHeaderViewer::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        onPropertyChanged(field, byUser);
    }

    void IWidgetLayerHeaderViewer::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after)
    {
        onPropertyChangeCompleted(path, before, after);
    }
}
// --- META ---

DECLARE_CLASS(Editor::IWidgetLayerHeaderViewer, Editor__IWidgetLayerHeaderViewer);
// --- END META ---
