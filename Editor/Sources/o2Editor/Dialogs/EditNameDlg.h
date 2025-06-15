#pragma once

#include "o2/Events/CursorEventsArea.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Singleton.h"

using namespace o2;

namespace o2
{
    class EditBox;
    class Label;
    class VerticalProgress;
    class Widget;
    class WidgetLayer;
    class Window;
}

namespace Editor
{
    // -------------------
    // Name editing dialog
    // -------------------
    class NameEditDlg: public Singleton<NameEditDlg>, public CursorEventsListener
    {
    public:
        // Default constructor
        NameEditDlg(RefCounter* refCounter);

        // Destructor
        ~NameEditDlg();

        // Shows name editing window. Delegate onCompleted is called when ok pressed and onCancelled when cancel
        static void Show(const String& name, 
                         Function<void(const String&)> onCompleted,
                         Function<void()> onCancelled = Function<void()>());

        REF_COUNTERABLE_IMPL(Singleton<NameEditDlg>, CursorEventsListener);

    protected:
        Function<void(const String&)> mOnCompletedCallback; // On completed callback
        Function<void()>              mOnCancelledCallback; // On cancelled callback

        Ref<o2::Window> mWindow;      // Dialog window
        Ref<EditBox>    mNameEditBox; // Name edit box

    protected:
        // Calls when hiding dialog
        void OnHide();

        // Initializes edit box and buttons
        void InitializeControls();

        // Called when Ok button pressed, calls mOnCompletedCallback and closes window
        void OnOkPressed();

        // Called when Cancel button pressed, calls mOnCancelledCallback and closes window
        void OnCancelPressed();

        // Called when cursor pressed outside from window
        void OnCursorPressedOutside();
    };
}
