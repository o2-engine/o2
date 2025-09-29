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

    void IActorHeaderViewer::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        onPropertyChanged(field, byUser);
    }

    void IActorHeaderViewer::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after)
    {
        onPropertyChangeCompleted(path, before, after);
    }
}
// --- META ---

DECLARE_CLASS(Editor::IActorHeaderViewer, Editor__IActorHeaderViewer);
// --- END META ---
