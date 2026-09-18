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
    void PipelineEditor::OnNodePressed(const Ref<PipelineNodeWidget>& node)
    {
        // Pressed cards come to the front
        mNodeWidgets.Remove(node);
        mNodeWidgets.Add(node);
        mNodesContainer->RemoveChild(node);
        mNodesContainer->AddChild(node);

        if (!mSelectedEdgeId.IsEmpty())
        {
            mSelectedEdgeId = "";
            RebuildBendHandles();
        }
    }

    void PipelineEditor::OnNodeDragged(const Ref<PipelineNodeWidget>& node, const Vec2F& position)
    {
        if (!mEditSnapshotActive)
            BeginContinuousEdit("Move nodes");

        Vec2F delta = CanvasToNode(position) - node->GetNode()->position;
        for (auto& widget : mNodeWidgets)
        {
            if (widget == node || widget->IsSelected())
            {
                widget->GetNode()->position += delta;
                widget->UpdateFromNode();
            }
        }

        RecalculateViewArea();
        mNeedRedraw = true;
    }

    void PipelineEditor::OnNodeDragCompleted(const Ref<PipelineNodeWidget>& node)
    {
        EndContinuousEdit();
    }

    void PipelineEditor::OnNodeResized(const Ref<PipelineNodeWidget>& node, bool completed)
    {
        if (!completed)
        {
            if (!mEditSnapshotActive)
                BeginContinuousEdit("Resize node");
            RecalculateViewArea();
            mNeedRedraw = true;
            return;
        }

        EndContinuousEdit();
    }

    void PipelineEditor::BeginContinuousEdit(const String& name)
    {
        if (mEditSnapshotActive)
            return;

        mEditSnapshot = SerializeGraph();
        mEditSnapshotName = name;
        mEditSnapshotActive = true;
    }

    void PipelineEditor::EndContinuousEdit()
    {
        if (!mEditSnapshotActive)
            return;

        mEditSnapshotActive = false;
        String after = SerializeGraph();
        if (after != mEditSnapshot)
            RecordAction(mEditSnapshotName, mEditSnapshot, after);
    }

    String PipelineEditor::SerializeGraph() const
    {
        auto graph = GetGraph();
        if (!graph)
            return "";

        DataDocument doc;
        doc.Set(*graph);
        return doc.SaveAsString();
    }

    void PipelineEditor::RestoreGraph(const String& serialized)
    {
        auto graph = GetGraph();
        if (!graph)
            return;

        DataDocument doc;
        doc.LoadFromData(serialized);
        doc.Get(*graph);

        mSelectedEdgeId = "";
        mSelectedPointIndex = -1;
        RebuildAll();
        LoadPreviews();
        RefreshFreshness();
        MarkChanged();
    }

    void PipelineEditor::RecordAction(const String& name, const String& before, const String& after)
    {
        if (before == after)
            return;

        auto action = mmake<PipelineGraphAction>();
        action->editor = Ref(this);
        action->name = name;
        action->before = before;
        action->after = after;

        if (actionsListDelegate)
            actionsListDelegate->DoneAction(action);
        else
            mActionsList.DoneAction(action);

        MarkChanged();
    }

    void PipelineEditor::MarkChanged()
    {
        if (auto asset = mAsset.Lock())
        {
            mGraph.SaveToAsset(*asset);
            asset->SetDirty();
        }

        RefreshCardsWithChangedInputs();

        if (onChanged)
            onChanged();

        RefreshFreshness();
        mNeedRedraw = true;
    }

    void PipelineEditor::OnNodeConfigChanged(const Ref<PipelineNodeWidget>& node, const String& key, bool completed /*= true*/)
    {
        bool uiOnly = PipelineGraph::GetUiOnlyConfigKeys().Contains(key);

        if (completed)
        {
            if (mEditSnapshotActive)
                EndContinuousEdit();
            else if (!uiOnly)
                MarkChanged();
            else if (auto asset = mAsset.Lock())
                asset->SetDirty();
        }
        else if (!mEditSnapshotActive)
            BeginContinuousEdit("Edit " + key);

        if (uiOnly)
            return;

        auto schema = node->GetSchema();
        if (schema && schema->category == PipelineNodeCategory::Source)
        {
            RefreshSourceOutput(node);
            return;
        }

        bool chromaKey = key == "chromaColor" || key == "chromaTolerance" || key == "chromaSoftness" || key == "chromaSpill";
        bool instant = schema && schema->instant;
        if (instant || (chromaKey && node->GetNode()->GetConfigBool("transparentBg", false) &&
                        node->GetNode()->GetConfigString("transparentMode", "twoPass") == "chroma"))
        {
            ScheduleAutoApply(node->GetNode()->id);
        }
    }

    void PipelineEditor::ScheduleAutoApply(const String& nodeId)
    {
        auto graph = GetGraph();
        auto node = graph ? graph->FindNode(nodeId) : nullptr;
        if (!node)
            return;

        // Every input must have a rendered result, otherwise the run is refused anyway
        auto incoming = graph->GetIncomingEdges(nodeId);
        if (incoming.IsEmpty())
            return;

        for (auto& edge : incoming)
        {
            auto rt = GetRuntime(edge->fromNodeId);
            if (!rt || !rt->output.IsValid())
                return;
        }

        if (!mAutoApplyQueue.Contains(nodeId))
            mAutoApplyQueue.Add(nodeId);
        mAutoApplyTimer = 0.0f;
    }

    void PipelineEditor::AddCustomInput(const Ref<PipelineNodeWidget>& node)
    {
        auto schema = node->GetSchema();
        if (!schema || schema->addableInputs.IsEmpty())
            return;

        if (schema->addableInputs.Count() == 1)
        {
            String before = SerializeGraph();
            auto customs = node->GetNode()->GetCustomInputs();
            customs.Add(PipelinePort(PipelineNode::GenerateId(), "", schema->addableInputs[0], true));
            node->GetNode()->SetCustomInputs(customs);
            PipelineNodeRegistry::SyncNodeWithSchema(node->GetNode());
            RefreshNodeWidget(node);
            RecordAction("Add input", before, SerializeGraph());
            return;
        }

        mContextNode = node;
        mNodeContextMenu->RemoveAllItems();
        for (auto type : schema->addableInputs)
        {
            mNodeContextMenu->AddItem(PipelinePortTypeToString(type), [this, type]()
            {
                if (!mContextNode) return;
                String before = SerializeGraph();
                auto customs = mContextNode->GetNode()->GetCustomInputs();
                customs.Add(PipelinePort(PipelineNode::GenerateId(), "", type, true));
                mContextNode->GetNode()->SetCustomInputs(customs);
                PipelineNodeRegistry::SyncNodeWithSchema(mContextNode->GetNode());
                RefreshNodeWidget(mContextNode);
                RecordAction("Add input", before, SerializeGraph());
            });
        }
        mNodeContextMenu->Show();
    }

    void PipelineEditor::RenameCustomInput(const Ref<PipelineNodeWidget>& node, const String& portId, const String& name)
    {
        String before = SerializeGraph();
        auto customs = node->GetNode()->GetCustomInputs();
        for (auto& c : customs)
        {
            if (c.id == portId)
                c.name = name;
        }
        node->GetNode()->SetCustomInputs(customs);
        PipelineNodeRegistry::SyncNodeWithSchema(node->GetNode());
        RefreshNodeWidget(node);
        RecordAction("Rename input", before, SerializeGraph());
        ScheduleAutoApply(node->GetNode()->id);
    }

    void PipelineEditor::RemoveCustomInput(const Ref<PipelineNodeWidget>& node, const String& portId)
    {
        String before = SerializeGraph();
        auto customs = node->GetNode()->GetCustomInputs();
        customs.RemoveAll([&](const PipelinePort& p) { return p.id == portId; });
        node->GetNode()->SetCustomInputs(customs);
        PipelineNodeRegistry::SyncNodeWithSchema(node->GetNode());
        if (auto graph = GetGraph())
            graph->RemoveDanglingEdges();
        RefreshNodeWidget(node);
        RecordAction("Remove input", before, SerializeGraph());
    }

    void PipelineEditor::BeginEdgeDrag(const Ref<PipelineNodeWidget>& node, const PipelineNodeWidget::PortView& port)
    {
        mPendingEdge.active = true;
        mPendingEdge.nodeId = node->GetNode()->id;
        mPendingEdge.portId = port.port.id;
        mPendingEdge.fromInput = port.input;
        mPendingEdge.type = port.port.portType;
        mPendingEdge.cursor = node->GetPortPosition(port.port.id, port.input);

        // Dragging from a connected input detaches its link and continues from the source
        if (port.input)
        {
            auto graph = GetGraph();
            if (auto edge = graph ? graph->FindEdgeToPort(node->GetNode()->id, port.port.id) : nullptr)
            {
                String before = SerializeGraph();
                mPendingEdge.nodeId = edge->fromNodeId;
                mPendingEdge.portId = edge->fromPortId;
                mPendingEdge.fromInput = false;
                graph->RemoveEdge(edge->id);
                RecordAction("Detach link", before, SerializeGraph());
            }
        }

        mNeedRedraw = true;
    }

    bool PipelineEditor::ConnectPorts(const String& fromNodeId, const String& fromPortId, const String& toNodeId, const String& toPortId)
    {
        auto graph = GetGraph();
        if (!graph || fromNodeId == toNodeId)
            return false;

        auto from = graph->FindNode(fromNodeId);
        auto to = graph->FindNode(toNodeId);
        if (!from || !to)
            return false;

        auto fromPort = from->FindOutput(fromPortId);
        auto toPort = to->FindInput(toPortId);
        if (!fromPort || !toPort || fromPort->portType != toPort->portType)
            return false;

        if (graph->WouldMakeCycle(fromNodeId, toNodeId))
        {
            if (onLog) onLog("This link would create a cycle");
            return false;
        }

        String before = SerializeGraph();
        if (auto existing = graph->FindEdgeToPort(toNodeId, toPortId))
            graph->RemoveEdge(existing->id);

        auto edge = mmake<PipelineEdge>();
        edge->id = PipelineNode::GenerateId();
        edge->fromNodeId = fromNodeId;
        edge->fromPortId = fromPortId;
        edge->toNodeId = toNodeId;
        edge->toPortId = toPortId;
        graph->edges.Add(edge);
        RecordAction("Connect", before, SerializeGraph());

        auto toSchema = PipelineNodeRegistry::GetSchema(to->nodeType);
        if (toSchema && toSchema->instant)
            ScheduleAutoApply(toNodeId);

        return true;
    }

    void PipelineEditor::FinishEdgeDrag(const Vec2F& p)
    {
        mPendingEdge.active = false;
        mNeedRedraw = true;

        for (int i = mNodeWidgets.Count() - 1; i >= 0; i--)
        {
            auto& widget = mNodeWidgets[i];
            if (auto port = widget->FindPortAt(p))
            {
                if (port->input == mPendingEdge.fromInput)
                    return;

                if (mPendingEdge.fromInput)
                    ConnectPorts(widget->GetNode()->id, port->port.id, mPendingEdge.nodeId, mPendingEdge.portId);
                else
                    ConnectPorts(mPendingEdge.nodeId, mPendingEdge.portId, widget->GetNode()->id, port->port.id);
                return;
            }

            if (!mPendingEdge.fromInput && widget->IsAddInputAt(p))
            {
                auto schema = widget->GetSchema();
                if (!schema || !schema->addableInputs.Contains(mPendingEdge.type))
                    return;

                String before = SerializeGraph();
                auto customs = widget->GetNode()->GetCustomInputs();
                PipelinePort newPort(PipelineNode::GenerateId(), "", mPendingEdge.type, true);
                customs.Add(newPort);
                widget->GetNode()->SetCustomInputs(customs);
                PipelineNodeRegistry::SyncNodeWithSchema(widget->GetNode());
                RefreshNodeWidget(widget);
                RecordAction("Add input", before, SerializeGraph());
                ConnectPorts(mPendingEdge.nodeId, mPendingEdge.portId, widget->GetNode()->id, newPort.id);
                return;
            }

            if (widget->GetCardRect().IsInside(p))
                return;
        }

        // Dropped on empty space: offer compatible nodes
        mContextMenuPos = p;
        PendingEdge filter = mPendingEdge;
        mPendingEdge = filter;
        mContextMenu->RemoveAllItems();
        FillAddNodeMenu(mContextMenu, &filter);
        mContextMenu->Show();
    }

    Ref<PipelineNode> PipelineEditor::CreateNodeAt(const String& type, const Vec2F& canvasPos)
    {
        PushEditorScopeOnStack scope;
        auto graph = GetGraph();
        if (!graph)
            return nullptr;

        auto node = PipelineNodeRegistry::CreateNode(type, CanvasToNode(canvasPos));
        if (!node)
            return nullptr;

        node->position = Vec2F(Math::Round(node->position.x / 20.0f) * 20.0f, Math::Round(node->position.y / 20.0f) * 20.0f);
        graph->nodes.Add(node);

        auto widget = mmake<PipelineNodeWidget>(Ref(this), node);
        mNodesContainer->AddChild(widget);
        mNodeWidgets.Add(widget);
        mNodeWidgetsById[node->id] = widget;
        mHandleToNode[widget->dragHandle] = widget;
        RecalculateViewArea();
        mNeedRedraw = true;
        return node;
    }

    Ref<PipelineNode> PipelineEditor::AddNodeAtViewCenter(const String& type)
    {
        auto graph = GetGraph();
        if (!graph)
            return nullptr;

        auto schema = PipelineNodeRegistry::GetSchema(type);
        Vec2F size = schema && schema->defaultSize.Length() > 0.0f ? schema->defaultSize : Vec2F(220, 140);
        Vec2F center = GetVisibleCanvasRect().Center();
        Vec2F position(center.x - size.x*0.5f, center.y + size.y*0.5f);

        // Adds in a row would land exactly on top of one another, so an occupied spot steps down-right
        for (int i = 0; i < 12; i++)
        {
            Vec2F node = CanvasToNode(position);
            node = Vec2F(Math::Round(node.x/20.0f)*20.0f, Math::Round(node.y/20.0f)*20.0f);
            if (!graph->nodes.Any([&](const Ref<PipelineNode>& x) { return x->position == node; }))
                break;

            position += Vec2F(40, -40);
        }

        String before = SerializeGraph();
        auto node = CreateNodeAt(type, position);
        if (!node)
            return nullptr;

        RecordAction("Add node", before, SerializeGraph());
        SelectNodes({ node->id });
        return node;
    }

    void PipelineEditor::CreateNodeFromPendingEdge(const String& type, const Vec2F& canvasPos)
    {
        String before = SerializeGraph();
        auto node = CreateNodeAt(type, canvasPos);
        if (!node)
            return;

        auto schema = PipelineNodeRegistry::GetSchema(type);
        String toNodeId, toPortId, fromNodeId, fromPortId;
        if (mPendingEdge.fromInput)
        {
            auto port = node->outputs.Find([&](const PipelinePort& p) { return p.portType == mPendingEdge.type; });
            if (port)
            {
                fromNodeId = node->id; fromPortId = port->id;
                toNodeId = mPendingEdge.nodeId; toPortId = mPendingEdge.portId;
            }
            // The new node sits to the left of the target
            node->position.x -= (node->size.x > 0 ? node->size.x : PipelineNodeWidget::defaultWidth);
        }
        else
        {
            auto port = node->inputs.Find([&](const PipelinePort& p) { return p.portType == mPendingEdge.type; });
            if (!port && schema && schema->addableInputs.Contains(mPendingEdge.type))
            {
                auto customs = node->GetCustomInputs();
                customs.Add(PipelinePort(PipelineNode::GenerateId(), "", mPendingEdge.type, true));
                node->SetCustomInputs(customs);
                PipelineNodeRegistry::SyncNodeWithSchema(node);
                port = node->inputs.Find([&](const PipelinePort& p) { return p.portType == mPendingEdge.type; });
            }
            if (port)
            {
                fromNodeId = mPendingEdge.nodeId; fromPortId = mPendingEdge.portId;
                toNodeId = node->id; toPortId = port->id;
            }
        }

        RebuildAll();
        RecordAction("Add node", before, SerializeGraph());
        if (!toNodeId.IsEmpty())
            ConnectPorts(fromNodeId, fromPortId, toNodeId, toPortId);

        SelectNodes({ node->id });
    }

    void PipelineEditor::DeleteSelection()
    {
        auto graph = GetGraph();
        if (!graph)
            return;

        auto selected = GetSelectedNodes();
        if (selected.IsEmpty())
        {
            if (!mSelectedEdgeId.IsEmpty())
            {
                String before = SerializeGraph();
                if (auto edge = graph->FindEdge(mSelectedEdgeId))
                {
                    if (mSelectedPointIndex >= 0 && mSelectedPointIndex < edge->points.Count())
                    {
                        edge->points.RemoveAt(mSelectedPointIndex);
                        mSelectedPointIndex = -1;
                        RebuildBendHandles();
                        RecordAction("Remove link point", before, SerializeGraph());
                        return;
                    }
                    graph->RemoveEdge(mSelectedEdgeId);
                }
                mSelectedEdgeId = "";
                RebuildBendHandles();
                RecordAction("Delete link", before, SerializeGraph());
            }
            return;
        }

        String before = SerializeGraph();
        for (auto& widget : selected)
            graph->RemoveNode(widget->GetNode()->id);

        DeselectAll();
        RebuildAll();
        RecordAction("Delete nodes", before, SerializeGraph());
    }

    void PipelineEditor::CopySelection()
    {
        auto graph = GetGraph();
        auto selected = GetSelectedNodes();
        if (!graph || selected.IsEmpty())
            return;

        PipelineGraph clip;
        Vector<String> ids;
        for (auto& widget : selected)
        {
            clip.nodes.Add(mmake<PipelineNode>(*widget->GetNode()));
            ids.Add(widget->GetNode()->id);
        }
        for (auto& edge : graph->edges)
        {
            if (ids.Contains(edge->fromNodeId) && ids.Contains(edge->toNodeId))
                clip.edges.Add(mmake<PipelineEdge>(*edge));
        }

        DataDocument doc;
        doc.Set(clip);
        Clipboard::SetText("o2pipeline:" + doc.SaveAsString());
    }

    void PipelineEditor::Paste(const Vec2F& canvasPos, bool useCursor)
    {
        auto graph = GetGraph();
        if (!graph)
            return;

        String text = (String)Clipboard::GetText();
        if (!text.StartsWith("o2pipeline:"))
            return;

        DataDocument doc;
        if (!doc.LoadFromData(text.SubStr(11)))
            return;

        PipelineGraph clip;
        doc.Get(clip);
        if (clip.nodes.IsEmpty())
            return;

        // Fresh ids everywhere so a copy never aliases its original
        Map<String, String> idMap;
        for (auto& node : clip.nodes)
        {
            String newId = PipelineNode::GenerateId();
            idMap[node->id] = newId;
            node->id = newId;
            for (auto& port : node->inputs) { String pid = PipelineNode::GenerateId(); idMap[port.id] = pid; port.id = pid; }
            for (auto& port : node->outputs) { String pid = PipelineNode::GenerateId(); idMap[port.id] = pid; port.id = pid; }
            auto customs = node->GetCustomInputs();
            for (auto& c : customs) { String pid; if (idMap.TryGetValue(c.id, pid)) c.id = pid; }
            node->SetCustomInputs(customs);
        }
        for (auto& edge : clip.edges)
        {
            edge->id = PipelineNode::GenerateId();
            String v;
            if (idMap.TryGetValue(edge->fromNodeId, v)) edge->fromNodeId = v;
            if (idMap.TryGetValue(edge->fromPortId, v)) edge->fromPortId = v;
            if (idMap.TryGetValue(edge->toNodeId, v)) edge->toNodeId = v;
            if (idMap.TryGetValue(edge->toPortId, v)) edge->toPortId = v;
        }

        Vec2F minPos = clip.nodes[0]->position;
        for (auto& node : clip.nodes)
        {
            minPos.x = Math::Min(minPos.x, node->position.x);
            minPos.y = Math::Min(minPos.y, node->position.y);
        }

        Vec2F target = useCursor ? minPos + Vec2F(20, 20) : CanvasToNode(canvasPos);
        Vec2F delta = target - minPos;

        String before = SerializeGraph();
        Vector<String> newIds;
        for (auto& node : clip.nodes)
        {
            node->position += delta;
            graph->nodes.Add(node);
            newIds.Add(node->id);
        }
        for (auto& edge : clip.edges)
            graph->edges.Add(edge);

        RebuildAll();
        RecordAction("Paste", before, SerializeGraph());
        SelectNodes(newIds);
    }

    void PipelineEditor::DuplicateSelection()
    {
        CopySelection();
        Paste(Vec2F(), true);
    }
}
