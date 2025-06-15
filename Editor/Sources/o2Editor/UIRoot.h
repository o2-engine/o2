#pragma once

#include "o2/Events/ApplicationEventsListener.h"
#include "o2/Events/CursorAreaEventsListenersLayer.h"
#include "o2/Utils/Singleton.h"

using namespace o2;

namespace o2
{
    class Widget;
}

// Editor UI root accessor macros
#define EditorUIRoot UIRoot::Instance()

namespace Editor
{
    // --------------
    // Editor UI root
    // --------------
    class UIRoot: public Singleton<UIRoot>, public ApplicationEventsListener
    {
    public:
        // Default constructor, creates root widget
        UIRoot(RefCounter* refCounter);

        // Destructor
        ~UIRoot();

        // Adds widget to root
        Ref<Widget> AddWidget(const Ref<Widget>& widget);

        // Removes widget from root
        void RemoveWidget(const Ref<Widget>& widget);

        // Removes all widgets from root
        void RemoveAllWidgets();

        // Returns root widget
        const Ref<Widget>& GetRootWidget();

    private:
        Ref<Widget> mRootWidget; // Root widget for editor UI

    private:
        // Draws root widget
        void Draw();

        // Updates root widget
        void Update(float dt);

        // Called when application frame was resized, updates root size
        void OnApplicationSized() override;

        REF_COUNTERABLE_IMPL(RefCounterable);

        friend class EditorApplication;
    };
}
