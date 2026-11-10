#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"

using namespace o2;

namespace Editor
{
    class PipelineEditor;
    class UiWsWindow;

    // -----------------------------------------------------------------------------------------------------------------
    // The AI UI layout node's workspace (port of AssetsLine frontend/src/nodeEditor/uiLayout/UiWorkspace.tsx and Stage.tsx):
    // a window over the editor with the hierarchy on the left, the screen on a zoomable board with Unity-style handles in
    // the middle, and the Agent / Inspector / Text / History tabs on the right. Every edit is a UiDoc op written into the
    // node's config as the web writes it (steps + uiRedo; hand edits folded into one step of actions), one undo step of the
    // editor each, and the node auto-applies; the view keys (uiTab, uiSelected, uiView, uiShowIds, uiDraft, uiDevice...)
    // are written without an undo step and never re-run the node. The agent is not called here: Send hands the request to
    // the handler set with SetAgentSendHandler
    // -----------------------------------------------------------------------------------------------------------------
    namespace UiWorkspace
    {
        // Opens the window for the node (made on first use; its size, state and panel widths are remembered)
        void Open(const Ref<PipelineEditor>& editor, const String& nodeId);

        // Returns true while the window is open
        bool IsOpen();

        // Returns true while the window shows the node
        bool IsOpenFor(const String& nodeId);

        // Closes the window (the request being typed is kept in the node's uiDraft)
        void Close();

        // Reads the node again when the window shows it ("" = whatever it shows): call it after the node's config changed
        // elsewhere (the editor's undo, a sync, the agent's answer). The window also notices changes by itself, a few
        // times a second
        void Refresh(const String& nodeId = "");

        // Sets what is called when the window closes
        void SetOnClosed(const Function<void()>& onClosed);

        // --- the agent ---------------------------------------------------------------------------------------------------

        // The handler of Send (Ctrl+Enter): the node, the request and the selected block ("" for the screen). It is expected
        // to mark the agent busy, call the model, then append the step (AppendAgentStep) or set an error, and mark it idle
        using AgentSendHandler = Function<void(const Ref<PipelineEditor>& editor, const String& nodeId, const String& prompt,
                                               const String& selected)>;

        // Sets the handler of Send; none disables Send
        void SetAgentSendHandler(const AgentSendHandler& handler);

        // Marks the node's agent busy (the chat shows it working, Send and the hand edits on the stage wait) or idle
        void SetAgentBusy(const String& nodeId, bool busy);

        // Returns true while the node's agent works
        bool IsAgentBusy(const String& nodeId);

        // Shows the error the agent's request ended with in the chat ("" clears it)
        void SetAgentError(const String& nodeId, const String& error);

        // Appends the agent's answer to the node the way the web does - {steps: [...steps, step], uiRedo: [], uiDraft: ""}
        // - as one undo step, auto-applies the node and refreshes the window; false when the node is gone
        bool AppendAgentStep(const Ref<PipelineEditor>& editor, const String& nodeId, const UiDoc::Json& step);

        // --- export ------------------------------------------------------------------------------------------------------

        // The handler of the header's Download (the export target is the node's exportFor); none disables the button
        void SetDownloadHandler(const Function<void(const Ref<PipelineEditor>& editor, const String& nodeId)>& handler);

        // --- tests -------------------------------------------------------------------------------------------------------

        // Returns the window, null before the first Open
        Ref<UiWsWindow> GetWindow();

        // Destroys the window; the next Open builds it again in the theme of the moment
        void Reset();
    }
}
