#include "o2Editor/stdafx.h"
#include "PipelineEditor.h"

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Assets/Types/ImageAsset.h"
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
    void PipelineEditor::OnCursorPressed(const Input::Cursor& cursor)
    {
        Focus();
        Vec2F p = cursor.position;

        for (int i = mNodeWidgets.Count() - 1; i >= 0; i--)
        {
            auto& widget = mNodeWidgets[i];
            if (auto port = widget->FindPortAt(p))
            {
                BeginEdgeDrag(widget, *port);
                return;
            }
        }

        if (auto edge = FindEdgeAt(p))
        {
            DeselectAll();
            mSelectedEdgeId = edge->id;
            mSelectedPointIndex = -1;
            RebuildBendHandles();
            mNeedRedraw = true;
            return;
        }

        if (!mSelectedEdgeId.IsEmpty())
        {
            mSelectedEdgeId = "";
            mSelectedPointIndex = -1;
            RebuildBendHandles();
            mNeedRedraw = true;
        }

        mSelecting = true;
        mSelectingPressedPoint = p;
        BeginPreSelect();
    }

    void PipelineEditor::OnCursorReleased(const Input::Cursor& cursor)
    {
        Vec2F p = cursor.position;

        if (mPendingEdge.active)
        {
            FinishEdgeDrag(p);
            return;
        }

        if (mSelecting)
        {
            mSelecting = false;
            EndPreSelect();
        }
    }

    void PipelineEditor::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (mPendingEdge.active)
        {
            mPendingEdge.cursor = cursor.position;
            mNeedRedraw = true;
            return;
        }

        if (!mSelecting)
            return;

        Vector<Ref<DragHandle>> preSelected;
        RectF selectionRect = RectF(mSelectingPressedPoint, ScreenToLocalPoint(o2Input.cursorPos));
        for (auto& widget : mNodeWidgets)
        {
            if (selectionRect.IsIntersects(widget->GetCardRect()))
                preSelected.Add(widget->dragHandle);
        }

        UpdatePreSelect(preSelected);
    }

    void PipelineEditor::OnCursorDblClicked(const Input::Cursor& cursor)
    {
        Vec2F p = cursor.position;
        int segment = 0;
        if (auto edge = FindEdgeAt(p, &segment))
        {
            String before = SerializeGraph();
            int index = Math::Clamp(segment, 0, edge->points.Count());
            edge->points.Insert(CanvasToNode(p), index);
            mSelectedEdgeId = edge->id;
            mSelectedPointIndex = index;
            RebuildBendHandles();
            RecordAction("Add link point", before, SerializeGraph());
            mNeedRedraw = true;
        }
    }

    void PipelineEditor::OnCursorRightMouseReleased(const Input::Cursor& cursor)
    {
        if (!mViewCameraMoved)
        {
            Vec2F p = cursor.position;
            mContextMenuPos = p;
            if (auto edge = FindEdgeAt(p))
            {
                mContextEdgeId = edge->id;
                mSelectedEdgeId = edge->id;
                RebuildBendHandles();
                mEdgeContextMenu->RemoveAllItems();
                if (!edge->points.IsEmpty())
                {
                    mEdgeContextMenu->AddItem("Straighten link", [this]()
                    {
                        if (auto g = GetGraph())
                        {
                            if (auto e = g->FindEdge(mContextEdgeId))
                            {
                                String before = SerializeGraph();
                                e->points.Clear();
                                RebuildBendHandles();
                                RecordAction("Straighten link", before, SerializeGraph());
                            }
                        }
                    });
                }
                mEdgeContextMenu->AddItem("Delete link", [this]()
                {
                    if (auto g = GetGraph())
                    {
                        String before = SerializeGraph();
                        g->RemoveEdge(mContextEdgeId);
                        mSelectedEdgeId = "";
                        RebuildBendHandles();
                        RecordAction("Delete link", before, SerializeGraph());
                        RefreshFreshness();
                    }
                }, AssetRef<ImageAsset>(), ShortcutKeys({ VK_DELETE }));
                mEdgeContextMenu->Show();
            }
            else
            {
                mContextMenu->RemoveAllItems();
                FillAddNodeMenu(mContextMenu, nullptr);
                mContextMenu->AddItem("---");
                mContextMenu->AddItem("Paste", [this]() { Paste(mContextMenuPos, false); }, AssetRef<ImageAsset>(), ShortcutKeys({ VK_V, VK_CTRL_CMD }));
                mContextMenu->AddItem("Select all", [this]() { SelectAllNodes(); }, AssetRef<ImageAsset>(), ShortcutKeys({ VK_A, VK_CTRL_CMD }));
                if (!mSelectedHandles.IsEmpty())
                {
                    mContextMenu->AddItem("Copy selection", [this]() { CopySelection(); }, AssetRef<ImageAsset>(), ShortcutKeys({ VK_C, VK_CTRL_CMD }));
                    mContextMenu->AddItem("Delete selection", [this]() { DeleteSelection(); }, AssetRef<ImageAsset>(), ShortcutKeys({ VK_DELETE }));
                }
                mContextMenu->AddItem("---");
                mContextMenu->AddItem("Fit view", [this]() { FitView(); });
                mContextMenu->Show();
            }
        }

        FrameScrollView::OnCursorRightMouseReleased(cursor);
    }

    void PipelineEditor::OnKeyPressed(const Input::Key& key)
    {
        if (o2UI.GetFocusedWidget().Get() != this)
            return;

        bool ctrl = o2Input.IsKeyDown(VK_CTRL_CMD);
        if (key.keyCode == VK_ESCAPE)
        {
            if (mPendingEdge.active)
            {
                mPendingEdge.active = false;
                mNeedRedraw = true;
            }
            else
                DeselectAll();
        }
        else if (key.keyCode == VK_DELETE || key.keyCode == VK_BACK)
            DeleteSelection();
        else if (ctrl && key.keyCode == VK_C)
            CopySelection();
        else if (ctrl && key.keyCode == VK_V)
            Paste(Vec2F(), true);
        else if (ctrl && key.keyCode == VK_D)
            DuplicateSelection();
        else if (ctrl && key.keyCode == VK_A)
            SelectAllNodes();
    }

    void PipelineEditor::InitializeContextMenus()
    {
        PushEditorScopeOnStack scope;
        mContextMenu = o2UI.CreateWidget<ContextMenu>();
        mNodeContextMenu = o2UI.CreateWidget<ContextMenu>();
        mEdgeContextMenu = o2UI.CreateWidget<ContextMenu>();
        mPopupMenu = o2UI.CreateWidget<ContextMenu>();

        AddChild(mContextMenu);
        AddChild(mNodeContextMenu);
        AddChild(mEdgeContextMenu);
        AddChild(mPopupMenu);

        onFocused = [&]() { mContextMenu->SetItemsMaxPriority(); };
    }

    void PipelineEditor::FillAddNodeMenu(const Ref<ContextMenu>& menu, const PendingEdge* filter)
    {
        static const Vector<Pair<PipelineNodeCategory, String>> categories = {
            { PipelineNodeCategory::Source, "Source" },
            { PipelineNodeCategory::Transform, "Transform" },
            { PipelineNodeCategory::AI, "AI" },
            { PipelineNodeCategory::Output, "Output" },
        };

        for (auto& category : categories)
        {
            for (auto schema : PipelineNodeRegistry::AllSchemas())
            {
                if (schema->category != category.first)
                    continue;

                if (filter)
                {
                    bool ok = filter->fromInput ? schema->ProducesOutputType(filter->type) : schema->AcceptsInputType(filter->type);
                    if (!ok)
                        continue;
                }

                String type = schema->type;
                String path = (filter ? String() : "Add " + category.second + "/") + schema->label;
                AssetRef<ImageAsset> icon(PipelineNodeWidget::IconForType(type));
                menu->AddItem(path, [this, type, filter = filter != nullptr]()
                {
                    if (filter)
                        CreateNodeFromPendingEdge(type, mContextMenuPos);
                    else
                    {
                        String before = SerializeGraph();
                        auto node = CreateNodeAt(type, mContextMenuPos);
                        if (node)
                        {
                            RecordAction("Add node", before, SerializeGraph());
                            SelectNodes({ node->id });
                        }
                    }
                }, icon);
            }
        }
    }

    void PipelineEditor::ShowPopupMenu(const Vector<Pair<String, Function<void()>>>& items)
    {
        mPopupMenu->RemoveAllItems();
        for (auto& item : items)
            mPopupMenu->AddItem(item.first, item.second);

        mPopupMenu->Show();
    }

    void PipelineEditor::OpenNodeContextMenu(const Ref<PipelineNodeWidget>& node)
    {
        if (!node->IsSelected())
        {
            DeselectAll();
            node->dragHandle->SetSelected(true);
        }

        mContextNode = node;
        mNodeContextMenu->RemoveAllItems();
        mNodeContextMenu->AddItem("Duplicate", [this]() { DuplicateSelection(); }, AssetRef<ImageAsset>(), ShortcutKeys({ VK_D, VK_CTRL_CMD }));
        mNodeContextMenu->AddItem("Copy", [this]() { CopySelection(); }, AssetRef<ImageAsset>(), ShortcutKeys({ VK_C, VK_CTRL_CMD }));
        mNodeContextMenu->AddItem("---");
        if (mContextNode && mContextNode->GetRuntime().output.IsValid())
            mNodeContextMenu->AddItem("Clear result", [this]() { if (mContextNode) ClearNodeResult(mContextNode->GetNode()->id); });
        mNodeContextMenu->AddItem("Delete", [this]() { DeleteSelection(); }, AssetRef<ImageAsset>(), ShortcutKeys({ VK_DELETE }));
        mNodeContextMenu->Show();
    }

    void PipelineEditor::ShowNodeError(const Ref<PipelineNodeWidget>& node)
    {
        auto& runtime = node->GetRuntime();
        String title = (node->GetSchema() ? node->GetSchema()->label : node->GetNode()->nodeType) + " failed";
        o2Debug.LogError(title + ": " + runtime.error);
        if (onLog)
            onLog(title + ": " + runtime.error);

        YesNoCancelDlg::ShowYesNo(title + "\n\n" + runtime.error, []() {}, []() {});
    }

    void PipelineEditor::OnSelectionChanged()
    {
        for (auto& widget : mNodeWidgets)
            widget->SetSelected(mSelectedHandles.Contains(widget->dragHandle));
        mNeedRedraw = true;
    }

    void PipelineEditor::DeselectAll()
    {
        SelectableDragHandlesGroup::DeselectAll();
        for (auto& widget : mNodeWidgets)
            widget->SetSelected(false);
        mNeedRedraw = true;
    }

    Vector<Ref<PipelineNodeWidget>> PipelineEditor::GetSelectedNodes() const
    {
        Vector<Ref<PipelineNodeWidget>> res;
        for (auto& widget : mNodeWidgets)
        {
            if (widget->IsSelected())
                res.Add(widget);
        }
        return res;
    }

    void PipelineEditor::SelectNodes(const Vector<String>& ids)
    {
        DeselectAll();
        for (auto& id : ids)
        {
            Ref<PipelineNodeWidget> widget;
            if (mNodeWidgetsById.TryGetValue(id, widget))
                widget->dragHandle->SetSelected(true);
        }
    }

    void PipelineEditor::SelectAllNodes()
    {
        for (auto& widget : mNodeWidgets)
            widget->dragHandle->SetSelected(true);
    }
}
