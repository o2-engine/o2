#include "o2Editor/stdafx.h"
#include "IActionsUIBridge.h"

#include "o2/Utils/Editor/SceneEditableObject.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/TreeWindow/TreeWindow.h"

using namespace o2;

namespace Editor
{
    // ----------------------------------------------------
    // Default bridge: forwards to real editor singletons,
    // no-op when they aren't initialized (headless safe)
    // ----------------------------------------------------
    class DefaultActionsUIBridge : public IActionsUIBridge
    {
    public:
        void HighlightObjectTreeNode(const Ref<SceneEditableObject>& object) override
        {
            if (Singleton<TreeWindow>::IsSingletonInitialzed())
                TreeWindow::Instance().HighlightObjectTreeNode(object);
        }

        void UpdateTreeView() override
        {
            if (Singleton<TreeWindow>::IsSingletonInitialzed())
                TreeWindow::Instance().UpdateTreeView();
        }

        void SelectObjectsWithoutAction(Vector<Ref<SceneEditableObject>> objects, bool additive) override
        {
            if (Singleton<SceneEditScreen>::IsSingletonInitialzed())
                SceneEditScreen::Instance().SelectObjectsWithoutAction(objects, additive);
        }

        void SelectObjectWithoutAction(const Ref<SceneEditableObject>& object, bool additive) override
        {
            if (Singleton<SceneEditScreen>::IsSingletonInitialzed())
                SceneEditScreen::Instance().SelectObjectWithoutAction(object, additive);
        }

        void ClearSelectionWithoutAction(bool sendSelectedMessage) override
        {
            if (Singleton<SceneEditScreen>::IsSingletonInitialzed())
                SceneEditScreen::Instance().ClearSelectionWithoutAction(sendSelectedMessage);
        }
    };

    namespace
    {
        DefaultActionsUIBridge gDefaultHost;
        IActionsUIBridge*      sCurrent = &gDefaultHost;
    }

    IActionsUIBridge& ActionsUIBridge::Current()
    {
        return *sCurrent;
    }

    void ActionsUIBridge::SetCurrent(IActionsUIBridge* host)
    {
        sCurrent = host ? host : &gDefaultHost;
    }

    void ActionsUIBridge::ResetToDefault()
    {
        sCurrent = &gDefaultHost;
    }
}
