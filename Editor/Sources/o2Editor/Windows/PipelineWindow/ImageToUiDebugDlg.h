#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

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
    // Debug view of AI image to UI: the steps of the node's last run as its trace recorded them (trace.json and the
    // pictures of every step in the node's work folder) - the analysis with the model's raw answer, every sprite cut, the
    // backdrop, the asset list, then every turn of the Image-To-UI skill run by the model: its note, its action, what the
    // skill's scripts answered and their evidence at full width, design | reconstruction under one divider after each check
    // and target. The window follows the trace while the node runs; the selected step is kept in the node's debugStep
    // -----------------------------------------------------------------------------------------------------------------
    namespace ImageToUiDebug
    {
        // Shows the window for the node; onClose is called when the user closes it
        void Show(const Ref<PipelineEditor>& editor, const Ref<PipelineNode>& node, const Function<void()>& onClose);

        // Hides the window when it shows the node
        void Hide(const String& nodeId);

        // Returns true while the window shows the node
        bool IsShownFor(const String& nodeId);

        // Reads the trace again when the window shows the node
        void Refresh(const String& nodeId);

        // Marks the node running or not: a running node's trace is followed
        void SetRunning(const String& nodeId, bool running);

        // Selects a step by its id, as a click on it does
        void SelectStep(const String& stepId);

        // Opens or closes the 1:1 view of an image of the selected step
        void Zoom(int imageIndex);

        // Returns the ids of the steps the window lists
        Vector<String> GetStepIds();

        // Returns the window, null before the first Show
        Ref<Window> GetWindow();
    }
}
