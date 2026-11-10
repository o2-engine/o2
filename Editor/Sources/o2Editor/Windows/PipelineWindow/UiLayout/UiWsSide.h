#pragma once

#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsInspector.h"

namespace o2
{
    class Button;
}

namespace Editor
{
    class UiWsAgentTab;
    class UiWsTextTab;
    class UiWsHistoryTab;

    // Diff chips of a step: +added ~changed ↕moved −removed and the screen mark; returns their width (draw = false measures)
    float DrawDiffChips(const UiDoc::DocDiff& diff, const Vec2F& leftCenter, bool draw = true);

    // -----------------------------------------------------------------------------------------------
    // The workspace's right panel: the tabs Agent / Inspector / Text / History (the History tab counts the
    // steps; a narrow panel names only the open tab) and the open tab under them (config.uiTab)
    // -----------------------------------------------------------------------------------------------
    class UiWsSidePanel : public UiWsPanel
    {
    public:
        // Default constructor
        explicit UiWsSidePanel(RefCounter* refCounter);

        // Binds the tabs to the context
        void Bind(const Ref<UiWsContext>& context);

        // Opens a tab: agent, inspect, text or history
        void OpenTab(const String& tab);

        const Ref<UiWsAgentTab>& GetAgent() const { return mAgent; }
        const Ref<UiWsInspector>& GetInspector() const { return mInspector; }
        const Ref<UiWsTextTab>& GetText() const { return mText; }
        const Ref<UiWsHistoryTab>& GetHistory() const { return mHistory; }

        void Update(float dt) override;

    protected:
        Ref<UiWsAgentTab>   mAgent;
        Ref<UiWsInspector>  mInspector;
        Ref<UiWsTextTab>    mText;
        Ref<UiWsHistoryTab> mHistory;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
    };

    // -----------------------------------------------------------------------------------------------
    // The Agent tab (port of AgentMessages and the compose part of UiWorkspace.tsx): the steps as a chat
    // - the request, the answer with its diff chips and edits (folded), the check pass's result; hand
    // edits as a quiet line - the agent working, its error; then the selected block as context, the
    // request field (config.uiDraft), the model, the check-pass switch (agentReview) and Send. The agent
    // itself is not called here: Send hands the request to UiWorkspace's onAgentSend
    // -----------------------------------------------------------------------------------------------
    class UiWsAgentTab : public UiWsPanel
    {
    public:
        Function<void(const String& prompt, const String& selected)> onSend; // The request to send (the workspace wires it)

    public:
        // Default constructor
        explicit UiWsAgentTab(RefCounter* refCounter);

        // Sends the request typed (Send, Ctrl+Enter); false when it is empty, the agent is busy or nothing takes it
        bool Send();

        // Returns the request field
        const Ref<EditBox>& GetPromptField() const { return mPrompt; }

        // Writes the request typed into the node (uiDraft) when it changed
        void SaveDraft();

        // Unfolds or folds the edits of an agent step
        void ToggleEdits(int step);

        void OnScrolled(float scroll) override;
        void Update(float dt) override;

    protected:
        Ref<EditBox>      mPrompt;
        String            mLastPrompt; // The request as it was before this frame's keys
        Ref<o2::Button>   mModel;
        Vector<int>       mOpen;        // Agent steps whose edits are unfolded
        float             mScroll = 0.0f;
        float             mContentHeight = 0.0f;
        int               mSeenSteps = -1; // Steps when the list was last scrolled to its end
        bool              mSeenBusy = false;
        bool              mStickToEnd = true;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;

        // Lays the messages out (draw = false only measures); returns the content height
        float LayMessages(const UiWsDraw::Box& box, float top, float bottom, bool draw);

        // Opens the model menu at the field
        void OpenModelPicker();
    };

    // -----------------------------------------------------------------------------------------------
    // The Text tab (port of TextTab): the layout as UIML with line numbers; an edit is kept as a draft until
    // Apply (or Ctrl+Enter) parses it - errors are listed, a clean text becomes one hand action {op: doc};
    // Reset drops the draft
    // -----------------------------------------------------------------------------------------------
    class UiWsTextTab : public UiWsPanel
    {
    public:
        // Default constructor
        explicit UiWsTextTab(RefCounter* refCounter);

        // Applies the text typed; false when it has errors (they are shown) or there is no draft
        bool Apply();

        // Drops the draft
        void Reset();

        // Returns true while the text differs from the layout
        bool HasDraft() const { return mDraft; }

        // Returns the errors of the last apply
        const Vector<String>& GetErrors() const { return mErrors; }

        // Returns the text field
        const Ref<EditBox>& GetField() const { return mField; }

        void Update(float dt) override;

    protected:
        Ref<EditBox>   mField;
        bool           mDraft = false;
        String         mShownFor;  // The UIML the field shows when there is no draft
        Vector<String> mErrors;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
    };

    // -----------------------------------------------------------------------------------------------
    // The History tab: Undo / Redo (by action, the redo count), the timeline from the empty screen - each
    // step with its title, reply, the actions of a run of hand edits (folded), the diff chips, the time and
    // the model; a click peeks at a step (the stage shows the screen after it), Peek / Revert on hover
    // -----------------------------------------------------------------------------------------------
    class UiWsHistoryTab : public UiWsPanel
    {
    public:
        // Default constructor
        explicit UiWsHistoryTab(RefCounter* refCounter);

        // Unfolds or folds the actions of a step
        void ToggleActions(int step);

        void OnScrolled(float scroll) override;

    protected:
        Vector<int> mOpen;
        float       mScroll = 0.0f;
        float       mContentHeight = 0.0f;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
    };
}
