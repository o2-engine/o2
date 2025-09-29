#include "o2Editor/Windows/PropertiesWindow/IPropertiesViewer.h"
#include "o2Editor/stdafx.h"
#include "DefaultPropertiesViewer.h"

#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Properties/ObjectViewer.h"
#include "o2Editor/Properties/IPropertyField.h"

namespace Editor
{
    DefaultPropertiesViewer::DefaultPropertiesViewer()
    {
        auto scrollArea = o2UI.CreateScrollArea("backless");
        *scrollArea->layout = WidgetLayout::BothStretch(0, 0, 15, 0);
        scrollArea->SetViewLayout(Layout::BothStretch());
        scrollArea->SetClippingLayout(Layout::BothStretch());
        scrollArea->name = "scroll area";
        mContentWidget = scrollArea;

        using thisclass = DefaultPropertiesViewer;

        mViewer = mmake<ObjectViewer>();
        *mViewer->layout = WidgetLayout::BothStretch(5, 0, 5, 5);
        mViewer->onPropertyChanged = THIS_FUNC(OnPropertyChanged);
        mViewer->onPropertyChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
        mContentWidget->AddChild(mViewer);
    }

    DefaultPropertiesViewer::~DefaultPropertiesViewer()
    {}

    void DefaultPropertiesViewer::Refresh()
    {
        if (mTargets.IsEmpty())
            return;

        mViewer->Refresh(mTargets);
    }

    void DefaultPropertiesViewer::SetTargets(const Vector<IObject*>& targets)
    {
        mTargets = targets;
        Refresh();
    }

    void DefaultPropertiesViewer::OnPropertiesEnabled()
    {
        if (mViewer)
            mViewer->OnPropertiesEnabled();
    }

    void DefaultPropertiesViewer::OnPropertiesDisabled()
    {
        if (mViewer)
            mViewer->OnPropertiesDisabled();
    }

    void DefaultPropertiesViewer::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        IPropertiesViewer::OnPropertyChanged(field, byUser);
    }

    void DefaultPropertiesViewer::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after)
    {
        IPropertiesViewer::OnPropertyChangeCompleted(path, before, after);
    }

}
// --- META ---

DECLARE_CLASS(Editor::DefaultPropertiesViewer, Editor__DefaultPropertiesViewer);
// --- END META ---
