#include "o2Editor/stdafx.h"
#include "SceneEditScreen.h"

#include "o2/Physics/PhysicsWorld.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayerRef.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Tree.h"
#include "o2/Utils/Math/Math.h"
#include "o2Editor/Windows/AssetsWindow/AssetsIconsScroll.h"
#include "o2Editor/Actions/Select.h"
#include "o2Editor/EditorApplication.h"
#include "o2Editor/Tools/FrameTool.h"
#include "o2Editor/Tools/IEditorTool.h"
#include "o2Editor/Tools/MoveTool.h"
#include "o2Editor/Tools/RotateTool.h"
#include "o2Editor/Tools/ScaleTool.h"
#include "o2Editor/Tools/SelectionTool.h"
#include "o2Editor/ToolsPanel.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/WindowsManager.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle.h"
#include "o2Editor/Windows/SceneWindow/SceneEditorLayer.h"
#include "o2Editor/Windows/TreeWindow/DrawOrderTree.h"
#include "o2Editor/Windows/TreeWindow/SceneHierarchyTree.h"
#include "o2Editor/Windows/TreeWindow/TreeWindow.h"
#include "o2Editor/Windows/WindowsManager.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"

DECLARE_SINGLETON(Editor::SceneEditScreen);

namespace Editor
{
    SceneEditScreen::SceneEditScreen(RefCounter* refCounter):
        Singleton<SceneEditScreen>(refCounter), ScrollView(refCounter)
    {
        InitializeTools();
        SelectTool<MoveTool>();

        mLeftTopWidgetsContainer = InitializeWidgetsContainer(BaseCorner::LeftTop);
        mRightTopWidgetsContainer = InitializeWidgetsContainer(BaseCorner::RightTop);
        mLeftBottomWidgetsContainer = InitializeWidgetsContainer(BaseCorner::LeftBottom);
        mRightBottomWidgetsContainer = InitializeWidgetsContainer(BaseCorner::RightBottom);
    }

    SceneEditScreen::SceneEditScreen(RefCounter* refCounter, const SceneEditScreen& other) :
        SceneEditScreen(refCounter)
    {}

    SceneEditScreen::~SceneEditScreen()
    {}

    void SceneEditScreen::Draw()
	{
        ScrollView::Draw();

 		if (mEnabledTool)
 			mEnabledTool->DrawScreen();
 
 		mEditorLayers.SortBy<int>([](auto& l) { return l->GetOrder(); });
 
 		for (auto& layer : mEditorLayers)
 		{
 			if (layer->IsEnabled() && IsLayerEnabled(layer->GetName()))
 				layer->DrawOverScene();
 		}
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

        for (auto& layer : mEditorLayers)
        {
            if (layer->IsEnabled() && IsLayerEnabled(layer->GetName()))
                layer->Update(dt);
        }

        for (auto& tool : mTools)
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
        return point * mViewCamera.GetScale();
    }

    Vec2F SceneEditScreen::SceneToScreenVector(const Vec2F& point)
    {
        return point / mViewCamera.GetScale();
    }

    void SceneEditScreen::InitializeTools()
    {
        mTools.Add(mmake<SelectionTool>());
        mTools.Add(mmake<MoveTool>());
        mTools.Add(mmake<RotateTool>());
        mTools.Add(mmake<ScaleTool>());
        mTools.Add(mmake<FrameTool>());
    }

    Ref<HorizontalLayout> SceneEditScreen::InitializeWidgetsContainer(BaseCorner baseCorner)
    {
        auto controlsWidget = mmake<HorizontalLayout>();
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

        o2EditorTree.SetSelectedObjects(mSelectedObjects);

        if (mEnabledTool)
            mEnabledTool->OnObjectsSelectionChanged(mSelectedObjects);

        onSelectionChanged(mSelectedObjects);
        o2EditorPropertiesWindow.SetTargets(mSelectedObjects.Convert<IObject*>([](auto x) { return (IObject*)x.Get(); }));
    }

    void SceneEditScreen::RedrawContent()
    {
        DrawGrid();
        DrawObjects();
        o2Debug.Draw(false);
        DrawSelection();

        for (auto& layer : mEditorLayers)
        {
            if (layer->IsEnabled() && IsLayerEnabled(layer->GetName()))
                layer->DrawScene();
        }

        if (mEnabledTool)
        {
            mEnabledTool->DrawScene();
            mEnabledTool->mNeedRedraw = false;
		}

		for (auto& handle : mDragHandles)
			handle->Draw();
    }

    void SceneEditScreen::DrawObjects()
    {
        if (o2EditorTree.GetSceneTree()->IsEditorWatching())
        {
            static bool drawing = false;
            if (drawing)
                return;

            drawing = true;
            //EditorUIRoot.GetRootWidget()->Draw();
            drawing = false;

            mNeedRedraw = true;
        }
        else
        {
            o2Scene.BeginDrawingScene();

            for (auto& layer : o2Scene.GetLayers())
            {
                if (!layer->visible)
                    continue;

                for (auto& drawable : layer->GetDrawables()) 
                    drawable->Draw();
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
            for (auto& object : mSelectedObjects)
                DrawObjectSelection(object, mMultiSelectedObjectColor);
        }
    }

    void SceneEditScreen::DrawObjectSelection(const Ref<SceneEditableObject>& object, const Color4& color)
    {
        o2Render.DrawAABasis(object->GetTransform(), color, color, color);
        // 
        //     auto bs = actor->transform->GetWorldNonSizedBasis();
        //     o2Render.DrawLine(bs.offs, bs.offs + bs.xv*100.0f);
        //     o2Render.DrawLine(bs.offs, bs.offs + bs.yv*100.0f);
    }

    void SceneEditScreen::SelectObjects(const Vector<Ref<SceneEditableObject>>& objects, bool additive /*= true*/)
    {
        auto prevSelectedObjects = mSelectedObjects;

        SelectObjectsWithoutAction(objects, additive);

        if (mSelectedObjects != prevSelectedObjects)
        {
            auto selectionAction = mmake<SelectAction>(mSelectedObjects, prevSelectedObjects);
            o2EditorSceneWindow.DoneAction(selectionAction);
        }
    }

    void SceneEditScreen::SelectObject(const Ref<SceneEditableObject>& actor, bool additive /*= true*/)
    {
        auto prevSelectedObjects = mSelectedObjects;

        SelectObjectWithoutAction(actor, additive);

        if (mSelectedObjects != prevSelectedObjects)
        {
            auto selectionAction = mmake<SelectAction>(mSelectedObjects, prevSelectedObjects);
            o2EditorSceneWindow.DoneAction(selectionAction);
        }
    }

    void SceneEditScreen::SelectAllObjects()
    {
        auto prevSelectedObjects = mSelectedObjects;

        mSelectedObjects = o2Scene.GetAllActors().
            FindAll([](auto& x) { return !x.Lock()->IsLockedInHierarchy(); }).
            Convert<Ref<SceneEditableObject>>([](auto& x) { return DynamicCast<SceneEditableObject>(x.Lock()); });

        mNeedRedraw = true;
        OnObjectsSelectedFromThis();

        if (mSelectedObjects != prevSelectedObjects)
        {
            auto selectionAction = mmake<SelectAction>(mSelectedObjects, prevSelectedObjects);
            o2EditorSceneWindow.DoneAction(selectionAction);
        }
    }

    void SceneEditScreen::ClearSelection()
    {
        auto prevSelectedObjects = mSelectedObjects;

        ClearSelectionWithoutAction();

        if (mSelectedObjects != prevSelectedObjects)
        {
            auto selectionAction = mmake<SelectAction>(mSelectedObjects, prevSelectedObjects);
            o2EditorSceneWindow.DoneAction(selectionAction);
        }
    }

    void SceneEditScreen::SelectObjectsByIdsWithoutAction(const Vector<SceneUID>& ids)
    {
        mSelectedObjects.Clear();
        for (auto id : ids)
        {
            if (auto obj = o2Scene.GetEditableObjectByID(id))
                mSelectedObjects.Add(obj);
        }

        UpdateTopSelectedObjects();
        OnObjectsSelectedFromThis();
        mNeedRedraw = true;
    }

    const Ref<HorizontalLayout>& SceneEditScreen::GetLeftTopWidgetsContainer()
    {
        return mLeftTopWidgetsContainer;
    }

    const Ref<HorizontalLayout>& SceneEditScreen::GetRightTopWidgetsContainer()
    {
        return mRightTopWidgetsContainer;
    }

    const Ref<HorizontalLayout>& SceneEditScreen::GetLeftBottomWidgetsContainer()
    {
        return mLeftBottomWidgetsContainer;
    }

    const Ref<HorizontalLayout>& SceneEditScreen::GetRightBottomWidgetsContainer()
    {
        return mRightBottomWidgetsContainer;
    }

    void SceneEditScreen::AddEditorLayer(const Ref<SceneEditorLayer>& layer)
    {
        mEditorLayers.Add(layer);
    }

    void SceneEditScreen::RemoveEditorLayer(const Ref<SceneEditorLayer>& layer)
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

    void SceneEditScreen::SelectTool(const Ref<IEditTool>& tool)
    {
        if (tool == mEnabledTool)
            return;

        if (mEnabledTool)
            mEnabledTool->OnDisabled();

        mEnabledTool = tool;
        if (auto toggle = mEnabledTool->GetPanelToggle())
            toggle->SetValue(true);

        if (mEnabledTool)
            mEnabledTool->OnEnabled();
    }

    const Ref<IEditTool>& SceneEditScreen::GetSelectedTool() const
    {
        return mEnabledTool;
    }

    void SceneEditScreen::AddTool(const Ref<IEditTool>& tool)
    {
        mTools.Add(tool);
        o2EditorTools.AddToolToggle(tool->GetPanelToggle());
    }

    void SceneEditScreen::RemoveTool(const Ref<IEditTool>& tool)
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

    const Vector<Ref<IEditTool>>& SceneEditScreen::GetTools() const
    {
        return mTools;
    }

    const Vector<Ref<SceneEditableObject>>& SceneEditScreen::GetSelectedObjects() const
    {
        return mSelectedObjects;
    }

    const Vector<Ref<SceneEditableObject>>& SceneEditScreen::GetTopSelectedObjects() const
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
        return true;
    }

    Ref<RefCounterable> SceneEditScreen::CastToRefCounterable(const Ref<SceneEditScreen>& ref)
    {
        return DynamicCast<Singleton<SceneEditScreen>>(ref);
    }

    void SceneEditScreen::BindSceneTree()
    {
        o2EditorTree.GetSceneTree()->onObjectsSelectionChanged += THIS_FUNC(OnTreeSelectionChanged);
        o2EditorTree.GetDrawOrderTree()->onObjectsSelectionChanged += THIS_FUNC(OnTreeSelectionChanged);

        o2Scene.onObjectsChanged += Function<void(Vector<Ref<SceneEditableObject>>)>(this, &SceneEditScreen::OnSceneChanged);
    }

    void SceneEditScreen::OnTreeSelectionChanged(Vector<Ref<SceneEditableObject>> selectedObjects)
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

        auto selectedIObjects = mSelectedObjects.Convert<IObject*>([](auto& x) { return dynamic_cast<IObject*>(x.Get()); });

        if (mSelectedObjects != prevSelectedObjects ||
            selectedIObjects != o2EditorPropertiesWindow.GetTargets())
        {
            auto selectionAction = mmake<SelectAction>(mSelectedObjects, prevSelectedObjects);
            if (auto sceneWindow = o2EditorWindows.GetWindow<SceneWindow>())
                sceneWindow->DoneAction(selectionAction);

            onSelectionChanged(mSelectedObjects);
            o2EditorPropertiesWindow.SetTargets(selectedIObjects);
        }
    }

    void SceneEditScreen::UpdateTopSelectedObjects()
    {
        mTopSelectedObjects.Clear();
        for (auto& object : mSelectedObjects)
        {
            bool processing = true;

            auto parent = object->GetEditableParent();
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

    void SceneEditScreen::OnSceneChanged(Vector<Ref<SceneEditableObject>> actors)
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

    void SceneEditScreen::SelectObjectsWithoutAction(Vector<Ref<SceneEditableObject>> objects, bool additive /*= true*/)
    {
        if (!additive)
            mSelectedObjects.Clear();

        mSelectedObjects.Add(objects);
        mNeedRedraw = true;

        UpdateTopSelectedObjects();
        OnObjectsSelectedFromThis();
    }

    void SceneEditScreen::SelectObjectWithoutAction(const Ref<SceneEditableObject>& object, bool additive /*= true*/)
    {
        if (!additive)
            mSelectedObjects.Clear();

        mSelectedObjects.Add(object);
        mNeedRedraw = true;

        UpdateTopSelectedObjects();
        OnObjectsSelectedFromThis();
    }

    void SceneEditScreen::OnDropped(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        auto assetsScroll = DynamicCast<AssetsIconsScrollArea>(group);
        if (!assetsScroll)
            return;

        assetsScroll->RegObjectsCreationAction();

        o2UI.FocusWidget(o2EditorTree.GetSceneTree());
        o2EditorTree.GetSceneTree()->SetSelectedObjects(assetsScroll->mInstantiatedSceneDragObjects);

        assetsScroll->mInstantiatedSceneDragObjects.Clear();

        o2Application.SetCursor(CursorType::Arrow);
    }

    void SceneEditScreen::OnDragEnter(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        auto assetsScroll = DynamicCast<AssetsIconsScrollArea>(group);
        if (!assetsScroll)
            return;

        assetsScroll->InstantiateDraggingAssets();
        if (assetsScroll->mInstantiatedSceneDragObjects.Count() > 0)
            o2Application.SetCursor(CursorType::Hand);
    }

    void SceneEditScreen::OnDraggedAbove(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        auto assetsScroll = DynamicCast<AssetsIconsScrollArea>(group);
        if (!assetsScroll)
            return;

        for (auto& object : assetsScroll->mInstantiatedSceneDragObjects)
        {
            object->UpdateTransform();
            Basis transform = object->GetTransform();
            transform.origin = ScreenToScenePoint(o2Input.cursorPos) - (transform.xv + transform.yv) * 0.5f;
            object->SetTransform(transform);
        }
    }

    void SceneEditScreen::OnDragExit(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        auto assetsScroll = DynamicCast<AssetsIconsScrollArea>(group);
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
        if (mParentWidget)
			mParentWidget.Lock()->Focus();

        o2EditorTree.OnSceneFocused();

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
		if (mParentWidget)
			mParentWidget.Lock()->Focus();

		o2EditorTree.OnSceneFocused();

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

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::SceneEditScreen>);
// --- META ---

DECLARE_CLASS(Editor::SceneEditScreen, Editor__SceneEditScreen);
// --- END META ---
