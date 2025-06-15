#include "o2Editor/stdafx.h"
#include "Reparent.h"

#include "o2/Scene/Actor.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/TreeWindow/SceneHierarchyTree.h"
#include "o2Editor/Windows/TreeWindow/TreeWindow.h"

namespace Editor
{
    ReparentAction::ReparentAction()
    {}

    ReparentAction::ReparentAction(const Vector<Ref<SceneEditableObject>>& beginObjects)
    {
        for (auto& object : beginObjects)
        {
            ObjectInfo info;

            auto parent = object->GetEditableParent();

            Vector<Ref<SceneEditableObject>> parentChildren = parent ? 
                parent->GetEditableChildren() : 
                DynamicCastVector<SceneEditableObject>(o2Scene.GetRootActors());

            int actorIdx = parentChildren.IndexOf(object);

            info.objectId = object->GetID();
            info.objectHierarchyIdx = o2Scene.GetObjectHierarchyIdx(object);
            info.lastParentId = parent ? parent->GetID() : 0;
            info.lastPrevObjectId = actorIdx > 0 ? parentChildren[actorIdx - 1]->GetID() : 0;
            info.transform = object->GetTransform();

            objectsInfos.Add(info);
        }

        objectsInfos.Sort([](auto& a, auto& b) { return a.objectHierarchyIdx < b.objectHierarchyIdx; });
    }

    ReparentAction::~ReparentAction()
    {}

    void ReparentAction::ObjectsReparented(const Ref<SceneEditableObject>& newParent, const Ref<SceneEditableObject>& prevActor)
    {
        newParentId = newParent ? newParent->GetID() : 0;
        newPrevObjectId = prevActor ? prevActor->GetID() : 0;
    }

    String ReparentAction::GetName() const
    {
        return "Actors rearrange";
    }

    void ReparentAction::Redo()
    {
        auto parent = o2Scene.GetEditableObjectByID(newParentId);
        auto prevObject = o2Scene.GetEditableObjectByID(newPrevObjectId);

        if (parent)
        {
            int insertIdx = parent->GetEditableChildren().IndexOf(prevObject) + 1;

            for (auto& info : objectsInfos)
            {
                auto object = o2Scene.GetEditableObjectByID(info.objectId);

                object->SetEditableParent(nullptr);
                parent->AddEditableChild(object, insertIdx++);
                object->SetTransform(info.transform);
            }
        }
        else
        {
            int insertIdx = 0;
            
            if (auto prevActor = DynamicCast<Actor>(prevObject))
                insertIdx = o2Scene.GetRootActors().IndexOf(prevActor) + 1;

            for (auto& info : objectsInfos)
            {
                auto object = o2Scene.GetEditableObjectByID(info.objectId);

                object->SetEditableParent(nullptr);
                object->SetIndexInSiblings(insertIdx++);
                object->SetTransform(info.transform);
            }
        }

        o2EditorTree.UpdateTreeView();
    }

    void ReparentAction::Undo()
    {
        for (auto& info : objectsInfos)
        {
            auto object = o2Scene.GetEditableObjectByID(info.objectId);
            auto parent = o2Scene.GetEditableObjectByID(info.lastParentId);
            auto prevObject = o2Scene.GetEditableObjectByID(info.lastPrevObjectId);

            object->SetEditableParent(nullptr);

            if (parent)
            {
                int idx = parent->GetEditableChildren().IndexOf(prevObject) + 1;
                parent->AddEditableChild(object, idx);
                object->SetTransform(info.transform);
            }
            else
            {
                int idx = o2Scene.GetRootEditableObjects().IndexOf(prevObject) + 1;
                object->SetIndexInSiblings(idx);
                object->SetTransform(info.transform);
            }
        }

        o2EditorTree.UpdateTreeView();
    }
}
// --- META ---

DECLARE_CLASS(Editor::ReparentAction, Editor__ReparentAction);
// --- END META ---
