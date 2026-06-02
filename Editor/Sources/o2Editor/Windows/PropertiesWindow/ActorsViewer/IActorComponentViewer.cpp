#include "o2Editor/stdafx.h"
#include "IActorComponentViewer.h"

#include "o2/Application/VKCodes.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/Image.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/StringUtils.h"
#include "o2/Utils/System/Clipboard.h"
#include "o2/Utils/System/ShortcutKeys.h"
#include "o2Editor/Actions/AddComponent.h"
#include "o2Editor/Actions/RemoveComponent.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/UI/SpoilerWithHead.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"

namespace Editor
{
    IActorComponentViewer::IActorComponentViewer()
    {
        PushEditorScopeOnStack scope;

        mSpoiler = o2UI.CreateWidget<SpoilerWithHead>();

        mSpoiler->expandHeight = false;
        mSpoiler->expandWidth = true;
        mSpoiler->fitByChildren = true;
        mSpoiler->borderBottom = 5;
        mSpoiler->SetCaption("Component");
        mSpoiler->GetIcon()->SetImageName("ui/UI4_component_icon.png");
        mSpoiler->GetIcon()->layout->center -= Vec2F(2, 0);
        mSpoiler->GetIcon()->GetImage()->SetColor(Color4(235, 255, 253));

        mRemoveButton = o2UI.CreateButton("", THIS_FUNC(RemoveTargetComponents), "close");
        *mRemoveButton->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(1, 0));
        mSpoiler->AddInternalWidget(mRemoveButton);

        mOptionsMenu = o2UI.CreateWidget<ContextMenu>("standard");
        mOptionsMenu->name = "options context";
        mOptionsMenu->AddItem("Copy", THIS_FUNC(CopyComponent));
        mOptionsMenu->AddItem("Cut", THIS_FUNC(CutComponent));
        mOptionsMenu->AddItem("Paste", THIS_FUNC(PasteComponent));
        mSpoiler->AddInternalWidget(mOptionsMenu);

        mOptionsButton = o2UI.CreateWidget<Button>("arrow");
        mOptionsButton->name = "optionsButton";
        *mOptionsButton->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(-15, 0));
        mOptionsButton->onClick = [this]() {
            mOptionsMenu->Show(mOptionsButton->transform->worldCenter);
        };
        mSpoiler->AddInternalWidget(mOptionsButton);

        mSpoiler->SetExpanded(true);
    }

    IActorComponentViewer::~IActorComponentViewer()
    {}

    void IActorComponentViewer::SetTargetComponents(const Vector<Ref<Component>>& components)
    {
        mTargetComponents = components;

        if (!components.IsEmpty())
        {
            String caption = components[0]->GetType().InvokeStatic<String>("GetName");
            if (caption.IsEmpty())
                caption = GetSmartName(components[0]->GetType().GetName());

            mSpoiler->SetCaption(caption);
            mSpoiler->GetIcon()->SetImageName(components[0]->GetType().InvokeStatic<String>("GetIcon"));
        }
    }

    Ref<Widget> IActorComponentViewer::GetWidget() const
    {
        return mSpoiler;
    }

    void IActorComponentViewer::Expand()
    {
        mSpoiler->Expand();
    }

    void IActorComponentViewer::Collapse()
    {
        mSpoiler->Collapse();
    }

    void IActorComponentViewer::Refresh()
    {    }

    void IActorComponentViewer::SetPropertiesEnabled(bool enabled)
    {
        if (mPropertiesEnabled == enabled)
            return;

        mPropertiesEnabled = enabled;

        if (mPropertiesEnabled)
            OnPropertiesEnabled();
        else
            OnPropertiesDisabled();
    }

    bool IActorComponentViewer::IsPropertiesEnabled() const
    {
        return mPropertiesEnabled;
    }

    void IActorComponentViewer::RemoveTargetComponents()
    {
        auto action = mmake<RemoveComponentAction>(mTargetComponents);
        action->Redo();
        o2EditorSceneWindow.DoneAction(action);

        mTargetComponents.Clear();
    }

    void IActorComponentViewer::CopyComponent()
    {
        if (mTargetComponents.IsEmpty())
            return;

        DataDocument data;
        data = mTargetComponents[0];

        WString clipboardData = data.SaveAsString();
        Clipboard::SetText(clipboardData);
    }

    void IActorComponentViewer::CutComponent()
    {
        if (mTargetComponents.IsEmpty())
            return;

        mOptionsMenu->Hide(true);

        CopyComponent();
        RemoveTargetComponents();
    }

    void IActorComponentViewer::PasteComponent()
    {
        if (mTargetComponents.IsEmpty())
            return;

        WString clipboardData = Clipboard::GetText();
        if (clipboardData.IsEmpty())
            return;

        DataDocument data;
        if (!data.LoadFromData(clipboardData))
            return;

        ForcePopEditorScopeOnStack scope;

        Vector<Ref<Actor>> actors;
        Vector<IObject*>   targets;
        for (auto& comp : mTargetComponents)
        {
            auto actor = comp->GetActor();
            actors.Add(actor);
            targets.Add((IObject*)actor.Get());
        }

        auto action = mmake<AddComponentAction>(actors, data);
        action->Redo();
        o2EditorSceneWindow.DoneAction(action);

        o2EditorPropertiesWindow.SetTargets(targets);
    }
    void IActorComponentViewer::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        onPropertyChanged(field, byUser);
    }

    void IActorComponentViewer::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                                          const Vector<DataDocument>& after)
    {
        onPropertyChangeCompleted(path, before, after);
    }

}
// --- META ---

DECLARE_CLASS(Editor::IActorComponentViewer, Editor__IActorComponentViewer);
// --- END META ---
