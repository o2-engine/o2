#include "o2Editor/stdafx.h"
#include "PipelineEditor.h"

#include "o2/Application/Application.h"
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
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeBody.h"

namespace Editor
{
    Ref<PipelineNodeWidget> PipelineEditor::GetNodeWidget(const String& nodeId) const
    {
        Ref<PipelineNodeWidget> widget;
        mNodeWidgetsById.TryGetValue(nodeId, widget);
        return widget;
    }

    const PipelineNodeRuntime* PipelineEditor::GetRuntime(const String& nodeId) const
    {
        Ref<PipelineNodeWidget> widget;
        if (!mNodeWidgetsById.TryGetValue(nodeId, widget))
            return nullptr;

        return &widget->GetRuntime();
    }

    PipelineValue PipelineEditor::GetInputValue(const Ref<PipelineNode>& node, const String& portName) const
    {
        auto port = node->FindInputByName(portName);
        return port ? GetInputValueById(node, port->id) : PipelineValue();
    }

    PipelineValue PipelineEditor::GetInputValueById(const Ref<PipelineNode>& node, const String& portId) const
    {
        auto graph = GetGraph();
        auto edge = graph ? graph->FindEdgeToPort(node->id, portId) : nullptr;
        if (!edge)
            return PipelineValue();

        auto rt = GetRuntime(edge->fromNodeId);
        return rt ? rt->output : PipelineValue();
    }

    bool PipelineEditor::IsRunning() const
    {
        return mExecutor && mExecutor->IsRunning();
    }

    Vector<String> PipelineEditor::UpstreamNodes(const String& targetId) const
    {
        Vector<String> result;
        auto graph = GetGraph();
        if (!graph)
            return result;

        Vector<String> stack = { targetId };
        while (!stack.IsEmpty())
        {
            String id = stack.Last();
            stack.RemoveAt(stack.Count() - 1);
            if (result.Contains(id))
                continue;
            result.Add(id);
            for (auto& edge : graph->GetIncomingEdges(id))
                stack.Add(edge->fromNodeId);
        }
        return result;
    }

    void PipelineEditor::MarkBranchQueued(const String& targetId)
    {
        for (auto& id : UpstreamNodes(targetId))
        {
            if (auto widget = GetNodeWidget(id))
            {
                auto& rt = widget->GetRuntime();
                if (rt.state != "running")
                {
                    rt.state = "queued";
                    rt.error = "";
                    rt.retryAttempt = 0;
                    widget->ApplyRuntime();
                }
            }
        }
        mNeedRedraw = true;
    }

    void PipelineEditor::ResetTransientStates()
    {
        for (auto& widget : mNodeWidgets)
        {
            auto& rt = widget->GetRuntime();
            if (rt.state == "queued" || rt.state == "running")
                rt.state = "idle";
            rt.applying = false;
            rt.retryAttempt = 0;
            widget->ApplyRuntime();
        }
        mNeedRedraw = true;
    }

    void PipelineEditor::StartRun(const String& targetId, const Vector<String>& bypass, bool cachedOnly)
    {
        auto graph = GetGraph();
        if (!graph || IsRunning())
            return;

        mRunTarget = targetId;
        if (!cachedOnly)
        {
            for (auto& widget : mNodeWidgets)
            {
                auto& rt = widget->GetRuntime();
                if (rt.state == "error") { rt.state = "idle"; rt.error = ""; }
            }
            MarkBranchQueued(targetId);
        }

        mExecutor->Execute(GetPipelineId(), *graph, targetId, bypass, cachedOnly);
    }

    void PipelineEditor::RunNode(const String& nodeId, bool bypassSelf)
    {
        if (IsRunning())
        {
            if (!mRunQueue.Contains(nodeId))
                mRunQueue.Add(nodeId);
            MarkBranchQueued(nodeId);
            return;
        }

        mRunIsAutoApply = false;
        StartRun(nodeId, bypassSelf ? Vector<String>{ nodeId } : Vector<String>{}, false);
    }

    void PipelineEditor::RunAll()
    {
        auto graph = GetGraph();
        if (!graph)
            return;

        // Every branch end, not just the finish nodes: a graph whose results are saved by one
        // finish node still has whole chains hanging off other ends, and they are part of "run all"
        auto targets = graph->GetRunTargets();
        if (targets.IsEmpty())
        {
            if (onLog) onLog("Nothing to run");
            return;
        }

        for (auto& id : targets)
            RunNode(id, false);
    }

    void PipelineEditor::StopRun()
    {
        mRunQueue.Clear();
        mAutoApplyQueue.Clear();
        if (mExecutor)
            mExecutor->Cancel();
        ResetTransientStates();
    }

    void PipelineEditor::ClearNodeResult(const String& nodeId)
    {
        auto widget = GetNodeWidget(nodeId);
        auto graph = GetGraph();
        if (!widget || !graph)
            return;

        auto sigs = graph->ComputeSignatures();
        String sig;
        sigs.TryGetValue(nodeId, sig);
        PipelineExecutor::ClearNodeCache(GetPipelineId(), *widget->GetNode(), sig);
        widget->GetRuntime().output = PipelineValue();
        widget->GetRuntime().previewPath = "";
        widget->GetRuntime().srcPreviewPath = "";
        widget->GetRuntime().state = "idle";
        widget->OnOutputChanged();
        if (auto graph = GetGraph())
        {
            for (auto& edge : graph->GetOutgoingEdges(nodeId))
            {
                if (auto downstream = GetNodeWidget(edge->toNodeId))
                    downstream->OnOutputChanged();
            }
        }
        RefreshFreshness();
    }

    void PipelineEditor::OnExecutorEvent(const PipelineExecEvent& event)
    {
        // Executor events arrive from a job between frames; the cards they refresh build widgets,
        // and widgets built outside the editor scope are registered as scene objects
        PushEditorScopeOnStack scope;

        using Type = PipelineExecEvent::Type;
        switch (event.type)
        {
            case Type::NodeState:
            {
                if (auto widget = GetNodeWidget(event.nodeId))
                {
                    auto& rt = widget->GetRuntime();
                    if (mRunIsAutoApply)
                    {
                        if (event.state == "error") { rt.state = "error"; rt.error = event.error; }
                    }
                    else
                    {
                        rt.state = event.state == "done" ? String("idle") : event.state;
                        rt.error = event.error;
                        if (event.state == "done") rt.fresh = true;
                    }
                    rt.retryAttempt = 0;
                    widget->ApplyRuntime();
                }
                break;
            }
            case Type::NodeOutput:
            {
                if (auto widget = GetNodeWidget(event.nodeId))
                {
                    auto& rt = widget->GetRuntime();
                    rt.output = event.value;
                    rt.previewPath = event.previewPath;
                    rt.srcPreviewPath = event.srcPreviewPath;
                    widget->OnOutputChanged();

                    // Downstream local nodes follow a new upstream result
                    if (auto graph = GetGraph())
                    {
                        for (auto& edge : graph->GetOutgoingEdges(event.nodeId))
                        {
                            if (auto downstream = GetNodeWidget(edge->toNodeId))
                                downstream->OnOutputChanged();

                            auto to = graph->FindNode(edge->toNodeId);
                            auto schema = to ? PipelineNodeRegistry::GetSchema(to->nodeType) : nullptr;
                            if (schema && schema->instant && edge->toNodeId != mRunTarget)
                                ScheduleAutoApply(edge->toNodeId);
                        }
                    }
                }
                break;
            }
            case Type::Retry:
            {
                if (auto widget = GetNodeWidget(event.nodeId))
                {
                    auto& rt = widget->GetRuntime();
                    rt.retryAttempt = event.attempt;
                    rt.retryMax = event.maxAttempts;
                    rt.retryStatus = event.status;
                    widget->ApplyRuntime();
                }
                if (onLog) onLog("Retry " + (String)event.attempt + "/" + (String)event.maxAttempts + " (" + event.message + ")");
                break;
            }
            case Type::Log:
                if (onLog) onLog(event.message);
                break;
            case Type::Done:
            case Type::Fatal:
            {
                bool wasAutoApply = mRunIsAutoApply;
                mRunIsAutoApply = false;
                if (event.type == Type::Fatal && !wasAutoApply && onLog)
                    onLog("Pipeline failed: " + event.error);

                if (wasAutoApply)
                {
                    if (auto widget = GetNodeWidget(mRunTarget))
                    {
                        widget->GetRuntime().applying = false;
                        widget->ApplyRuntime();
                    }
                }
                else
                    ResetTransientStates();

                RefreshFreshness();
                break;
            }
        }

        mNeedRedraw = true;
    }
}
