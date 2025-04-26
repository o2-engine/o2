#include "o2Editor/stdafx.h"
#include "ObjectViewer.h"

#include "IObjectPropertiesViewer.h"
#include "Properties.h"

namespace Editor
{
    ObjectViewer::ObjectViewer(RefCounter* refCounter):
        VerticalLayout(refCounter)
    {
        spacing = 5;
        expandHeight = false;
        expandWidth = true;
        fitByChildren = true;
        *layout = WidgetLayout::BothStretch();
    }

    void ObjectViewer::Refresh(const Vector<Pair<IObject*, IObject*>>& targetObjects)
    {
        if (targetObjects.IsEmpty())
            return;

        const Type* objectType = !targetObjects.IsEmpty() ? &targetObjects[0].first->GetType() : nullptr;

        for (auto& target : targetObjects)
        {
            if (!target.first->GetType().IsBasedOn(*objectType))
                return;
        }

        bool requiredNewViewer = mPropertiesViewer ? 
            !objectType->IsBasedOn(*mPropertiesViewer->GetViewingObjectType()) : 
            objectType != nullptr;

        if (requiredNewViewer)
        {
            if (mPropertiesViewer)
            {
                RemoveChild(mPropertiesViewer->GetSpoiler());
                o2EditorProperties.FreeObjectViewer(mPropertiesViewer);
            }

            mPropertiesViewer = o2EditorProperties.CreateObjectViewer(objectType, "");
            mPropertiesViewer->CheckCreateSpoiler(Ref(this));
            mPropertiesViewer->SetHeaderEnabled(false);
            mPropertiesViewer->SetParentContext(mParentContext);
        }

        if (mPropertiesViewer)
            mPropertiesViewer->Refresh(targetObjects);
    }

    void ObjectViewer::Refresh(const Vector<IObject*>& targetObjets)
    {
        auto protoTargets = targetObjets.Convert<Pair<IObject*, IObject*>>(
            [](IObject* x) { return Pair<IObject*, IObject*>(x, nullptr); });

        Refresh(protoTargets);
    }

    void ObjectViewer::SetParentContext(const Ref<PropertiesContext>& context)
    {
        mParentContext = context;

        if (mPropertiesViewer)
            mPropertiesViewer->SetParentContext(context);
    }

    const Type* ObjectViewer::GetViewingObjectType() const
    {
        if (mPropertiesViewer)
            return mPropertiesViewer->GetViewingObjectType();

        return &TypeOf(IObject);
    }

    String ObjectViewer::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void ObjectViewer::OnPropertiesEnabled()
    {
        if (mPropertiesViewer)
            mPropertiesViewer->OnPropertiesEnabled();
    }

    void ObjectViewer::OnPropertiesDisabled()
    {
        if (mPropertiesViewer)
            mPropertiesViewer->OnPropertiesDisabled();
    }

}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::ObjectViewer>);
// --- META ---

DECLARE_CLASS(Editor::ObjectViewer, Editor__ObjectViewer);
// --- END META ---
