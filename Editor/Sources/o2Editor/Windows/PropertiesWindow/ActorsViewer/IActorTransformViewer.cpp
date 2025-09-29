#include "o2Editor/stdafx.h"
#include "IActorTransformViewer.h"

#include "o2/Render/Sprite.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Image.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UI/SpoilerWithHead.h"

namespace Editor
{
    IActorTransformViewer::IActorTransformViewer()
    {
        PushEditorScopeOnStack scope;

        mSpoiler = o2UI.CreateWidget<SpoilerWithHead>();

        mSpoiler->borderBottom = 5;
        mSpoiler->SetCaption("Transform");
        mSpoiler->GetIcon()->SetImageName("ui/UI4_transform_icon_white.png");

        mSpoiler->SetExpanded(true);
    }

    IActorTransformViewer::~IActorTransformViewer()
    {}

    Ref<Widget> IActorTransformViewer::GetWidget() const
    {
        return mSpoiler;
    }

    void IActorTransformViewer::Expand()
    {
        mSpoiler->Expand();
    }

    void IActorTransformViewer::Collapse()
    {
        mSpoiler->Collapse();
    }

    void IActorTransformViewer::Refresh()
    {}

    void IActorTransformViewer::SetPropertiesEnabled(bool enabled)
    {
        if (mPropertiesEnabled == enabled)
            return;

        mPropertiesEnabled = enabled;

        if (mPropertiesEnabled)
            OnPropertiesEnabled();
        else
            OnPropertiesDisabled();
    }

    bool IActorTransformViewer::IsPropertiesEnabled() const
    {
        return mPropertiesEnabled;
    }

    void IActorTransformViewer::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        onPropertyChanged(field, byUser);
    }

    void IActorTransformViewer::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                                          const Vector<DataDocument>& after)
    {
        onPropertyChangeCompleted(path, before, after);
    }

}
// --- META ---

DECLARE_CLASS(Editor::IActorTransformViewer, Editor__IActorTransformViewer);
// --- END META ---
