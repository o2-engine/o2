#include "o2Editor/stdafx.h"
#include "PipelineEditor.h"

#include "o2/Application/Application.h"
#include "o2/Assets/Assets.h"
#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/System/Clipboard.h"
#include "o2Editor/Dialogs/YesNoCancelDlg.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeBody.h"

namespace Editor
{
    void PipelineGraphAction::Redo()
    {
        if (auto e = editor.Lock())
            e->RestoreGraph(after);
    }

    void PipelineGraphAction::Undo()
    {
        if (auto e = editor.Lock())
            e->RestoreGraph(before);
    }

    static const float edgeWidth = 1.5f;
    static const float activeEdgeWidth = 2.5f;
    static const float minEdgePixels = 1.0f;
    static const float bendHandleRadius = 5.0f;
    static const float farViewScale = 4.5f;
    static const float cullingMargin = 0.1f;
    static const float edgeCullingMargin = 200.0f;
    static const float edgeSegmentLength = 5.0f;

    PipelineEditor::PipelineEditor(RefCounter* refCounter):
        FrameScrollView(refCounter), SelectableDragHandlesGroup(refCounter)
    {
        mSelectionSprite = mmake<Sprite>(Color4(0, 150, 136, 30));
        mNodesContainer = mmake<Widget>();
        *mNodesContainer->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(), Vec2F());
        mViewCameraMinScale = 0.15f;
        mViewCameraMaxScale = 40.0f;

        // The card layer hides the view from the scroll pass of the event system, so the wheel over a card comes back through it
        mListenersLayer->passScrollThrough = true;
        mListenersLayer->onScrollPassed = [this](float scroll) { OnScrolled(scroll); };

        // The same for the right button: cards and their controls would take it, so the canvas pans over them
        mListenersLayer->passRightButtonThrough = true;
        mListenersLayer->onRightButtonPassPressed = [this](const Input::Cursor& cursor) { OnCursorRightMousePressed(cursor); };
        mListenersLayer->onRightButtonPassDown = [this](const Input::Cursor& cursor) { OnCursorRightMouseStayDown(cursor); };
        mListenersLayer->onRightButtonPassReleased = [this](const Input::Cursor& cursor) { OnRightButtonClickOrPanEnd(cursor); };

        mExecutor = mmake<PipelineExecutor>();
        WeakRef<PipelineEditor> weakThis(this);
        mExecutor->onEvent = [weakThis](const PipelineExecEvent& event)
        {
            if (auto self = weakThis.Lock())
                self->OnExecutorEvent(event);
        };

        InitializeContextMenus();

        mReady = true;
    }

    PipelineEditor::~PipelineEditor()
    {
        if (mExecutor)
            mExecutor->Cancel();
    }

    Ref<RefCounterable> PipelineEditor::CastToRefCounterable(const Ref<PipelineEditor>& ref)
    {
        return DynamicCast<FrameScrollView>(ref);
    }

    Color4 PipelineEditor::GetPortColor(PipelinePortType type)
    {
        switch (type)
        {
            case PipelinePortType::Image: return Color4(255, 152, 0, 255);
            case PipelinePortType::Video: return Color4(156, 39, 176, 255);
            case PipelinePortType::Audio: return Color4(0, 150, 136, 255);
            default: return Color4(33, 150, 243, 255);
        }
    }

    void PipelineEditor::SetAsset(const Ref<PipelineAsset>& asset)
    {
        StopRun();
        mAsset = asset;
        mSelectedEdgeId = "";
        mSelectedPointIndex = -1;
        mAutoApplyQueue.Clear();
        mAutoApplyKeys.Clear();
        mRunQueue.Clear();

        if (asset)
        {
            mGraph.LoadFromAsset(*asset);
            // Older assets keyed their cache by the asset UID; adopting it keeps their results
            if (mGraph.id.IsEmpty())
                mGraph.id = (String)asset->GetUID();
        }
        else
            mGraph = PipelineGraph();

        RebuildAll();
        LoadPreviews();
        RefreshFreshness();

        if (asset && mGraph.cameraScale > 0.0f && !mGraph.nodes.IsEmpty() &&
            (mGraph.cameraPosition != Vec2F() || mGraph.cameraScale != 1.0f))
        {
            // A hand-written or imported asset may carry a scale outside the view's range: taken as is
            // it locks the canvas at a magnification the wheel can no longer leave
            float scale = Math::Clamp(mGraph.cameraScale, mViewCameraMinScale, mViewCameraMaxScale);
            mViewCamera.center = mGraph.cameraPosition;
            mViewCameraTargetPos = mGraph.cameraPosition;
            mViewCameraTargetScale = Vec2F(scale, scale);
            mViewCamera.scale = Vec2F(scale, scale);
            mNeedAdjustView = false;
        }
        else
            mNeedAdjustView = true;

        mNeedRedraw = true;
    }

    Ref<PipelineAsset> PipelineEditor::GetAsset() const
    {
        return mAsset.Lock();
    }

    PipelineGraph* PipelineEditor::GetGraph() const
    {
        return mAsset.Lock() ? const_cast<PipelineGraph*>(&mGraph) : nullptr;
    }

    String PipelineEditor::GetPipelineId() const
    {
        if (!mGraph.id.IsEmpty())
            return mGraph.id;

        auto asset = mAsset.Lock();
        return asset ? (String)asset->GetUID() : String("none");
    }

    RectF PipelineEditor::GetVisibleCanvasRect() const
    {
        RectF rect = layout->GetWorldRect();
        Vec2F a = rect.LeftBottom() * mScreenToLocalTransform;
        Vec2F b = rect.RightTop() * mScreenToLocalTransform;
        return RectF(Math::Min(a.x, b.x), Math::Max(a.y, b.y), Math::Max(a.x, b.x), Math::Min(a.y, b.y));
    }

    void PipelineEditor::UpdateCardsVisibility()
    {
        RectF visible = GetVisibleCanvasRect();
        Vec2F margin = visible.Size() * cullingMargin;
        visible.left -= margin.x;
        visible.right += margin.x;
        visible.bottom -= margin.y;
        visible.top += margin.y;
        mCullingRect = visible;

        bool detailed = mViewCamera.GetScale2D().x < farViewScale;
        for (auto& widget : mNodeWidgets)
        {
            widget->SetCulled(!visible.IsIntersects(widget->GetCardRect()));
            widget->SetDetailed(detailed);
        }
    }

    const Ref<ContextMenu>& PipelineEditor::GetContextMenu() const
    {
        return mContextMenu;
    }

    void PipelineEditor::RebuildAll()
    {
        PushEditorScopeOnStack scope;
        Map<String, PipelineNodeRuntime> runtimes;
        for (auto& widget : mNodeWidgets)
        {
            runtimes[widget->GetNode()->id] = widget->GetRuntime();
            mNodesContainer->RemoveChild(widget);
        }

        ClearHandles();
        mNodeWidgets.Clear();
        mNodeWidgetsById.Clear();
        mHandleToNode.Clear();
        mBendHandles.Clear();

        auto graph = GetGraph();
        if (!graph)
        {
            RecalculateViewArea();
            return;
        }

        for (auto& node : graph->nodes)
        {
            PipelineNodeRegistry::SyncNodeWithSchema(node);
            auto widget = mmake<PipelineNodeWidget>(Ref(this), node);
            PipelineNodeRuntime runtime;
            if (runtimes.TryGetValue(node->id, runtime))
            {
                widget->GetRuntime() = runtime;
                widget->ApplyRuntime();
            }
            mNodesContainer->AddChild(widget);
            mNodeWidgets.Add(widget);
            mNodeWidgetsById[node->id] = widget;
            mHandleToNode[widget->dragHandle] = widget;
        }

        // Every card exists and holds its result before any of them reads its inputs
        for (auto& widget : mNodeWidgets)
            widget->OnOutputChanged();

        graph->RemoveDanglingEdges();
        mInputLinks.Clear();
        RefreshCardsWithChangedInputs();
        RebuildBendHandles();
        RecalculateViewArea();
        mNeedRedraw = true;
    }

    void PipelineEditor::LoadPreviews()
    {
        String pipelineId = GetPipelineId();
        for (auto& widget : mNodeWidgets)
        {
            String path;
            PipelineValue value = PipelineExecutor::LoadPreview(pipelineId, *widget->GetNode(), &path);

            // A source is its config: the file it points at now, not what the last run read
            PipelineValue sourceValue;
            String error;
            auto schema = widget->GetSchema();
            if (schema && schema->category == PipelineNodeCategory::Source &&
                ResolveSourceValue(*widget->GetNode(), o2Assets.GetAssetsPath(), sourceValue, error))
            {
                value = sourceValue;
            }

            auto& runtime = widget->GetRuntime();
            runtime.output = value;
            runtime.previewPath = path;

            // A per-port node keeps a preview per output, so its parts come back with the asset
            runtime.portOutputs.Clear();
            if (schema && schema->perPortRun)
            {
                for (auto& port : widget->GetNode()->outputs)
                {
                    PipelineValue portValue = PipelineExecutor::LoadPortPreview(pipelineId, widget->GetNode()->id, port.id, port.portType);
                    if (portValue.IsValid())
                        runtime.portOutputs[port.id] = portValue;
                }

                if (!runtime.output.IsValid() && !runtime.portOutputs.empty())
                    runtime.output = runtime.portOutputs.begin()->second;
            }

            String srcPath = PipelineExecutor::GetSourcePreviewPath(pipelineId, widget->GetNode()->id);
            runtime.srcPreviewPath = o2FileSystem.IsFileExist(srcPath) ? srcPath : String();
        }

        for (auto& widget : mNodeWidgets)
            widget->OnOutputChanged();
    }

    void PipelineEditor::RefreshSourceOutput(const Ref<PipelineNodeWidget>& widget)
    {
        auto schema = widget ? widget->GetSchema() : nullptr;
        auto graph = GetGraph();
        if (!schema || schema->category != PipelineNodeCategory::Source || !graph)
            return;

        PipelineValue value;
        String error;
        if (!ResolveSourceValue(*widget->GetNode(), o2Assets.GetAssetsPath(), value, error))
            value = PipelineValue();

        widget->GetRuntime().output = value;
        widget->OnOutputChanged();

        for (auto& edge : graph->GetOutgoingEdges(widget->GetNode()->id))
        {
            if (auto downstream = GetNodeWidget(edge->toNodeId))
            {
                downstream->OnOutputChanged();
                if (downstream->GetSchema() && downstream->GetSchema()->instant)
                    ScheduleAutoApply(edge->toNodeId);
            }
        }
    }

    void PipelineEditor::RefreshCardsWithChangedInputs()
    {
        auto graph = GetGraph();
        if (!graph)
            return;

        Map<String, String> links;
        for (auto& edge : graph->edges)
            links[edge->toNodeId] += edge->fromNodeId + ":" + edge->fromPortId + ">" + edge->toPortId + ";";

        for (auto& widget : mNodeWidgets)
        {
            String id = widget->GetNode()->id;
            String now, was;
            links.TryGetValue(id, now);
            mInputLinks.TryGetValue(id, was);
            if (now != was)
                widget->OnOutputChanged();
        }

        mInputLinks = links;
    }

    void PipelineEditor::RefreshFreshness()
    {
        auto graph = GetGraph();
        if (!graph)
            return;

        auto fresh = PipelineExecutor::ComputeFreshNodes(GetPipelineId(), *graph);
        for (auto& widget : mNodeWidgets)
        {
            widget->GetRuntime().fresh = fresh.Contains(widget->GetNode()->id);
            widget->ApplyRuntime();
        }
    }

    void PipelineEditor::RefreshNodeWidget(const Ref<PipelineNodeWidget>& node)
    {
        node->Rebuild();
        node->UpdateFromNode();
        mNeedRedraw = true;
    }

    void PipelineEditor::RecalculateViewArea()
    {
        if (mNodeWidgets.IsEmpty())
            mAvailableArea = RectF(Vec2F(-500, -500), Vec2F(500, 500));
        else
        {
            mAvailableArea = mNodeWidgets[0]->GetCardRect();
            for (auto& widget : mNodeWidgets)
                mAvailableArea = mAvailableArea.Expand(widget->GetCardRect());
        }

        Vec2F size = mAvailableArea.Size();
        float border = Math::Max(400.0f, Math::Max(size.x, size.y) * 0.5f);
        mAvailableArea.left -= border;
        mAvailableArea.right += border;
        mAvailableArea.top += border;
        mAvailableArea.bottom -= border;

        mHorScrollbar->SetValueRange(mAvailableArea.left, mAvailableArea.right);
        mVerScrollbar->SetValueRange(mAvailableArea.bottom, mAvailableArea.top);
    }

    void PipelineEditor::FitView()
    {
        if (mNodeWidgets.IsEmpty())
        {
            mViewCameraTargetPos = Vec2F();
            mViewCameraTargetScale = Vec2F(1, 1);
            return;
        }

        RectF bounds = mNodeWidgets[0]->GetCardRect();
        for (auto& widget : mNodeWidgets)
            bounds = bounds.Expand(widget->GetCardRect());

        Vec2F viewSize = layout->GetSize2D();
        Vec2F boundsSize = bounds.Size() + Vec2F(80, 80);
        float scale = Math::Max(boundsSize.x / Math::Max(1.0f, viewSize.x), boundsSize.y / Math::Max(1.0f, viewSize.y));
        scale = Math::Clamp(scale, mViewCameraMinScale, mViewCameraMaxScale);
        mViewCameraTargetPos = bounds.Center();
        mViewCameraTargetScale = Vec2F(scale, scale);
        mCameraDirty = true;
    }

    void PipelineEditor::SetView(const Vec2F& center, float scale)
    {
        float clamped = Math::Clamp(scale, mViewCameraMinScale, mViewCameraMaxScale);
        mViewCamera.center = center;
        mViewCamera.scale = Vec2F(clamped, clamped);
        mViewCameraTargetPos = center;
        mViewCameraTargetScale = Vec2F(clamped, clamped);
        mNeedAdjustView = false;
        mNeedRedraw = true;
    }

    void PipelineEditor::Draw()
    {
        ScrollView::Draw();
        DrawSelection();
    }

    void PipelineEditor::RedrawContent()
    {
        DrawGrid();
        DrawEdges();
        mNodesContainer->Draw();
        for (auto& widget : mNodeWidgets)
            widget->DrawPorts();

        DrawBendHandles();
    }

    void PipelineEditor::DrawInheritedDepthChildren()
    {
        mContextMenu->Draw();
        mNodeContextMenu->Draw();
        mEdgeContextMenu->Draw();
        mPopupMenu->Draw();
    }

    void PipelineEditor::DrawSelection()
    {
        if (mSelecting && mIsPressed)
        {
            mSelectionSprite->rect = RectF(LocalToScreenPoint(mSelectingPressedPoint), o2Input.cursorPos);
            mSelectionSprite->Draw();
            o2Render.DrawAARectFrame(mSelectionSprite->rect, Color4(0, 150, 136, 200), 1.0f);
        }
    }

    Vector<Vec2F> PipelineEditor::BuildEdgePolyline(const Vec2F& a, const Vec2F& b, const Vector<Vec2F>& points) const
    {
        Vector<Vec2F> result;

        auto cubic = [&](const Vec2F& p0, const Vec2F& c1, const Vec2F& c2, const Vec2F& p3)
        {
            // Segment count follows the curve length on screen so long links stay smooth and far zoom stays cheap
            float length = (c1 - p0).Length() + (c2 - c1).Length() + (p3 - c2).Length();
            int segments = Math::Clamp((int)(length / (edgeSegmentLength * Math::Max(1.0f, mViewCamera.GetScale2D().x))), 8, 160);
            for (int i = 0; i <= segments; i++)
            {
                float t = (float)i / segments;
                float u = 1 - t;
                Vec2F p = p0 * (u * u * u) + c1 * (3 * u * u * t) + c2 * (3 * u * t * t) + p3 * (t * t * t);
                if (result.IsEmpty() || (result.Last() - p).SqrLength() > 0.01f)
                    result.Add(p);
            }
        };

        if (points.IsEmpty())
        {
            float dx = Math::Max(40.0f, Math::Abs(b.x - a.x) * 0.5f);
            cubic(a, Vec2F(a.x + dx, a.y), Vec2F(b.x - dx, b.y), b);
            return result;
        }

        Vector<Vec2F> route;
        route.Add(a);
        for (auto& p : points)
            route.Add(NodeToCanvas(p));
        route.Add(b);

        float lead = Math::Max(40.0f, Math::Abs(route[1].x - a.x) * 0.5f);
        float tail = Math::Max(40.0f, Math::Abs(b.x - route[route.Count() - 2].x) * 0.5f);
        Vec2F first(a.x - lead, a.y);
        Vec2F last(b.x + tail, b.y);

        for (int i = 0; i < route.Count() - 1; i++)
        {
            Vec2F p0 = i == 0 ? first : route[i - 1];
            Vec2F p1 = route[i];
            Vec2F p2 = route[i + 1];
            Vec2F p3 = i + 2 < route.Count() ? route[i + 2] : last;
            Vec2F c1 = p1 + (p2 - p0) / 6.0f;
            Vec2F c2 = p2 - (p3 - p1) / 6.0f;
            cubic(p1, c1, c2, p2);
        }

        return result;
    }

    void PipelineEditor::DrawEdge(const Vec2F& from, const Vec2F& to, const Vector<Vec2F>& points, const Color4& color, float width)
    {
        auto line = BuildEdgePolyline(from, to, points);
        if (line.Count() < 2)
            return;

        // Canvas units on screen, floored so a zoomed-out link never fades below a pixel
        float pixels = Math::Max(minEdgePixels, width / mViewCamera.GetScale2D().x);
        o2Render.DrawAALine(line, color, pixels);
    }

    bool PipelineEditor::GetEdgeEnds(const PipelineEdge& edge, Vec2F& from, Vec2F& to) const
    {
        Ref<PipelineNodeWidget> fromWidget, toWidget;
        if (!mNodeWidgetsById.TryGetValue(edge.fromNodeId, fromWidget) || !mNodeWidgetsById.TryGetValue(edge.toNodeId, toWidget))
            return false;

        from = fromWidget->GetPortPosition(edge.fromPortId, false);
        to = toWidget->GetPortPosition(edge.toPortId, true);
        return true;
    }

    void PipelineEditor::DrawEdges()
    {
        auto graph = GetGraph();
        if (!graph)
            return;

        bool culling = mCullingRect.Width() > 0.0f;
        for (auto& edge : graph->edges)
        {
            Vec2F from, to;
            if (!GetEdgeEnds(*edge, from, to))
                continue;

            if (culling)
            {
                RectF bounds(Math::Min(from.x, to.x), Math::Max(from.y, to.y), Math::Max(from.x, to.x), Math::Min(from.y, to.y));
                for (auto& point : edge->points)
                {
                    bounds.left = Math::Min(bounds.left, point.x);
                    bounds.right = Math::Max(bounds.right, point.x);
                    bounds.bottom = Math::Min(bounds.bottom, point.y);
                    bounds.top = Math::Max(bounds.top, point.y);
                }

                bounds.left -= edgeCullingMargin;
                bounds.right += edgeCullingMargin;
                bounds.bottom -= edgeCullingMargin;
                bounds.top += edgeCullingMargin;
                if (!bounds.IsIntersects(mCullingRect))
                    continue;
            }

            auto fromNode = graph->FindNode(edge->fromNodeId);
            auto port = fromNode ? fromNode->FindOutput(edge->fromPortId) : nullptr;
            Color4 color = port ? GetPortColor(port->portType) : Color4(96, 125, 139, 255);

            auto fromRt = GetRuntime(edge->fromNodeId);
            auto toRt = GetRuntime(edge->toNodeId);
            bool fromActive = fromRt && fromRt->state != "idle";
            bool toActive = toRt && toRt->state != "idle";
            float width = edgeWidth;
            if (fromActive && toActive)
            {
                if (fromRt->state == "error" || toRt->state == "error") color = Color4(249, 93, 72, 255);
                else if (fromRt->state == "running" || toRt->state == "running" || fromRt->state == "queued") color = Color4(33, 150, 243, 255);
                else if (fromRt->state == "done") color = Color4(76, 175, 80, 255);
                width = activeEdgeWidth;
            }

            bool selected = edge->id == mSelectedEdgeId;
            if (selected)
            {
                DrawEdge(from, to, edge->points, Color4(0, 150, 136, 80), width + 3.0f);
                width += 1.0f;
            }

            DrawEdge(from, to, edge->points, color, width);
        }

        if (mPendingEdge.active)
        {
            Ref<PipelineNodeWidget> widget;
            if (mNodeWidgetsById.TryGetValue(mPendingEdge.nodeId, widget))
            {
                Vec2F portPos = widget->GetPortPosition(mPendingEdge.portId, mPendingEdge.fromInput);
                Vec2F from = mPendingEdge.fromInput ? mPendingEdge.cursor : portPos;
                Vec2F to = mPendingEdge.fromInput ? portPos : mPendingEdge.cursor;
                Color4 color = GetPortColor(mPendingEdge.type);
                color.a = 200;
                DrawEdge(from, to, {}, color, edgeWidth);
            }
        }
    }

    void PipelineEditor::RebuildBendHandles()
    {
        PushEditorScopeOnStack scope;
        mBendHandles.Clear();
        auto graph = GetGraph();
        if (!graph || mSelectedEdgeId.IsEmpty())
            return;

        auto edge = graph->FindEdge(mSelectedEdgeId);
        if (!edge)
            return;

        for (int i = 0; i < edge->points.Count(); i++)
        {
            auto regular = mmake<Sprite>("ui/pipeline/port_fill.png");
            regular->color = Color4(96, 125, 139, 255);
            auto hover = mmake<Sprite>("ui/pipeline/port_fill.png");
            hover->color = Color4(0, 150, 136, 255);
            auto handle = mmake<DragHandle>(regular, hover, hover);
            handle->SetDrawablesSize(Vec2F(bendHandleRadius * 2, bendHandleRadius * 2));
            handle->position = NodeToCanvas(edge->points[i]);
            handle->messageFallDownListener = this;
            String edgeId = edge->id;
            int index = i;
            WeakRef<PipelineEditor> weakThis(this);
            handle->onBeganDragging = [weakThis]()
            {
                if (auto self = weakThis.Lock())
                {
                    self->mBendDragging = true;
                    self->BeginContinuousEdit("Move link point");
                }
            };
            handle->onChangedPos = [weakThis, edgeId, index](const Vec2F& pos)
            {
                auto self = weakThis.Lock();
                if (!self) return;
                auto g = self->GetGraph();
                auto e = g ? g->FindEdge(edgeId) : nullptr;
                if (e && index < e->points.Count())
                {
                    e->points[index] = CanvasToNode(pos);
                    self->mNeedRedraw = true;
                }
            };
            handle->onChangeCompleted = [weakThis]()
            {
                if (auto self = weakThis.Lock())
                {
                    self->mBendDragging = false;
                    self->EndContinuousEdit();
                }
            };
            handle->onPressed = [weakThis, index]() { if (auto self = weakThis.Lock()) self->mSelectedPointIndex = index; };
            handle->onDblClicked = [weakThis, edgeId, index]()
            {
                auto self = weakThis.Lock();
                if (!self) return;
                auto g = self->GetGraph();
                auto e = g ? g->FindEdge(edgeId) : nullptr;
                if (e && index < e->points.Count())
                {
                    String before = self->SerializeGraph();
                    e->points.RemoveAt(index);
                    self->mSelectedPointIndex = -1;
                    self->RebuildBendHandles();
                    self->RecordAction("Remove link point", before, self->SerializeGraph());
                }
            };
            mBendHandles.Add(handle);
        }
    }

    void PipelineEditor::DrawBendHandles()
    {
        Vec2F scale = mViewCamera.GetScale2D();
        for (auto& handle : mBendHandles)
        {
            handle->SetDrawablesSize(Vec2F(bendHandleRadius * 2, bendHandleRadius * 2) * scale);
            handle->Draw();
        }
    }

    void PipelineEditor::Update(float dt)
    {
        PushEditorScopeOnStack scope;

        FrameScrollView::Update(dt);

        UpdateCardsVisibility();
        mNodesContainer->Update(dt);
        mNodesContainer->UpdateChildren(dt);

        if (mReady && mResEnabledInHierarchy && !mIsClipped && mNeedAdjustView)
        {
            mNeedAdjustView = false;
            FitView();
        }

        if (mPendingEdge.active)
        {
            Vec2F cursor = ScreenToLocalPoint(o2Input.cursorPos);
            if (cursor != mPendingEdge.cursor)
            {
                mPendingEdge.cursor = cursor;
                mNeedRedraw = true;
            }
        }

        if (mBendDragging || !mSelectedEdgeId.IsEmpty())
            mNeedRedraw = true;

        // Queued targets start here: the executor's Done arrives from inside its coroutine, where IsRunning() still holds
        if (!IsRunning() && !mRunQueue.IsEmpty())
        {
            String next = mRunQueue[0];
            mRunQueue.RemoveAt(0);
            mRunIsAutoApply = false;
            StartRun(next, {}, false);
        }

        // Auto-apply of local nodes, debounced
        if (!mAutoApplyQueue.IsEmpty())
        {
            mAutoApplyTimer += dt;
            if (mAutoApplyTimer > 0.26f && !IsRunning())
            {
                mAutoApplyTimer = 0.0f;
                String nodeId = mAutoApplyQueue[0];
                mAutoApplyQueue.RemoveAt(0);
                if (mNodeWidgetsById.ContainsKey(nodeId))
                {
                    mRunIsAutoApply = true;
                    mNodeWidgetsById[nodeId]->GetRuntime().applying = true;
                    StartRun(nodeId, {}, true);
                }
            }
        }

        if (mViewCameraMoved || mCameraDirty)
        {
            mCameraDirty = false;
            SaveCameraToGraph();
        }
    }

    void PipelineEditor::UpdateSelfTransform()
    {
        FrameScrollView::UpdateSelfTransform();
        UpdateLocalScreenTransforms();
        OnCameraTransformChanged();
    }

    void PipelineEditor::SaveCameraToGraph()
    {
        if (auto graph = GetGraph())
        {
            graph->cameraPosition = mViewCamera.GetPosition2D();
            graph->cameraScale = mViewCamera.GetScale2D().x;
        }

        if (auto asset = mAsset.Lock())
            mGraph.SaveToAsset(*asset);
    }

    void PipelineEditor::OnScrolled(float scroll)
    {
        Vec2F newScale = mViewCameraTargetScale * (1.0f - WheelZoomStep(scroll));
        ChangeCameraScaleRelativeToCursor(newScale);
        mCameraDirty = true;
    }

    Ref<PipelineNodeWidget> PipelineEditor::FindNodeAt(const Vec2F& p) const
    {
        for (int i = mNodeWidgets.Count() - 1; i >= 0; i--)
        {
            if (mNodeWidgets[i]->GetCardRect().IsInside(p))
                return mNodeWidgets[i];
        }
        return nullptr;
    }

    Ref<PipelineEdge> PipelineEditor::FindEdgeAt(const Vec2F& p, int* segmentOut /*= nullptr*/) const
    {
        auto graph = GetGraph();
        if (!graph)
            return nullptr;

        float threshold = 8.0f * mViewCamera.GetScale2D().x;
        Ref<PipelineEdge> best;
        float bestDist = threshold;
        int bestSegment = 0;

        for (auto& edge : graph->edges)
        {
            Vec2F from, to;
            if (!GetEdgeEnds(*edge, from, to))
                continue;

            auto line = BuildEdgePolyline(from, to, edge->points);
            for (int i = 0; i < line.Count() - 1; i++)
            {
                Vec2F a = line[i], b = line[i + 1];
                Vec2F ab = b - a;
                float len2 = ab.SqrLength();
                float t = len2 > 0 ? Math::Clamp((p - a).Dot(ab) / len2, 0.0f, 1.0f) : 0.0f;
                float dist = (p - (a + ab * t)).Length();
                if (dist < bestDist)
                {
                    bestDist = dist;
                    best = edge;
                    // Segment index in terms of route points: which bend gap the polyline segment belongs to
                    bestSegment = (i * (edge->points.Count() + 1)) / Math::Max(1, line.Count() - 1);
                }
            }
        }

        if (segmentOut)
            *segmentOut = bestSegment;

        return best;
    }
}
// --- META ---

DECLARE_CLASS(Editor::PipelineGraphAction, Editor__PipelineGraphAction);

DECLARE_CLASS(Editor::PipelineEditor, Editor__PipelineEditor);
// --- END META ---
