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

    void IPropertiesViewer::SetTargets(const Vector<IObject*>& targets)
    {
        mTargets = targets;
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

    void IPropertiesViewer::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        onPropertyChanged(mTargets, field, byUser);
    }

    void IPropertiesViewer::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                                     const Vector<DataDocument>& after)
    {
        onPropertyChangeCompleted(mTargets, path, before, after);
    }

}
// --- META ---

DECLARE_CLASS(Editor::IPropertiesViewer, Editor__IPropertiesViewer);
// --- END META ---
