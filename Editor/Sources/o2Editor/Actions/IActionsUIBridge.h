#pragma once

#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    class SceneEditableObject;
}

namespace Editor
{
    // -------------------------------------------------
    // UI side-effects bridge invoked by scene actions
    // -------------------------------------------------
    class IActionsUIBridge
    {
    public:
        // Virtual destructor
        virtual ~IActionsUIBridge() = default;

        // Highlights tree node of given object
        virtual void HighlightObjectTreeNode(const o2::Ref<o2::SceneEditableObject>& object) = 0;

        // Refreshes scene tree view
        virtual void UpdateTreeView() = 0;

        // Selects objects, doesn't push a select-action
        virtual void SelectObjectsWithoutAction(o2::Vector<o2::Ref<o2::SceneEditableObject>> objects, bool additive) = 0;

        // Selects single object, doesn't push a select-action
        virtual void SelectObjectWithoutAction(const o2::Ref<o2::SceneEditableObject>& object, bool additive) = 0;

        // Clears selection, doesn't push a select-action
        virtual void ClearSelectionWithoutAction(bool sendSelectedMessage) = 0;
    };

    class DefaultActionsUIBridge;

    // ----------------------------------
    // Static accessor for current bridge
    // ----------------------------------
    class ActionsUIBridge
    {
    public:
        // Returns currently installed bridge
        static IActionsUIBridge& Current();

        // Installs custom bridge; null restores default
        static void SetCurrent(IActionsUIBridge* host);

        // Restores default forwarder
        static void ResetToDefault();
    };
}
