#include "o2Editor/stdafx.h"
#include "IWidgetLayerPropertiesViewer.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Image.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UI/SpoilerWithHead.h"

namespace Editor
{
    IWidgetLayerPropertiesViewer::IWidgetLayerPropertiesViewer()
    {
        PushEditorScopeOnStack scope;

        mSpoiler = o2UI.CreateWidget<SpoilerWithHead>();

        mSpoiler->expandHeight = false;
        mSpoiler->expandWidth = true;
        mSpoiler->fitByChildren = true;
        mSpoiler->borderBottom = 5;
        mSpoiler->SetCaption("Transform");
        mSpoiler->GetIcon()->SetImageName("ui/UI4_transform_icon_white.png");

        mSpoiler->SetExpanded(true);
    }

    IWidgetLayerPropertiesViewer::~IWidgetLayerPropertiesViewer()
    {}

    Ref<Widget> IWidgetLayerPropertiesViewer::GetWidget() const
    {
        return mSpoiler;
    }

    void IWidgetLayerPropertiesViewer::Expand()
    {
        mSpoiler->Expand();
    }

    void IWidgetLayerPropertiesViewer::Collapse()
    {
        mSpoiler->Collapse();
    }

    void IWidgetLayerPropertiesViewer::Refresh()
    {}

    bool IWidgetLayerPropertiesViewer::IsEmpty() const
    {
        return true;
    }

    void IWidgetLayerPropertiesViewer::SetPropertiesEnabled(bool enabled)
    {
        if (mPropertiesEnabled == enabled)
            return;

        mPropertiesEnabled = enabled;

        if (mPropertiesEnabled)
            OnPropertiesEnabled();
        else
            OnPropertiesDisabled();
    }

    bool IWidgetLayerPropertiesViewer::IsPropertiesEnabled() const
    {
        return mPropertiesEnabled;
    }

    void IWidgetLayerPropertiesViewer::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        onPropertyChanged(field, byUser);
    }

    void IWidgetLayerPropertiesViewer::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after)
    {
        onPropertyChangeCompleted(path, before, after);
    }
}
// --- META ---

DECLARE_CLASS(Editor::IWidgetLayerPropertiesViewer, Editor__IWidgetLayerPropertiesViewer);
// --- END META ---
