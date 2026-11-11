#pragma once

#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"
#include "o2Editor/Pipeline/UiDoc/UiKitModel.h"

using namespace o2;

namespace Editor
{
    class PipelineEditor;
    class PipelineNode;

    // ---------------------------------------------------------------------------------------------------------------
    // The model calls of the Text style, UI kit and AI UI layout cards and of the layout's workspace (the cards' side of
    // AssetsLine's routes/uiAi.ts): what the call needs is gathered from the editor (the node's config, its inputs as the
    // canvas shows them, the kit made again from its own inputs), the call runs as a coroutine on the main thread, and
    // its answer is written into the node's config as one undo step. One call per node and kind at a time
    // ---------------------------------------------------------------------------------------------------------------
    namespace PipelineUiAi
    {
        // Connects the workspace's Send and Download to these calls (done once, by the first layout card)
        void InstallWorkspaceHandlers();

        // Asks the layout agent: the request, the selected block ("" = the screen); the step is appended to the node
        void SendAgent(const Ref<PipelineEditor>& editor, const String& nodeId, const String& prompt, const String& selected);

        // Generates the Text style node's style from its stylePrompt (pictures wired in are references)
        void GenerateStyle(const Ref<PipelineEditor>& editor, const String& nodeId);

        // Analyses the UI kit node's pictures (portId "" = all of them) and merges the answer into its items
        void AnalyzeKit(const Ref<PipelineEditor>& editor, const String& nodeId, const String& portId);

        // Exports the layout node for its exportFor target (the card's and the workspace's Download): unity and json ask
        // where to save the file, o2 writes the prototype into the project's Assets
        void Download(const Ref<PipelineEditor>& editor, const String& nodeId);

        // Exports the layout node for its exportFor target without asking: unity / json into the file (the o2 target
        // ignores it); the result and what the target does not reproduce go to the editor's log
        void DownloadTo(const Ref<PipelineEditor>& editor, const String& nodeId, const String& outputPath);

        // Returns true while a call of the kind ("agent", "style", "kit", "export") runs for the node
        bool IsBusy(const String& kind, const String& nodeId);

        // Returns the port the kit analysis runs for: "*" for all, "" when none runs
        String KitBusyPort(const String& nodeId);

        // Returns the error the last call of the kind ended with for the node, "" when it went well
        String LastError(const String& kind, const String& nodeId);

        // Returns the kit the layout node is wired to, made again from the kit node's own inputs when it is one (so it is
        // never stale), else its value as the canvas shows it; false when no kit is connected or produced
        bool CurrentKit(const Ref<PipelineEditor>& editor, const PipelineNode& layoutNode, UiDoc::UiKit& kit);
    }
}
