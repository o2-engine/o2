#include "o2Editor/stdafx.h"
#include "DefaultActorComponentViewer.h"

#include "o2Editor/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/UI/SpoilerWithHead.h"
#include "o2Editor/Windows/WindowsManager.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"

namespace Editor
{
    DefaultActorComponentViewer::DefaultActorComponentViewer()
    {}

    DefaultActorComponentViewer::~DefaultActorComponentViewer()
    {}

    void DefaultActorComponentViewer::SetTargetComponents(const Vector<Ref<Component>>& components)
    {
        IActorComponentViewer::SetTargetComponents(components);
        Refresh();
    }

    const Type* DefaultActorComponentViewer::GetComponentType() const
    {
        return mComponentType;
    }

    void DefaultActorComponentViewer::Refresh()
    {
        const Type* objectsType = &mTargetComponents[0]->GetType();

        if (mComponentType != objectsType)
        {
            mComponentType = objectsType;
            mSpoiler->name = "component " + mComponentType->GetName();

            if (mViewer)
                o2EditorProperties.FreeObjectViewer(mViewer);

            mViewer = o2EditorProperties.CreateObjectViewer(mComponentType, (String)"component:" + mComponentType->GetName(),
                                                            THIS_FUNC(OnPropertyChanged));

            mViewer->CheckCreateSpoiler(mSpoiler);
            mViewer->SetHeaderEnabled(false);
        }

        if (mViewer)
        {
            mViewer->Refresh(mTargetComponents.Convert<Pair<IObject*, IObject*>>([](const Ref<Component>& x) {
                return Pair<IObject*, IObject*>(dynamic_cast<IObject*>(x.Get()), dynamic_cast<IObject*>(x->GetPrototypeLink().Lock().Get()));
            }));
        }
    }

    void DefaultActorComponentViewer::OnPropertyChanged(const String& path, const Vector<DataDocument>& before,
                                                        const Vector<DataDocument>& after)
    {
        for (auto& component : mTargetComponents)
            component->GetActor()->OnChanged();

        o2EditorSceneWindow.DoneActorPropertyChangeAction(path, before, after);
    }

    void DefaultActorComponentViewer::OnPropertiesEnabled()
    {
        if (mViewer)
            mViewer->OnPropertiesEnabled();
    }

    void DefaultActorComponentViewer::OnPropertiesDisabled()
    {
        if (mViewer)
            mViewer->OnPropertiesDisabled();
    }

}
// --- META ---

DECLARE_CLASS(Editor::DefaultActorComponentViewer, Editor__DefaultActorComponentViewer);
// --- END META ---
