#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // -----------------------------------------------------------------------------------------------------------------
    // The question of the AssetsLine sync about synced pipelines missing here but still stored in the linked project:
    // delete them in AssetsLine too, or download them again. Closing the window keeps them. A question asked while
    // another is open is shown after it
    // -----------------------------------------------------------------------------------------------------------------
    namespace AssetsLineDeleteDlg
    {
        // Asks about these pipelines of the synced folder (relative to Assets); answer gets true to delete them there
        void Show(const Vector<String>& names, const String& folder, const String& projectName,
                  const Function<void(bool)>& answer);

        // Returns the text of the question
        String Message(const Vector<String>& names, const String& folder, const String& projectName);

        // Returns true while a question is shown
        bool IsShown();

        // Returns the number of questions waiting behind the shown one
        int GetQueuedCount();

        // Answers the shown question as its buttons do
        void Answer(bool deleteThere);
    }
}
