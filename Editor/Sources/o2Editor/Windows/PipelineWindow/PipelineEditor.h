#pragma once

#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2Editor/Actions/ActionsList.h"
#include "o2Editor/Actions/IAction.h"
#include "o2/Assets/Types/PipelineAsset.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/UI/FrameScrollView.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"

using namespace o2;

namespace o2
{
    class ContextMenu;
    class Sprite;
}

namespace Editor
{
    // ----------------------------------------------------
    // Undo step: the whole graph before and after a change
    // ----------------------------------------------------
    class PipelineGraphAction : public IAction
    {
    public:
        WeakRef<class PipelineEditor> editor; // Editor whose graph is restored on undo and redo
        String                        name;   // Action name shown in the undo history
        String                        before; // Serialized graph before the change
        String                        after;  // Serialized graph after the change

    public:
        // Returns action name for the undo history
        String GetName() const override { return name; }

        // Restores the graph after the change
        void Redo() override;

        // Restores the graph before the change
        void Undo() override;

        SERIALIZABLE(PipelineGraphAction);
    };

    // ----------------------------------------------------------------------
    // Node graph canvas of a pipeline asset: cards, links, selection, menus,
    // clipboard, undo and the run controls wired to the pipeline executor
    // ----------------------------------------------------------------------
    class PipelineEditor : public FrameScrollView, public SelectableDragHandlesGroup, public KeyboardEventsListener
    {
    public:
        Ref<ActionsList>              actionsListDelegate; // Undo list of the owner window; local when null
        Function<void()>              onChanged;           // Graph changed (asset dirty)
        Function<void(const String&)> onLog;               // Execution log line

    public:
        // Default constructor
        explicit PipelineEditor(RefCounter* refCounter);

        // Destructor, cancels the running executor
        ~PipelineEditor() override;

        // Sets edited asset, rebuilds cards and restores the camera saved in the graph
        void SetAsset(const Ref<PipelineAsset>& asset);

        // Returns edited asset
        Ref<PipelineAsset> GetAsset() const;

        // Returns graph of the edited asset, null without an asset
        PipelineGraph* GetGraph() const;

        // Returns the results cache key: the id stored in the graph, else the asset UID
        String GetPipelineId() const;

        // Returns the canvas rectangle the view shows
        RectF GetVisibleCanvasRect() const;

        // Draws widget and the rubber-band selection
        void Draw() override;

        // Updates cards, pending link, debounced auto-apply and camera saving
        void Update(float dt) override;

        // Updates layout and camera transforms
        void UpdateSelfTransform() override;

        // Converts node position (y down) to canvas point (y up)
        static Vec2F NodeToCanvas(const Vec2F& p) { return Vec2F(p.x, -p.y); }

        // Converts canvas point (y up) to node position (y down)
        static Vec2F CanvasToNode(const Vec2F& p) { return Vec2F(p.x, -p.y); }

        // Returns port and link color for port type
        static Color4 GetPortColor(PipelinePortType type);

        // Called when node card is pressed, brings it to front and drops link selection
        void OnNodePressed(const Ref<PipelineNodeWidget>& node);

        // Called when node card is dragged, moves all selected nodes by the same delta
        void OnNodeDragged(const Ref<PipelineNodeWidget>& node, const Vec2F& position);

        // Called when node drag completed, records the move as one undo step
        void OnNodeDragCompleted(const Ref<PipelineNodeWidget>& node);

        // Called when node card is resized, records an undo step when completed
        void OnNodeResized(const Ref<PipelineNodeWidget>& node, bool completed);

        // Adds custom input to node, asks for the type when the schema allows several
        void AddCustomInput(const Ref<PipelineNodeWidget>& node);

        // Renames custom input port and schedules auto-apply
        void RenameCustomInput(const Ref<PipelineNodeWidget>& node, const String& portId, const String& name);

        // Removes custom input port with its links
        void RemoveCustomInput(const Ref<PipelineNodeWidget>& node, const String& portId);

        // Opens context menu of node: duplicate, copy, clear result, delete
        void OpenNodeContextMenu(const Ref<PipelineNodeWidget>& node);

        // Shows last execution error of node in a dialog and the log
        void ShowNodeError(const Ref<PipelineNodeWidget>& node);

        // Called when node body edits config key. Continuous edits (sliders, typing) pass completed=false and once completed=true to record a single undo step
        void OnNodeConfigChanged(const Ref<PipelineNodeWidget>& node, const String& key, bool completed = true);

        // Takes graph snapshot at the start of a continuous edit, EndContinuousEdit records it
        void BeginContinuousEdit(const String& name);

        // Records the continuous edit as one undo step when the graph changed
        void EndContinuousEdit();

        // Runs node with its upstream branch, queued when a run is in progress
        void RunNode(const String& nodeId, bool bypassSelf);

        // Runs every finish node
        void RunAll();

        // Cancels the executor and clears run and auto-apply queues
        void StopRun();

        // Returns true while the executor is running
        bool IsRunning() const;

        // Drops cached result and previews of node
        void ClearNodeResult(const String& nodeId);

        // Returns card widget by node id
        Ref<PipelineNodeWidget> GetNodeWidget(const String& nodeId) const;

        // Returns execution runtime of node, null when there is no card
        const PipelineNodeRuntime* GetRuntime(const String& nodeId) const;

        // Returns value connected to input port by name
        PipelineValue GetInputValue(const Ref<PipelineNode>& node, const String& portName) const;

        // Returns value connected to input port by id
        PipelineValue GetInputValueById(const Ref<PipelineNode>& node, const String& portId) const;

        // Rebuilds all cards from the graph, keeping their runtimes
        void RebuildAll();

        // Animates the camera to show all cards
        void FitView();

        // Moves the camera to a canvas point at a scale without animation
        void SetView(const Vec2F& center, float scale);

        // Replaces the graph with serialized one, used by undo and redo
        void RestoreGraph(const String& serialized);

        // Returns graph serialized to string
        String SerializeGraph() const;

        // Returns canvas context menu
        const Ref<ContextMenu>& GetContextMenu() const;

        // Shows a flat menu of labelled actions at the cursor; card controls use it, as their own popups would live in canvas space
        void ShowPopupMenu(const Vector<Pair<String, Function<void()>>>& items);

        // Returns the menu shown by ShowPopupMenu
        const Ref<ContextMenu>& GetPopupMenu() const { return mPopupMenu; }

        // Returns selected node cards
        Vector<Ref<PipelineNodeWidget>> GetSelectedNodes() const;

        // Selects nodes by ids, deselecting the others
        void SelectNodes(const Vector<String>& ids);

        // Deletes selected nodes, or the selected link or its bend point
        void DeleteSelection();

        // Copies selected nodes with their inner links to clipboard
        void CopySelection();

        // Pastes nodes from clipboard at canvas point, or shifted from the originals when useCursor
        void Paste(const Vec2F& canvasPos, bool useCursor);

        // Copies and pastes selected nodes
        void DuplicateSelection();

        // Selects all nodes
        void SelectAllNodes();

        // Dynamic cast to RefCounterable via FrameScrollView
        static Ref<RefCounterable> CastToRefCounterable(const Ref<PipelineEditor>& ref);

        SERIALIZABLE(PipelineEditor);

    protected:
        // Link being dragged from a port
        struct PendingEdge
        {
            bool             active = false;                // True while dragging
            String           nodeId;                        // Source node id
            String           portId;                        // Source port id
            bool             fromInput = false;             // True when dragging from an input port
            PipelinePortType type = PipelinePortType::Text; // Port type used to filter targets
            Vec2F            cursor;                        // Free end position in canvas space
        };

    protected:
        WeakRef<PipelineAsset> mAsset; // Edited asset
        PipelineGraph          mGraph; // Edited graph, read from the asset document and written back on every change

        Ref<Widget>                                       mNodesContainer;  // Parent widget of node cards
        Vector<Ref<PipelineNodeWidget>>                   mNodeWidgets;     // Node cards in drawing order
        Map<String, Ref<PipelineNodeWidget>>              mNodeWidgetsById; // Node cards by node id
        Map<WeakRef<DragHandle>, Ref<PipelineNodeWidget>> mHandleToNode;    // Node cards by drag handle

        Ref<ContextMenu>        mContextMenu;     // Canvas context menu
        Ref<ContextMenu>        mNodeContextMenu; // Node context menu
        Ref<ContextMenu>        mEdgeContextMenu; // Link context menu
        Ref<ContextMenu>        mPopupMenu;       // Menu of a card control, shown by ShowPopupMenu
        Vec2F                   mContextMenuPos;  // Canvas point where a context menu was opened
        Ref<PipelineNodeWidget> mContextNode;     // Node the context menu was opened for
        String                  mContextEdgeId;   // Link the context menu was opened for

        Ref<Sprite> mSelectionSprite;       // Rubber-band selection sprite @SERIALIZABLE
        Vec2F       mSelectingPressedPoint; // Point where selection started, in local space
        bool        mSelecting = false;     // True while rubber-band selecting

        PendingEdge             mPendingEdge;             // Link being dragged
        String                  mSelectedEdgeId;          // Selected link id
        int                     mSelectedPointIndex = -1; // Selected bend point of the selected link
        Vector<Ref<DragHandle>> mBendHandles;             // Drag handles of the selected link bend points
        bool                    mBendDragging = false;    // True while a bend point is dragged

        Ref<PipelineExecutor> mExecutor;               // Graph executor
        Vector<String>        mRunQueue;               // Nodes waiting to run after the current run
        String                mRunTarget;              // Target node of the current run
        bool                  mRunIsAutoApply = false; // True when the current run is an auto-apply

        Vector<String>      mAutoApplyQueue;        // Local nodes waiting for auto-apply
        float               mAutoApplyTimer = 0.0f; // Debounce timer of auto-apply
        Map<String, String> mAutoApplyKeys;         // Last edited config key per node pending auto-apply

        String mEditSnapshot;               // Graph snapshot at the start of a continuous edit
        String mEditSnapshotName;           // Undo step name of the continuous edit
        bool   mEditSnapshotActive = false; // True while a continuous edit is in progress

        bool  mNeedAdjustView = false; // True when the view must be fitted at next update
        bool  mCameraDirty = false;    // True when the camera must be saved to the graph
        RectF mCullingRect;            // Visible canvas rectangle with a margin; cards and links outside it are skipped

        ActionsList mActionsList; // Local actions list, used when actionsListDelegate is null

    protected:
        // Called when scrolling, zooms the camera around the cursor
        void OnScrolled(float scroll) override;

        // Called when cursor pressed: starts a link from a port, selects a link or starts rubber-band selection
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released, finishes link drag or rubber-band selection
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor stays down, moves the pending link end or updates pre-selection
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called on double click, adds a bend point to the link under cursor
        void OnCursorDblClicked(const Input::Cursor& cursor) override;

        // Called when right mouse button released, opens link or canvas context menu
        void OnCursorRightMouseReleased(const Input::Cursor& cursor) override;

        // Called when key pressed, handles escape, delete and clipboard shortcuts
        void OnKeyPressed(const Input::Key& key) override;

        // Draws context menus instead of children
        void DrawInheritedDepthChildren() override;

        // Redraws grid, links, cards and bend handles into render target
        void RedrawContent() override;

        // Called when handles selection changed, updates cards selection
        void OnSelectionChanged() override;

        // Deselects all handles and cards
        void DeselectAll() override;

        // Creates canvas, node and link context menus
        void InitializeContextMenus();

        // Fills menu with node types by category; filter keeps only types compatible with the pending link
        void FillAddNodeMenu(const Ref<ContextMenu>& menu, const PendingEdge* filter);

        // Recalculates scrollable area by cards bounds
        void RecalculateViewArea();

        // Marks cards outside the view as culled and drops card details when zoomed far out
        void UpdateCardsVisibility();

        // Draws all links and the pending one
        void DrawEdges();

        // Draws one link as a smooth curve through bend points
        void DrawEdge(const Vec2F& from, const Vec2F& to, const Vector<Vec2F>& points, const Color4& color, float width);

        // Returns polyline of a link curve through bend points
        Vector<Vec2F> BuildEdgePolyline(const Vec2F& from, const Vec2F& to, const Vector<Vec2F>& points) const;

        // Draws rubber-band selection rectangle
        void DrawSelection();

        // Draws bend handles of the selected link scaled with the camera
        void DrawBendHandles();

        // Recreates bend handles for the selected link
        void RebuildBendHandles();

        // Returns topmost node card under canvas point
        Ref<PipelineNodeWidget> FindNodeAt(const Vec2F& canvasPoint) const;

        // Returns link near canvas point, segmentOut receives the index of the bend gap that was hit
        Ref<PipelineEdge> FindEdgeAt(const Vec2F& canvasPoint, int* segmentOut = nullptr) const;

        // Returns port positions of link ends, false when a node card is missing
        bool GetEdgeEnds(const PipelineEdge& edge, Vec2F& from, Vec2F& to) const;

        // Starts dragging a link from port, from a connected input it detaches the existing link
        void BeginEdgeDrag(const Ref<PipelineNodeWidget>& node, const PipelineNodeWidget::PortView& port);

        // Drops the pending link at canvas point: connects a port, adds an input or offers compatible nodes
        void FinishEdgeDrag(const Vec2F& canvasPoint);

        // Connects output to input when types match and no cycle appears, replacing the input link
        bool ConnectPorts(const String& fromNodeId, const String& fromPortId, const String& toNodeId, const String& toPortId);

        // Creates node of type at canvas point snapped to grid and its card
        Ref<PipelineNode> CreateNodeAt(const String& type, const Vec2F& canvasPos);

        // Creates node at canvas point and connects it to the pending link
        void CreateNodeFromPendingEdge(const String& type, const Vec2F& canvasPos);

        // Pushes undo step with graph before and after, marks the asset changed
        void RecordAction(const String& name, const String& before, const String& after);

        // Marks the asset dirty, notifies onChanged and refreshes freshness
        void MarkChanged();

        // Updates fresh flags of cards by cached results
        void RefreshFreshness();

        // Rebuilds card after its ports changed
        void RefreshNodeWidget(const Ref<PipelineNodeWidget>& node);

        // Called by executor: updates cards states, outputs, retries and starts the queued run
        void OnExecutorEvent(const PipelineExecEvent& event);

        // Starts execution of target node branch, cachedOnly reuses cached inputs without marking the branch queued
        void StartRun(const String& targetId, const Vector<String>& bypass, bool cachedOnly);

        // Marks target node and its upstream as queued
        void MarkBranchQueued(const String& targetId);

        // Resets queued, running and applying states of all cards
        void ResetTransientStates();

        // Queues node for debounced auto-apply when all its inputs have results
        void ScheduleAutoApply(const String& nodeId);

        // Returns target node id with all its upstream node ids
        Vector<String> UpstreamNodes(const String& targetId) const;

        // Loads cached previews of all nodes from disk
        void LoadPreviews();

        // Stores camera position and scale into the graph
        void SaveCameraToGraph();

        REF_COUNTERABLE_IMPL(FrameScrollView, SelectableDragHandlesGroup);

        friend class PipelineNodeWidget;
        friend class PipelineGraphAction;
    };
}
// --- META ---

CLASS_BASES_META(Editor::PipelineGraphAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineGraphAction)
{
    FIELD().PUBLIC().NAME(editor);
    FIELD().PUBLIC().NAME(name);
    FIELD().PUBLIC().NAME(before);
    FIELD().PUBLIC().NAME(after);
}
END_META;
CLASS_METHODS_META(Editor::PipelineGraphAction)
{

    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;

CLASS_BASES_META(Editor::PipelineEditor)
{
    BASE_CLASS(Editor::FrameScrollView);
    BASE_CLASS(o2::SelectableDragHandlesGroup);
    BASE_CLASS(o2::KeyboardEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineEditor)
{
    FIELD().PUBLIC().NAME(actionsListDelegate);
    FIELD().PUBLIC().NAME(onChanged);
    FIELD().PUBLIC().NAME(onLog);
    FIELD().PROTECTED().NAME(mAsset);
    FIELD().PROTECTED().NAME(mGraph);
    FIELD().PROTECTED().NAME(mNodesContainer);
    FIELD().PROTECTED().NAME(mNodeWidgets);
    FIELD().PROTECTED().NAME(mNodeWidgetsById);
    FIELD().PROTECTED().NAME(mHandleToNode);
    FIELD().PROTECTED().NAME(mContextMenu);
    FIELD().PROTECTED().NAME(mNodeContextMenu);
    FIELD().PROTECTED().NAME(mEdgeContextMenu);
    FIELD().PROTECTED().NAME(mPopupMenu);
    FIELD().PROTECTED().NAME(mContextMenuPos);
    FIELD().PROTECTED().NAME(mContextNode);
    FIELD().PROTECTED().NAME(mContextEdgeId);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSelectionSprite);
    FIELD().PROTECTED().NAME(mSelectingPressedPoint);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSelecting);
    FIELD().PROTECTED().NAME(mPendingEdge);
    FIELD().PROTECTED().NAME(mSelectedEdgeId);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mSelectedPointIndex);
    FIELD().PROTECTED().NAME(mBendHandles);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mBendDragging);
    FIELD().PROTECTED().NAME(mExecutor);
    FIELD().PROTECTED().NAME(mRunQueue);
    FIELD().PROTECTED().NAME(mRunTarget);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mRunIsAutoApply);
    FIELD().PROTECTED().NAME(mAutoApplyQueue);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mAutoApplyTimer);
    FIELD().PROTECTED().NAME(mAutoApplyKeys);
    FIELD().PROTECTED().NAME(mEditSnapshot);
    FIELD().PROTECTED().NAME(mEditSnapshotName);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mEditSnapshotActive);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mNeedAdjustView);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mCameraDirty);
    FIELD().PROTECTED().NAME(mCullingRect);
    FIELD().PROTECTED().NAME(mActionsList);
}
END_META;
CLASS_METHODS_META(Editor::PipelineEditor)
{

    typedef const Vector<Pair<String, Function<void()>>>& _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAsset, const Ref<PipelineAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<PipelineAsset>, GetAsset);
    FUNCTION().PUBLIC().SIGNATURE(PipelineGraph*, GetGraph);
    FUNCTION().PUBLIC().SIGNATURE(String, GetPipelineId);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetVisibleCanvasRect);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vec2F, NodeToCanvas, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vec2F, CanvasToNode, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Color4, GetPortColor, PipelinePortType);
    FUNCTION().PUBLIC().SIGNATURE(void, OnNodePressed, const Ref<PipelineNodeWidget>&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnNodeDragged, const Ref<PipelineNodeWidget>&, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnNodeDragCompleted, const Ref<PipelineNodeWidget>&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnNodeResized, const Ref<PipelineNodeWidget>&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, AddCustomInput, const Ref<PipelineNodeWidget>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RenameCustomInput, const Ref<PipelineNodeWidget>&, const String&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveCustomInput, const Ref<PipelineNodeWidget>&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, OpenNodeContextMenu, const Ref<PipelineNodeWidget>&);
    FUNCTION().PUBLIC().SIGNATURE(void, ShowNodeError, const Ref<PipelineNodeWidget>&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnNodeConfigChanged, const Ref<PipelineNodeWidget>&, const String&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginContinuousEdit, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, EndContinuousEdit);
    FUNCTION().PUBLIC().SIGNATURE(void, RunNode, const String&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, RunAll);
    FUNCTION().PUBLIC().SIGNATURE(void, StopRun);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsRunning);
    FUNCTION().PUBLIC().SIGNATURE(void, ClearNodeResult, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<PipelineNodeWidget>, GetNodeWidget, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const PipelineNodeRuntime*, GetRuntime, const String&);
    FUNCTION().PUBLIC().SIGNATURE(PipelineValue, GetInputValue, const Ref<PipelineNode>&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(PipelineValue, GetInputValueById, const Ref<PipelineNode>&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildAll);
    FUNCTION().PUBLIC().SIGNATURE(void, FitView);
    FUNCTION().PUBLIC().SIGNATURE(void, SetView, const Vec2F&, float);
    FUNCTION().PUBLIC().SIGNATURE(void, RestoreGraph, const String&);
    FUNCTION().PUBLIC().SIGNATURE(String, SerializeGraph);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ContextMenu>&, GetContextMenu);
    FUNCTION().PUBLIC().SIGNATURE(void, ShowPopupMenu, _tmp1);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ContextMenu>&, GetPopupMenu);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Ref<PipelineNodeWidget>>, GetSelectedNodes);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectNodes, const Vector<String>&);
    FUNCTION().PUBLIC().SIGNATURE(void, DeleteSelection);
    FUNCTION().PUBLIC().SIGNATURE(void, CopySelection);
    FUNCTION().PUBLIC().SIGNATURE(void, Paste, const Vec2F&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, DuplicateSelection);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectAllNodes);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<PipelineEditor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnScrolled, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorDblClicked, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawInheritedDepthChildren);
    FUNCTION().PROTECTED().SIGNATURE(void, RedrawContent);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectionChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, DeselectAll);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeContextMenus);
    FUNCTION().PROTECTED().SIGNATURE(void, FillAddNodeMenu, const Ref<ContextMenu>&, const PendingEdge*);
    FUNCTION().PROTECTED().SIGNATURE(void, RecalculateViewArea);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateCardsVisibility);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawEdges);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawEdge, const Vec2F&, const Vec2F&, const Vector<Vec2F>&, const Color4&, float);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Vec2F>, BuildEdgePolyline, const Vec2F&, const Vec2F&, const Vector<Vec2F>&);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawSelection);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawBendHandles);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildBendHandles);
    FUNCTION().PROTECTED().SIGNATURE(Ref<PipelineNodeWidget>, FindNodeAt, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Ref<PipelineEdge>, FindEdgeAt, const Vec2F&, int*);
    FUNCTION().PROTECTED().SIGNATURE(bool, GetEdgeEnds, const PipelineEdge&, Vec2F&, Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, BeginEdgeDrag, const Ref<PipelineNodeWidget>&, const PipelineNodeWidget::PortView&);
    FUNCTION().PROTECTED().SIGNATURE(void, FinishEdgeDrag, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(bool, ConnectPorts, const String&, const String&, const String&, const String&);
    FUNCTION().PROTECTED().SIGNATURE(Ref<PipelineNode>, CreateNodeAt, const String&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, CreateNodeFromPendingEdge, const String&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, RecordAction, const String&, const String&, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, MarkChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, RefreshFreshness);
    FUNCTION().PROTECTED().SIGNATURE(void, RefreshNodeWidget, const Ref<PipelineNodeWidget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnExecutorEvent, const PipelineExecEvent&);
    FUNCTION().PROTECTED().SIGNATURE(void, StartRun, const String&, const Vector<String>&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, MarkBranchQueued, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, ResetTransientStates);
    FUNCTION().PROTECTED().SIGNATURE(void, ScheduleAutoApply, const String&);
    FUNCTION().PROTECTED().SIGNATURE(Vector<String>, UpstreamNodes, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadPreviews);
    FUNCTION().PROTECTED().SIGNATURE(void, SaveCameraToGraph);
}
END_META;
// --- END META ---
