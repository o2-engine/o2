#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"
#include "o2Editor/Pipeline/PipelineImageToUi.h"

using namespace o2;

namespace o2
{
    class Window;
}

namespace Editor
{
    class PipelineEditor;
    class PipelineNode;

    // -----------------------------------------------------------------------------------------------------------------
    // Edits of AI image to UI after a run (AssetsLine's UiEditsDialog): the source and the result side by side - a click
    // puts a numbered point, each point gets a note, "+ Without a point" adds a note about the whole screen; the rounds
    // applied, each showing its notes and its points grey; "Undo the last round"; "Apply (n)" sends the notes as one round
    // (config edits) and runs the node, which continues the skill from the current layout. The notes not sent yet are the
    // node's editsDraft, so closing the window keeps them
    // -----------------------------------------------------------------------------------------------------------------
    namespace ImageToUiEdits
    {
        // Shows the window for the node; onClose is called when the user closes it
        void Show(const Ref<PipelineEditor>& editor, const Ref<PipelineNode>& node, const Function<void()>& onClose);

        // Hides the window when it shows the node
        void Hide(const String& nodeId);

        // Returns true while the window shows the node
        bool IsShownFor(const String& nodeId);

        // Reads the node's rounds, source and result again when the window shows the node; the draft stays as typed
        void Refresh(const String& nodeId);

        // Marks the node running or not: a running node takes no notes and shows no result
        void SetRunning(const String& nodeId, bool running);

        // Puts a point on the source (design) or the result at a pixel and adds its note, as a click does
        void Point(const String& on, int x, int y);

        // Adds a note without a point
        void AddGeneralNote();

        // Types the text of a note
        void SetNoteText(int index, const String& text);

        // Removes a note
        void RemoveNote(int index);

        // Opens an applied round (its notes and grey points), or closes it when it is open; -1 closes any
        void ViewRound(int index);

        // Presses Apply; false when it is disabled
        bool Apply();

        // Presses "Undo the last round"; false when there is no round or the node runs
        bool Undo();

        // Returns true when Apply is enabled
        bool CanApply();

        // Returns the caption of the Apply button
        String GetApplyCaption();

        // Returns the notes not sent yet
        Vector<PipelineImageToUiEditItem> GetDraft();

        // Returns the points drawn over the pictures
        Vector<PipelineImageToUiPin> GetPins();

        // Returns the window, null before the first Show
        Ref<Window> GetWindow();
    }
}
