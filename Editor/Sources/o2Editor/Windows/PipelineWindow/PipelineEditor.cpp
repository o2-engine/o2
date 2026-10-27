#include "o2Editor/stdafx.h"
#include "PipelineEditor.h"

#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

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
#include "o2/Render/Texture.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Jobs/JobSystem.h"

#include <chrono>
#include <map>
#include <thread>

namespace Editor
{
    // Editors alive by address with the number each was made with; touched on the main thread only
    static std::map<const PipelineEditor*, int>& LiveEditors()
    {
        static auto* editors = new std::map<const PipelineEditor*, int>();
        return *editors;
    }

    static int lastEditorSerial = 0;

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

    static const float edgeWidth = 2.0f;
    static const float activeEdgeWidth = 2.5f;
    static const float selectedEdgeWidth = 3.5f;
    static const float minEdgePixels = 1.0f;
    static const float bendHandleRadius = 5.0f;
    static const float farViewScale = 4.5f;
    static const float cullingMargin = 0.1f;
    static const float edgeCullingMargin = 200.0f;
    static const float edgeSegmentLength = 5.0f;
    static const float gridStep = 20.0f;
    static const int gridMajorEvery = 10;

    static double NowMs()
    {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    PipelineEditor::PipelineEditor(RefCounter* refCounter):
        FrameScrollView(refCounter), SelectableDragHandlesGroup(refCounter)
    {
        mSelectionSprite = mmake<Sprite>(Color4(0, 0, 0, 0));
        mNodesContainer = mmake<Widget>();
        *mNodesContainer->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(), Vec2F());
        mViewCameraMinScale = 0.25f;
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
        mExecutor->deferAssetsRebuild = true;
        mImageLoader = mmake<PipelineImageLoader>();
        int serial = ++lastEditorSerial;
        LiveEditors()[this] = serial;
        PipelineEditor* self = this;
        mExecutor->onEvent = [self, serial](const PipelineExecEvent& event)
        {
            // The executor's coroutines resume on the job workers: the cards are touched on the main thread only, and the event
            // carries no decoded bitmap the worker may still drop (references are not atomic)
            PipelineExecEvent copy = event;
            copy.value.DropDecoded();
            o2Jobs.Schedule([self, serial, copy]()
            {
                auto it = LiveEditors().find(self);
                if (it != LiveEditors().end() && it->second == serial)
                    self->OnExecutorEvent(copy);
            }, JobPriority::Normal, JobThread::Main);
        };

        InitializeContextMenus();
        ApplyTheme();

        mReady = true;
    }

    PipelineEditor::~PipelineEditor()
    {
        LiveEditors().erase(this);
        if (mExecutor)
            mExecutor->Cancel();
    }

    Ref<RefCounterable> PipelineEditor::CastToRefCounterable(const Ref<PipelineEditor>& ref)
    {
        return DynamicCast<FrameScrollView>(ref);
    }

    Color4 PipelineEditor::GetPortColor(PipelinePortType type)
    {
        return PipelineTheme::Get().PortColor(type);
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

            // A pipeline is named after its file, the way a linked AssetsLine project lists it
            if (!asset->GetPath().IsEmpty())
                mGraph.name = o2FileSystem.GetFileNameWithoutExtension(o2FileSystem.GetPathWithoutDirectories(asset->GetPath()));
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

    void PipelineEditor::ReloadGraph()
    {
        auto asset = mAsset.Lock();
        if (!asset)
            return;

        PipelineGraph loaded;
        loaded.LoadFromAsset(*asset);
        if (loaded.id.IsEmpty())
            loaded.id = mGraph.id;

        loaded.name = mGraph.name;
        mGraph.cameraPosition = loaded.cameraPosition;
        mGraph.cameraScale = loaded.cameraScale;
        mSelectedPointIndex = -1;
        SyncGraphTo(loaded);
        LoadPreviews();
        RefreshFreshness();
        mNeedRedraw = true;
    }

    void PipelineEditor::ReloadResults()
    {
        LoadPreviews();
        RefreshFreshness();
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

        // The field the menu writes to is rebuilt with its card
        if (mModelPicker)
            mModelPicker->Close();
        if (mOptionPicker)
            mOptionPicker->Close();

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

    // Keeps the value already shown when the file holds the same bytes, so its picture is not decoded again
    static void KeepOrTake(PipelineValue& shown, const PipelineValue& loaded)
    {
        bool same = shown.IsValid() == loaded.IsValid() && shown.type == loaded.type && shown.mimeType == loaded.mimeType &&
            shown.data == loaded.data;
        if (!same)
        {
            shown = loaded;
            shown.ShareDecoded();
        }
    }

    void PipelineEditor::LoadPreview(const Ref<PipelineNodeWidget>& widget)
    {
        String pipelineId = GetPipelineId();
        String path;
        PipelineValue value = PipelineExecutor::LoadPreview(pipelineId, *widget->GetNode(), &path);

        // A source is its config: the file it points at now, not what the last run read
        PipelineValue sourceValue;
        String error;
        auto schema = widget->GetSchema();
        if (schema && schema->category == PipelineNodeCategory::Source &&
            ResolveSourceValue(*widget->GetNode(), o2Assets.GetAssetsPath(), sourceValue, error, false))
        {
            value = sourceValue;
        }

        auto& runtime = widget->GetRuntime();
        KeepOrTake(runtime.output, value);
        runtime.previewPath = path;

        // A per-port node keeps a preview per output, so its parts come back with the asset
        auto previous = runtime.portOutputs;
        runtime.portOutputs.Clear();
        auto takePort = [&](const String& portId, const PipelineValue& portValue)
        {
            if (!portValue.IsValid())
                return;

            PipelineValue shown;
            previous.TryGetValue(portId, shown);
            KeepOrTake(shown, portValue);
            runtime.portOutputs[portId] = shown;
        };

        if (schema && schema->dynamicOutputs)
        {
            auto& outputs = widget->GetNode()->outputs;
            for (int i = 1; i < outputs.Count(); i++)
                takePort(outputs[i].id, PipelineExecutor::LoadPortPreview(pipelineId, widget->GetNode()->id, outputs[i].id, outputs[i].portType));
        }
        else if (schema && schema->perPortRun)
        {
            for (auto& port : widget->GetNode()->outputs)
                takePort(port.id, PipelineExecutor::LoadPortPreview(pipelineId, widget->GetNode()->id, port.id, port.portType));

            if (!runtime.output.IsValid() && !runtime.portOutputs.empty())
                runtime.output = runtime.portOutputs.begin()->second;

            // AssetsLine keeps a single part only as the node's result
            auto& outputs = widget->GetNode()->outputs;
            if (outputs.Count() == 1 && runtime.output.IsValid() && !runtime.portOutputs.ContainsKey(outputs[0].id))
                runtime.portOutputs[outputs[0].id] = runtime.output;
        }

        String srcPath = PipelineExecutor::GetSourcePreviewPath(pipelineId, widget->GetNode()->id);
        runtime.srcPreviewPath = o2FileSystem.IsFileExist(srcPath) ? srcPath : String();
        runtime.srcPreviewLoaded = "";
    }

    void PipelineEditor::LoadPreviews()
    {
        for (auto& widget : mNodeWidgets)
            LoadPreview(widget);

        for (auto& widget : mNodeWidgets)
            widget->OnOutputChanged();
    }

    Ref<PipelineNodeWidget> PipelineEditor::AddCard(const Ref<PipelineNode>& node)
    {
        PushEditorScopeOnStack scope;
        PipelineNodeRegistry::SyncNodeWithSchema(node);
        auto widget = mmake<PipelineNodeWidget>(Ref(this), node);
        mNodesContainer->AddChild(widget);
        mNodeWidgets.Add(widget);
        mNodeWidgetsById[node->id] = widget;
        mHandleToNode[widget->dragHandle] = widget;
        return widget;
    }

    void PipelineEditor::RemoveCard(const String& nodeId)
    {
        Ref<PipelineNodeWidget> widget;
        if (!mNodeWidgetsById.TryGetValue(nodeId, widget))
            return;

        // A menu writes into a field of its card
        if (mModelPicker)
            mModelPicker->Close();
        if (mOptionPicker)
            mOptionPicker->Close();

        widget->dragHandle->SetSelectionGroup(nullptr);
        mHandleToNode.Remove(widget->dragHandle);
        mNodeWidgets.Remove(widget);
        mNodeWidgetsById.Remove(nodeId);
        mNodesContainer->RemoveChild(widget);
    }

    static bool SamePorts(const Vector<PipelinePort>& a, const Vector<PipelinePort>& b)
    {
        if (a.Count() != b.Count())
            return false;

        for (int i = 0; i < a.Count(); i++)
        {
            if (!(a[i] == b[i]))
                return false;
        }
        return true;
    }

    void PipelineEditor::SyncGraphTo(const PipelineGraph& target)
    {
        PushEditorScopeOnStack scope;
        auto graph = GetGraph();
        if (!graph)
            return;

        Map<String, Ref<PipelineNode>> current;
        for (auto& node : graph->nodes)
            current[node->id] = node;

        Vector<Ref<PipelineNode>> nodes;
        Vector<Ref<PipelineNodeWidget>> added;
        for (auto& wanted : target.nodes)
        {
            Ref<PipelineNode> node;
            if (!current.TryGetValue(wanted->id, node))
            {
                node = mmake<PipelineNode>(*wanted);
                nodes.Add(node);
                added.Add(AddCard(node));
                continue;
            }

            current.Remove(wanted->id);
            nodes.Add(node);
            auto widget = GetNodeWidget(node->id);
            bool sameContent = node->nodeType == wanted->nodeType && SamePorts(node->inputs, wanted->inputs) &&
                SamePorts(node->outputs, wanted->outputs) && node->config == wanted->config && node->extra == wanted->extra;
            bool samePlace = node->position == wanted->position && node->size == wanted->size;
            if (sameContent && samePlace)
                continue;

            // The card keeps its node object, so its body and the views holding the node stay valid
            *node = *wanted;
            if (!widget)
                continue;

            if (sameContent)
                widget->UpdateFromNode();
            else
                RefreshNodeWidget(widget);
        }

        for (auto& gone : current)
            RemoveCard(gone.first);

        graph->nodes = nodes;
        graph->edges.Clear();
        for (auto& edge : target.edges)
            graph->edges.Add(mmake<PipelineEdge>(*edge));

        graph->id = target.id;
        graph->name = target.name;
        graph->extra = static_cast<const DataValue&>(target.extra);
        graph->RemoveDanglingEdges();

        for (auto& widget : added)
            LoadPreview(widget);
        for (auto& widget : added)
            widget->OnOutputChanged();

        if (!mSelectedEdgeId.IsEmpty() && !graph->FindEdge(mSelectedEdgeId))
            mSelectedEdgeId = "";
        RebuildBendHandles();
        RefreshCardsWithChangedInputs();
        RecalculateViewArea();
        mNeedRedraw = true;
    }

    void PipelineEditor::RefreshSourceOutput(const Ref<PipelineNodeWidget>& widget)
    {
        auto schema = widget ? widget->GetSchema() : nullptr;
        auto graph = GetGraph();
        if (!schema || schema->category != PipelineNodeCategory::Source || !graph)
            return;

        PipelineValue value;
        String error;
        if (!ResolveSourceValue(*widget->GetNode(), o2Assets.GetAssetsPath(), value, error, false))
            value = PipelineValue();

        // A text source typed into keeps its value object, so the cards reading it are refreshed only when it changed
        auto& output = widget->GetRuntime().output;
        if (output.IsValid() == value.IsValid() && output.type == value.type && output.data == value.data)
            return;

        output = value;
        output.ShareDecoded();
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

    Ref<Bitmap> PipelineEditor::GetPicture(const PipelineValue& value, const String& nodeId)
    {
        return mImageLoader->Request(value, nodeId);
    }

    bool PipelineEditor::IsPicturePending(const PipelineValue& value) const
    {
        return mImageLoader->IsPending(value);
    }

    bool PipelineEditor::IsWaitingForPictures(const String& nodeId) const
    {
        return mImageLoader->IsWaiting(nodeId);
    }

    String PipelineEditor::GetBackgroundStatus() const
    {
        String status;
        int pending = mImageLoader->GetPendingCount();
        if (pending > 0)
        {
            int finished = mImageLoader->GetFinishedCount();
            status = "Loading previews " + (String)finished + "/" + (String)(finished + pending);
        }

        if (mRunIsAutoApply && IsRunning())
        {
            String name;
            if (auto widget = GetNodeWidget(mRunTarget))
                name = widget->GetSchema() ? widget->GetSchema()->label : widget->GetNode()->nodeType;
            status += (status.IsEmpty() ? "" : " · ") + String("Applying ") + name;
        }

        if (Assets::IsSingletonInitialzed() && o2Assets.IsRebuildingAssets())
            status += (status.IsEmpty() ? "" : " · ") + String("Rebuilding assets");

        return status;
    }

    void PipelineEditor::FinishBackgroundWork()
    {
        PushEditorScopeOnStack scope;
        for (auto& nodeId : mImageLoader->WaitAll())
        {
            if (auto widget = GetNodeWidget(nodeId))
                widget->OnOutputChanged();
        }

        // The tree of a background assets rebuild is reloaded by a main-thread job
        for (int i = 0; i < 30000 && Assets::IsSingletonInitialzed() && o2Assets.IsRebuildingAssets(); i++)
        {
            o2Jobs.ExecuteMainThreadJobs(-1.0f);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        mNeedRedraw = true;
    }

    void PipelineEditor::WarmUpCards(float dt)
    {
        // A couple of milliseconds a frame: enough to lay out the whole graph within a second or two after opening it
        const double budgetMs = 2.0;
        double start = NowMs();
        for (auto& widget : mNodeWidgets)
        {
            if (widget->IsWarm())
                continue;

            widget->WarmUp(dt);
            if (NowMs() - start > budgetMs)
                break;
        }
    }

    void PipelineEditor::ApplyDecodedPictures()
    {
        if (mImageLoader->GetPendingCount() == 0)
            return;

        // The budget, refreshing the cards included, keeps a burst of arrivals (opening a big pipeline) spread over frames
        const double budgetMs = 6.0;
        mNeedRedraw = true;
        double start = NowMs();
        while (NowMs() - start < budgetMs)
        {
            auto ready = mImageLoader->TakeReady(0.0f);
            if (ready.IsEmpty())
                break;

            for (auto& nodeId : ready)
            {
                if (auto widget = GetNodeWidget(nodeId))
                    widget->OnOutputChanged();
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

    Vec2F PipelineEditor::SnapToGrid(const Vec2F& p)
    {
        return Vec2F(Math::Round(p.x/gridStep)*gridStep, Math::Round(p.y/gridStep)*gridStep);
    }

    void PipelineEditor::DrawGrid()
    {
        DrawCanvasGradient();
        DrawGridDots();
    }

    void PipelineEditor::DrawCanvasGradient()
    {
        // AssetsLine --canvas-bg: radial-gradient(ellipse 120% 95% at 30% -20%, inner 0%, mid 42%, outer 78%) over the view;
        // past the last stop the cleared outer colour shows
        auto& theme = PipelineTheme::Get();
        RectF view = GetVisibleCanvasRect();
        float w = view.Width(), h = view.Height();
        Vec2F center(view.left + w * 0.3f, view.top + h * 0.2f);
        Vec2F radius(w * 1.2f, h * 0.95f);

        const int segments = 96;
        const float stops[] = { 0.0f, 0.42f, 0.78f };
        const Color4 colors[] = { theme.canvasInner, theme.canvasMid, theme.canvasOuter };

        Vector<Vertex> vertices;
        Vector<VertexIndex> indices;
        vertices.Add(Vertex(center, colors[0].ABGR(), 0, 0));
        for (int ring = 1; ring < 3; ring++)
        {
            for (int i = 0; i < segments; i++)
            {
                float a = Math::PI() * 2.0f * i / segments;
                Vec2F p = center + Vec2F(Math::Cos(a) * radius.x, Math::Sin(a) * radius.y) * stops[ring];
                vertices.Add(Vertex(p, colors[ring].ABGR(), 0, 0));
            }
        }

        for (int i = 0; i < segments; i++)
        {
            int j = (i + 1) % segments;
            indices.Add(0); indices.Add((VertexIndex)(1 + i)); indices.Add((VertexIndex)(1 + j));
            int a0 = 1 + i, a1 = 1 + j, b0 = 1 + segments + i, b1 = 1 + segments + j;
            indices.Add((VertexIndex)a0); indices.Add((VertexIndex)b0); indices.Add((VertexIndex)b1);
            indices.Add((VertexIndex)a0); indices.Add((VertexIndex)b1); indices.Add((VertexIndex)a1);
        }

        o2Render.DrawBuffer(PrimitiveType::Polygon, vertices.Data(), vertices.Count(), indices.Data(), indices.Count() / 3,
                            o2Render.GetDefaultMaterial(), TextureRef());
    }

    // Dot fade by its on-screen spacing: gone below 3 pixels, full from 9 (Canvas.tsx gridOpacity)
    static float DotsOpacity(float spacing)
    {
        return Math::Clamp01((spacing - 3.0f) / 6.0f);
    }

    void PipelineEditor::DrawGridDots()
    {
        // A dot per grid point, 2 screen pixels below-right of it, fading in as the grid opens up; every tenth one brighter,
        // drawn over it and fading by its own spacing (Canvas.tsx grid-pattern / grid-pattern-major)
        auto& theme = PipelineTheme::Get();
        float unitsPerPixel = Math::Max(mViewCamera.GetScale().x, 0.0001f);
        float spacing = gridStep / unitsPerPixel;
        float majorWorld = gridStep * gridMajorEvery;
        if (DotsOpacity(spacing * gridMajorEvery) <= 0.0f)
            return;

        RectF view = GetVisibleCanvasRect();

        // Dense dots are one quad over the view with a repeated tile of ten by ten cells: as quads they would be hundreds
        // of thousands per frame when zoomed out on a large screen
        const float quadsSpacing = 32.0f;
        if (spacing < quadsSpacing)
        {
            int tileSize = Math::Max(1, Math::RoundToInt(spacing * gridMajorEvery));
            if (!mDotsTile || tileSize != mDotsTileSize || mDotsTileTheme != PipelineTheme::GetVersion())
                BuildDotsTile(tileSize);

            // The tile spans exactly ten cells of the canvas whatever its rounded pixel size; v runs down like the tile's rows
            ULong white = Color4::White().ABGR();
            Vertex vertices[4] = {
                Vertex(Vec2F(view.left, view.top), white, view.left / majorWorld, -view.top / majorWorld),
                Vertex(Vec2F(view.right, view.top), white, view.right / majorWorld, -view.top / majorWorld),
                Vertex(Vec2F(view.right, view.bottom), white, view.right / majorWorld, -view.bottom / majorWorld),
                Vertex(Vec2F(view.left, view.bottom), white, view.left / majorWorld, -view.bottom / majorWorld)
            };
            VertexIndex indices[6] = { 0, 1, 2, 0, 2, 3 };
            o2Render.DrawBuffer(PrimitiveType::Polygon, vertices, 4, indices, 2, o2Render.GetDefaultMaterial(), mDotsTile);
            return;
        }

        // Sparse dots, a few thousand at most: a 2 x 2 pixel quad each
        float size = 2.0f * unitsPerPixel;
        Vector<Vertex> vertices;
        Vector<VertexIndex> indices;
        auto dots = [&](float step, const Color4& dotColor)
        {
            float opacity = DotsOpacity(step / unitsPerPixel);
            if (opacity <= 0.0f)
                return;

            // A pixel-wide circle covers each of its four pixels by about pi/4
            ULong dcolor = PipelineTheme::Alpha(dotColor, opacity * 0.785f).ABGR();
            for (int i = Math::FloorToInt(view.left / step); i * step <= view.right; i++)
            {
                for (int j = Math::FloorToInt(view.bottom / step); j * step <= view.top + step; j++)
                {
                    float x = i * step, y = j * step;
                    VertexIndex base = (VertexIndex)vertices.Count();
                    vertices.Add(Vertex(Vec2F(x, y - size), dcolor, 0, 0));
                    vertices.Add(Vertex(Vec2F(x + size, y - size), dcolor, 0, 0));
                    vertices.Add(Vertex(Vec2F(x + size, y), dcolor, 0, 0));
                    vertices.Add(Vertex(Vec2F(x, y), dcolor, 0, 0));
                    indices.Add(base); indices.Add(base + 1); indices.Add(base + 2);
                    indices.Add(base); indices.Add(base + 2); indices.Add(base + 3);
                }
            }
        };

        dots(gridStep, theme.gridDot);
        dots(majorWorld, theme.gridDotMajor);
        if (!vertices.IsEmpty())
            o2Render.DrawBuffer(PrimitiveType::Polygon, vertices.Data(), vertices.Count(), indices.Data(), indices.Count() / 3,
                                o2Render.GetDefaultMaterial(), TextureRef());
    }

    void PipelineEditor::BuildDotsTile(int tileSize)
    {
        // Rows run down from the grid point at the tile's top-left corner; each dot is a 2 x 2 pixel box at its cell's corner,
        // spread over the pixels it overlaps, wrapping around the edges so the repeats join
        auto& theme = PipelineTheme::Get();
        float cell = tileSize / (float)gridMajorEvery;
        float minorOpacity = DotsOpacity(cell), majorOpacity = DotsOpacity((float)tileSize);

        Vector<float> rgba;
        rgba.Resize(tileSize * tileSize * 4);
        for (auto& v : rgba)
            v = 0.0f;

        auto splat = [&](float x0, float y0, const Color4& color, float opacity)
        {
            float alpha = color.a / 255.0f * opacity * 0.785f;
            if (alpha <= 0.0f)
                return;

            for (int py = Math::FloorToInt(y0); py < Math::CeilToInt(y0 + 2.0f); py++)
            {
                for (int px = Math::FloorToInt(x0); px < Math::CeilToInt(x0 + 2.0f); px++)
                {
                    float cover = Math::Max(0.0f, Math::Min(px + 1.0f, x0 + 2.0f) - Math::Max((float)px, x0)) *
                                  Math::Max(0.0f, Math::Min(py + 1.0f, y0 + 2.0f) - Math::Max((float)py, y0));
                    float a = alpha * cover;
                    if (a <= 0.0f)
                        continue;

                    // Source over what is there (premultiplied)
                    int wx = ((px % tileSize) + tileSize) % tileSize, wy = ((py % tileSize) + tileSize) % tileSize;
                    float* dst = &rgba[(wy * tileSize + wx) * 4];
                    dst[0] = color.r / 255.0f * a + dst[0] * (1.0f - a);
                    dst[1] = color.g / 255.0f * a + dst[1] * (1.0f - a);
                    dst[2] = color.b / 255.0f * a + dst[2] * (1.0f - a);
                    dst[3] = a + dst[3] * (1.0f - a);
                }
            }
        };

        for (int j = 0; j < gridMajorEvery; j++)
        {
            for (int i = 0; i < gridMajorEvery; i++)
                splat(i * cell, j * cell, theme.gridDot, minorOpacity);
        }
        splat(0.0f, 0.0f, theme.gridDotMajor, majorOpacity);

        Bitmap bitmap(PixelFormat::R8G8B8A8, Vec2I(tileSize, tileSize));
        UInt8* data = bitmap.GetData();
        for (int k = 0; k < tileSize * tileSize; k++)
        {
            const float* src = &rgba[k * 4];
            float a = src[3];
            data[k * 4 + 0] = (UInt8)Math::Clamp(Math::RoundToInt(a > 0.0f ? src[0] / a * 255.0f : 0.0f), 0, 255);
            data[k * 4 + 1] = (UInt8)Math::Clamp(Math::RoundToInt(a > 0.0f ? src[1] / a * 255.0f : 0.0f), 0, 255);
            data[k * 4 + 2] = (UInt8)Math::Clamp(Math::RoundToInt(a > 0.0f ? src[2] / a * 255.0f : 0.0f), 0, 255);
            data[k * 4 + 3] = (UInt8)Math::Clamp(Math::RoundToInt(a * 255.0f), 0, 255);
        }

        mDotsTile = TextureRef(bitmap);
        mDotsTile->SetFilter(Texture::Filter::Linear);
        mDotsTile->SetWrap(Texture::Wrap::Repeat);
        mDotsTileSize = tileSize;
        mDotsTileTheme = PipelineTheme::GetVersion();
    }

    void PipelineEditor::RedrawContent()
    {
        DrawGrid();
        CollectEdgeDraws();
        DrawEdges(false);
        mNodesContainer->Draw();
        DrawEdges(true);
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
        if (mModelPicker)
            mModelPicker->Draw();
        if (mOptionPicker)
            mOptionPicker->Draw();
    }

    void PipelineEditor::DrawSelection()
    {
        if (mSelecting && mIsPressed)
        {
            auto& theme = PipelineTheme::Get();
            RectF rect(LocalToScreenPoint(mSelectingPressedPoint), o2Input.cursorPos);
            o2Render.DrawFilledPolygon({ rect.LeftBottom(), Vec2F(rect.left, rect.top), rect.RightTop(), Vec2F(rect.right, rect.bottom) },
                                       PipelineTheme::Alpha(theme.accent, 0.08f));
            o2Render.DrawAARectFrame(rect, theme.accent, 1.0f, LineType::Dash);
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

    void PipelineEditor::CollectEdgeDraws()
    {
        mEdgeDraws.Clear();
        auto graph = GetGraph();
        if (!graph || !drawEdges)
            return;

        std::map<const PipelineNodeWidget*, RectF> rects;
        auto rectOf = [&](const Ref<PipelineNodeWidget>& widget) -> const RectF&
        {
            auto it = rects.find(widget.Get());
            if (it == rects.end())
                it = rects.emplace(widget.Get(), widget->GetCardRect()).first;
            return it->second;
        };

        bool culling = mCullingRect.Width() > 0.0f;
        for (auto& edge : graph->edges)
        {
            Ref<PipelineNodeWidget> fromWidget, toWidget;
            if (!mNodeWidgetsById.TryGetValue(edge->fromNodeId, fromWidget) || !mNodeWidgetsById.TryGetValue(edge->toNodeId, toWidget))
                continue;

            EdgeDraw draw;
            draw.edge = edge;
            draw.from = fromWidget->GetPortPosition(edge->fromPortId, false, rectOf(fromWidget));
            draw.to = toWidget->GetPortPosition(edge->toPortId, true, rectOf(toWidget));

            if (culling)
            {
                RectF bounds(Math::Min(draw.from.x, draw.to.x), Math::Max(draw.from.y, draw.to.y),
                             Math::Max(draw.from.x, draw.to.x), Math::Min(draw.from.y, draw.to.y));
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

            Vec2F bodyOffset;
            draw.overCards = fromWidget->IsBodyPort(edge->fromPortId, bodyOffset);

            auto port = fromWidget->GetNode()->FindOutput(edge->fromPortId);
            auto& theme = PipelineTheme::Get();
            Color4 color = port ? GetPortColor(port->portType) : theme.fgDim;

            // AssetsLine .edge-path: the port colour at 90%, the running link in the accent, a failed one red
            auto& fromRt = fromWidget->GetRuntime();
            auto& toRt = toWidget->GetRuntime();
            float width = edgeWidth;
            float opacity = 0.9f;
            if (fromRt.state == "error" || toRt.state == "error") { color = theme.red; width = activeEdgeWidth; opacity = 1.0f; }
            else if (toRt.state == "running") { color = theme.accent; width = activeEdgeWidth; opacity = 1.0f; }
            else if (toRt.state == "queued") opacity = 0.55f;
            else if (fromRt.state == "done" && toRt.state == "done") opacity = 1.0f;

            draw.selected = edge->id == mSelectedEdgeId;
            if (draw.selected)
            {
                width = selectedEdgeWidth;
                opacity = 1.0f;
            }

            draw.color = PipelineTheme::Alpha(color, opacity);
            draw.width = width;
            mEdgeDraws.Add(draw);
        }
    }

    void PipelineEditor::DrawEdges(bool overCards)
    {
        for (auto& draw : mEdgeDraws)
        {
            if (draw.overCards != overCards)
                continue;

            DrawEdge(draw.from, draw.to, draw.edge->points, draw.color, draw.width);
        }

        if (mPendingEdge.active && !overCards)
        {
            Ref<PipelineNodeWidget> widget;
            if (mNodeWidgetsById.TryGetValue(mPendingEdge.nodeId, widget))
            {
                Vec2F portPos = widget->GetPortPosition(mPendingEdge.portId, mPendingEdge.fromInput);
                Vec2F from = mPendingEdge.fromInput ? mPendingEdge.cursor : portPos;
                Vec2F to = mPendingEdge.fromInput ? portPos : mPendingEdge.cursor;
                DrawEdge(from, to, {}, PipelineTheme::Alpha(GetPortColor(mPendingEdge.type), 0.7f), edgeWidth);
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
            auto regular = mmake<VectorSprite>("ui/pipeline/port_fill.svg");
            regular->color = PipelineTheme::Get().cardSolid;
            auto hover = mmake<VectorSprite>("ui/pipeline/port_fill.svg");
            hover->color = PipelineTheme::Get().accent;
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
        ApplyDecodedPictures();
        mNodesContainer->Update(dt);
        mNodesContainer->UpdateChildren(dt);
        WarmUpCards(dt);

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

        // The running bars of the cards busy in the background move every frame
        if (mBendDragging || !mSelectedEdgeId.IsEmpty() || (mRunIsAutoApply && IsRunning()))
            mNeedRedraw = true;

        // Queued targets start here: the executor's Done arrives from inside its coroutine, where IsRunning() still holds
        if (!IsRunning() && !mRunQueue.IsEmpty())
        {
            String next = mRunQueue[0];
            mRunQueue.RemoveAt(0);
            mRunIsAutoApply = false;
            StartRun(next, {}, false);
        }

        // Finish nodes saving what they receive, one after another
        if (!IsRunning() && !mSaveQueue.IsEmpty())
        {
            String next = mSaveQueue[0];
            mSaveQueue.RemoveAt(0);
            mRunIsAutoApply = false;
            StartSingleRun(next);
        }

        // The assets the finish nodes of a run all wrote are rebuilt once, after the last queued run
        if (mAssetsRebuildPending && !IsRunning() && mRunQueue.IsEmpty() && mSaveQueue.IsEmpty())
        {
            mAssetsRebuildPending = false;
            if (onLog)
                onLog("Rebuilding assets");
            o2Assets.RebuildAssetsInBackground();
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
                    auto widget = mNodeWidgetsById[nodeId];
                    widget->GetRuntime().applying = true;
                    // An effect re-applies from the results on screen: its own stored result may be made from an
                    // upstream picture that was generated again since, under the same signature
                    if (widget->GetSchema() && widget->GetSchema()->instant)
                        StartSingleRun(nodeId);
                    else
                        StartRun(nodeId, {}, true);
                }
            }
        }

        // The view setting of the image-to-image nodes changed: their cards are built again, at the same height
        if (mIoViewVersion != PipelinePairLayout::GetIoViewVersion())
        {
            mIoViewVersion = PipelinePairLayout::GetIoViewVersion();
            for (auto& widget : mNodeWidgets)
            {
                if (PipelinePairLayout::IsPairNode(widget->GetNode()->nodeType))
                {
                    widget->Rebuild();
                    widget->UpdateFromNode();
                }
            }
            mNeedRedraw = true;
        }

        if (mThemeVersion != PipelineTheme::GetVersion())
            ApplyTheme();

        // The graph follows the camera every frame; the asset document, a write of the whole graph, once it rests
        if (mViewCameraMoved || mCameraDirty)
        {
            mCameraDirty = false;
            if (auto graph = GetGraph())
            {
                graph->cameraPosition = mViewCamera.GetPosition2D();
                graph->cameraScale = mViewCamera.GetScale2D().x;
            }
            mCameraSaveDelay = 0.25f;
        }
        else if (mCameraSaveDelay > 0.0f)
        {
            mCameraSaveDelay -= dt;
            if (mCameraSaveDelay <= 0.0f)
                SaveCameraToGraph();
        }
    }

    void PipelineEditor::ApplyTheme()
    {
        PushEditorScopeOnStack scope;
        bool rebuild = mThemeVersion >= 0;
        mThemeVersion = PipelineTheme::GetVersion();

        auto& theme = PipelineTheme::Get();
        mBackColor = theme.canvasOuter;

        // Cards take their colours when built: built again, with the selection kept
        if (rebuild)
        {
            SkinMenus();
            Vector<String> selected;
            for (auto& handle : GetSelectedHandles())
            {
                Ref<PipelineNodeWidget> widget;
                if (mHandleToNode.TryGetValue(handle, widget))
                    selected.Add(widget->GetNode()->id);
            }

            RebuildAll();
            for (auto& id : selected)
            {
                Ref<PipelineNodeWidget> widget;
                if (mNodeWidgetsById.TryGetValue(id, widget))
                    SelectHandle(widget->dragHandle);
            }
        }
        mNeedRedraw = true;
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
