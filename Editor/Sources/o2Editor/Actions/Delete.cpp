#include "o2Editor/stdafx.h"
#include "Delete.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/IActionsUIBridge.h"

namespace Editor
{

    DeleteAction::DeleteAction()
    {}

    DeleteAction::DeleteAction(const Vector<Ref<SceneEditableObject>>& objects)
    {
        for (auto& object : objects)
        {
            ObjectInfo info;
            info.objectData.Set(object);
            info.idx = o2Scene.GetObjectHierarchyIdx(object);

            if (auto parent = object->GetEditableParent())
            {
                info.parentId = parent->GetID();

                auto parentChilds = parent->GetEditableChildren();
                info.prevObjectId = 0;

                for (auto& child : parentChilds)
                {
                    if (child == object)
                        break;

                    info.prevObjectId = child->GetID();
                }
            }
            else
            {
                info.parentId = 0;

                auto rootObjects = o2Scene.GetRootEditableObjects();
                info.prevObjectId = 0;

                for (auto& child : rootObjects)
                {
                    if (child == object)
                        break;

                    info.prevObjectId = child->GetID();
                }
            }

            objectsInfos.Add(info);
        }

        objectsInfos.Sort([](auto& a, auto& b) { return a.idx < b.idx; });
    }

    String DeleteAction::GetName() const
    {
        return "Actors deletion";
    }

    void DeleteAction::Redo()
    {
        for (auto& info : objectsInfos)
        {
            auto object = o2Scene.GetEditableObjectByID((SceneUID)info.objectData["Value"]["Id"]);
//             if (object)
//                 delete object;
        }

        ActionsUIBridge::Current().ClearSelectionWithoutAction(true);
        ActionsUIBridge::Current().UpdateTreeView();
    }

    void DeleteAction::Undo()
    {
        Ref<SceneEditableObject> lastRestored;

        for (auto& info : objectsInfos)
        {
            auto parent = o2Scene.GetEditableObjectByID(info.parentId);
            if (parent)
            {
                SceneUID prevId = info.prevObjectId;
                int idx = parent->GetEditableChildren().IndexOf([=](auto& x) { return x->GetID() == prevId; }) + 1;

                Ref<SceneEditableObject> newObject;
                info.objectData.Get(newObject);
                parent->AddEditableChild(newObject, idx);

                ActionsUIBridge::Current().SelectObjectWithoutAction(newObject, true);
                lastRestored = newObject;
            }
            else
            {
                int idx = o2Scene.GetRootActors().IndexOf([&](auto& x) { return x->GetID() == info.prevObjectId; }) + 1;

                Ref<SceneEditableObject> newObject;
                info.objectData.Get(newObject);
                newObject->SetIndexInSiblings(idx);

                ActionsUIBridge::Current().SelectObjectWithoutAction(newObject, true);
                lastRestored = newObject;
            }
        }

        ActionsUIBridge::Current().HighlightObjectTreeNode(lastRestored);
        ActionsUIBridge::Current().UpdateTreeView();
    }

    bool DeleteAction::ObjectInfo::operator==(const ObjectInfo& other) const
    {
        return objectData == other.objectData && parentId == other.parentId && prevObjectId == other.prevObjectId;
    }
}
// --- META ---

DECLARE_CLASS(Editor::DeleteAction, Editor__DeleteAction);

DECLARE_CLASS(Editor::DeleteAction::ObjectInfo, Editor__DeleteAction__ObjectInfo);
// --- END META ---
