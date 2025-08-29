#include "o2Editor/stdafx.h"

#include "o2Editor/Windows/AnimationWindow/Tree.h"

#include "PropertiesListDlg.h"
#include "TrackControls/SubTrackControl.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/AnimationWindow/AnimationWindow.h"
#include "o2Editor/Windows/AnimationWindow/TrackControls/KeyFramesTrackControl.h"
#include "o2Editor/Windows/AnimationWindow/TrackControls/MapKeyFramesTrackControl.h"
#include "o2Editor/Windows/AnimationWindow/TrackControls/Vec2KeyFramesTrackControl.h"

namespace Editor
{
    AnimationTree::AnimationTree(RefCounter* refCounter) :
        Tree(refCounter)
    {
        SetRearrangeType(Tree::RearrangeType::Disabled);
        InitializeContext();
    }

    AnimationTree::AnimationTree(RefCounter* refCounter, const AnimationTree& other) :
        Tree(refCounter, other)
    {
        SetRearrangeType(Tree::RearrangeType::Disabled);
        InitializeContext();
    }

    AnimationTree::AnimationTree(const AnimationTree& other) :
        AnimationTree(nullptr, other)
    {}

    AnimationTree::~AnimationTree()
    {}

    AnimationTree& AnimationTree::operator=(const AnimationTree& other)
    {
        Tree::operator=(other);

        mZebraBackLine = other.mZebraBackLine->CloneAsRef<Sprite>();
        InitializeContext();

        return *this;
    }

    void AnimationTree::Draw()
    {
        o2Render.EnableScissorTest(mAbsoluteClipArea);
        DrawZebraBack();
        o2Render.DisableScissorTest();

        mAnimationWindow->mHandlesSheet->Draw();

        // Disable zebra
        auto buf = mZebraBackLine;
        mZebraBackLine = nullptr;

        Tree::Draw();

        // Enable
        mZebraBackLine = buf;

        mAnimationWindow->mHandlesSheet->UpdateInputDrawOrder();
    }

    void AnimationTree::SetAnimation(const Ref<AnimationClip>& animation)
    {
        mAnimationWindow->mHandlesSheet->UnregAllTrackControls();

        mPrevSelectedNodes.Clear();

        RebuildAnimationTree();
        ExpandAll();
        OnObjectsChanged({ (void*)mRootValue.Get() });
    }

    void AnimationTree::SetTreeWidth(float width)
    {
        mTreeWidth = width;
        UpdateTreeWidth();
    }

    void AnimationTree::SetAnimationValueColor(const String& path, const Color4& color)
    {
        auto node = mRootValue;
        auto itPath = path;
        while (!itPath.IsEmpty())
        {
            int fnd = itPath.Find('/', 0);
            String pathPart = itPath.SubStr(0, fnd);

            for (auto& child : node->children)
            {
                if (child->name == pathPart)
                {
                    node = child;
                    break;
                }
            }
            
            if (fnd < 0)
                break;

            itPath = itPath.SubStr(fnd + 1);
        }

        if (!node)
            return;

        node->color = color;

        if (node->trackControl)
            node->trackControl->SetCurveViewColor(color);
    }

    float AnimationTree::GetLineNumber(float worldPosition) const
    {
        return (layout->GetWorldTop() - worldPosition + mScrollPos.y) / mNodeWidgetSample->layout->GetMinHeight();
    }

    float AnimationTree::GetLineWorldPosition(float lineNumber) const
    {
        return -(lineNumber * mNodeWidgetSample->layout->GetMinHeight() - mScrollPos.y - layout->GetWorldTop());
    }

    String AnimationTree::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void AnimationTree::InitializeContext()
    {
        mContextMenu = o2UI.CreateWidget<ContextMenu>();

        mContextMenu->AddItem("New", [&]() {});
        mContextMenu->AddItem("Save", [&]() {}, AssetRef<ImageAsset>(), ShortcutKeys('S', true));
        mContextMenu->AddItem("Save as...", [&]() {}, AssetRef<ImageAsset>(), ShortcutKeys('S', true, true));
        mContextMenu->AddItem("---");
        mContextMenu->AddItem("Delete property", THIS_FUNC(OnDeletePropertyPressed));
        mContextMenu->AddItem("---");
        mContextMenu->AddItem("Add properties", [&]() { 
            auto animation = mAnimationWindow->mAnimation.Lock();
            auto targetActor = mAnimationWindow->mTargetActor.Lock();
            if (!animation || !targetActor)
                return;
            PropertiesListDlg::Show(animation, targetActor); 
        });

        onFocused = [&]() { mAnimationWindow->mHandlesSheet->GetContextMenu()->SetItemsMaxPriority(); mContextMenu->SetItemsMaxPriority(); };

        AddInternalWidget(mContextMenu);
    }

    void AnimationTree::RebuildAnimationTree()
    {
        mRootValue = nullptr;

        if (!mAnimationWindow->mAnimation)
            return;

        auto animation = mAnimationWindow->mAnimation.Lock();
        if (!animation)
            return;
            
        mAnimationValuesCount = animation->GetTracks().Count();

        mRootValue = mmake<TrackNode>();
        mRootValue->name = "Track name";

        if (mAnimationWindow->mPreviewPlayer)
        {
            for (auto& trackPlayer : mAnimationWindow->mPreviewPlayer->GetTrackPlayers())
                AddAnimationTrack(trackPlayer->GetTrack(), trackPlayer);
        }
        else
        {
            for (auto& track : animation->GetTracks())
                AddAnimationTrack(track, nullptr);
        }

        UpdateNodesStructure();
    }

    void AnimationTree::AddAnimationTrack(const Ref<IAnimationTrack>& track, const Ref<IAnimationTrack::IPlayer>& player /*= nullptr*/)
    {
        Ref<TrackNode> current;

        int lastDel = 0;
        while (lastDel >= 0)
        {
            int del = track->path.Find('/', lastDel);
            String subPath = track->path.SubStr(lastDel, del);
            auto next = (current ? current->children : mRootValue->children)
                .FindOrDefault([&](auto& x) { return x->name == subPath; });

            if (!next)
            {
                next = mmake<TrackNode>();
                next->name = subPath;
                next->path = track->path;

                if (current)
                {
                    next->parent = current;
                    current->children.Add(next);
                }
                else
                    mRootValue->children.Add(next);
            }

            current = next;
            lastDel = del >= 0 ? del + 1 : -1;
        }

        current->track = track;
        current->player = player;
    }

#undef DrawText

    void AnimationTree::UpdateTreeWidth()
    {
        for (auto& node : mVisibleNodes)
        {
            if (auto trackNode = DynamicCast<AnimationTreeNode>(node->widget))
                trackNode->SetTreeWidth(mTreeWidth);
        }
    }

    void AnimationTree::SetCurveViewMode(bool enable)
    {
        for (auto& node : mVisibleNodes)
        {
            if (auto trackNode = DynamicCast<AnimationTreeNode>(node->widget))
                trackNode->mTrackControl->SetCurveViewEnabled(enable);
        }
    }

    void AnimationTree::OnAnimationChanged()
    {
        auto animation = mAnimationWindow->mAnimation.Lock();
        if (!animation)
            return;
            
        if (animation->GetTracks().Count() != mAnimationValuesCount)
        {
            mAnimationWindow->mHandlesSheet->UnregAllTrackControls();

            RebuildAnimationTree();
            ExpandAll();
            OnObjectsChanged({ (void*)mRootValue.Get() });
        }
    }

    void* AnimationTree::GetObjectParent(void* object)
    {
        if (!object)
            return nullptr;

        auto treeNode = (TrackNode*)object;
        return treeNode->parent.Lock().Get();
    }

    Vector<void*> AnimationTree::GetObjectChilds(void* object)
    {
        if (!object)
            return { (void*)mRootValue.Get() };

        auto treeNode = (TrackNode*)object;
        return treeNode->children.Convert<void*>([](auto& x) { return x.Get(); });
    }

    String AnimationTree::GetObjectDebug(void* object)
    {
        auto treeNode = (TrackNode*)object;
        return treeNode ? treeNode->name : String("Empty");
    }

    void AnimationTree::FillNodeDataByObject(const Ref<TreeNode>& nodeWidget, void* object)
    {
        auto node = DynamicCast<AnimationTreeNode>(nodeWidget);
        node->Setup(Ref((TrackNode*)object), mAnimationWindow->mTimeline, mAnimationWindow->mHandlesSheet);
    }

    void AnimationTree::FreeNodeData(const Ref<TreeNode>& nodeWidget, void* object)
    {
        auto node = DynamicCast<AnimationTreeNode>(nodeWidget);
        node->Free();
    }

    void AnimationTree::UpdateVisibleNodes()
    {
        Tree::UpdateVisibleNodes();
        UpdateTreeWidth();
    }

    void AnimationTree::OnNodeRBClick(const Ref<TreeNode>& node)
    {
        o2UI.FocusWidget(Ref(this));
        mContextMenu->Show();
    }

    void AnimationTree::OnNodesSelectionChanged(Vector<void*> objects)
    {
        for (auto& node : mPrevSelectedNodes)
            node->trackControl->SetActive(false);

        mAnimationWindow->mHandlesSheet->DeselectAll();

        mPrevSelectedNodes.Clear();
        for (auto& obj : objects) 
        {
            auto node = Ref((TrackNode*)obj);
            mPrevSelectedNodes.Add(node);

            node->trackControl->SetActive(true);

            for (auto& handle : node->trackControl->GetKeyHandles())
                handle->handle->SetSelected(true);
        }
    }

    Ref<TreeNode> AnimationTree::CreateTreeNodeWidget()
    {
        PushEditorScopeOnStack scope;
        return Tree::CreateTreeNodeWidget();
    }

    void AnimationTree::OnDeletePropertyPressed()
    {
        auto animation = mAnimationWindow->mAnimation.Lock();
        if (!animation)
            return;
            
        for (auto& obj : GetSelectedObjects())
        {
            TrackNode* data = (TrackNode*)obj;
            animation->RemoveTrack(data->path);
        }
    }

    AnimationTreeNode::AnimationTreeNode(RefCounter* refCounter) :
        TreeNode(refCounter)
    {}

    AnimationTreeNode::AnimationTreeNode(RefCounter* refCounter, const AnimationTreeNode& other) :
        TreeNode(refCounter, other)
    {
        InitializeControls();
    }

    AnimationTreeNode& AnimationTreeNode::operator=(const AnimationTreeNode& other)
    {
        TreeNode::operator=(other);
        InitializeControls();

        return *this;
    }

    void AnimationTreeNode::Setup(const Ref<AnimationTree::TrackNode>& node, const Ref<AnimationTimeline>& timeline, 
                                  const Ref<KeyHandlesSheet>& handlesSheet)
    {
        mTimeline = timeline;
        mHandlesSheet = handlesSheet;

        mTimeline->onViewChanged -= THIS_FUNC(UpdateTrackControlView);
        mTimeline->onViewChanged += THIS_FUNC(UpdateTrackControlView);

        mData = node;
        mNameDrawable->text = node->name;

        InitilizeTrackControl();

        mData->trackControl = mTrackControl;
    }

    void AnimationTreeNode::Free()
    {
        FreeTrackControl();
    }

    void AnimationTreeNode::SetTreeWidth(float width)
    {
        if (mTrackControl)
        {
            *mTrackControl->layout = WidgetLayout::BothStretch(width, 0, 0, 0);

            float right = width - layout->GetOffsetLeft();
            if (auto controls = mTrackControl->GetTreePartControls())
            {
                *controls->layout = WidgetLayout::VerStretch(HorAlign::Left, mPropertyBorder, mPropertyBorder + 1, mPropertySize,
                                                             right - mPropertySize - mPropertyBorder);
            }
        }
    }

    void AnimationTreeNode::OnDoubleClicked(const Input::Cursor& cursor)
    {
        if (mTrackControl)
            mTrackControl->InsertNewKey(mTimeline->WorldToLocal(cursor.position.x));
    }

    String AnimationTreeNode::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void AnimationTreeNode::OnDeserialized(const DataValue& node)
    {
        TreeNode::OnDeserialized(node);
        InitializeControls();
    }

    void AnimationTreeNode::InitializeControls()
    {
        mNameDrawable = GetLayerDrawable<Text>("name");
    }

    Map<const Type*, o2::Vector<Ref<ITrackControl>>> AnimationTreeNode::mTrackControlsCache;

    void AnimationTreeNode::InitilizeTrackControl()
    {
        PushEditorScopeOnStack scope;

        static Map<const Type*, const Type*> trackToControlTrackTypes =
        {
            { &TypeOf(AnimationTrack<float>), &TypeOf(KeyFramesTrackControl<AnimationTrack<float>>) },
            { &TypeOf(AnimationTrack<bool>), &TypeOf(KeyFramesTrackControl<AnimationTrack<bool>>) },
            { &TypeOf(AnimationTrack<Vec2F>), &TypeOf(Vec2KeyFramesTrackControl) },
            { &TypeOf(AnimationTrack<Color4>), &TypeOf(KeyFramesTrackControl<AnimationTrack<Color4>>) }
        };

        FreeTrackControl();

        if (!mData->track)
        {
            Ref<MapKeyFramesTrackControl> trackControl;
            auto trackControlType = &TypeOf(MapKeyFramesTrackControl);
            if (mTrackControlsCache.ContainsKey(trackControlType) && !mTrackControlsCache[trackControlType].IsEmpty())
                trackControl = DynamicCast<MapKeyFramesTrackControl>(mTrackControlsCache[trackControlType].PopBack());
            else
                trackControl = mmake<MapKeyFramesTrackControl>();

            mTrackControl = trackControl;

            trackControl->Initialize(mTimeline, mHandlesSheet);
            trackControl->SetMappedTracks(*mData);

             AddChild(mTrackControl);
        }
        else
        {
            const Type* trackControlType = nullptr;

            auto trackType = &mData->track->GetType();
            if (trackType == &TypeOf(AnimationSubTrack))
            {
                trackControlType = &TypeOf(SubTrackControl);
            }
            else if (trackToControlTrackTypes.ContainsKey(trackType))
            {
                trackControlType = dynamic_cast<const ObjectType*>(trackToControlTrackTypes[trackType]);
            }
            else
            {
                o2Debug.LogWarning("Can't create control track for type:" + trackType->GetName());
                return;
            }

            if (mTrackControlsCache.ContainsKey(trackControlType) && !mTrackControlsCache[trackControlType].IsEmpty())
                mTrackControl = mTrackControlsCache[trackControlType].PopBack();
            else
                mTrackControl = DynamicCast<ITrackControl>(trackControlType->CreateSampleRef());

            mTrackControl->Initialize(mTimeline, mHandlesSheet);
            mTrackControl->SetTrack(mData->track, mData->player, mData->path);

            AddChild(mTrackControl);
        }

        if (auto controls = mTrackControl->GetTreePartControls())
            AddChild(controls);

        auto animTree = DynamicCast<AnimationTree>(mOwnerTree.Lock());
        mTrackControl->SetCurveViewEnabled(animTree->mAnimationWindow->IsCurvesMode());
        mTrackControl->SetCurveViewColor(mData->color);

        mHandlesSheet->RegTrackControl(mTrackControl, mData->path);
    }

    void AnimationTreeNode::FreeTrackControl()
    {
        if (mTrackControl)
        {
            auto trackType = &mTrackControl->GetType();
            if (!mTrackControlsCache.ContainsKey(trackType))
                mTrackControlsCache.Add(trackType, {});

            mTrackControlsCache[trackType].Add(mTrackControl);

            RemoveChild(mTrackControl, false);

            if (auto controls = mTrackControl->GetTreePartControls())
                RemoveChild(controls, false);

            mHandlesSheet->UnregTrackControl(mTrackControl);
        }

        mTrackControl = nullptr;
    }

    void AnimationTreeNode::UpdateTrackControlView()
    {
        if (mTrackControl)
            mTrackControl->UpdateHandles();
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::AnimationTreeNode>);
// --- META ---

DECLARE_CLASS(Editor::AnimationTree, Editor__AnimationTree);

DECLARE_CLASS(Editor::AnimationTreeNode, Editor__AnimationTreeNode);
// --- END META ---
