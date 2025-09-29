#include "o2Editor/stdafx.h"
#include "DefaultActorPropertiesViewer.h"

#include "o2/Utils/StringUtils.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/UI/SpoilerWithHead.h"
#include "o2Editor/Windows/WindowsManager.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"

namespace Editor
{
    DefaultActorPropertiesViewer::DefaultActorPropertiesViewer()
    {}

    DefaultActorPropertiesViewer::~DefaultActorPropertiesViewer()
    {}

    void DefaultActorPropertiesViewer::SetTargetActors(const Vector<Actor*>& actors)
    {
        if (!actors.IsEmpty())
            mSpoiler->SetCaption(GetSmartName(mActorType->GetName()));

        mTargetActors = actors;
        Refresh();
    }

    const Type* DefaultActorPropertiesViewer::GetActorType() const
    {
        return mActorType;
    }

    void DefaultActorPropertiesViewer::SpecializeActorType(const Type* type)
    {
        mSpoiler->name = "actor " + type->GetName();
        mActorType = type;
    }

    void DefaultActorPropertiesViewer::Refresh()
    {
        bool requiredNewViewer = mViewer ? mViewer->GetViewingObjectType() != mActorType : mActorType != nullptr;
        if (requiredNewViewer) 
        {
            if (mViewer)
                o2EditorProperties.FreeObjectViewer(mViewer);

            mViewer = o2EditorProperties.CreateObjectViewer(mActorType, "", THIS_FUNC(OnPropertyChangeCompleted), THIS_FUNC(OnPropertyChanged));
            mViewer->CheckCreateSpoiler(mSpoiler);
            mViewer->SetHeaderEnabled(false);
        }

        if (mViewer)
        {
            mViewer->Refresh(mTargetActors.Convert<Pair<IObject*, IObject*>>([](Actor* x) {
                return Pair<IObject*, IObject*>(dynamic_cast<IObject*>(x), dynamic_cast<IObject*>(x->GetPrototypeLink().Get()));
            }));
        }
    }

    bool DefaultActorPropertiesViewer::IsEmpty() const
    {
        return mViewer && mViewer->IsEmpty();
    }

    void DefaultActorPropertiesViewer::OnPropertiesEnabled()
    {
        if (mViewer)
            mViewer->OnPropertiesEnabled();
    }

    void DefaultActorPropertiesViewer::OnPropertiesDisabled()
    {
        if (mViewer)
            mViewer->OnPropertiesDisabled();
    }

    void DefaultActorPropertiesViewer::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        IActorPropertiesViewer::OnPropertyChanged(field, byUser);
    }

    void DefaultActorPropertiesViewer::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after)
    {
        IActorPropertiesViewer::OnPropertyChangeCompleted(path, before, after);

        for (auto& actors : mTargetActors)
            actors->OnChanged();

        o2EditorSceneWindow.DoneActorPropertyChangeAction(path, before, after);
    }
}
// --- META ---

DECLARE_CLASS(Editor::DefaultActorPropertiesViewer, Editor__DefaultActorPropertiesViewer);
// --- END META ---
