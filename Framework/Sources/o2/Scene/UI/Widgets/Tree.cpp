#include "o2/stdafx.h"
#include "Tree.h"

#include "o2/Animation/Animate.h"
#include "o2/Animation/Tracks/AnimationFloatTrack.h"
#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Utils/Tasks/TaskManager.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    TreeNode::TreeNode(RefCounter* refCounter):
        Widget(refCounter)
    {
        onDraw = [&]() { CursorAreaEventsListener::OnDrawn(); };
    }

    TreeNode::TreeNode(RefCounter* refCounter, const TreeNode& other):
        Widget(refCounter, other)
    {
        onDraw = [&]() { CursorAreaEventsListener::OnDrawn(); };

        mExpandBtn = DynamicCast<Button>(GetChild("expandBtn"));
        if (mExpandBtn)
            mExpandBtn->onClick = [&]() { if (IsExpanded()) Collapse(); else Expand(); o2UI.FocusWidget(mOwnerTree.Lock()); };

        SetSelectionGroup(other.GetSelectionGroup());

        RetargetStatesAnimations();
        SetLayoutDirty();
    }

    TreeNode::~TreeNode()
    {}

    TreeNode& TreeNode::operator=(const TreeNode& other)
    {
        Widget::operator=(other);

        onDraw = [&]() { CursorAreaEventsListener::OnDrawn(); };

        mExpandBtn = DynamicCast<Button>(GetChild("expandBtn"));
        if (mExpandBtn)
            mExpandBtn->onClick = [&]() { if (IsExpanded()) Collapse(); else Expand(); o2UI.FocusWidget(mOwnerTree.Lock()); };

        SetSelectionGroup(other.GetSelectionGroup());

        RetargetStatesAnimations();
        SetLayoutDirty();

        return *this;
    }

    void TreeNode::SetExpanded(bool expanded, bool forcible /*= false*/)
    {
        if (expanded == IsExpanded())
            return;

        if (expanded) 
            mOwnerTree.Lock()->ExpandNode(mNodeDef.Lock());
        else          
            mOwnerTree.Lock()->CollapseNode(mNodeDef.Lock());

        SetState("expanded", expanded);
        mNodeDef.Lock()->isExpanded = expanded;
    }

    bool TreeNode::IsExpanded() const
    {
        return mNodeDef.Lock()->isExpanded;
    }

    void TreeNode::Expand(bool forcible /*= false*/)
    {
        SetExpanded(true, forcible);
    }

    void TreeNode::Collapse(bool forcible /*= false*/)
    {
        SetExpanded(false, forcible);
    }

    void* TreeNode::GetObject() const
    {
        return mNodeDef.Lock()->object;
    }

    bool TreeNode::IsUnderPoint(const Vec2F& point)
    {
        return Widget::IsUnderPoint(point);
    }

    void TreeNode::SetSelectedState(bool state)
    {
        if (!mSelectedState)
            mSelectedState = GetStateObject("selected");

        if (mSelectedState)
            mSelectedState->SetStateForcible(state);
    }

    void TreeNode::SetFocusedState(bool state)
    {
        if (mFocusedState)
            mFocusedState->SetStateForcible(state);
    }

    String TreeNode::GetCreateMenuGroup()
    {
        return "Tree";
    }

    void TreeNode::OnStateAdded(const Ref<WidgetState>& state)
    {
        Widget::OnStateAdded(state);

        if (state->name == "selected")
            mSelectedState = state;
    }

    void TreeNode::UpdateTreeLayout(float dt)
    {
        mOwnerTree.Lock()->mIsNeedUdateLayout = true;
    }

    void TreeNode::OnCursorDblClicked(const Input::Cursor& cursor)
    {
        if (mOwnerTree)
            mOwnerTree.Lock()->OnNodeDblClick(Ref(this));
    }

    void TreeNode::OnCursorEnter(const Input::Cursor& cursor)
    {
        if (mOwnerTree)
            mOwnerTree.Lock()->UpdateHover(Ref(this));
    }

    void TreeNode::OnCursorExit(const Input::Cursor& cursor)
    {
        if (mOwnerTree)
            mOwnerTree.Lock()->UpdateHover(nullptr);
    }

    void TreeNode::OnDragStart(const Input::Cursor& cursor)
    {
        mOwnerTree.Lock()->BeginDragging(Ref(this));
    }

    void TreeNode::OnDragged(const Input::Cursor& cursor, const Ref<DragDropArea>& area)
    {
        mOwnerTree.Lock()->UpdateDraggingGraphics();
    }

    void TreeNode::OnDragEnd(const Input::Cursor& cursor)
    {
        mOwnerTree.Lock()->mDragEnded = true;
    }

    void TreeNode::OnSelected()
    {
        mOwnerTree.Lock()->Focus();
    }

    void TreeNode::OnDeselected()
    {}

    Tree::Tree(RefCounter* refCounter):
        ScrollArea(refCounter)
    {
        mNodeWidgetSample = mmake<TreeNode>();
        mNodeWidgetSample->layout->minHeight = 20;
        mNodeWidgetSample->AddLayer("caption", nullptr);

        mFakeDragNode = mmake<TreeNode>();
        mHoverDrawable = mmake<Sprite>();
        mHighlightSprite = mmake<Sprite>();

        mHighlightAnim->SetTarget(mHighlightSprite.Get());
    }

    Tree::Tree(RefCounter* refCounter, const Tree& other):
        ScrollArea(refCounter, other)
    {
        mRearrangeType = other.mRearrangeType;
        mMultiSelectAvailable = other.mMultiSelectAvailable;
        mNodeWidgetSample = other.mNodeWidgetSample->CloneAsRef<TreeNode>();
        mFakeDragNode = other.mNodeWidgetSample->CloneAsRef<TreeNode>();
        mHoverDrawable = other.mHoverDrawable->CloneAsRef<Sprite>();
        mHighlightSprite = other.mHighlightSprite->CloneAsRef<Sprite>();

        if (other.mZebraBackLine)
            mZebraBackLine = other.mZebraBackLine->CloneAsRef<Sprite>();

        if (other.mHighlighClip)
            mHighlighClip = other.mHighlighClip->CloneAsRef<AnimationClip>();

        mHighlightAnim->SetTarget(mHighlightSprite.Get());
        mHighlightAnim->SetClip(mHighlighClip);

        mHoverLayout = other.mHoverLayout;
        mHighlightLayout = other.mHighlightLayout;

        RetargetStatesAnimations();
        SetLayoutDirty();

        const int widgetsBufferInitialSize = 20;
        Vector<Ref<TreeNode>> widgets;
        for (int i = 0; i < widgetsBufferInitialSize; i++)
            widgets.Add(CreateTreeNodeWidget());

        mNodeWidgetsBuf.Add(widgets);
    }

    Tree::Tree(const Tree& other):
        Tree(nullptr, other)
    {}

    Tree::~Tree()
    {}

    Tree& Tree::operator=(const Tree& other)
    {
        ScrollArea::operator=(other);

        mRearrangeType = other.mRearrangeType;
        mMultiSelectAvailable = other.mMultiSelectAvailable;
        mNodeWidgetSample = other.mNodeWidgetSample->CloneAsRef<TreeNode>();
        mFakeDragNode = other.mNodeWidgetSample->CloneAsRef<TreeNode>();
        mHoverDrawable = other.mHoverDrawable->CloneAsRef<Sprite>();
        mHighlightSprite = other.mHighlightSprite->CloneAsRef<Sprite>();

        mHighlighClip = other.mHighlighClip->CloneAsRef<AnimationClip>();
        mHighlightAnim->SetTarget(mHighlightSprite.Get());
        mHighlightAnim->SetClip(mHighlighClip);

        mHoverLayout = other.mHoverLayout;
        mHighlightLayout = other.mHighlightLayout;

        RetargetStatesAnimations();
        SetLayoutDirty();

        return *this;
    }

    void Tree::Draw()
    {
        PROFILE_SAMPLE_FUNC();

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        if (mIsDraggingNodes)
            o2UI.DrawWidgetAtTop(mFakeDragNode);

        for (auto& layer : mDrawingLayers)
            layer->Draw();

        IDrawable::OnDrawn();

        o2Render.EnableScissorTest(mAbsoluteClipArea);

        if (mZebraBackLine)
            DrawZebraBack();

        if (mExpandingNodeState == ExpandState::None)
        {
            for (auto& node : mVisibleNodes)
                node->widget->Draw();
        }
        else
        {
            float clipTop = mAbsoluteViewArea.top - mExpandingNodePosition + mScrollPos.y;
            float clipBottom = mAbsoluteViewArea.top - (mExpandingNodePosition + mExpandingNodeCurrHeight) + mScrollPos.y;
            int clipTopIdx = Math::Max(mMinVisibleNodeIdx, mExpandingNodeIdx + 1);
            int clipBottomIdx = Math::Min(mMaxVisibleNodeIdx, mExpandingNodeIdx + mExpandingNodeChildsCount);

            o2Render.EnableScissorTest(RectF(mAbsoluteClipArea.left, clipTop, mAbsoluteClipArea.right, clipBottom));

            for (int i = clipTopIdx; i <= clipBottomIdx; i++)
            {
                auto node = mAllNodes[i];
                if (node->widget)
                    node->widget->Draw();
            }

            o2Render.DisableScissorTest();

            for (int i = mMinVisibleNodeIdx; i <= mMaxVisibleNodeIdx; i++)
            {
                auto node = mAllNodes[i];
                if (node->widget && (i < clipTopIdx || i > clipBottomIdx))
                    node->widget->Draw();
            }
        }

        mHoverDrawable->Draw();
        mHighlightSprite->Draw();

        o2Render.DisableScissorTest();

        ScrollArea::OnDrawn();

        for (auto& layer : mTopDrawingLayers)
            layer->Draw();

        if (mOwnHorScrollBar)
            mHorScrollBar->Draw();

        if (mOwnVerScrollBar)
            mVerScrollBar->Draw();

        DrawDebugFrame();
    }

    void Tree::Update(float dt)
    {
        PROFILE_SAMPLE_FUNC();

        UpdateHighlighting(dt);
        UpdateNodeExpanding(dt);

        if (mDragEnded)
            EndDragging();

        if (mIsDraggingNodes)
            UpdateDraggingInsertionAnim(dt);

        if (mIsNeedUpdateView)
            UpdateNodesStructure();

        if (mIsNeedUdateLayout)
            SetLayoutDirty();

        if (mIsNeedUpdateVisibleNodes)
            UpdateVisibleNodes();

        ScrollArea::Update(dt);

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        const float rectLerpCoef = 10.0f;
        if (mCurrentHoverRect != mTargetHoverRect)
        {
            mCurrentHoverRect = Math::Lerp(mCurrentHoverRect, mTargetHoverRect, dt*rectLerpCoef);
            mHoverDrawable->SetRect(mCurrentHoverRect);

            float alpha = mHoverDrawable->GetTransparency();
            mHoverDrawable->SetTransparency(alpha);
        }

        UpdatePressedNodeExpand(dt);

        mPressedTime += dt;
    }

    void Tree::UpdateHighlighting(float dt)
    {
        if (mHighlightAnim->IsPlaying())
        {
            if (mHighlightObject && !mHighlighNode)
                mHighlighNode = mAllNodes.FindOrDefault([=](auto& x) { return x->object == mHighlightObject; });

            if (mHighlighNode && mHighlighNode.Lock()->widget)
            {
                mHighlightSprite->SetScale(Vec2F(1.0f, 1.0f));
                mHighlightSprite->SetRect(mHighlightLayout.Calculate(mHighlighNode.Lock()->widget->layout->worldRect));
            }

            mHighlightAnim->Update(dt);

            if (!mHighlightAnim->IsPlaying())
            {
                mHighlighNode = nullptr;
                mHighlightObject = nullptr;
            }
        }
        else mHighlightSprite->transparency = 0.0f;
    }

    void Tree::UpdateChildren(float dt)
    {
        ScrollArea::UpdateChildren(dt);

        if (mLayoutUpdated && mResEnabledInHierarchy)
            UpdateVisibleNodes();
    }

    void Tree::UpdateChildrenTransforms()
    {
        ScrollArea::UpdateChildrenTransforms();
        UpdateVisibleNodes();
    }

    void* Tree::GetObjectParent(void* object)
    {
        return getObjectParentDelegate(object);
    }

    Vector<void*> Tree::GetObjectChilds(void* object)
    {
        return getObjectChildrenDelegate(object);
    }

    String Tree::GetObjectDebug(void* object)
    {
        return getDebugForObject(object);
    }

    void Tree::FillNodeDataByObject(const Ref<TreeNode>& nodeWidget, void* object)
    {
        fillNodeDataByObjectDelegate(nodeWidget, object);
    }

    void Tree::FreeNodeData(const Ref<TreeNode>& nodeWidget, void* object)
    {
        freeNodeDataDelegate(nodeWidget, object);
    }

    void Tree::OnNodeDblClick(const Ref<TreeNode>& nodeWidget)
    {
        onNodeDoubleClicked(nodeWidget);
    }

    void Tree::OnNodeRBClick(const Ref<TreeNode>& nodeWidget)
    {
        onNodeRightButtonClicked(nodeWidget);
    }

    void Tree::OnNodesSelectionChanged(Vector<void*> objects)
    {
        onObjectsSelectionChanged(objects);
    }

    void Tree::OnDraggedObjects(Vector<void*> objects, void* newParent, void* prevObject)
    {
        onDraggedObjects(objects, newParent, prevObject);
    }

    Vector<Ref<SelectableDragableObject>> Tree::GetSelectedDragObjects() const
    {
        return {};
    }

    Vector<Ref<SelectableDragableObject>> Tree::GetAllObjects() const
    {
        return {};
    }

    void Tree::Select(const Ref<SelectableDragableObject>& object, bool sendOnSelectionChanged)
    {
        bool someSelected = false;
        auto uiNode = DynamicCast<TreeNode>(object);

        if (o2Input.IsKeyDown(VK_SHIFT) && mSelectedNodes.Count() > 0)
        {
            someSelected = CheckMultipleSelection(o2Input.GetCursorPos());
        }
        else if (!o2Input.IsKeyDown(VK_CONTROL))
        {
            for (auto& sel : mSelectedNodes)
                sel->SetSelected(false);

            mSelectedNodes.Clear();
            mSelectedObjects.Clear();

            someSelected = true;
        }

        if (!mSelectedNodes.Contains([=](auto& x) { return x->widget == uiNode; }))
        {
            someSelected = true;

            uiNode->mIsSelected = true;

            auto node = mAllNodes.FindOrDefault([=](auto& x) { return x->widget == uiNode; });
            node->SetSelected(true);
            mSelectedNodes.Add(node);
            mSelectedObjects.Add(node->object);
        }

        if (someSelected && sendOnSelectionChanged)
            OnSelectionChanged();
    }

    void Tree::Select(const Ref<SelectableDragableObject>& object)
    {
        Select(object, true);
    }

    void Tree::Deselect(const Ref<SelectableDragableObject>& object)
    {
        auto uiNode = DynamicCast<TreeNode>(object);
        uiNode->mIsSelected = false;

        int idx = mSelectedNodes.IndexOf([&](auto x) { return x->widget == uiNode; });

        if (idx < 0)
            return;

        mSelectedNodes[idx]->SetSelected(false);
        mSelectedObjects.Remove(mSelectedNodes[idx]->object);
        mSelectedNodes.RemoveAt(idx);

        OnSelectionChanged();
    }

    void Tree::AddSelectableObject(const Ref<SelectableDragableObject>& object)
    {}

    void Tree::RemoveSelectableObject(SelectableDragableObject* object)
    {}

    void Tree::OnSelectableObjectCursorReleased(const Ref<SelectableDragableObject>& object, const Input::Cursor& cursor)
    {
        if (!o2Input.IsKeyDown(VK_SHIFT) && !o2Input.IsKeyDown(VK_CONTROL))
        {
            for (auto& sel : mSelectedNodes)
                sel->SetSelected(false);

            mSelectedNodes.Clear();
            mSelectedObjects.Clear();
        }

        if (!o2Input.IsKeyDown(VK_SHIFT))
            object->SetSelected(!object->IsSelected());
    }

    void Tree::OnSelectableObjectBeganDragging(const Ref<SelectableDragableObject>& object)
    {
        mBeforeDragSelectedItems = mSelectedObjects;

        if (!object->IsSelected())
            Select(object, false);
    }

    bool Tree::CheckMultipleSelection(const Vec2F& point)
    {
        if (mSelectedNodes.IsEmpty())
            return false;

        Vec2F currClickPos = mAbsoluteViewArea.LeftTop() - point + mScrollPos;

        float selectionUp = Math::Max<float>(currClickPos.y, mLastClickPos.y);
        float selectionDown = Math::Min<float>(currClickPos.y, mLastClickPos.y);

        bool someSelected = false;

        int idx = 0;
        float nodeHeight = mNodeWidgetSample->layout->GetMinHeight();
        for (auto& node : mAllNodes)
        {
            if (mSelectedNodes.Contains(node))
            {
                idx++;
                continue;
            }

            float top = (float)idx*nodeHeight;
            float bottom = top + nodeHeight;

            if ((top > selectionDown && top < selectionUp) || (bottom > selectionDown && bottom < selectionUp))
            {
                node->SetSelected(true);
                mSelectedNodes.Add(node);
                mSelectedObjects.Add(node->object);

                someSelected = true;
            }

            idx++;
        }

        return someSelected;
    }

    void Tree::UpdatePressedNodeExpand(float dt)
    {
        if (o2Input.IsCursorDown() && mIsDraggingNodes)
        {
            if (mRearrangeType != RearrangeType::Disabled)
            {
                auto underCursorItem = GetTreeNodeUnderPoint(o2Input.GetCursorPos());

                if (underCursorItem != mExpandNodeCandidate)
                {
                    mExpandNodeCandidate = underCursorItem;
                    mExpandInsertTime = mNodeExpandTime;
                }
            }

            if (mExpandInsertTime <= 0.0f && mExpandNodeCandidate)
            {
                mExpandNodeCandidate->Expand();

                if (mIsDraggingNodes)
                    mExpandNodeCandidate->mNodeDef.Lock()->inserting = false;

                mExpandNodeCandidate = nullptr;
            }
        }

        mExpandInsertTime -= dt;
    }

    void Tree::UpdateNodesView(bool immediately /*= true*/)
    {
        if (immediately)
        {
            UpdateNodesStructure();
            for (int i = 0; i < mAllNodes.Count(); i++)
            {
                if (mAllNodes[i]->widget)
                    UpdateNodeView(mAllNodes[i], mAllNodes[i]->widget, i);
            }
        }
        else 
            mIsNeedUpdateView = true;
    }

    Ref<TreeNode> Tree::GetNode(void* object)
    {
        auto fnd = mAllNodes.FindOrDefault([=](auto& x) { return x->object == object; });
        if (fnd)
            return fnd->widget;

        return nullptr;
    }

    void Tree::ExpandAll()
    {
        bool isAllExpanded = false;
        while (!isAllExpanded)
        {
            UpdateNodesView(true);
            UpdateVisibleNodes();
            isAllExpanded = true;

            auto nodes = mAllNodes;
            for (auto& child : nodes)
            {
                auto childNode = child->widget;
                if (childNode && !childNode->IsExpanded())
                {
                    childNode->Expand();
                    isAllExpanded = false;
                }
            }
        }
    }

    void Tree::CollapseAll()
    {
        for (auto& child : mAllNodes)
        {
            auto childNode = DynamicCast<TreeNode>(child);
            childNode->Collapse();
        }
    }

    Vector<void*> Tree::GetSelectedObjects() const
    {
        return mSelectedObjects;
    }

    void Tree::SetSelectedObjects(const Vector<void*>& objects)
    {
        for (auto& sel : mSelectedNodes)
            sel->SetSelected(false);

        mSelectedNodes.Clear();
        mSelectedObjects.Clear();

        for (auto& obj : objects)
        {
            auto node = mAllNodes.FindOrDefault([=](auto& x) { return x->object == obj; });

            if (!node)
                continue;

            node->SetSelected(true);
            mSelectedNodes.Add(node);
        }

        mSelectedObjects.Add(objects);

        OnSelectionChanged();
        SetLayoutDirty();
    }

    void Tree::SelectObject(void* object)
    {
        if (!mMultiSelectAvailable)
            DeselectAllObjects();

        if (mSelectedObjects.Contains(object))
        {
            o2Debug.Log("Already selected");
            return;
        }

        auto node = mAllNodes.FindOrDefault([=](auto& x) { return x->object == object; });
        if (!node)
            return;

        node->SetSelected(true);
        mSelectedNodes.Add(node);
        mSelectedObjects.Add(node->object);

        OnSelectionChanged();
        SetLayoutDirty();
    }

    void Tree::SelectAndHighlightObject(void* object)
    {
        DeselectAllObjects();
        UpdateNodesStructure();
        ScrollToAndHighlight(object);
        SelectObject(object);
    }

    void Tree::DeselectObject(void* object)
    {
        int idx = mSelectedNodes.IndexOf([&](auto x) { return x->object == object; });

        if (idx < 0)
            return;

        mSelectedNodes[idx]->SetSelected(false);
        mSelectedObjects.Remove(mSelectedNodes[idx]->object);
        mSelectedNodes.RemoveAt(idx);

        OnSelectionChanged();
        SetLayoutDirty();
    }

    void Tree::DeselectAllObjects()
    {
        for (auto& sel : mSelectedNodes)
            sel->SetSelected(false);

        mSelectedNodes.Clear();
        mSelectedObjects.Clear();
        OnSelectionChanged();
    }

    void Tree::ScrollTo(void* object)
    {
        if (!object)
        {
            ResetScroll();
            return;
        }

        ExpandParentObjects(object);

        int idx = mAllNodes.IndexOf([=](auto& x) { return x->object == object; });

        if (idx >= 0)
            SetScroll(Vec2F(mScrollPos.x, (float)idx*mNodeWidgetSample->layout->minHeight - layout->height*0.5f));
    }

    void Tree::ScrollToAndHighlight(void* object)
    {
        if (!object)
        {
            ResetScroll();
            return;
        }

        ExpandParentObjects(object);

        int idx = mAllNodes.IndexOf([=](auto& x) { return x->object == object; });

        if (idx >= 0)
        {
            float position = (float)idx*mNodeWidgetSample->layout->minHeight;
            float scroll = position - layout->height*0.5f;
            SetScroll(Vec2F(mScrollPos.x, scroll));

            mHighlighNode = mAllNodes[idx];
            mHighlightObject = object;
            mHighlightAnim->RewindAndPlay();
        }
    }

    void Tree::ExpandParentObjects(void* object)
    {
        Vector<void*> parentsStack;
        void* current = GetObjectParent(object);
        while (current)
        {
            parentsStack.Add(current);
            current = GetObjectParent(current);
        }

        for (int i = parentsStack.Count() - 1; i >= 0; i--)
        {
            auto node = mAllNodes.FindOrDefault([=](auto& x) { return x->object == parentsStack[i]; });

            if (!node)
            {
                o2Debug.LogWarning("UITree: failed to expand parents for node");
                return;
            }

            if (node->isExpanded)
                continue;

            if (node->widget)
                node->widget->SetExpanded(true);
            else
                ExpandNode(node);
        }

        UpdateNodeExpanding(mExpandNodeTime);
        SetLayoutDirty();
    }

    void Tree::OnObjectCreated(void* object, void* parent)
    {
        mIsNeedUpdateView = true;
    }

    void Tree::OnObjectRemoved(void* object)
    {
        mIsNeedUpdateView = true;
    }

    void Tree::OnObjectsChanged(const Vector<void*>& objects)
    {
        if (mIsNeedUpdateView)
            return;

        for (auto& object : objects)
        {
            auto nodeIdx = mVisibleNodes.IndexOf([=](auto& x) { return x->object == object; });
            if (nodeIdx < 0)
                continue;

            auto node = mVisibleNodes[nodeIdx];
            if (node->widget)
                UpdateNodeView(node, node->widget, -1);
        }
    }

    void Tree::UpdateNodesStructure()
    {
        mIsNeedUpdateView = false;

        mHighlighNode = nullptr;

        if (mExpandingNodeState != ExpandState::None)
        {
            UpdateNodeExpanding(mExpandNodeTime);

            for (auto& child : mChildWidgets)
                child->SetLayoutDirty();
        }

        Vector<void*> rootObjects = GetObjectChilds(nullptr);

        mVisibleWidgetsCache.Clear();
        for (auto& node : mVisibleNodes)
        {
            if (!node->widget)
                continue;

            VisibleWidgetDef cache;
            cache.object = node->object;
            cache.widget = node->widget;
            cache.position = mAllNodes.IndexOf(node);

            mVisibleWidgetsCache.Add(cache);
        }

        mNodesBuf.Add(mAllNodes);

        mAllNodes.Clear();
        mVisibleNodes.Clear();
        mChildren.Clear();
        mChildWidgets.Clear();
        mSelectedNodes.Clear();
        mMinVisibleNodeIdx = 0;
        mMaxVisibleNodeIdx = -1;

        int position = 0;
        for (auto& object : rootObjects)
        {
            if (mIsDraggingNodes && mSelectedObjects.Contains(object))
                continue;

            auto node = CreateNode(object, nullptr);
            mAllNodes.Insert(node, position++);
            position += InsertNodes(node, position);
        }

        SetLayoutDirty();
    }

    int Tree::InsertNodes(const Ref<Node>& parentNode, int position, Vector<Ref<Node>>* newNodes /*= nullptr*/)
    {
        int initialPosition = position;

        if (mExpandedObjects.Contains(parentNode->object))
        {
            auto childObjects = GetObjectChilds(parentNode->object);
            for (auto& child : childObjects)
            {
                if (mIsDraggingNodes && mSelectedObjects.Contains(child))
                    continue;

                if (parentNode->object == child)
                    continue;

                auto node = CreateNode(child, parentNode);

                mAllNodes.Insert(node, position++);

                if (newNodes)
                    newNodes->Add(node);

                position += InsertNodes(node, position, newNodes);
            }
        }

        return position - initialPosition;
    }

    void Tree::RemoveNodes(const Ref<Node>& parentNode)
    {
        int begin = mAllNodes.IndexOf(parentNode) + 1;
        int end = begin - 1 + parentNode->GetChildCount();

        mAllNodes.RemoveRange(begin, end);
    }

    Ref<Tree::Node> Tree::CreateNode(void* object, const Ref<Node>& parent)
    {
        auto node = mNodesBuf.IsEmpty() ? mmake<Node>() : mNodesBuf.PopBack();
        node->childs.Clear();

        node->parent = parent;
        node->object = object;
        node->widget = nullptr;
        node->isSelected = mSelectedObjects.Contains(object);
        node->isExpanded = mExpandedObjects.Contains(object);
        node->level = parent ? parent->level + 1 : 0;

        node->id = GetObjectDebug(object);

        if (parent)
            parent->childs.Add(node);

        if (node->isSelected)
            mSelectedNodes.Add(node);

        return node;
    }

    void Tree::OnFocused()
    {
        for (auto& node : mVisibleNodes)
        {
            if (node->widget)
                node->widget->SetState("focused", true);
        }

        onFocused();
    }

    void Tree::OnUnfocused()
    {
        for (auto& node : mVisibleNodes)
        {
            if (node->widget)
                node->widget->SetState("focused", false);
        }

        onUnfocused();
    }

    void Tree::DrawZebraBack()
    {
        float pos = -mScrollPos.y;
        float lineSize = mNodeWidgetSample->layout->GetMinHeight();
        while (pos < -lineSize)
            pos += lineSize*2.0f;

        RectF layoutRect = layout->GetWorldRect();
        while (pos < layoutRect.Height())
        {
            mZebraBackLine->SetRect(RectF(layoutRect.left, layoutRect.top - pos, layoutRect.right, layoutRect.top - pos - lineSize));
            mZebraBackLine->Draw();
            pos += lineSize*2.0f;
        }
    }

    void Tree::UpdateSelfTransform()
    {
        ScrollArea::UpdateSelfTransform();
        mIsNeedUdateLayout = false;
    }

    bool Tree::IsInputTransparent() const
    {
        return ScrollArea::IsInputTransparent();
    }

    void Tree::UpdateVisibleNodes()
    {
        UpdateSelfTransform();

        mIsNeedUpdateVisibleNodes = false;

        int lastMinVisible = mMinVisibleNodeIdx;
        int lastMaxVisible = mMaxVisibleNodeIdx;

        float topVisiblePosition = mScrollPos.y;
        float bottomVisiblePosition = mScrollPos.y + mAbsoluteViewArea.Height();

        mMinVisibleNodeIdx = GetNodeIndex(topVisiblePosition);
        mMaxVisibleNodeIdx = GetNodeIndex(bottomVisiblePosition);

        if (mAllNodes.IsEmpty())
            mMaxVisibleNodeIdx = -1;

        for (int i = lastMinVisible; i <= lastMaxVisible; i++)
        {
            if (i < mMinVisibleNodeIdx || i > mMaxVisibleNodeIdx)
            {
                if (i >= mAllNodes.Count())
                    break;

                auto node = mAllNodes[i];

                if (node->widget)
                {
                    FreeNodeData(node->widget, node->object);

                    mNodeWidgetsBuf.Add(node->widget);
                    mChildren.Remove(node->widget);
                    mChildWidgets.Remove(node->widget);
                    mChildrenInheritedDepth.Remove(node->widget);

                    node->widget->mParent = nullptr;
                    node->widget->mParentWidget = nullptr;
                    node->widget->mNodeDef = nullptr;
                    node->widget = nullptr;
                }
            }
        }

        float nodeHeight = mNodeWidgetSample->layout->minHeight;
        for (int i = mMinVisibleNodeIdx; i <= mMaxVisibleNodeIdx && i < mAllNodes.Count(); i++)
        {
            auto node = mAllNodes[i];

            if (i < lastMinVisible || i > lastMaxVisible)
                CreateVisibleNodeWidget(node, i);

            if (!node->widget)
            {
                float realPosition = (float)i*nodeHeight;
                if (realPosition > topVisiblePosition && realPosition < bottomVisiblePosition)
                    CreateVisibleNodeWidget(node, i);
            }
        }

        mVisibleNodes = mAllNodes.Take(mMinVisibleNodeIdx, mMaxVisibleNodeIdx + 1);

        for (auto& node : mVisibleNodes)
        {
            if (node->widget)
            {
                node->widget->UpdateSelfTransform();
                node->widget->UpdateChildrenTransforms();
                node->widget->CheckClipping(mAbsoluteClipArea);
            }
        }

        mNodeWidgetsBuf.Add(mVisibleWidgetsCache
                            .FindAll([](const VisibleWidgetDef& x) { return x.widget != nullptr; })
                            .Convert<Ref<TreeNode>>([&](const VisibleWidgetDef& x) { FreeNodeData(x.widget, x.object); return x.widget; }));

        mVisibleWidgetsCache.Clear();

        if (mIsDraggingNodes)
            OnDraggedAbove(Ref(this));
    }

    void Tree::CreateVisibleNodeWidget(const Ref<Node>& node, int i)
    {
        int cacheIdx = mVisibleWidgetsCache.IndexOf([=](const VisibleWidgetDef& x) {
            return x.object == node->object && x.position == i; });

        Ref<TreeNode> widget;

        if (cacheIdx < 0)
            widget = CreateTreeNodeWidget();
        else
        {
            widget = mVisibleWidgetsCache[cacheIdx].widget;
            mVisibleWidgetsCache[cacheIdx].widget = nullptr;
        }

        UpdateNodeView(node, widget, i);

        widget->mIsSelected = node->isSelected;
        widget->mNodeDef = node;
        node->widget = widget;
        widget->mParent = Ref(this);
        widget->mParentWidget = Ref(this);
        widget->mResEnabledInHierarchy = true;
        widget->mResEnabled = true;

        mChildren.Add(widget);
        mChildWidgets.Add(widget);
        mChildrenInheritedDepth.Add(widget);
    }

    void Tree::UpdateNodeView(const Ref<Node>& node, const Ref<TreeNode>& widget, int idx)
    {
        node->widget = widget;

        FillNodeDataByObject(widget, node->object);

        if (GetObjectChilds(node->object).IsEmpty())
        {
            if (widget->mExpandBtn)
                widget->mExpandBtn->Hide(true);
        }
        else
        {
            widget->SetStateForcible("expanded", node->isExpanded);
            if (widget->mExpandBtn)
                widget->mExpandBtn->Show(true);
        }

        widget->mIsSelected = node->isSelected;
        widget->SetSelectedState(node->isSelected);
        widget->SetFocusedState(mIsFocused);

        if (idx >= 0)
            UpdateNodeWidgetLayout(node, idx);
    }

    void Tree::UpdateNodeWidgetLayout(const Ref<Node>& node, int idx)
    {
        float nodeHeight = mNodeWidgetSample->layout->GetMinHeight();
        float dragModeOffset = 0.0f;

        if (mIsDraggingNodes)
            dragModeOffset = mExpandingNodeFunc.Evaluate(node->insertCoef)*nodeHeight;

        node->widget->layout->CopyFrom(WidgetLayout::HorStretch(VerAlign::Top, mChildrenOffset*(float)node->level, 0,
                                       nodeHeight, GetNodePosition(idx) + dragModeOffset));
    }

    int Tree::GetNodeIndex(float position) const
    {
        int res = 0;
        float nodeHeight = mNodeWidgetSample->layout->GetMinHeight();

        if (mExpandingNodeState != ExpandState::None && position > mExpandingNodeBottomPosition)
        {
            res = Math::Min(Math::FloorToInt((position + mExpandingNodeTargetHeight - mExpandingNodeCurrHeight)/nodeHeight),
                            Math::Max(0, mAllNodes.Count() - 1));
        }
        else res = Math::Min(Math::FloorToInt(position/nodeHeight), Math::Max(0, mAllNodes.Count() - 1));

        return res;
    }

    float Tree::GetNodePosition(int idx) const
    {
        float nodeHeight = mNodeWidgetSample->layout->GetMinHeight();

        float res = (float)idx*nodeHeight;

        if (mExpandingNodeState != ExpandState::None && idx > mExpandingNodeIdx + mExpandingNodeChildsCount)
            res -= mExpandingNodeTargetHeight - mExpandingNodeCurrHeight;

        return res;
    }

    void Tree::ExpandNode(const Ref<Node>& node)
    {
        int position = mAllNodes.IndexOf(node) + 1;

        if (mExpandingNodeState != ExpandState::None && mExpandingNodeIdx != position - 1)
            UpdateNodeExpanding(mExpandNodeTime);

        mExpandedObjects.Add(node->object);

        node->isExpanded = true;

        auto thisRef = Ref(this);

        if (mExpandingNodeState != ExpandState::None && mExpandingNodeIdx == position - 1)
            mExpandingNodeState = ExpandState::Expanding;
        else
        {
            Vector<Ref<Node>> newNodes;
            InsertNodes(node, position, &newNodes);

            float nodeHeight = mNodeWidgetSample->layout->GetMinHeight();
            float topViewBorder = mScrollPos.y;
            float bottomViewBorder = topViewBorder + mAbsoluteViewArea.Height();
            int idx = position;

            if (idx < mMinVisibleNodeIdx)
            {
                mMinVisibleNodeIdx += newNodes.Count();
                mMaxVisibleNodeIdx += newNodes.Count();
            }
            else if (idx <= mMaxVisibleNodeIdx)
            {
                for (auto& node : newNodes)
                {
                    float position = (float)idx*nodeHeight;
                    if (position > bottomViewBorder)
                        break;

                    const Ref<TreeNode>& nodeWidget = CreateTreeNodeWidget();

                    node->widget = nodeWidget;
                    nodeWidget->mNodeDef = node;
                    nodeWidget->mParent = nullptr;

                    UpdateNodeView(node, nodeWidget, idx);

                    nodeWidget->mParent = thisRef;
                    nodeWidget->mParentWidget = thisRef;

                    mVisibleNodes.Add(node);
                    mChildren.Add(nodeWidget);
                    mChildWidgets.Add(nodeWidget);
                    mChildrenInheritedDepth.Add(nodeWidget);

                    idx++;
                }
                mMaxVisibleNodeIdx += newNodes.Count();
            }

            StartExpandingAnimation(ExpandState::Expanding, node, newNodes.Count());
        }

        OnNodeExpanded(node->object);

        mIsNeedUdateLayout = true;
    }

    void Tree::CollapseNode(const Ref<Node>& node)
    {
        int idx = mAllNodes.IndexOf(node);

        if (mExpandingNodeState != ExpandState::None && mExpandingNodeIdx != idx)
            UpdateNodeExpanding(mExpandNodeTime);

        mExpandedObjects.Remove(node->object);

        node->isExpanded = false;

        int collapseNodesCount = node->GetChildCount();

        if (mExpandingNodeState != ExpandState::None && mExpandingNodeIdx == idx)
            mExpandingNodeState = ExpandState::Collaping;
        else
            StartExpandingAnimation(ExpandState::Collaping, node, collapseNodesCount);

        OnNodeCollapsed(node->object);

        mIsNeedUdateLayout = true;
    }

    void Tree::StartExpandingAnimation(ExpandState direction, const Ref<Node>& node, int childrenCount)
    {
        int idx = mAllNodes.IndexOf(node);

        float nodeHeight = mNodeWidgetSample->layout->GetMinHeight();

        mExpandingNodeIdx = idx;
        mExpandingNodePosition = mExpandingNodeIdx*nodeHeight + nodeHeight;
        mExpandingNodeTargetHeight = (float)childrenCount*nodeHeight;
        mExpandingNodeBottomPosition = direction == ExpandState::Expanding ? mExpandingNodePosition :
            mExpandingNodePosition + mExpandingNodeTargetHeight;
        mExpandingNodeChildsCount = childrenCount;
        mExpandingNodeCurrCoef = direction == ExpandState::Expanding ? 0.0f : 1.0f;
        mExpandingNodeCurrHeight = direction == ExpandState::Expanding ? 0.0f : mExpandingNodeTargetHeight;
        mExpandingNodeState = direction;
    }

    void Tree::UpdateNodeExpanding(float dt)
    {
        if (mExpandingNodeState == ExpandState::None)
            return;

        if (mExpandingNodeState == ExpandState::Expanding)
        {
            mExpandingNodeCurrCoef += dt/mExpandNodeTime;

            if (mExpandingNodeCurrCoef > 1.0f)
            {
                mExpandingNodeCurrCoef = 1.0f;
                mExpandingNodeState = ExpandState::None;
            }
        }
        else
        {
            mExpandingNodeCurrCoef -= dt/mExpandNodeTime;

            if (mExpandingNodeCurrCoef < 0.0f)
            {
                mExpandingNodeCurrCoef = 0.0f;
                mExpandingNodeState = ExpandState::None;

                mAllNodes[mExpandingNodeIdx]->childs.Clear();

                for (int i = mExpandingNodeIdx + 1; i <= mExpandingNodeIdx + mExpandingNodeChildsCount && i < mAllNodes.Count(); i++)
                {
                    auto node = mAllNodes[i];
                    if (node->widget)
                    {
                        FreeNodeData(node->widget, node->object);

                        mNodeWidgetsBuf.Add(node->widget);
                        mChildren.Remove(node->widget);
                        mChildWidgets.Remove(node->widget);
                        mChildrenInheritedDepth.Remove(node->widget);

                        node->widget->mParent = nullptr;
                        node->widget->mParentWidget = nullptr;
                        node->widget->mNodeDef = nullptr;
                        node->widget = nullptr;
                    }

                    mNodesBuf.Add(node);

                    if (node->isSelected)
                        mSelectedNodes.Remove(node);
                }

                mAllNodes.RemoveRange(mExpandingNodeIdx + 1, mExpandingNodeIdx + mExpandingNodeChildsCount + 1);
                mExpandingNodeChildsCount = 0;
            }
        }

        float lastExpandBottom = mExpandingNodeBottomPosition;
        mExpandingNodeCurrHeight = mExpandingNodeFunc.Evaluate(mExpandingNodeCurrCoef)*mExpandingNodeTargetHeight;
        mExpandingNodeBottomPosition = mExpandingNodePosition + mExpandingNodeCurrHeight;

        float offs = mExpandingNodeBottomPosition - lastExpandBottom;
        for (int i = Math::Max(mExpandingNodeIdx + mExpandingNodeChildsCount + 1, mMinVisibleNodeIdx); i <= mMaxVisibleNodeIdx && i < mAllNodes.Count(); i++)
        {
            auto node = mAllNodes[i];

            if (node->widget)
            {
                node->widget->GetLayoutData().offsetMin.y -= offs;
                node->widget->GetLayoutData().offsetMax.y -= offs;
            }
        }

        mIsNeedUdateLayout = true;
    }

    void Tree::CalculateScrollArea()
    {
        const float bottomAdditionalSpace = 100;

        Vec2F offset;
        InitializeScrollAreaRectCalculation(offset);

        float itemsHeight = (float)mAllNodes.Count()*mNodeWidgetSample->layout->minHeight + bottomAdditionalSpace;
        RecalculateScrollAreaRect(RectF(0, mAbsoluteViewArea.Height(), mAbsoluteViewArea.Width(), mAbsoluteViewArea.Height() - itemsHeight), Vec2F());
    }

    void Tree::MoveScrollPosition(const Vec2F& delta)
    {
        ScrollArea::MoveScrollPosition(delta);
        UpdateVisibleNodes();

        mTargetHoverRect += delta;
        mCurrentHoverRect += delta;
    }

    void Tree::OnCursorPressed(const Input::Cursor& cursor)
    {
        Focus();
    }

    void Tree::UpdateHover(const Ref<TreeNode>& itemUnderCursor)
    {
        bool hoverVisible = false;
        if (itemUnderCursor)
        {
            mTargetHoverRect = mHoverLayout.Calculate(itemUnderCursor->layout->worldRect);
            hoverVisible = true;
        }

        if (auto hoverState = state["hover"])
            hoverState->SetState(hoverVisible);
        else
            mHoverDrawable->SetEnabled(hoverVisible);
    }

    void Tree::OnCursorMoved(const Input::Cursor& cursor)
    {
        if (!mIsDraggingNodes)
            UpdateHover(GetTreeNodeUnderPoint(cursor.position));
    }

    void Tree::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (!mIsDraggingNodes)
        {
            float a = (float)mAllNodes.Count()*mNodeWidgetSample->layout->GetMinHeight();
            float b = mAbsoluteViewArea.top - cursor.position.y + mScrollPos.y;
            bool isCursorUnderNode = a > b;

            if (o2Input.IsKeyDown(VK_SHIFT))
            {
                if (CheckMultipleSelection(cursor.position))
                {
                    OnSelectionChanged();
                    SetLayoutDirty();
                }
            }
            else if (!o2Input.IsKeyDown(VK_CONTROL) && !isCursorUnderNode)
                DeselectAllObjects();

            mLastClickPos = mAbsoluteViewArea.LeftTop() - cursor.position + mScrollPos;
        }
    }

    void Tree::OnCursorRightMouseReleased(const Input::Cursor& cursor)
    {
        auto node = GetTreeNodeUnderPoint(cursor.position);
        if (mSelectedNodes.Count() < 2)
        {
            for (auto& sel : mSelectedNodes)
                sel->SetSelected(false);

            mSelectedNodes.Clear();
            mSelectedObjects.Clear();

            if (node)
                SelectObject(node->GetObject());
            else
                OnSelectionChanged();
        }

        Focus();

        OnNodeRBClick(node);
    }

    void Tree::OnCursorPressBreak(const Input::Cursor& cursor)
    {}

    void Tree::OnCursorExit(const Input::Cursor& cursor)
    {
        if (auto hoverState = state["hover"])
            hoverState->SetState(false);
        else
            mHoverDrawable->SetEnabled(false);
    }

    void Tree::OnScrolled(float scroll)
    {
        ScrollArea::OnScrolled(scroll);
    }

    Ref<TreeNode> Tree::GetTreeNodeUnderPoint(const Vec2F& point)
    {
        for (auto& node : mVisibleNodes)
        {
            if (node->widget && node->widget->layout->IsPointInside(point))
                return node->widget;
        }

        return nullptr;
    }

    Ref<TreeNode> Tree::CreateTreeNodeWidget()
    {
        Ref<TreeNode> res;

        if (mNodeWidgetsBuf.IsEmpty())
        {
            res = mNodeWidgetSample->CloneAsRef<TreeNode>();
            res->RemoveFromScene();
            res->Show(true);
            res->SetSelectionGroup(Ref(this));
            res->messageFallDownListener = static_cast<ScrollArea*>(this);
            res->mOwnerTree = Ref(this);
        }
        else 
            res = mNodeWidgetsBuf.PopBack();

        res->SetInteractable(!mIsDraggingNodes);
        res->mIsSelected = false;
        res->mIsClipped = false;
        res->SetEnabledForcible(true);
        Widget::UpdateTransparency();

        mIsNeedUpdateView = false;

        return res;
    }

    void Tree::BeginDragging(const Ref<TreeNode>& node)
    {
        if (mRearrangeType == RearrangeType::Disabled)
            return;

        mDragOffset = node->layout->worldCenter - node->GetCursorPressedPoint();
        mFakeDragNode->Show(true);
        mFakeDragNode->SetInteractable(false);
        mFakeDragNode->SetStateForcible("selected", true);
        mFakeDragNode->SetStateForcible("focused", true);

        FillNodeDataByObject(mFakeDragNode, mSelectedNodes.Last()->object);

        if (mSelectedNodes.Count() > 1)
        {
            if (auto nameLayer = mFakeDragNode->GetLayerDrawableByType<Text>())
                nameLayer->text = String::Format("%i items", mSelectedNodes.Count());
        }

        for (auto& node : mVisibleNodes)
            node->widget->SetInteractable(false);

        mIsDraggingNodes = true;
        UpdateNodesStructure();
    }

    void Tree::EndDragging(bool droppedToThis /*= false*/)
    {
        mIsDraggingNodes = false;
        mDragEnded = false;
        mFakeDragNode->Hide(true);

        int idx = 0;
        for (auto& node : mAllNodes)
        {
            node->inserting = false;
            node->insertCoef = 0.0f;

            if (node->widget)
            {
                node->widget->SetInteractable(true);
                UpdateNodeWidgetLayout(node, idx);
                node->widget->SetLayoutDirty();
            }

            idx++;
        }

        if (!droppedToThis)
        {
            UpdateNodesStructure();
            SetSelectedObjects(mBeforeDragSelectedItems);
        }
    }

    void Tree::UpdateDraggingGraphics()
    {
        const Input::Cursor& cursor = *o2Input.GetCursor(0);

        float width = layout->width;
        float height = mNodeWidgetSample->layout->minHeight;
        RectF dragNodeRect = RectF(cursor.position.x - width*0.5f, cursor.position.y + height*0.5f,
                                   cursor.position.x + width*0.5f, cursor.position.y - height*0.5f);

        dragNodeRect += mDragOffset;
        mFakeDragNode->layout->worldRect = dragNodeRect;
        mFakeDragNode->Widget::UpdateTransform();
    }

    void Tree::UpdateDraggingInsertion()
    {
        bool first = true;
        float ycursor = o2Input.GetCursor(0)->position.y;
        for (auto& node : mVisibleNodes)
        {
            if (!node->widget)
                return;

            float y = node->widget->layout->GetWorldCenter().y;
            float h = node->widget->layout->height;
            float cy = first ? y : (y > ycursor ? y + h*mNodeDragIntoZone : y - h*mNodeDragIntoZone);
            first = false;

            node->inserting = cy < ycursor;
        }
    }

    void Tree::UpdateDraggingInsertionAnim(float dt)
    {
        for (auto& node : mVisibleNodes)
        {
            bool changed = false;
            if (node->inserting && node->insertCoef < 1.0f)
            {
                node->insertCoef += dt/mExpandNodeTime*2.0f;

                if (node->insertCoef > 1.0f)
                    node->insertCoef = 1.0f;

                changed = true;
            }

            if (!node->inserting && node->insertCoef > 0.0f)
            {
                node->insertCoef -= dt/mExpandNodeTime*2.0f;

                if (node->insertCoef < 0.0f)
                    node->insertCoef = 0.0f;

                changed = true;
            }

            if (node->widget && changed)
            {
                UpdateNodeWidgetLayout(node, mAllNodes.IndexOf(node));
                node->widget->SetLayoutDirty();
            }
        }
    }

    void Tree::OnDragEnter(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        mInsertNodeCandidate = nullptr;
        OnDraggedAbove(group);
    }

    void Tree::OnDraggedAbove(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        const Input::Cursor& cursor = *o2Input.GetCursor(0);

        Ref<TreeNode> currentInsertCandidate;
        float nodeDst = FLT_MAX;
        bool first = true;
        for (auto& node : mVisibleNodes)
        {
            //             if (node->widget->mDragSizeCoef < 0.95f)
            //                 continue;
            //                 
            if (!node->widget)
                continue;

            float dst = cursor.position.y - node->widget->layout->GetWorldCenter().y;
            if (!first)
                dst += node->widget->layout->height*mNodeDragIntoZone;

            if (dst > 0 && dst < nodeDst)
            {
                nodeDst = dst;
                currentInsertCandidate = node->widget;
            }

            first = false;
        }

        if (currentInsertCandidate != mInsertNodeCandidate)
        {
            mInsertNodeCandidate = currentInsertCandidate;

            if (mRearrangeType == RearrangeType::Enabled)
                UpdateDraggingInsertion();
        }

        UpdateHover(mInsertNodeCandidate);

        if (mInsertNodeCandidate && !mInsertNodeCandidate->IsUnderPoint(cursor.position))
            mTargetHoverRect += Vec2F::Up()*mInsertNodeCandidate->layout->height;
    }

    void Tree::OnDragExit(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        for (auto& node : mVisibleNodes)
            node->inserting = false;
    }

    void Tree::OnDropped(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        if (DynamicCast<Tree>(group) == nullptr)
            return;

        auto underCursorItem = GetTreeNodeUnderPoint(o2Input.GetCursorPos());

        EndDragging(true);
        OnSelectionChanged();

        Vector<void*> objects;
        void* targetParent = nullptr;
        void* targetPrevObject = nullptr;
        auto insertNodeCandidate = mInsertNodeCandidate ? mInsertNodeCandidate->mNodeDef.Lock() : nullptr;

        if (underCursorItem)
            targetParent = underCursorItem->mNodeDef.Lock()->object;
        else
        {
            if (insertNodeCandidate)
            {
                targetParent = insertNodeCandidate->parent ? insertNodeCandidate->parent.Lock()->object : nullptr;
                auto parentChilds = GetObjectChilds(targetParent);

                int idx = parentChilds.IndexOf(insertNodeCandidate->object);
                if (idx > 0)
                {
                    targetPrevObject = parentChilds[idx - 1];

                    if (mSelectedObjects.Contains(targetPrevObject) && idx > 1)
                        targetPrevObject = parentChilds[idx - 2];
                }
                else
                    targetPrevObject = nullptr;
            }
            else
            {
                auto rootObjects = GetObjectChilds(nullptr);
                if (rootObjects.Count() > 0)
                {
                    targetPrevObject = rootObjects.Last();

                    if (mSelectedObjects.Contains([&](void* x) { return ((Node*)x)->object == targetPrevObject; }))
                    {
                        if (rootObjects.Count() > 1)
                            targetPrevObject = rootObjects[rootObjects.Count() - 2];
                        else
                            targetPrevObject = nullptr;
                    }
                }
            }
        }

        for (auto& sel : mSelectedObjects)
        {
            bool processing = true;

            void* parent = GetObjectParent(sel);
            while (parent)
            {
                if (mSelectedObjects.Contains(parent))
                {
                    processing = false;
                    break;
                }

                parent = GetObjectParent(parent);
            }

            if (processing)
                objects.Add(sel);
        }

        o2Debug.Log("Drop parent:" + GetObjectDebug(targetParent) + ", prev:" + GetObjectDebug(targetPrevObject));
        OnDraggedObjects(objects, targetParent, targetPrevObject);

        UpdateNodesStructure();
    }

    void Tree::OnDeserialized(const DataValue& node)
    {
        ScrollArea::OnDeserialized(node);
        mHighlightAnim->SetTarget(mHighlightSprite.Get());
        mHighlightAnim->SetClip(mHighlighClip);
    }

    void Tree::OnSelectionChanged()
    {
        OnNodesSelectionChanged(mSelectedObjects);
    }

    const Ref<TreeNode>& Tree::GetNodeSample() const
    {
        return mNodeWidgetSample;
    }

    void Tree::SetNodeSample(const Ref<TreeNode>& sample)
    {
        mNodeWidgetSample = sample;
    }

    const Ref<Sprite>& Tree::GetHoverDrawable() const
    {
        return mHoverDrawable;
    }

    const Ref<Sprite>& Tree::GetHighlightDrawable() const
    {
        return mHighlightSprite;
    }

    void Tree::SetHighlightAnimation(const Ref<AnimationClip>& animation)
    {
        mHighlighClip = animation;
        mHighlightAnim->SetTarget(mHighlightSprite.Get());
        mHighlightAnim->SetClip(mHighlighClip);
    }

    void Tree::SetHighlightLayout(const Layout& layout)
    {
        mHighlightLayout = layout;
    }

    void Tree::SetZebraBackLine(const Ref<Sprite>& sprite)
    {
        mZebraBackLine = sprite;
    }

    const Ref<Sprite>& Tree::GetZebraBackLine() const
    {
        return mZebraBackLine;
    }

    bool Tree::IsScrollable() const
    {
        return true;
    }

    void Tree::SetRearrangeType(RearrangeType type)
    {
        mRearrangeType = type;
    }

    Tree::RearrangeType Tree::GetRearrangeType() const
    {
        return mRearrangeType;
    }

    void Tree::SetMultipleSelectionAvailable(bool available)
    {
        mMultiSelectAvailable = available;

        if (!mMultiSelectAvailable)
        {
            if (!mSelectedObjects.IsEmpty())
            {
                void* lastSelected = mSelectedObjects.Last();
                DeselectAllObjects();
                SelectObject(lastSelected);
            }
        }
    }

    bool Tree::IsMultiSelectionAvailable() const
    {
        return mMultiSelectAvailable;
    }

    void Tree::SetNodeExpandTimer(float time)
    {
        mExpandInsertTime = time;
    }

    float Tree::GetNodeExpandTimer() const
    {
        return mExpandInsertTime;
    }

    void Tree::SetChildsNodesOffset(float offset)
    {
        mChildrenOffset = offset;
        SetLayoutDirty();
    }

    float Tree::GetChildsNodesOffset() const
    {
        return mChildrenOffset;
    }

    void Tree::SetHoverLayout(const Layout& layout)
    {
        mHoverLayout = layout;
    }

    bool Tree::IsFocusable() const
    {
        return true;
    }

    bool Tree::IsUnderPoint(const Vec2F& point)
    {
        return Widget::IsUnderPoint(point);
    }

    Tree::Node::~Node()
    {}

    float Tree::Node::GetHeight() const
    {
        float res = 20;
        return res;
    }

    void Tree::Node::SetSelected(bool selected)
    {
        this->isSelected = selected;
        if (widget)
        {
            widget->SetState("selected", selected);
            widget->mIsSelected = selected;

            if (selected)
                widget->OnSelected();
            else
                widget->OnDeselected();
        }
    }

    int Tree::Node::GetChildCount() const
    {
        return childs.Count() + childs.Sum<int>([](auto& x) { return x->GetChildCount(); });
    }

    bool Tree::VisibleWidgetDef::operator==(const VisibleWidgetDef& other) const
    {
        return object == other.object;
    }

    String Tree::GetCreateMenuGroup()
    {
        return "Tree";
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::Tree>);
// --- META ---

ENUM_META(o2::Tree::RearrangeType)
{
    ENUM_ENTRY(Disabled);
    ENUM_ENTRY(Enabled);
    ENUM_ENTRY(OnlyReparent);
}
END_ENUM_META;

ENUM_META(o2::Tree::ExpandState)
{
    ENUM_ENTRY(Collaping);
    ENUM_ENTRY(Expanding);
    ENUM_ENTRY(None);
}
END_ENUM_META;

DECLARE_CLASS(o2::Tree, o2__Tree);

DECLARE_CLASS(o2::TreeNode, o2__TreeNode);
// --- END META ---
