#include "o2Editor/stdafx.h"
#include "SceneEditScreen.h"

#include "o2/Physics/PhysicsWorld.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/DrawableComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Tree.h"
#include "o2/Utils/Math/Math.h"
#include "o2Editor/AssetsWindow/AssetsIconsScroll.h"
#include "o2Editor/Core/Actions/Select.h"
#include "o2Editor/Core/EditorApplication.h"
#include "o2Editor/Core/Tools/FrameTool.h"
#include "o2Editor/Core/Tools/IEditorTool.h"
#include "o2Editor/Core/Tools/MoveTool.h"
#include "o2Editor/Core/Tools/RotateTool.h"
#include "o2Editor/Core/Tools/ScaleTool.h"
#include "o2Editor/Core/Tools/SelectionTool.h"
#include "o2Editor/Core/ToolsPanel.h"
#include "o2Editor/Core/UIRoot.h"
#include "o2Editor/Core/WindowsSystem/WindowsManager.h"
#include "o2Editor/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/SceneWindow/SceneDragHandle.h"
#include "o2Editor/SceneWindow/SceneEditorLayer.h"
#include "o2Editor/TreeWindow/SceneTree.h"
#include "o2Editor/TreeWindow/TreeWindow.h"

DECLARE_SINGLETON(Editor::SceneEditScreen);

namespace Editor
{
	SceneEditScreen::SceneEditScreen()
	{
		InitializeTools();
		SelectTool<MoveTool>();

		mLeftTopWidgetsContainer = InitializeWidgetsContainer(BaseCorner::LeftTop);
		mRightTopWidgetsContainer = InitializeWidgetsContainer(BaseCorner::RightTop);
		mLeftBottomWidgetsContainer = InitializeWidgetsContainer(BaseCorner::LeftBottom);
		mRightBottomWidgetsContainer = InitializeWidgetsContainer(BaseCorner::RightBottom);
	}

	SceneEditScreen::SceneEditScreen(const SceneEditScreen& other):
		SceneEditScreen()
	{}

	SceneEditScreen::~SceneEditScreen()
	{
		for (auto tool : mTools)
			delete tool;
	}

	void SceneEditScreen::Draw()
	{
		if (!mReady)
			return;

		if (mNeedRedraw || (mEnabledTool && mEnabledTool->mNeedRedraw) || true)
			RedrawRenderTarget();

		mRenderTargetSprite->Draw();

		CursorAreaEventsListener::OnDrawn();

		if (mEnabledTool)
			mEnabledTool->DrawScreen();

		for (auto handle : mDragHandles)
			handle->Draw();

		mEditorLayers.SortBy<int>([](SceneEditorLayer* l) { return l->GetOrder(); });

		for (auto layer : mEditorLayers)
		{
			if (layer->IsEnabled() && IsLayerEnabled(layer->GetName()))
				layer->DrawOverScene();
		}

		Widget::Draw();
	}

	void SceneEditScreen::NeedRedraw()
	{
		mNeedRedraw = true;
	}

#undef DrawText

	void SceneEditScreen::Update(float dt)
	{
		Widget::Update(dt);

		UpdateCamera(dt);
		o2Scene.CheckChangedObjects();

		for (auto layer : mEditorLayers)
		{
			if (layer->IsEnabled() && IsLayerEnabled(layer->GetName()))
				layer->Update(dt);
		}

		for (auto tool : mTools)
			tool->Update(dt);
	}

	bool SceneEditScreen::IsScrollable() const
	{
		return true;
	}

	Vec2F SceneEditScreen::ScreenToScenePoint(const Vec2F& point)
	{
		return ScreenToLocalPoint(point);
	}

	Vec2F SceneEditScreen::SceneToScreenPoint(const Vec2F& point)
	{
		return LocalToScreenPoint(point);
	}

	Vec2F SceneEditScreen::ScreenToSceneVector(const Vec2F& point)
	{
		return point*mViewCamera.GetScale();
	}

	Vec2F SceneEditScreen::SceneToScreenVector(const Vec2F& point)
	{
		return point / mViewCamera.GetScale();
	}

	void SceneEditScreen::InitializeTools()
	{
		mTools.Add(mnew SelectionTool());
		mTools.Add(mnew MoveTool());
		mTools.Add(mnew RotateTool());
		mTools.Add(mnew ScaleTool());
		mTools.Add(mnew FrameTool());
	}

	HorizontalLayout* SceneEditScreen::InitializeWidgetsContainer(BaseCorner baseCorner)
	{
		auto controlsWidget = mnew HorizontalLayout();
		*controlsWidget->layout = WidgetLayout::BothStretch();
		controlsWidget->baseCorner = baseCorner;
		controlsWidget->spacing = 5;
		controlsWidget->expandHeight = false;
		controlsWidget->expandWidth = false;
		controlsWidget->border = BorderF(5, 5, 5, 5);
		controlsWidget->layout->pivot = Vec2F(1, 1);
		AddInternalWidget(controlsWidget);

		return controlsWidget;
	}

	bool SceneEditScreen::IsHandleWorking(const Input::Cursor& cursor) const
	{
		return false;
	}

	void SceneEditScreen::OnObjectsSelectedFromThis()
	{
		mSelectedFromThis = true;
		mSceneTree->SetSelectedObjects(mSelectedObjects);

		if (mEnabledTool)
			mEnabledTool->OnObjectsSelectionChanged(mSelectedObjects);

		onSelectionChanged(mSelectedObjects);
		o2EditorPropertiesWindow.SetTargets(mSelectedObjects.Convert<IObject*>([](auto x) { return (IObject*)x; }));
	}

	void SceneEditScreen::RedrawContent()
	{
		DrawGrid();
 		DrawObjects();
		DrawSelection();

		mEditorLayers.SortBy<int>([](SceneEditorLayer* l) { return l->GetOrder(); });

		for (auto layer : mEditorLayers)
		{
			if (layer->IsEnabled() && IsLayerEnabled(layer->GetName()))
				layer->DrawScene();
		}

		if (mEnabledTool)
		{
			mEnabledTool->DrawScene();
			mEnabledTool->mNeedRedraw = false;
		}
	}

	void SceneEditScreen::DrawObjects()
	{
		if (o2EditorTree.GetSceneTree()->IsEditorWatching())
		{
			static bool drawing = false;
			if (drawing)
				return;
			
			drawing = true;
			EditorUIRoot.GetRootWidget()->Draw();
			drawing = false;

			mNeedRedraw = true;
		}
		else
		{
			o2Scene.BeginDrawingScene();

			for (auto layer : o2Scene.GetLayers())
			{
				if (!layer->visible)
					continue;

				for (auto drw : layer->GetEnabledDrawables())
					drw->Draw();
			}

			o2Scene.EndDrawingScene();

			o2Physics.DrawDebug();
		}
	}

	void SceneEditScreen::DrawSelection()
	{
		if (mSelectedObjects.Count() == 1)
		{
			DrawObjectSelection(mSelectedObjects[0], mSelectedObjectColor);
		}
		else
		{
			for (auto object : mSelectedObjects)
				DrawObjectSelection(object, mMultiSelectedObjectColor);
		}
	}

	void SceneEditScreen::DrawObjectSelection(SceneEditableObject* object, const Color4& color)
	{
		o2Render.DrawAABasis(object->GetTransform(), color, color, color);
		// 
		// 	auto bs = actor->transform->GetWorldNonSizedBasis();
		// 	o2Render.DrawLine(bs.offs, bs.offs + bs.xv*100.0f);
		// 	o2Render.DrawLine(bs.offs, bs.offs + bs.yv*100.0f);
	}

	void SceneEditScreen::SelectObjects(Vector<SceneEditableObject*> objects, bool additive /*= true*/)
	{
		auto prevSelectedObjects = mSelectedObjects;

		SelectObjectsWithoutAction(objects, additive);

		if (mSelectedObjects != prevSelectedObjects)
		{
			auto selectionAction = mnew SelectAction(mSelectedObjects, prevSelectedObjects);
			o2EditorApplication.DoneAction(selectionAction);
		}
	}

	void SceneEditScreen::SelectObject(SceneEditableObject* actor, bool additive /*= true*/)
	{
		auto prevSelectedObjects = mSelectedObjects;

		SelectObjectWithoutAction(actor, additive);

		if (mSelectedObjects != prevSelectedObjects)
		{
			auto selectionAction = mnew SelectAction(mSelectedObjects, prevSelectedObjects);
			o2EditorApplication.DoneAction(selectionAction);
		}
	}

	void SceneEditScreen::SelectAllObjects()
	{
		auto prevSelectedObjects = mSelectedObjects;

		mSelectedObjects.Clear();
		for (auto layer : o2Scene.GetLayers())
		{
			mSelectedObjects.Add(layer->GetEnabledActors().
								 FindAll([](auto x) { return !x->IsLockedInHierarchy(); }).
								 Convert<SceneEditableObject*>([](auto x) { return dynamic_cast<SceneEditableObject*>(x); }));
		}

		mNeedRedraw = true;
		OnObjectsSelectedFromThis();

		if (mSelectedObjects != prevSelectedObjects)
		{
			auto selectionAction = mnew SelectAction(mSelectedObjects, prevSelectedObjects);
			o2EditorApplication.DoneAction(selectionAction);
		}
	}

	void SceneEditScreen::ClearSelection()
	{
		auto prevSelectedObjects = mSelectedObjects;

		ClearSelectionWithoutAction();

		if (mSelectedObjects != prevSelectedObjects)
		{
			auto selectionAction = mnew SelectAction(mSelectedObjects, prevSelectedObjects);
			o2EditorApplication.DoneAction(selectionAction);
		}
	}

	HorizontalLayout* SceneEditScreen::GetLeftTopWidgetsContainer()
	{
		return mLeftTopWidgetsContainer;
	}

	HorizontalLayout* SceneEditScreen::GetRightTopWidgetsContainer()
	{
		return mRightTopWidgetsContainer;
	}

	HorizontalLayout* SceneEditScreen::GetLeftBottomWidgetsContainer()
	{
		return mLeftBottomWidgetsContainer;
	}

	HorizontalLayout* SceneEditScreen::GetRightBottomWidgetsContainer()
	{
		return mRightBottomWidgetsContainer;
	}

	void SceneEditScreen::AddEditorLayer(SceneEditorLayer* layer)
	{
		mEditorLayers.Add(layer);
	}

	void SceneEditScreen::RemoveEditorLayer(SceneEditorLayer* layer)
	{
		mEditorLayers.Remove(layer);
	}

	void SceneEditScreen::SetLayerEnabled(const String& name, bool enabled)
	{
		mEditorLayersEnabled[name] = enabled;
	}

	bool SceneEditScreen::IsLayerEnabled(const String& name) const
	{
		bool enabled = true;
		mEditorLayersEnabled.TryGetValue(name, enabled);
		return enabled;
	}

	void SceneEditScreen::SelectTool(const IEditTool* tool)
	{
		if (tool == mEnabledTool)
			return;

		if (mEnabledTool)
			mEnabledTool->OnDisabled();

		mEnabledTool = const_cast<IEditTool*>(tool);
		if (auto toggle = mEnabledTool->GetPanelToggle())
			toggle->SetValue(true);

		if (mEnabledTool)
			mEnabledTool->OnEnabled();
	}

	IEditTool* SceneEditScreen::GetSelectedTool() const
	{
		return mEnabledTool;
	}

	void SceneEditScreen::AddTool(IEditTool* tool)
	{
		mTools.Add(tool);
		o2EditorTools.AddToolToggle(tool->GetPanelToggle());
	} 

	void SceneEditScreen::RemoveTool(IEditTool* tool)
	{
		if (tool == mEnabledTool)
		{
			if (!mTools.IsEmpty())
				SelectTool(mTools[0]);
			else
				SelectTool(nullptr);
		}

		mTools.Remove(tool);
		o2EditorTools.RemoveToolToggle(tool->GetPanelToggle());
	}

	const Vector<IEditTool*>& SceneEditScreen::GetTools() const
	{
		return mTools;
	}

	const Vector<SceneEditableObject*>& SceneEditScreen::GetSelectedObjects() const
	{
		return mSelectedObjects;
	}

	const Vector<SceneEditableObject*>& SceneEditScreen::GetTopSelectedObjects() const
	{
		return mTopSelectedObjects;
	}

	const Color4& SceneEditScreen::GetSingleObjectSelectionColor() const
	{
		return mSelectedObjectColor;
	}

	const Color4& SceneEditScreen::GetManyObjectsSelectionColor() const
	{
		return mMultiSelectedObjectColor;
	}

	bool SceneEditScreen::IsUnderPoint(const Vec2F& point)
	{
		return Widget::IsUnderPoint(point);
	}

	void SceneEditScreen::BindSceneTree()
	{
		mSceneTree = o2EditorWindows.GetWindow<TreeWindow>()->GetSceneTree();

		mSceneTree->onObjectsSelectionChanged += THIS_FUNC(OnTreeSelectionChanged);

		o2Scene.onObjectsChanged += Function<void(Vector<SceneEditableObject*>)>(this, &SceneEditScreen::OnSceneChanged);
	}

	void SceneEditScreen::OnTreeSelectionChanged(Vector<SceneEditableObject*> selectedObjects)
	{
		if (mSelectedFromThis)
		{
			mSelectedFromThis = false;
			return;
		}

		auto prevSelectedObjects = mSelectedObjects;

		mSelectedObjects = selectedObjects;
		mNeedRedraw = true;

		UpdateTopSelectedObjects();

		if (mEnabledTool)
			mEnabledTool->OnObjectsSelectionChanged(mSelectedObjects);

		auto selectedIObjects = mSelectedObjects.Convert<IObject*>([](auto x) { return dynamic_cast<IObject*>(x); });

		if (mSelectedObjects != prevSelectedObjects || 
			selectedIObjects != o2EditorPropertiesWindow.GetTargets())
		{
			auto selectionAction = mnew SelectAction(mSelectedObjects, prevSelectedObjects);
			o2EditorApplication.DoneAction(selectionAction);

			onSelectionChanged(mSelectedObjects);
			o2EditorPropertiesWindow.SetTargets(selectedIObjects);
		}
	}

	void SceneEditScreen::UpdateTopSelectedObjects()
	{
		mTopSelectedObjects.Clear();
		for (auto object : mSelectedObjects)
		{
			bool processing = true;

			SceneEditableObject* parent = object->GetEditableParent();
			while (parent)
			{
				if (mSelectedObjects.Contains([&](auto x) { return parent == x; }))
				{
					processing = false;
					break;
				}

				parent = parent->GetEditableParent();
			}

			if (processing)
				mTopSelectedObjects.Add(object);
		}
	}

	void SceneEditScreen::OnSceneChanged(Vector<SceneEditableObject*> actors)
	{
		mNeedRedraw = true;

		if (mEnabledTool)
			mEnabledTool->OnSceneChanged(actors);
	}

	void SceneEditScreen::OnSceneChanged()
	{
		mNeedRedraw = true;
	}

	void SceneEditScreen::ClearSelectionWithoutAction(bool sendSelectedMessage /*= true*/)
	{
		mSelectedObjects.Clear();
		mTopSelectedObjects.Clear();
		mNeedRedraw = true;

		if (sendSelectedMessage)
			OnObjectsSelectedFromThis();
	}

	void SceneEditScreen::SelectObjectsWithoutAction(Vector<SceneEditableObject*> objects, bool additive /*= true*/)
	{
		if (!additive)
			mSelectedObjects.Clear();

		mSelectedObjects.Add(objects);
		mNeedRedraw = true;

		UpdateTopSelectedObjects();
		OnObjectsSelectedFromThis();
	}

	void SceneEditScreen::SelectObjectWithoutAction(SceneEditableObject* object, bool additive /*= true*/)
	{
		if (!additive)
			mSelectedObjects.Clear();

		mSelectedObjects.Add(object);
		mNeedRedraw = true;

		UpdateTopSelectedObjects();
		OnObjectsSelectedFromThis();
	}

	void SceneEditScreen::OnDropped(ISelectableDragableObjectsGroup* group)
	{
		auto assetsScroll = dynamic_cast<AssetsIconsScrollArea*>(group);
		if (!assetsScroll)
			return;

		assetsScroll->RegObjectsCreationAction();
		
		o2UI.FocusWidget(o2EditorTree.GetSceneTree());
		o2EditorTree.GetSceneTree()->SetSelectedObjects(assetsScroll->mInstantiatedSceneDragObjects);
		
		assetsScroll->mInstantiatedSceneDragObjects.Clear();

		o2Application.SetCursor(CursorType::Arrow);
	}

	void SceneEditScreen::OnDragEnter(ISelectableDragableObjectsGroup* group)
	{
		auto assetsScroll = dynamic_cast<AssetsIconsScrollArea*>(group);
		if (!assetsScroll)
			return;

		assetsScroll->InstantiateDraggingAssets();
		if (assetsScroll->mInstantiatedSceneDragObjects.Count() > 0)
			o2Application.SetCursor(CursorType::Hand);
	}

	void SceneEditScreen::OnDraggedAbove(ISelectableDragableObjectsGroup* group)
	{
		auto assetsScroll = dynamic_cast<AssetsIconsScrollArea*>(group);
		if (!assetsScroll)
			return;

		for (auto object : assetsScroll->mInstantiatedSceneDragObjects)
		{
			object->UpdateTransform();
			Basis transform = object->GetTransform();
			transform.origin = ScreenToScenePoint(o2Input.cursorPos) - (transform.xv + transform.yv)*0.5f;
			object->SetTransform(transform);
		}
	}

	void SceneEditScreen::OnDragExit(ISelectableDragableObjectsGroup* group)
	{
		auto assetsScroll = dynamic_cast<AssetsIconsScrollArea*>(group);
		if (!assetsScroll)
			return;

		assetsScroll->ClearInstantiatedDraggingAssets();
		o2Application.SetCursor(CursorType::Arrow);

		mNeedRedraw = true;
	}

	bool SceneEditScreen::IsInputTransparent() const
	{
		return true;
	}

	void SceneEditScreen::OnScrolled(float scroll)
	{
		ScrollView::OnScrolled(scroll);

		if (mEnabledTool)
			mEnabledTool->OnScrolled(scroll);
	}

	void SceneEditScreen::OnKeyPressed(const Input::Key& key)
	{
		if (mEnabledTool)
			mEnabledTool->OnKeyPressed(key);
	}

	void SceneEditScreen::OnKeyReleased(const Input::Key& key)
	{
		if (mEnabledTool)
			mEnabledTool->OnKeyReleased(key);
	}

	void SceneEditScreen::OnKeyStayDown(const Input::Key& key)
	{
		if (mEnabledTool)
			mEnabledTool->OnKeyStayDown(key);
	}

	void SceneEditScreen::OnCursorPressed(const Input::Cursor& cursor)
	{
		o2UI.FocusWidget(mSceneTree);

		if (mEnabledTool && !IsHandleWorking(cursor))
			mEnabledTool->OnCursorPressed(cursor);
	}

	void SceneEditScreen::OnCursorReleased(const Input::Cursor& cursor)
	{
		if (mEnabledTool && !IsHandleWorking(cursor))
			mEnabledTool->OnCursorReleased(cursor);
	}

	void SceneEditScreen::OnCursorPressBreak(const Input::Cursor& cursor)
	{
		if (mEnabledTool && !IsHandleWorking(cursor))
			mEnabledTool->OnCursorPressBreak(cursor);
	}

	void SceneEditScreen::OnCursorStillDown(const Input::Cursor& cursor)
	{
		if (mEnabledTool && !IsHandleWorking(cursor))
			mEnabledTool->OnCursorStillDown(cursor);
	}

	void SceneEditScreen::OnCursorMoved(const Input::Cursor& cursor)
	{
		if (mEnabledTool && !IsHandleWorking(cursor))
			mEnabledTool->OnCursorMoved(cursor);
	}

	void SceneEditScreen::OnCursorEnter(const Input::Cursor& cursor)
	{
		if (mEnabledTool && !IsHandleWorking(cursor))
			mEnabledTool->OnCursorEnter(cursor);
	}

	void SceneEditScreen::OnCursorExit(const Input::Cursor& cursor)
	{
		if (mEnabledTool && !IsHandleWorking(cursor))
			mEnabledTool->OnCursorExit(cursor);
	}

	void SceneEditScreen::OnCursorRightMousePressed(const Input::Cursor& cursor)
	{
		o2UI.FocusWidget(mSceneTree);

		if (mEnabledTool && !IsHandleWorking(cursor))
			mEnabledTool->OnCursorRightMousePressed(cursor);

		ScrollView::OnCursorRightMousePressed(cursor);
	}

	void SceneEditScreen::OnCursorRightMouseStayDown(const Input::Cursor& cursor)
	{
		ScrollView::OnCursorRightMouseStayDown(cursor);

		if (mEnabledTool)
			mEnabledTool->OnCursorRightMouseStayDown(cursor);
	}

	void SceneEditScreen::OnCursorRightMouseReleased(const Input::Cursor& cursor)
	{
		if (mEnabledTool)
			mEnabledTool->OnCursorRightMouseReleased(cursor);

		ScrollView::OnCursorRightMouseReleased(cursor);
	}

	void SceneEditScreen::OnCursorMiddleMousePressed(const Input::Cursor& cursor)
	{
		if (mEnabledTool)
			mEnabledTool->OnCursorMiddleMousePressed(cursor);
	}

	void SceneEditScreen::OnCursorMiddleMouseStayDown(const Input::Cursor& cursor)
	{
		if (mEnabledTool)
			mEnabledTool->OnCursorMiddleMouseStayDown(cursor);
	}

	void SceneEditScreen::OnCursorMiddleMouseReleased(const Input::Cursor& cursor)
	{
		if (mEnabledTool)
			mEnabledTool->OnCursorMiddleMouseReleased(cursor);
	}
}

DECLARE_CLASS(Editor::SceneEditScreen);
