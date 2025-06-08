#include "o2Editor/stdafx.h"
#include "IActorHeaderViewer.h"

#include "o2/Scene/UI/Widget.h"

namespace Editor
{
    void IActorHeaderViewer::SetPropertiesEnabled(bool enabled)
    {
        if (mPropertiesEnabled == enabled)
            return;

        mPropertiesEnabled = enabled;

        if (mPropertiesEnabled)
            OnPropertiesEnabled();
        else
            OnPropertiesDisabled();
    }

    bool IActorHeaderViewer::IsPropertiesEnabled() const
    {
        return mPropertiesEnabled;
    }
}
// --- META ---

DECLARE_CLASS(Editor::IActorHeaderViewer, Editor__IActorHeaderViewer);
// --- END META ---
