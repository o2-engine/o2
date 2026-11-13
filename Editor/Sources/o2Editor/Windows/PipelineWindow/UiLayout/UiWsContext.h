#pragma once

#include <optional>

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Types/Ref.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsScreen.h"

using namespace o2;

namespace Editor
{
    class PipelineEditor;
    class PipelineNode;

    // -----------------------------------------------------------------------------------------------
    // What the workspace shows of one AI UI layout node, read from its config and its inputs, and the
    // edits it writes back. Every panel reads it; an edit goes through the editor (one undo step, the
    // asset saved, the node auto-applied) and the context reads the node again. UI-only keys (the tab, the
    // selection, the view...) are written without an undo step and never re-run the node
    // -----------------------------------------------------------------------------------------------
    class UiWsContext : public RefCounterable
    {
    public:
        WeakRef<PipelineEditor> editor;
        String                  nodeId;

        UiDoc::Json            config;    // The node's config as read last, without steps and uiRedo (see timeline, redo)
        UiDoc::Ws::UiTimeline  timeline;  // The screen after each step
        Vector<UiDoc::UiStep>  redo;      // config.uiRedo
        UiDoc::UiDoc           current;   // The layout now (after every step)
        int                    peek = -1; // The step shown from the history (-1: now)

        UiDoc::UiKit                   kit;       // The kit wired in (empty without one)
        bool                           hasKit = false;
        String                         kitText;   // The kit value as read last
        int                            kitVersion = 0;
        std::optional<UiDoc::UiScreen> reference; // The first reference picture as the screen the layout rebuilds (ReferenceScreenOf)
        UiDoc::UiSize                  referencePicture; // Its picture's own size, pixels
        String                         referencePng;     // Its picture's bytes, PNG or JPEG (the compare views draw it)
        UiDoc::UiTarget                target;           // The device the layout is laid out for: the kit's
        UiDoc::Ws::UiRefSetting        ref;              // Comparing with the reference (on only with one)
        bool                           compareNeedsBlocks = false; // The card: an empty layout is not compared

        UiDoc::Ws::UiPreviewSetting setting;
        UiDoc::Ws::UiPreview        preview;
        UiDoc::UiRects              rects;  // Every block's box at the preview frame
        UiDoc::UiKitReport          report; // How the layout uses its kit (with a kit only)
        int                         undescribed = 0;

        String selected;  // The selected block, "" for the screen
        String tab;       // agent | inspect | text | history
        String view;      // lines | kit
        bool   showIds = false;
        bool   review = true;
        String draft;     // The agent request being typed (uiDraft)
        String exportFor; // unity | o2 | json

        Vector<UiDoc::UiStepPoint> points;  // The points the request names as [1], [2]... (uiPoints, design pixels)
        bool                       pinning = false; // The point tool is on: a click on the stage places a point

        // A point placed on the stage (index -1) or moved (its index): the request field puts its chip in at the caret
        Function<void(double x, double y, int index)> onPin;

        int version = 0; // Changes whenever anything above changes

        Function<void()> onRefreshed; // Called after the context read the node again

    public:
        // The screen shown: the one peeked at in the history, else the current one
        const UiDoc::UiDoc& Doc() const;

        // Hand edits are allowed: nothing peeked at and the agent not busy
        bool Editable() const;

        // The agent is working on the node
        bool Busy() const;

        // What step i (0-based) changed, cached until the next read
        const UiDoc::DocDiff& DiffOf(int step);

        // The edits of step i as text, cached until the next read
        const String& OpsTextOf(int step);

        // The node, null when it is gone
        Ref<PipelineNode> Node() const;

        // Reads the node again
        void Refresh();

        // Reads the node again when its config or its kit changed since the last read (polled by the window)
        bool RefreshIfChanged();

        // --- edits ---

        // A hand action (typed fields fold into the one before within 4 s)
        void Op(const Vector<UiDoc::UiOp>& ops, const String& label);

        // A gesture on the stage or in the hierarchy: always its own action
        void Gesture(const Vector<UiDoc::UiOp>& ops, const String& label);

        // The ops of a hand edit: the first edit of an empty layout still at another density is preceded by the screen op
        // that puts it into its device's pixels (FirstScreenOp)
        Vector<UiDoc::UiOp> WithFirstScreen(const Vector<UiDoc::UiOp>& ops) const;

        // Brings a layout in older units to its device's pixels: one hand action with one screen op (undo takes it back); the
        // points marked for the next request follow. False when there is nothing to bring
        bool Bring();

        // Undo / redo by action, revert to the screen after k steps
        void Undo();
        void Redo();
        void Revert(int count);

        // Selects a block ("" = the screen)
        void Select(const String& id);

        // Shows the screen after k steps (-1: now)
        void SetPeek(int k);

        // Writes a UI-only key (no undo step, no run): uiTab, uiView, uiShowIds, uiDraft, agentReview, agentModel, exportFor,
        // uiDevice... A null value writes null (as the web's update({key: null}))
        void SetUi(const String& key, const UiDoc::Json& value);

        // Writes several UI-only keys at once
        void SetUi(const Vector<Pair<String, UiDoc::Json>>& values);

        // Writes the config through the editor: undoable records one undo step and auto-applies the node; the card follows.
        // change returns false when it changed nothing
        bool Write(const String& key, bool undoable, const Function<bool(DataValue&)>& change);

    private:
        Vector<UiDoc::DocDiff> mDiffs;      // Per step, filled lazily
        Vector<bool>           mDiffReady;
        Map<int, String>       mOpsTexts;
        DataDocument mSeen;        // The config as read last, to see a change made elsewhere
        DataDocument mStepsSeen;   // config.steps as the timeline read it
        bool         mStepsSeenValid = false;
        bool         mStepsPresent = false;
        String       mScreenNameSeen;
        bool         mSeenValid = false;
    };

    // --- the agent's state per node, shared by the card and the workspace ----------------------------

    namespace UiAgentState
    {
        // Marks the node's agent busy or idle
        void SetBusy(const String& nodeId, bool busy);

        // Returns true while the node's agent works
        bool IsBusy(const String& nodeId);

        // Sets the error the agent's last request ended with ("" clears it)
        void SetError(const String& nodeId, const String& error);

        // Returns the error of the node's agent's last request
        String GetError(const String& nodeId);

        // ---------------------------------------------------------------------------------------
        // A request to the node's agent going on now: its id, what it was asked, its acts so far
        // ({id, kind, title, status, ms?, note?, data?, images?}), the folder of their pictures and when it
        // started (UiDoc::NowMs)
        // ---------------------------------------------------------------------------------------
        struct LiveRun
        {
            String      runId;
            String      prompt;
            UiDoc::Json acts = UiDoc::Json::Array();
            String      dir;
            double      startedMs = 0;
            int         version = 0; // Grows with every change of the acts
        };

        // Sets the run going on for the node (its acts change as it goes)
        void SetRun(const String& nodeId, const LiveRun& run);

        // The run going on for the node; false when none
        bool GetRun(const String& nodeId, LiveRun& run);

        // Forgets the node's run (it ended)
        void ClearRun(const String& nodeId);

        // Asks the node's running request to stop (what it made so far still becomes a step)
        void RequestStop(const String& nodeId);

        // Returns true once a stop of the node's request was asked for; cleared by a new run
        bool IsStopRequested(const String& nodeId);
    }
}
