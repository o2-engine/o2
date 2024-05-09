#include "o2Editor/stdafx.h"
#include "SceneHierarchyTree.h"

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
#include "o2Editor/AssetsWindow/AssetIcon.h"
#include "o2Editor/AssetsWindow/AssetsIconsScroll.h"
#include "o2Editor/AssetsWindow/AssetsWindow.h"
#include "o2Editor/Core/Actions/Enable.h"
#include "o2Editor/Core/Actions/Lock.h"
#include "o2Editor/Core/Actions/PropertyChange.h"
#include "o2Editor/Core/Actions/Reparent.h"
#include "o2Editor/Core/EditorApplication.h"
#include "o2Editor/Core/Properties/Basic/ActorProperty.h"
#include "o2Editor/Core/Properties/Basic/ComponentProperty.h"
#include "o2Editor/Core/UIRoot.h"

namespace Editor
{
	SceneHierarchyTree::SceneHierarchyTree(RefCounter* refCounter) :
		Tree(refCounter), mAttachedToSceneEvents(false), mDragActorPropertyField(nullptr), mDragComponentPropertyField(nullptr)
	{
		mNodeWidgetSample = mmake<SceneHierarchyTreeNode>();
		mNodeWidgetSample->layout->minHeight = 20;
		mNodeWidgetSample->AddLayer("caption", nullptr);

		Initialize();
	}

	SceneHierarchyTree::SceneHierarchyTree(RefCounter* refCounter, const SceneHierarchyTree& other) :
		Tree(refCounter, other), mAttachedToSceneEvents(false), mDragActorPropertyField(nullptr), mDragComponentPropertyField(nullptr)
	{
		Initialize();
	}

	SceneHierarchyTree::~SceneHierarchyTree()
	{
		DeattachFromSceneEvents();
	}

	SceneHierarchyTree& SceneHierarchyTree::operator=(const SceneHierarchyTree& other)
	{
		Tree::operator=(other);
		return *this;
	}

	void SceneHierarchyTree::AttachToSceneEvents()
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

	void SceneHierarchyTree::DeattachFromSceneEvents()
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

	Ref<TreeNode> SceneHierarchyTree::GetNode(const Ref<SceneEditableObject>& object)
	{
		return Tree::GetNode((void*)object.Get());
	}

	Vector<Ref<SceneEditableObject>> SceneHierarchyTree::GetSelectedObjects() const
	{
		return Tree::GetSelectedObjects().Convert<Ref<SceneEditableObject>>([](auto x) { return Ref((SceneEditableObject*)x); });
	}

	void SceneHierarchyTree::SetSelectedObjects(const Vector<Ref<SceneEditableObject>>& objects)
	{
		Tree::SetSelectedObjects(objects.Convert<void*>([](auto& x) { return (void*)x.Get(); }));
	}

	void SceneHierarchyTree::SelectObject(const Ref<SceneEditableObject>& object)
	{
		Tree::SelectObject((void*)object.Get());
	}

	void SceneHierarchyTree::SelectAndHighlightObject(const Ref<SceneEditableObject>& object)
	{
		Tree::SelectAndHighlightObject((void*)object.Get());
	}

	void SceneHierarchyTree::ScrollToAndHighlight(const Ref<SceneEditableObject>& object)
	{
		Tree::ScrollToAndHighlight(object.Get());
	}

	void SceneHierarchyTree::DeselectObject(const Ref<SceneEditableObject>& object)
	{
		Tree::DeselectObject((void*)object.Get());
	}

	void SceneHierarchyTree::DeselectAllObjects()
	{
		Tree::DeselectAllObjects();
	}

	void SceneHierarchyTree::ScrollTo(const Ref<SceneEditableObject>& object)
	{
		Tree::ScrollTo((void*)object.Get());
	}

	void SceneHierarchyTree::SetEditorWatching(bool watching)
	{
		mWatchEditor = watching;
		UpdateNodesView();
	}

	bool SceneHierarchyTree::IsEditorWatching() const
	{
		return mWatchEditor;
	}

	String SceneHierarchyTree::GetCreateMenuCategory()
	{
		return "UI/Editor";
	}

	void SceneHierarchyTree::Initialize()
	{
		mEnableTogglesGroup = mmake<ToggleGroup>(ToggleGroup::Type::VerOneClick);
		mEnableTogglesGroup->onReleased = THIS_FUNC(EnableObjectsGroupReleased);

		mLockTogglesGroup = mmake<ToggleGroup>(ToggleGroup::Type::VerOneClick);
		mLockTogglesGroup->onReleased = THIS_FUNC(LockObjectsGroupReleased);

		auto objectNodeWidgetSample = DynamicCast<SceneHierarchyTreeNode>(mNodeWidgetSample);
		objectNodeWidgetSample->InitializeControls();
	}

	void SceneHierarchyTree::UpdateVisibleNodes()
	{
		PushEditorScopeOnStack scope;
		Tree::UpdateVisibleNodes();
	}

	Ref<TreeNode> SceneHierarchyTree::CreateTreeNodeWidget()
	{
		PushEditorScopeOnStack scope;
		return Tree::CreateTreeNodeWidget();
	}

	void* SceneHierarchyTree::GetObjectParent(void* object)
	{
		SceneEditableObject* sceneObject = (SceneEditableObject*)object;
		return sceneObject->GetEditableParent().Get();
	}

	Vector<void*> SceneHierarchyTree::GetObjectChilds(void* object)
	{
		if (object)
		{
			SceneEditableObject* parent = (SceneEditableObject*)object;
			return parent->GetEditableChildren().Convert<void*>([](auto& x) { return x.Get(); });
		}

		if (mWatchEditor)
			return EditorUIRoot.GetRootWidget()->GetEditableChildren().Convert<void*>([](auto& x) { return x.Get(); });

		return o2Scene.GetRootActors().Convert<void*>([](const Ref<Actor>& x) { return DynamicCast<SceneEditableObject>(x).Get(); });
	}

	String SceneHierarchyTree::GetObjectDebug(void* object)
	{
		return object ? ((SceneEditableObject*)object)->GetName() : String("null");
	}

	void SceneHierarchyTree::FillNodeDataByObject(const Ref<TreeNode>& nodeWidget, void* object)
	{
		auto node = DynamicCast<SceneHierarchyTreeNode>(nodeWidget);
		node->SetSceneObject(Ref((SceneEditableObject*)object));
		node->mLockToggle->SetToggleGroup(mLockTogglesGroup);
		node->mEnableToggle->SetToggleGroup(mEnableTogglesGroup);
	}

	void SceneHierarchyTree::OnNodeDblClick(const Ref<TreeNode>& nodeWidget)
	{
		DynamicCast<SceneHierarchyTreeNode>(nodeWidget)->EnableEditName();
	}

#undef GetObject

	void SceneHierarchyTree::OnDraggedObjects(Vector<void*> objects, void* newParent, void* prevObject)
	{
		auto newParentEditableObject = Ref((SceneEditableObject*)newParent);
		auto prevEditableObject = Ref((SceneEditableObject*)prevObject);
		auto editableObjects = objects.Convert<Ref<SceneEditableObject>>([](void* x) { return Ref((SceneEditableObject*)x); });

		auto action = mmake<ReparentAction>(editableObjects);

		o2Scene.ReparentEditableObjects(editableObjects, newParentEditableObject, prevEditableObject);

		action->ObjectsReparented(mParent.Lock(), prevEditableObject);
		o2EditorApplication.DoneAction(action);

		Tree::OnDraggedObjects(objects, newParent, prevObject);
	}

	void SceneHierarchyTree::EnableObjectsGroupPressed(bool value)
	{}

	void SceneHierarchyTree::EnableObjectsGroupReleased(bool value)
	{
		Vector<Ref<SceneEditableObject>> objects = mEnableTogglesGroup->GetToggled().Convert<Ref<SceneEditableObject>>(
			[](const WeakRef<Toggle>& x) {
				auto toggleParent = DynamicCast<TreeNode>(x.Lock()->GetParent().Lock());
				return Ref(static_cast<SceneEditableObject*>(toggleParent->GetObject()));
			});

		auto action = mmake<EnableAction>(objects, value);
		o2EditorApplication.DoneAction(action);
	}

	void SceneHierarchyTree::LockObjectsGroupPressed(bool value)
	{}

	void SceneHierarchyTree::LockObjectsGroupReleased(bool value)
	{
		Vector<Ref<SceneEditableObject>> objects = mLockTogglesGroup->GetToggled().Convert<Ref<SceneEditableObject>>(
			[](const WeakRef<Toggle>& x) {
				auto toggleParent = DynamicCast<TreeNode>(x.Lock()->GetParent().Lock());
				return Ref(static_cast<SceneEditableObject*>(toggleParent->GetObject()));
			});

		auto action = mmake<LockAction>(objects, value);
		o2EditorApplication.DoneAction(action);
	}

	void SceneHierarchyTree::OnNodesSelectionChanged(Vector<void*> objects)
	{
		onObjectsSelectionChanged(objects.Convert<Ref<SceneEditableObject>>([](auto x) { return Ref((SceneEditableObject*)x); }));
		Tree::OnNodesSelectionChanged(objects);
	}

	void SceneHierarchyTree::OnDragEnter(const Ref<ISelectableDragableObjectsGroup>& group)
	{
		auto assetsScroll = DynamicCast<AssetsIconsScrollArea>(group);
		if (assetsScroll)
		{
			assetsScroll->InstantiateDraggingAssets();

			if (!assetsScroll->mInstantiatedSceneDragObjects.IsEmpty())
			{
				o2Scene.UpdateAddedEntities();

				UpdateNodesView(true);
				SetLayoutDirty();

				for (auto& object : assetsScroll->mInstantiatedSceneDragObjects)
				{
					int idx = mAllNodes.IndexOf([=](auto& x) { return x->object == object.Get(); });
					CreateVisibleNodeWidget(mAllNodes[idx], idx);
				}

				Focus();
				SetSelectedObjects(assetsScroll->mInstantiatedSceneDragObjects);
				BeginDragging(GetNode(assetsScroll->mInstantiatedSceneDragObjects.Last()));
				mDragOffset = Vec2F();
			}
		}
		else Tree::OnDragEnter(group);
	}

	void SceneHierarchyTree::OnDragExit(const Ref<ISelectableDragableObjectsGroup>& group)
	{
		auto assetsScroll = DynamicCast<AssetsIconsScrollArea>(group);
		if (assetsScroll)
		{
			DeselectAllObjects();
			EndDragging();
			assetsScroll->ClearInstantiatedDraggingAssets();
			assetsScroll->Focus();
		}
		else Tree::OnDragExit(group);
	}

	void SceneHierarchyTree::OnDraggedAbove(const Ref<ISelectableDragableObjectsGroup>& group)
	{
		auto assetsScroll = DynamicCast<AssetsIconsScrollArea>(group);
		if (assetsScroll)
		{
			UpdateDraggingGraphics();
			Tree::OnDraggedAbove(Ref(this));
		}
		else Tree::OnDraggedAbove(group);
	}

	void SceneHierarchyTree::OnDropped(const Ref<ISelectableDragableObjectsGroup>& group)
	{
		auto assetsScroll = DynamicCast<AssetsIconsScrollArea>(group);
		if (assetsScroll)
		{
			Tree::OnDropped(Ref(this));

			assetsScroll->RegObjectsCreationAction();
			assetsScroll->mInstantiatedSceneDragObjects.Clear();
		}
		else Tree::OnDropped(group);
	}

	void SceneHierarchyTree::OnObjectCreated(const Ref<SceneEditableObject>& object)
	{
		Tree::OnObjectCreated(object.Get(), object->GetEditableParent().Get());
	}

	void SceneHierarchyTree::OnObjectDestroing(const Ref<SceneEditableObject>& object)
	{
		Tree::OnObjectRemoved(object.Get());
	}

	void SceneHierarchyTree::OnObjectsChanged(const Vector<Ref<SceneEditableObject>>& objects)
	{
        Tree::OnObjectsChanged(objects.Convert<void*>([](auto& x) { return x.Get(); }));
	}

	void SceneHierarchyTree::OnObjectChanged(const Ref<SceneEditableObject>& object)
	{
		Tree::OnObjectsChanged({ object.Get() });
	}

	SceneHierarchyTreeNode::SceneHierarchyTreeNode(RefCounter* refCounter) :
		RefCounterable(refCounter), TreeNode(refCounter)
	{}

	SceneHierarchyTreeNode::SceneHierarchyTreeNode(RefCounter* refCounter, const SceneHierarchyTreeNode& other) :
		RefCounterable(refCounter), TreeNode(refCounter, other)
	{
		InitializeControls();
	}

	Editor::SceneHierarchyTreeNode& SceneHierarchyTreeNode::operator=(const SceneHierarchyTreeNode& other)
	{
		TreeNode::operator=(other);
		InitializeControls();
		return *this;
	}

	void SceneHierarchyTreeNode::OnDeserialized(const DataValue& node)
	{
		TreeNode::OnDeserialized(node);
		InitializeControls();
	}

	void SceneHierarchyTreeNode::InitializeControls()
	{
		mNameDrawable = GetLayerDrawable<Text>("name");
		mLockToggle = GetChildByType<Toggle>("lockToggle");
		mEnableToggle = GetChildByType<Toggle>("enableToggle");
		mLinkBtn = GetChildByType<Button>("linkBtn");
		mNameEditBox = GetChildByType<EditBox>("nameEditBox");
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

	void SceneHierarchyTreeNode::SetSceneObject(const Ref<SceneEditableObject>& object)
	{
		mTargetObject = object;

		if (mName != object->GetName())
		{
			mName = object->GetName();
			mNameDrawable->SetText(mName);
		}

		float alpha = object->IsEnabledInHierarchy() ? 1.0f : 0.5f;
		if (!Math::Equals(alpha, mNameDrawable->GetTransparency()))
		{
			mNameDrawable->SetTransparency(alpha);
			mEnableToggle->SetTransparency(alpha);
			mLinkBtn->SetTransparency(alpha);
		}

		if (auto actor = DynamicCast<Actor>(object))
		{
			mLinkBtn->SetEnabled(actor->GetPrototype());
			mLinkBtnHalfHideState->SetState(!actor->GetPrototypeDirectly().IsValid());

			if (actor->GetPrototype())
			{
				mLinkBtn->onClick = [=]() {
					auto proto = actor->GetPrototype();
					o2EditorAssets.ShowAssetIcon(proto->GetPath());
				};
			}
		}
		else mLinkBtn->SetEnabled(false);

		if (object->IsSupportsDisabling())
		{
			mEnableToggle->SetValue(object->IsEnabled());
			mEnableToggle->SetEnabled(true);
		}
		else mEnableToggle->SetEnabled(false);

		mLockToggle->SetValue(object->IsLocked());
		mLockToggleLockedState->SetState(object->IsLockedInHierarchy());
		mLockToggleHalfHideState->SetState(object->IsLockedInHierarchy() && !object->IsLocked());
	}

	void SceneHierarchyTreeNode::EnableEditName()
	{
		mEditState->SetState(true);

		mNameEditBox->text = mTargetObject->GetName();
		mNameEditBox->SelectAll();
		mNameEditBox->Widget::Focus();
		mNameEditBox->ResetScroll();
	}

	String SceneHierarchyTreeNode::GetCreateMenuCategory()
	{
		return "UI/Editor";
	}

	void SceneHierarchyTreeNode::OnLockClicked()
	{
		mTargetObject->SetLocked(mLockToggle->GetValue());
	}

	void SceneHierarchyTreeNode::OnEnableCkicked()
	{
		mTargetObject->SetEnabled(mEnableToggle->GetValue());
	}

	void SceneHierarchyTreeNode::OnObjectNameChanged(const WString& text)
	{
		String prevName = mTargetObject->GetName();

		mTargetObject->SetName(text);
		mEditState->SetState(false);
		DynamicCast<SceneHierarchyTree>(mOwnerTree.Lock())->OnObjectChanged(mTargetObject);


		DataDocument prevData; prevData = prevName;
		DataDocument newData; newData = mTargetObject->GetName();

		auto action = new PropertyChangeAction({ mTargetObject }, "name", { prevData }, { newData });
		o2EditorApplication.DoneAction(Ref(action));
	}
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::SceneHierarchyTree>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::SceneHierarchyTreeNode>);
// --- META ---

DECLARE_CLASS(Editor::SceneHierarchyTree, Editor__SceneHierarchyTree);

DECLARE_CLASS(Editor::SceneHierarchyTreeNode, Editor__SceneHierarchyTreeNode);
// --- END META ---
