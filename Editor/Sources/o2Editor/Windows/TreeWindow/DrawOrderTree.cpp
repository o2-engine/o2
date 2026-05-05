#include "o2Editor/stdafx.h"
#include "DrawOrderTree.h"

#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Assets/Types/FolderAsset.h"
#include "o2/Events/EventSystem.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/AssetsWindow/AssetIcon.h"
#include "o2Editor/Windows/WindowsManager.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"
#include "o2Editor/Windows/AssetsWindow/AssetsIconsScroll.h"
#include "o2Editor/Windows/AssetsWindow/AssetsWindow.h"
#include "o2Editor/Actions/Enable.h"
#include "o2Editor/Actions/Lock.h"
#include "o2Editor/Actions/PropertyChange.h"
#include "o2Editor/Actions/Reparent.h"
#include "o2Editor/EditorApplication.h"
#include "o2Editor/Properties/Basic/ActorProperty.h"
#include "o2Editor/Properties/Basic/ComponentProperty.h"
#include "o2Editor/UIRoot.h"

namespace Editor
{
    DrawOrderTree::DrawOrderTree(RefCounter* refCounter) :
        Tree(refCounter), mAttachedToSceneEvents(false), mDragActorPropertyField(nullptr), mDragComponentPropertyField(nullptr)
    {
        mNodeWidgetSample = mmake<DrawOrderTreeNode>();
        mNodeWidgetSample->layout->minHeight = 20;
        mNodeWidgetSample->AddLayer("caption", nullptr);

        Initialize();
    }

    DrawOrderTree::DrawOrderTree(RefCounter* refCounter, const DrawOrderTree& other) :
        Tree(refCounter, other), mAttachedToSceneEvents(false), mDragActorPropertyField(nullptr), mDragComponentPropertyField(nullptr)
    {
        Initialize();
    }

    DrawOrderTree::DrawOrderTree(const DrawOrderTree& other):
        DrawOrderTree(nullptr, other)
    {}

    DrawOrderTree::~DrawOrderTree()
    {
        DeattachFromSceneEvents();
    }

    DrawOrderTree& DrawOrderTree::operator=(const DrawOrderTree& other)
    {
        Tree::operator=(other);
        return *this;
    }

    void DrawOrderTree::AttachToSceneEvents()
    {
        o2Scene.onObjectsChanged += THIS_FUNC(OnObjectsChanged);

        o2Scene.onAddedToScene += THIS_FUNC(OnObjectCreated);
        o2Scene.onRemovedFromScene += THIS_FUNC(OnObjectDestroing);

        auto updateTreeNode = THIS_FUNC(OnObjectChanged);
        o2Scene.onEnableChanged += updateTreeNode;
        o2Scene.onLockChanged += updateTreeNode;
        o2Scene.onNameChanged += updateTreeNode;
        o2Scene.onChildrenHierarchyChanged += updateTreeNode;

        mAttachedToSceneEvents = true;
    }

    void DrawOrderTree::DeattachFromSceneEvents()
    {
        if (Scene::IsSingletonInitialzed() && mAttachedToSceneEvents)
        {
            o2Scene.onObjectsChanged -= THIS_FUNC(OnObjectsChanged);

            o2Scene.onAddedToScene -= THIS_FUNC(OnObjectCreated);
            o2Scene.onRemovedFromScene -= THIS_FUNC(OnObjectDestroing);

            auto updateTreeNode = THIS_FUNC(OnObjectChanged);
            o2Scene.onEnableChanged -= updateTreeNode;
            o2Scene.onLockChanged -= updateTreeNode;
            o2Scene.onNameChanged -= updateTreeNode;
            o2Scene.onChildrenHierarchyChanged -= updateTreeNode;
        }
    }

    String DrawOrderTree::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void DrawOrderTree::Initialize()
    {
        mEnableTogglesGroup = mmake<ToggleGroup>(ToggleGroup::Type::VerOneClick);
        mEnableTogglesGroup->onReleased = THIS_FUNC(EnableObjectsGroupReleased);

        mLockTogglesGroup = mmake<ToggleGroup>(ToggleGroup::Type::VerOneClick);
        mLockTogglesGroup->onReleased = THIS_FUNC(LockObjectsGroupReleased);

        auto objectNodeWidgetSample = DynamicCast<DrawOrderTreeNode>(mNodeWidgetSample);
        objectNodeWidgetSample->InitializeControls();
    }

    void DrawOrderTree::SetSelectedObjects(const Vector<Ref<SceneEditableObject>>& objects)
    {
        Vector<Ref<OrderTreeNode>> nodes;
        for (auto& object : objects)
        {
            Ref<OrderTreeNode> node;
            if (mObjectToNodeMap.TryGetValue(object, node))
                nodes.Add(node);
        }

        Tree::SetSelectedObjects(nodes.Convert<void*>([](auto& x) { return (void*)x.Get(); }));
    }

    void DrawOrderTree::ScrollToAndHighlight(const Ref<SceneEditableObject>& object)
    {
        Ref<OrderTreeNode> node;
        if (mObjectToNodeMap.TryGetValue(object, node))
            Tree::ScrollToAndHighlight(node.Get());
    }

    void DrawOrderTree::RebuildOrderTree()
    {
        mRootOrderNodes.Clear();
        mObjectToNodeMap.Clear();

        mStartBatchIdx = INT_MAX;

        for (auto& weakCamera : o2Scene.GetCameras())
        {
            auto camera = weakCamera.Lock();
            Ref<OrderTreeNode> cameraNode = mmake<OrderTreeNode>();
            cameraNode->name = camera->GetName();
            cameraNode->type = OrderTreeNode::Type::Camera;
            cameraNode->object = camera;
            mObjectToNodeMap[cameraNode->object] = cameraNode;
            mRootOrderNodes.Add(cameraNode);

            for (auto& layer : camera->drawLayers.GetLayers())
            {
                Ref<OrderTreeNode> layerNode = mmake<OrderTreeNode>();
                layerNode->name = layer->GetName();
                layerNode->type = OrderTreeNode::Type::Layer;
                cameraNode->AddChild(layerNode);

                for (auto& drawable : layer->GetDrawables())
                    ProcessDrawableTreeNode(layerNode, drawable);
            }
        }
    }

    void DrawOrderTree::ProcessDrawableTreeNode(const Ref<OrderTreeNode>& parent, const Ref<ISceneDrawable>& drawable)
    {
        Ref<OrderTreeNode> drawableNode = mmake<OrderTreeNode>();

        drawableNode->inheritedOrderFromParent = drawable->IsDrawingDepthInheritedFromParent();
        drawableNode->customOrder = drawable->GetDrawingDepth();
        drawableNode->batchIdx = drawable->drawCallIdx;

        if (drawableNode->batchIdx < mStartBatchIdx)
            mStartBatchIdx = drawableNode->batchIdx;

        if (auto actor = DynamicCast<Actor>(drawable))
        {
            drawableNode->name = actor->name;
            drawableNode->type = OrderTreeNode::Type::Actor;
            drawableNode->object = actor;

            mObjectToNodeMap[drawableNode->object] = drawableNode; 

            CheckBatchEnd(drawableNode);
            parent->AddChild(drawableNode);
        }
        else if (auto root = DynamicCast<SceneLayerRootDrawablesContainer>(drawable))
        {
            drawableNode->type = OrderTreeNode::Type::Root;
            drawableNode->name = "Layer roots";

            CheckBatchEnd(drawableNode);
            parent->AddChild(drawableNode);
        }
        else
        {
            drawableNode->name = drawable->GetType().GetName();
            drawableNode->type = OrderTreeNode::Type::Drawable;

            CheckBatchEnd(drawableNode);
            parent->AddChild(drawableNode);
        }

        for (auto& inherited : drawable->GetChildrenInheritedDepth())
            ProcessDrawableTreeNode(drawableNode, inherited);
    }

    void DrawOrderTree::CheckBatchEnd(const Ref<OrderTreeNode>& node)
    {
        static Ref<OrderTreeNode> prevDrawableNode;

        if (prevDrawableNode && node->object != nullptr && node->batchIdx != prevDrawableNode->batchIdx)
        {
            Ref<OrderTreeNode> endNode = mmake<OrderTreeNode>();
            endNode->type = OrderTreeNode::Type::EndOfBatch;
            endNode->batchIdx = prevDrawableNode->batchIdx;

            endNode->name = "End of batch #" + (String)(endNode->batchIdx - mStartBatchIdx);

            if (auto parent = prevDrawableNode->GetParent().Lock())
                parent->AddChild(endNode);
        }

        if (node->object && node->batchIdx >= 0)
            prevDrawableNode = node;
    }

    void DrawOrderTree::UpdateNodesStructure()
    {
        RebuildOrderTree();
        Tree::UpdateNodesStructure();
    }

    void DrawOrderTree::UpdateVisibleNodes()
    {
        PushEditorScopeOnStack scope;
        Tree::UpdateVisibleNodes();
    }

    Ref<TreeNode> DrawOrderTree::CreateTreeNodeWidget()
    {
        PushEditorScopeOnStack scope;
        return Tree::CreateTreeNodeWidget();
    }

    void* DrawOrderTree::GetObjectParent(void* object)
    {
        if (!object)
            return nullptr;

        OrderTreeNode* treeNode = (OrderTreeNode*)(void*)object;
        return (void*)(treeNode->GetParent().Lock().Get());
    }

    Vector<void*> DrawOrderTree::GetObjectChilds(void* object)
    {
        OrderTreeNode* treeNode = (OrderTreeNode*)object;
        if (OrderTreeNode* treeNode = (OrderTreeNode*)object)
            return treeNode->GetChildren().Convert<void*>([](auto x) { return (void*)x.Get(); });

        return mRootOrderNodes.Convert<void*>([](auto x) { return (void*)x.Get(); });
    }

    void DrawOrderTree::FillNodeDataByObject(const Ref<TreeNode>& nodeWidget, void* object)
    {
        Ref<DrawOrderTreeNode> node = DynamicCast<DrawOrderTreeNode>(nodeWidget);
        node->Setup(Ref((OrderTreeNode*)object));
        node->mLockToggle->SetToggleGroup(mLockTogglesGroup);
        node->mEnableToggle->SetToggleGroup(mEnableTogglesGroup);
    }

    void DrawOrderTree::OnNodeDblClick(const Ref<TreeNode>& nodeWidget)
    {
        (DynamicCast<DrawOrderTreeNode>(nodeWidget))->EnableEditName();
    }

    void DrawOrderTree::OnDraggedObjects(Vector<void*> objects, void* newParent, void* prevObject)
    {
        auto newParentEditableObject = Ref((SceneEditableObject*)newParent);
        auto prevEditableObject = Ref((SceneEditableObject*)prevObject);
        auto editableObjects = objects.Convert<Ref<SceneEditableObject>>([](void* x) { return Ref((SceneEditableObject*)x); });

        auto action = mmake<ReparentAction>(editableObjects);

        o2Scene.ReparentEditableObjects(editableObjects, newParentEditableObject, prevEditableObject);

        action->ObjectsReparented(mParent.Lock(), prevEditableObject);
        o2EditorSceneWindow.DoneAction(action);

        Tree::OnDraggedObjects(objects, newParent, prevObject);
    }

    void DrawOrderTree::EnableObjectsGroupPressed(bool value)
    {}

    void DrawOrderTree::EnableObjectsGroupReleased(bool value)
    {
        Vector<Ref<SceneEditableObject>> objects = mEnableTogglesGroup->GetToggled().Convert<Ref<SceneEditableObject>>(
            [](const WeakRef<Toggle>& x) {
                auto toggleParent = DynamicCast<TreeNode>(x.Lock()->GetParent().Lock());
                return Ref(static_cast<SceneEditableObject*>(toggleParent->GetObject()));
            });

        auto action = mmake<EnableAction>(objects, value);
        o2EditorSceneWindow.DoneAction(action);
    }

    void DrawOrderTree::LockObjectsGroupPressed(bool value)
    {}

    void DrawOrderTree::LockObjectsGroupReleased(bool value)
    {
        Vector<Ref<SceneEditableObject>> objects = mLockTogglesGroup->GetToggled().Convert<Ref<SceneEditableObject>>(
            [](const WeakRef<Toggle>& x) {
                auto toggleParent = DynamicCast<TreeNode>(x.Lock()->GetParent().Lock());
                return Ref(static_cast<SceneEditableObject*>(toggleParent->GetObject()));
            });

        auto action = mmake<LockAction>(objects, value);
        o2EditorSceneWindow.DoneAction(action);
    }

    void DrawOrderTree::OnNodesSelectionChanged(Vector<void*> objects)
    {
        Vector<Ref<SceneEditableObject>> editableObjects = objects
            .Convert<Ref<OrderTreeNode>>([](auto x) { return Ref((OrderTreeNode*)x); })
            .FindAll([](auto& x) { return x->object.IsValid(); })
            .Convert<Ref<SceneEditableObject>>([](auto& x) { return x->object; });

        onObjectsSelectionChanged(editableObjects);
        Tree::OnNodesSelectionChanged(objects);
    }

    void DrawOrderTree::OnDragEnter(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        Tree::OnDragEnter(group);
    }

    void DrawOrderTree::OnDragExit(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        Tree::OnDragExit(group);
    }

    void DrawOrderTree::OnDraggedAbove(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        Tree::OnDraggedAbove(group);
    }

    void DrawOrderTree::OnDropped(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        Tree::OnDropped(group);
    }

    void DrawOrderTree::OnObjectCreated(const Ref<SceneEditableObject>& object)
    {
        RebuildOrderTree();
        Tree::OnObjectCreated(object.Get(), object->GetEditableParent().Get());
    }

    void DrawOrderTree::OnObjectDestroing(const Ref<SceneEditableObject>& object)
    {
        RebuildOrderTree();
        Tree::OnObjectRemoved(object.Get());
    }

    void DrawOrderTree::OnObjectsChanged(const Vector<Ref<SceneEditableObject>>& objects)
    {
        RebuildOrderTree();
        Tree::OnObjectsChanged(objects.Convert<void*>([](auto& x) { return x.Get(); }));
    }

    void DrawOrderTree::OnObjectChanged(const Ref<SceneEditableObject>& object)
    {
        Tree::OnObjectsChanged({ object.Get() });
    }

    DrawOrderTreeNode::DrawOrderTreeNode(RefCounter* refCounter) :
        TreeNode(refCounter)
    {}

    DrawOrderTreeNode::DrawOrderTreeNode(RefCounter* refCounter, const DrawOrderTreeNode& other) :
        TreeNode(refCounter, other)
    {
        InitializeControls();
    }

    Editor::DrawOrderTreeNode& DrawOrderTreeNode::operator=(const DrawOrderTreeNode& other)
    {
        TreeNode::operator=(other);
        InitializeControls();
        return *this;
    }

    void DrawOrderTreeNode::OnDeserialized(const DataValue& node)
    {
        TreeNode::OnDeserialized(node);
        InitializeControls();
    }

    void DrawOrderTreeNode::InitializeControls()
    {
        mNameDrawable = GetLayerDrawable<Text>("name");
        mOrderDrawable = GetLayerDrawable<Text>("batch");
        mIconSprite = GetLayerDrawable<Sprite>("icon");
        mBackSprite = GetLayerDrawable<Sprite>("back");
        mLockToggle = DynamicCast<Toggle>(GetChild("lockToggle"));
        mEnableToggle = DynamicCast<Toggle>(GetChild("enableToggle"));
        mLinkBtn = DynamicCast<Button>(GetChild("linkBtn"));
        mNameEditBox = DynamicCast<EditBox>(GetChild("nameEditBox"));
        mEditState = GetStateObject("edit");

        if (mLinkBtn)
            mLinkBtnHalfHideState = mLinkBtn->GetStateObject("halfHide");

        if (mLockToggle)
        {
            mLockToggleHalfHideState = mLockToggle->GetStateObject("halfHide");
            mLockToggleLockedState = mLockToggle->GetStateObject("locked");

            mLockToggle->onClick = THIS_FUNC(OnLockClicked);
        }

        if (mEnableToggle)
            mEnableToggle->onClick = THIS_FUNC(OnEnableCkicked);

        if (mNameEditBox)
            mNameEditBox->onChangeCompleted = THIS_FUNC(OnObjectNameChanged);
    }

    void DrawOrderTreeNode::Setup(const Ref<DrawOrderTree::OrderTreeNode>& object)
    {
        mTarget = object;

        Map<DrawOrderTree::OrderTreeNode::Type, String> icons =
        {
            { DrawOrderTree::OrderTreeNode::Type::Camera, "ui/UI4_camera_icn.png" },
            { DrawOrderTree::OrderTreeNode::Type::Layer, "ui/UI4_layer_big.png" },
            { DrawOrderTree::OrderTreeNode::Type::Drawable, "ui/UI4_image_icn.png" },
            { DrawOrderTree::OrderTreeNode::Type::Actor, "ui/UI4_image_icn.png" },
            { DrawOrderTree::OrderTreeNode::Type::Root, "ui/UI4_folder_icn.png" },
            { DrawOrderTree::OrderTreeNode::Type::EndOfBatch, "ui/UI4_color_type.png" },
        };

        String name = mTarget->name;

        if (mName != name)
        {
            mName = name;
            mNameDrawable->SetText(mName);
        }

        WString orderText;
        if ((!mTarget->inheritedOrderFromParent && mTarget->object) || mTarget->type == DrawOrderTree::OrderTreeNode::Type::Root)
            orderText = (WString)mTarget->customOrder;
        else if (mTarget->inheritedOrderFromParent)
            orderText = "inherited";

        if (orderText != mOrderDrawable->GetText())
            mOrderDrawable->SetText(orderText);

        String icon = icons[mTarget->type];
        if (mIconSprite->GetImageName() != icon)
            mIconSprite->SetImageAsset(AssetRef<ImageAsset>(icon));
        
        mBackSprite->color = Color4::SomeColor(mTarget->batchIdx, mBackSprite->GetColor().a);
        mBackSprite->enabled = mTarget->type != DrawOrderTree::OrderTreeNode::Type::Camera &&
            mTarget->type != DrawOrderTree::OrderTreeNode::Type::Layer &&
            mTarget->type != DrawOrderTree::OrderTreeNode::Type::Root;

        if (mTarget->object)
        {
            auto sceneObject = mTarget->object;
            float alpha = sceneObject->IsEnabledInHierarchy() ? 1.0f : 0.5f;
            if (!Math::Equals(alpha, mNameDrawable->GetTransparency()))
            {
                mNameDrawable->SetTransparency(alpha);
                mEnableToggle->SetTransparency(alpha);
                mLinkBtn->SetTransparency(alpha);
            }

            if (auto actor = DynamicCast<Actor>(sceneObject))
            {
                mLinkBtn->SetEnabled(actor->GetPrototype().IsValid());
                mLinkBtnHalfHideState->SetState(!actor->GetPrototypeDirectly().IsValid());

                if (actor->GetPrototype())
                {
                    mLinkBtn->onClick = [=]() {
                        auto proto = actor->GetPrototype();
                        o2EditorAssets.ShowAssetIcon(proto->GetPath());
                    };
                }
            }
            else
                mLinkBtn->SetEnabled(false);

            if (sceneObject->IsSupportsDisabling())
            {
                mEnableToggle->SetValue(sceneObject->IsEnabled());
                mEnableToggle->SetEnabled(true);
            }
            else
                mEnableToggle->SetEnabled(false);

            mLockToggle->SetEnabled(true);
            mLockToggle->SetValue(sceneObject->IsLocked());
            mLockToggleLockedState->SetState(sceneObject->IsLockedInHierarchy());
            mLockToggleHalfHideState->SetState(sceneObject->IsLockedInHierarchy() && !sceneObject->IsLocked());
        }
        else
        {
            mEnableToggle->SetEnabled(false);
            mLockToggle->SetEnabled(false);
            mLinkBtn->SetEnabled(false);
        }
    }

    void DrawOrderTreeNode::EnableEditName()
    {
        if (mTarget->object)
        {
            mEditState->SetState(true);

            mNameEditBox->text = mTarget->object->GetName();
            mNameEditBox->SelectAll();
            mNameEditBox->Widget::Focus();
            mNameEditBox->ResetScroll();
        }
    }

    String DrawOrderTreeNode::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void DrawOrderTreeNode::OnLockClicked()
    {
        if (!mTarget->object)
            return;

        auto action = mmake<LockAction>(Vector<Ref<SceneEditableObject>>{ mTarget->object }, mLockToggle->GetValue());
        action->Redo();
        o2EditorSceneWindow.DoneAction(action);
    }

    void DrawOrderTreeNode::OnEnableCkicked()
    {
        if (!mTarget->object)
            return;

        auto action = mmake<EnableAction>(Vector<Ref<SceneEditableObject>>{ mTarget->object }, mEnableToggle->GetValue());
        action->Redo();
        o2EditorSceneWindow.DoneAction(action);
    }

    void DrawOrderTreeNode::OnObjectNameChanged(const WString& text)
    {
        if (mTarget->object)
        {
            String prevName = mTarget->object->GetName();

            mTarget->object->SetName(text);
            mEditState->SetState(false);
            (DynamicCast<DrawOrderTree>(mOwnerTree.Lock()))->OnObjectChanged(mTarget->object);


            DataDocument prevData; prevData = prevName;
            DataDocument newData; newData = mTarget->object->GetName();

            auto action = mmake<PropertyChangeAction>(Vector<Ref<SceneEditableObject>>{ mTarget->object }, "name", 
                                                      Vector<DataDocument>{ prevData }, Vector<DataDocument>{ newData });
            o2EditorSceneWindow.DoneAction(action);
        }
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::DrawOrderTree>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::DrawOrderTreeNode>);
// --- META ---

ENUM_META(Editor::DrawOrderTree::OrderTreeNode::Type, Editor__DrawOrderTree__OrderTreeNode__Type)
{
    ENUM_ENTRY(Actor);
    ENUM_ENTRY(Camera);
    ENUM_ENTRY(Drawable);
    ENUM_ENTRY(EndOfBatch);
    ENUM_ENTRY(Layer);
    ENUM_ENTRY(Root);
}
END_ENUM_META;

DECLARE_CLASS(Editor::DrawOrderTree, Editor__DrawOrderTree);

DECLARE_CLASS(Editor::DrawOrderTreeNode, Editor__DrawOrderTreeNode);
// --- END META ---
