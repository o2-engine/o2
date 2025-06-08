#include "o2Editor/stdafx.h"
#include "IWidgetLayerHeadViewer.h"
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
}
// --- META ---

DECLARE_CLASS(Editor::IWidgetLayerHeaderViewer, Editor__IWidgetLayerHeaderViewer);
// --- END META ---
