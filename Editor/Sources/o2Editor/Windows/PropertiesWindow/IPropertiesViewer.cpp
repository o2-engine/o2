#include "o2Editor/stdafx.h"
#include "IPropertiesViewer.h"

#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Serialization/DataValue.h"

namespace Editor
{
    IPropertiesViewer::IPropertiesViewer()
    {}

    IPropertiesViewer::IPropertiesViewer(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    IPropertiesViewer::~IPropertiesViewer()
    {}

    const Type* IPropertiesViewer::GetViewingObjectType() const
    {
        return nullptr;
    }

    void IPropertiesViewer::Refresh()
    {}

    void IPropertiesViewer::SetPropertiesEnabled(bool enabled)
    {
        if (mPropertiesEnabled == enabled)
            return;

        mPropertiesEnabled = enabled;

        if (mPropertiesEnabled)
            OnPropertiesEnabled();
        else
            OnPropertiesDisabled();
    }

    bool IPropertiesViewer::IsEnabled() const
    {
        return mPropertiesEnabled;
    }

}
// --- META ---

DECLARE_CLASS(Editor::IPropertiesViewer, Editor__IPropertiesViewer);
// --- END META ---
