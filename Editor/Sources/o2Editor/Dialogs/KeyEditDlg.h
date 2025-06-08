#pragma once

#include "o2/Utils/Singleton.h"

using namespace o2;

namespace o2
{
    class EditBox;
    class Window;
}

namespace Editor
{
    // ---------------------
    // Curve key edit dialog
    // ---------------------
    class KeyEditDlg : public Singleton<KeyEditDlg>
    {
    public:
        // Constructor
        KeyEditDlg(RefCounter* refCounter);

        // Destructor
        ~KeyEditDlg();

        // Show the dialog
        static void Show(const Curve::Key& key, const Function<void(const Curve::Key& key)>& onClosed);

    private:
        Ref<o2::Window> mWindow; // Dialog window

        Ref<EditBox> mPosition;             // Key position
        Ref<EditBox> mValue;                // Key value
        Ref<EditBox> mLeftSupportPosition;  // Left support position
        Ref<EditBox> mLeftSupportValue;     // Left support value
        Ref<EditBox> mRightSupportPosition; // Right support position
        Ref<EditBox> mRightSupportValue;    // Right support value

        Function<void(const Curve::Key& key)> mOnClosed; // On closed callback

    private:
        // Called when Ok button pressed, calls onClosed function with new key parameters
        void OnOkPressed();
    };
}
