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
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
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

        // The card under the cursor lands on the grid; the other selected cards keep their distance to it
        Vec2F delta = SnapToGrid(CanvasToNode(position)) - node->GetNode()->position;
        if (delta == Vec2F())
        {
            node->UpdateFromNode();
            return;
        }

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

        return graph->ToJsonString();
    }

    void PipelineEditor::RestoreGraph(const String& serialized)
    {
        auto graph = GetGraph();
        if (!graph)
            return;

        PipelineGraph restored;
        if (!restored.LoadFromJsonString(serialized))
            return;

        mSelectedPointIndex = -1;
        SyncGraphTo(restored);
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
            mCameraSaveDelay = 0.0f;
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

        // An AI UI layout's graphic outputs are derived from its steps: an answer, an undo or a revert brings them in line, and
        // a link that hung on a graphic now gone goes with it
        if (completed && key == "steps" && node->GetNode()->nodeType == "uiLayout")
        {
            auto before = node->GetNode()->outputs;
            PipelineNodeRegistry::SyncNodeWithSchema(node->GetNode());
            auto& after = node->GetNode()->outputs;
            bool same = before.Count() == after.Count();
            for (int i = 0; same && i < after.Count(); i++)
                same = before[i].id == after[i].id && before[i].name == after[i].name;
            if (!same)
            {
                if (auto graph = GetGraph())
                    graph->RemoveDanglingEdges();
                node->RefreshPorts();
            }
        }

        auto schema = node->GetSchema();
        if (schema && schema->category == PipelineNodeCategory::Source)
        {
            RefreshSourceOutput(node);
            return;
        }

        // The cut settings re-cut the cached raw render, the node's or a part's own
        bool chromaKey = key == "chromaColor" || key == "chromaTolerance" || key == "chromaSoftness" || key == "chromaSpill";
        bool instant = schema && schema->instant;
        if (instant || (chromaKey && PipelineTransparency::AnyChromaPostStep(*node->GetNode())))
            ScheduleAutoApply(node->GetNode()->id);
    }

    void PipelineEditor::ScheduleAutoApply(const String& nodeId)
    {
        auto graph = GetGraph();
        auto node = graph ? graph->FindNode(nodeId) : nullptr;
        if (!node)
            return;

        // Every input must have a rendered result, otherwise the run is refused anyway; a node that renders with what
        // it has (the UI kit, the AI UI layout) goes without. References only the node's AI reads are no inputs here
        auto incoming = graph->GetRunInputEdges(nodeId);
        auto nodeSchema = PipelineNodeRegistry::GetSchema(node->nodeType);
        bool partial = nodeSchema && nodeSchema->partialInputs;
        if (incoming.IsEmpty() && !partial)
            return;

        for (auto& edge : partial ? Vector<Ref<PipelineEdge>>() : incoming)
        {
            // A part of a multi-output node has its own result; the node's main one is some other part
            auto rt = GetRuntime(edge->fromNodeId);
            auto from = graph->FindNode(edge->fromNodeId);
            auto schema = from ? PipelineNodeRegistry::GetSchema(from->nodeType) : nullptr;
            bool part = schema && schema->perPortRun && from->outputs.Count() > 1;
            if (!rt || (part ? !rt->portOutputs.ContainsKey(edge->fromPortId) : !rt->output.IsValid()))
                return;
        }

        if (!mAutoApplyQueue.Contains(nodeId))
            mAutoApplyQueue.Add(nodeId);
        mAutoApplyTimer = 0.0f;
    }

    void PipelineEditor::AddCustomInput(const Ref<PipelineNodeWidget>& node, PipelineLayerPlace place /*= PipelineLayerPlace::Front*/)
    {
        auto schema = node->GetSchema();
        if (!schema || schema->addableInputs.IsEmpty())
            return;

        if (schema->addableInputs.Count() == 1)
        {
            String before = SerializeGraph();
            auto customs = node->GetNode()->GetCustomInputs();
            PipelinePort port(PipelineNode::GenerateId(), "", schema->addableInputs[0], true);
            customs.Add(port);
            node->GetNode()->SetCustomInputs(customs);
            PipelineNodeRegistry::SyncNodeWithSchema(node->GetNode());
            PipelineComposerLayout::PlaceNewLayer(*node->GetNode(), port.id, port.portType, place);
            RefreshNodeWidget(node);
            RecordAction("Add input", before, SerializeGraph());
            return;
        }

        mContextNode = node;
        mNodeContextMenu->RemoveAllItems();
        for (auto type : schema->addableInputs)
        {
            String label = type == PipelinePortType::UiKit ? String("UI kit") : type == PipelinePortType::TextStyle ? String("text style") :
                PipelinePortTypeToString(type);
            mNodeContextMenu->AddItem(label, [this, type, place]()
            {
                if (!mContextNode) return;
                String before = SerializeGraph();
                auto customs = mContextNode->GetNode()->GetCustomInputs();
                PipelinePort port(PipelineNode::GenerateId(), "", type, true);
                customs.Add(port);
                mContextNode->GetNode()->SetCustomInputs(customs);
                PipelineNodeRegistry::SyncNodeWithSchema(mContextNode->GetNode());
                PipelineComposerLayout::PlaceNewLayer(*mContextNode->GetNode(), port.id, type, place);
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
        // A body holding the inputs (the composer's layer rows) shows the name itself; the rest of the card stays
        if (node->GetBody() && node->GetBody()->InputsInBody())
            node->OnConfigChanged();
        else
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
        mPendingEdge.ports.Clear();
        if (!port.input && o2Input.IsKeyDown(VK_SHIFT))
        {
            for (auto& output : node->GetNode()->outputs)
            {
                if (output.portType == port.port.portType)
                    mPendingEdge.ports.Add(output.id);
            }
            if (mPendingEdge.ports.Count() < 2)
                mPendingEdge.ports.Clear();
        }

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

    bool PipelineEditor::ConnectPorts(const String& fromNodeId, const String& fromPortId, const String& toNodeId, const String& toPortId,
                                      const String& undoBefore /*= ""*/, const String& undoName /*= "Connect"*/)
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

        String before = undoBefore.IsEmpty() ? SerializeGraph() : undoBefore;
        if (auto existing = graph->FindEdgeToPort(toNodeId, toPortId))
            graph->RemoveEdge(existing->id);

        auto edge = mmake<PipelineEdge>();
        edge->id = PipelineNode::GenerateId();
        edge->fromNodeId = fromNodeId;
        edge->fromPortId = fromPortId;
        edge->toNodeId = toNodeId;
        edge->toPortId = toPortId;
        graph->edges.Add(edge);
        RecordAction(undoName, before, SerializeGraph());

        auto toSchema = PipelineNodeRegistry::GetSchema(to->nodeType);
        if (toSchema && toSchema->instant)
            ScheduleAutoApply(toNodeId);

        return true;
    }

    int PipelineEditor::ConnectOutputs(const String& fromNodeId, const Vector<String>& fromPortIds, const String& toNodeId,
                                       const String& toPortId, PipelineLayerPlace place /*= PipelineLayerPlace::Front*/)
    {
        auto graph = GetGraph();
        auto from = graph ? graph->FindNode(fromNodeId) : nullptr;
        auto to = graph ? graph->FindNode(toNodeId) : nullptr;
        if (!from || !to || fromNodeId == toNodeId || fromPortIds.IsEmpty() || graph->WouldMakeCycle(fromNodeId, toNodeId))
            return 0;

        auto first = from->FindOutput(fromPortIds[0]);
        if (!first)
            return 0;

        PipelinePortType type = first->portType;
        Vector<String> ports = fromPortIds.FindAll([&](const String& id) { auto p = from->FindOutput(id); return p && p->portType == type; });
        auto schema = PipelineNodeRegistry::GetSchema(to->nodeType);
        bool addable = schema && schema->addableInputs.Contains(type);
        if (!toPortId.IsEmpty())
        {
            auto target = to->FindInput(toPortId);
            if (!target || target->portType != type)
                return 0;
        }

        String before = SerializeGraph();
        Vector<Pair<String, String>> pairs;
        Vector<String> rest = ports;
        if (!toPortId.IsEmpty())
        {
            pairs.Add({ rest[0], toPortId });
            rest.RemoveAt(0);
        }

        if (addable)
        {
            auto customs = to->GetCustomInputs();
            for (auto& id : rest)
            {
                PipelinePort port(PipelineNode::GenerateId(), "", type, true);
                customs.Add(port);
                pairs.Add({ id, port.id });
            }
            to->SetCustomInputs(customs);
            PipelineNodeRegistry::SyncNodeWithSchema(to);
            for (int i = toPortId.IsEmpty() ? 0 : 1; i < pairs.Count(); i++)
                PipelineComposerLayout::PlaceNewLayer(*to, pairs[i].second, type, place);
        }
        else
        {
            Vector<String> free;
            bool after = toPortId.IsEmpty();
            for (auto& input : to->inputs)
            {
                if (input.id == toPortId) { after = true; continue; }
                if (after && input.portType == type && !graph->FindEdgeToPort(toNodeId, input.id))
                    free.Add(input.id);
            }
            for (int i = 0; i < rest.Count() && i < free.Count(); i++)
                pairs.Add({ rest[i], free[i] });
        }

        if (pairs.IsEmpty())
            return 0;

        for (auto& pair : pairs)
        {
            if (auto existing = graph->FindEdgeToPort(toNodeId, pair.second))
                graph->RemoveEdge(existing->id);

            auto edge = mmake<PipelineEdge>();
            edge->id = PipelineNode::GenerateId();
            edge->fromNodeId = fromNodeId;
            edge->fromPortId = pair.first;
            edge->toNodeId = toNodeId;
            edge->toPortId = pair.second;
            graph->edges.Add(edge);
        }

        if (auto widget = GetNodeWidget(toNodeId))
            RefreshNodeWidget(widget);
        RecordAction("Connect", before, SerializeGraph());
        if (schema && schema->instant)
            ScheduleAutoApply(toNodeId);

        return pairs.Count();
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
                else if (!mPendingEdge.ports.IsEmpty())
                    ConnectOutputs(mPendingEdge.nodeId, mPendingEdge.ports, widget->GetNode()->id, port->port.id);
                else
                    ConnectPorts(mPendingEdge.nodeId, mPendingEdge.portId, widget->GetNode()->id, port->port.id);
                return;
            }

            if (!mPendingEdge.fromInput && widget->IsAddInputAt(p))
            {
                auto schema = widget->GetSchema();
                if (!schema || !schema->addableInputs.Contains(mPendingEdge.type))
                    return;

                if (!mPendingEdge.ports.IsEmpty())
                {
                    auto place = widget->GetBody() ? widget->GetBody()->GetAddInputPlace(p) : PipelineLayerPlace::Front;
                    ConnectOutputs(mPendingEdge.nodeId, mPendingEdge.ports, widget->GetNode()->id, "", place);
                    return;
                }

                String before = SerializeGraph();
                auto customs = widget->GetNode()->GetCustomInputs();
                PipelinePort newPort(PipelineNode::GenerateId(), "", mPendingEdge.type, true);
                customs.Add(newPort);
                widget->GetNode()->SetCustomInputs(customs);
                PipelineNodeRegistry::SyncNodeWithSchema(widget->GetNode());
                auto place = widget->GetBody() ? widget->GetBody()->GetAddInputPlace(p) : PipelineLayerPlace::Front;
                PipelineComposerLayout::PlaceNewLayer(*widget->GetNode(), newPort.id, newPort.portType, place);
                RefreshNodeWidget(widget);
                // The input, its place among the layers and the link are one undo step
                if (!ConnectPorts(mPendingEdge.nodeId, mPendingEdge.portId, widget->GetNode()->id, newPort.id, before, "Add input"))
                    RecordAction("Add input", before, SerializeGraph());
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

        // A new pair node gets room for its two panes; its height follows the content
        node->size = PipelinePairLayout::NewNodeSize(type, node->size);

        node->position = SnapToGrid(node->position);
        graph->nodes.Add(node);

        AddCard(node);
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
            node = SnapToGrid(node);
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

    Ref<PipelineNode> PipelineEditor::AddNodeBeside(const String& besideNodeId, const String& type,
                                                    const Function<void(PipelineNode&)>& setup, const String& undoName)
    {
        auto graph = GetGraph();
        auto beside = graph ? graph->FindNode(besideNodeId) : nullptr;
        if (!beside)
            return nullptr;

        PushEditorScopeOnStack scope;
        String before = SerializeGraph();
        auto node = PipelineNodeRegistry::CreateNode(type, beside->position);
        if (!node)
            return nullptr;

        node->size = PipelinePairLayout::NewNodeSize(type, node->size);
        if (setup)
            setup(*node);
        graph->nodes.Add(node);
        auto card = AddCard(node);

        // Boxes in node space (y down); a card not built yet counts as an average one
        auto boxOf = [&](const Ref<PipelineNode>& n)
        {
            auto widget = GetNodeWidget(n->id);
            Vec2F size = widget ? widget->GetCardSize() : Vec2F(n->size.x > 0.0f ? n->size.x : 280.0f, 320.0f);
            return RectF(n->position.x, n->position.y, n->position.x + size.x, n->position.y + size.y);
        };
        const float gap = 40.0f;
        RectF me = boxOf(beside), mine = boxOf(node);
        float x = me.right + 60.0f, y = me.bottom < me.top ? me.bottom : me.top;
        float w = Math::Abs(mine.right - mine.left), h = Math::Abs(mine.top - mine.bottom);
        for (int i = 0; i < 40; i++)
        {
            float lowest = -1e9f;
            for (auto& other : graph->nodes)
            {
                if (other == node)
                    continue;

                RectF b = boxOf(other);
                float bx = Math::Min(b.left, b.right), bw = Math::Abs(b.right - b.left);
                float by = Math::Min(b.top, b.bottom), bh = Math::Abs(b.top - b.bottom);
                if (x < bx + bw + gap && x + w + gap > bx && y < by + bh + gap && y + h + gap > by)
                    lowest = Math::Max(lowest, by + bh);
            }
            if (lowest < -1e8f)
                break;

            y = lowest + gap;
        }

        node->position = SnapToGrid(Vec2F(x, y));
        if (card)
            card->UpdateFromNode();
        RecalculateViewArea();
        mNeedRedraw = true;
        RecordAction(undoName, before, SerializeGraph());
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

        // The node may have got an input for the link
        if (auto widget = GetNodeWidget(node->id))
            RefreshNodeWidget(widget);
        RecalculateViewArea();
        RecordAction("Add node", before, SerializeGraph());
        if (!toNodeId.IsEmpty() && !mPendingEdge.fromInput && mPendingEdge.ports.Count() > 1)
        {
            // Shift-dragged outputs: the new node gets them all
            Vector<String> ports = mPendingEdge.ports;
            ports.Remove(fromPortId);
            ports.Insert(fromPortId, 0);
            ConnectOutputs(fromNodeId, ports, toNodeId, toPortId);
        }
        else if (!toNodeId.IsEmpty())
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
        DeselectAll();
        for (auto& widget : selected)
        {
            graph->RemoveNode(widget->GetNode()->id);
            RemoveCard(widget->GetNode()->id);
        }

        RefreshCardsWithChangedInputs();
        RecalculateViewArea();
        mNeedRedraw = true;
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

        // The pipeline format itself: a copy keeps every member of its nodes and edges
        Clipboard::SetText("o2pipeline:" + clip.ToJsonString());
    }

    void PipelineEditor::Paste(const Vec2F& canvasPos, bool useCursor)
    {
        auto graph = GetGraph();
        if (!graph)
            return;

        String text = (String)Clipboard::GetText();
        if (!text.StartsWith("o2pipeline:"))
            return;

        PipelineGraph clip;
        if (!clip.LoadFromJsonString(text.SubStr(11)) || clip.nodes.IsEmpty())
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
            node->RemapConfigPortIds(idMap);
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

        for (auto& node : clip.nodes)
            AddCard(node);

        RefreshCardsWithChangedInputs();
        RecalculateViewArea();
        mNeedRedraw = true;
        RecordAction("Paste", before, SerializeGraph());
        SelectNodes(newIds);
    }

    void PipelineEditor::DuplicateSelection()
    {
        CopySelection();
        Paste(Vec2F(), true);
    }
}
