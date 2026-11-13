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
    class PipelineNodeBody;

    // -----------------------------------------------------------------------------------------------------------------
    // The rows of a check against the reference in a card's parameter list (AssetsLine nodeRenderers.tsx / TextStyleExtractBody
    // .tsx): a divider, the switch (refCheck), and while it is on the limit (refCheckMax, at most 1 / 2 / 3) and the judge model
    // (refCheckModel). refresh is called after the switch or the limit changed: the rows below it follow
    // -----------------------------------------------------------------------------------------------------------------
    namespace PipelineCheckRows
    {
        void Add(PipelineNodeBody& body, const String& switchCaption, const String& limitCaption, const Function<void()>& refresh);
    }

    // -----------------------------------------------------------------------------------------------------------------
    // The debug popup of a check (AssetsLine ExtractCheckDialog.tsx, StyleCheckDialog.tsx): every attempt as a row - for an
    // AI extract part the reference with the part's region, the sprite laid over it and the sprite alone; for an AI text style
    // the picture and the sample drawn in the attempt's style - with the judge's verdict, likeness and differences, what it
    // asked of (or changed for) the next attempt, which attempt was kept and why the check stopped. Read from the node's work
    // folder (check/<part>.json, check/style.json); followed while the part or the node runs. A click on a picture shows it
    // 1:1, Escape closes that and then the window
    // -----------------------------------------------------------------------------------------------------------------
    namespace PipelineCheckDlg
    {
        // Shows the check of an AI extract part (partId) or of an AI text style (partId empty)
        void Show(const Ref<PipelineEditor>& editor, const Ref<PipelineNode>& node, const String& partId, const String& partName);

        // Hides the window
        void Hide();

        // Returns true while the window shows that check
        bool IsShownFor(const String& nodeId, const String& partId);

        // Reads the debug file again (tests)
        void Refresh();

        // Returns the stop line the window shows, empty when it shows none (tests)
        String GetStopLine();

        // Returns the number of attempt rows the window shows (tests)
        int GetAttemptCount();

        // Returns the window, null before the first Show
        Ref<Window> GetWindow();

        // Destroys the window; the next Show builds it in the theme of the moment (tests)
        void Reset();
    }
}
