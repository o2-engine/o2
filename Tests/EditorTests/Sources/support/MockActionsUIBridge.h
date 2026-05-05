#pragma once

#include "o2/Utils/Editor/SceneEditableObject.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Actions/IActionsUIBridge.h"

namespace Editor::Tests
{
    // -------------------------------------------
    // Recording double of IActionsUIBridge
    // Logs every side-effect call for assertions
    // -------------------------------------------
    class RecordingActionsUIBridge : public IActionsUIBridge
    {
    public:
        struct SelectObjectsCall
        {
            o2::Vector<o2::SceneUID> objectIds;
            bool                     additive;
        };

        struct SelectObjectCall
        {
            o2::SceneUID objectId;
            bool         additive;
        };

        o2::Vector<o2::SceneUID>      highlightedObjectIds; // Objects highlighted in tree, in call order
        int                           updateTreeViewCalls = 0; // Count of UpdateTreeView calls
        o2::Vector<SelectObjectsCall> selectObjectsCalls;   // SelectObjects calls, in call order
        o2::Vector<SelectObjectCall>  selectObjectCalls;    // SelectObject calls, in call order
        o2::Vector<int>               clearSelectionCalls;  // sendSelectedMessage flag per ClearSelection call

        void HighlightObjectTreeNode(const o2::Ref<o2::SceneEditableObject>& object) override
        {
            highlightedObjectIds.Add(object ? object->GetID() : 0);
        }

        void UpdateTreeView() override
        {
            updateTreeViewCalls++;
        }

        void SelectObjectsWithoutAction(o2::Vector<o2::Ref<o2::SceneEditableObject>> objects, bool additive) override
        {
            SelectObjectsCall call;
            call.additive = additive;
            for (auto& o : objects)
                call.objectIds.Add(o ? o->GetID() : 0);
            selectObjectsCalls.Add(call);
        }

        void SelectObjectWithoutAction(const o2::Ref<o2::SceneEditableObject>& object, bool additive) override
        {
            selectObjectCalls.Add({ object ? object->GetID() : 0, additive });
        }

        void ClearSelectionWithoutAction(bool sendSelectedMessage) override
        {
            clearSelectionCalls.Add(sendSelectedMessage ? 1 : 0);
        }
    };

    // ----------------------------------------------
    // RAII guard installing recorder as the bridge,
    // restores default forwarder on destruction
    // ----------------------------------------------
    class ScopedActionsUIBridge
    {
    public:
        explicit ScopedActionsUIBridge(RecordingActionsUIBridge& recorder)
        {
            ActionsUIBridge::SetCurrent(&recorder);
        }

        ~ScopedActionsUIBridge()
        {
            ActionsUIBridge::ResetToDefault();
        }

        ScopedActionsUIBridge(const ScopedActionsUIBridge&) = delete;
        ScopedActionsUIBridge& operator=(const ScopedActionsUIBridge&) = delete;
    };
}
