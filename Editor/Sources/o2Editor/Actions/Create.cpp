#include "o2Editor/stdafx.h"
#include "Create.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/IActionsUIBridge.h"

namespace Editor
{
    CreateAction::CreateAction()
    {}

    CreateAction::CreateAction(const Vector<Ref<SceneEditableObject>>& objects, 
                               const Ref<SceneEditableObject>& parent, const Ref<SceneEditableObject>& prevObject)
    {
        objectsIds = objects.Convert<SceneUID>([](auto& x) { return x->GetID(); });

        objectsData.Set(objects);

        insertParentId = parent ? parent->GetID() : 0;
        insertPrevObjectId = prevObject ? prevObject->GetID() : 0;
    }

    String CreateAction::GetName() const
    {
        return "Create objects";
    }

    void CreateAction::Redo()
    {
        auto parent = o2Scene.GetEditableObjectByID(insertParentId);
        auto prevObject = o2Scene.GetEditableObjectByID(insertPrevObjectId);
        Vector<Ref<SceneEditableObject>> objects;

        if (parent)
        {
            int insertIdx = parent->GetEditableChildren().IndexOf(prevObject) + 1;

            objectsData.Get(objects);

            for (auto& object : objects)
                parent->AddEditableChild(object, insertIdx++);
        }
        else
        {
            int insertIdx = o2Scene.GetRootEditableObjects().IndexOf(prevObject) + 1;

            objectsData.Get(objects);

            for (auto& object : objects)
                object->SetIndexInSiblings(insertIdx++);
        }

        ActionsUIBridge::Current().HighlightObjectTreeNode(objects.Last());
        ActionsUIBridge::Current().SelectObjectsWithoutAction(objects, false);
    }

    void CreateAction::Undo()
    {
        for (auto& objectId : objectsIds)
        {
            auto object = o2Scene.GetEditableObjectByID(objectId);
//             if (object)
//                 delete object;
        }

        ActionsUIBridge::Current().ClearSelectionWithoutAction(true);
    }

}
// --- META ---

DECLARE_CLASS(Editor::CreateAction, Editor__CreateAction);
// --- END META ---
