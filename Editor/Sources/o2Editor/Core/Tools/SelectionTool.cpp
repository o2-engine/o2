#include "o2Editor/stdafx.h"
#include "SelectionTool.h"

#include "o2/Render/Sprite.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Editor/SceneEditableObject.h"
#include "o2Editor/Core/Actions/Select.h"
#include "o2Editor/Core/EditorApplication.h"
#include "o2Editor/SceneWindow/SceneEditScreen.h"
#include "o2Editor/TreeWindow/TreeWindow.h"

namespace Editor
{
    SelectionTool::SelectionTool()
    {
        mSelectionSprite = mmake<Sprite>("ui/UI_Window_place.png");
    }

    SelectionTool::~SelectionTool()
    {}

    String SelectionTool::GetPanelIcon() const
    {
        return "ui/UI4_select_tool.png";
    }

    ShortcutKeys SelectionTool::GetShortcut() const
    {
        return ShortcutKeys('Q');
    }

    void SelectionTool::DrawScene()
    {
        for (auto& object : mCurrentSelectingObjects)
			o2EditorSceneScreen.DrawObjectSelection(object, o2EditorSceneScreen.GetManyObjectsSelectionColor());

		if (mSelectingObjects)
			mSelectionSprite->Draw();
    }

    void SelectionTool::DrawScreen()
    {}

    void SelectionTool::Update(float dt)
    {}

    void SelectionTool::OnEnabled()
    {}

    void SelectionTool::OnDisabled()
    {
        mSelectingObjects = false;
    }

    void SelectionTool::OnObjectsSelectionChanged(const Vector<Ref<SceneEditableObject>>& objects)
    {}

    void SelectionTool::OnCursorPressed(const Input::Cursor& cursor)
    {
        mPressPoint = cursor.position;
    }

    void SelectionTool::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (mSelectingObjects)
        {
            o2EditorSceneScreen.SelectObjectsWithoutAction(mCurrentSelectingObjects, true);
            mCurrentSelectingObjects.Clear();
            mSelectingObjects = false;

            auto selectionAction = mmake<SelectAction>(o2EditorSceneScreen.GetSelectedObjects(), mBeforeSelectingObjects);
            o2EditorApplication.DoneAction(selectionAction);
        }
        else
        {
            bool selected = false;
            Vec2F sceneSpaceCursor = o2EditorSceneScreen.ScreenToScenePoint(cursor.position);
            auto& drawnObjects = o2Scene.GetDrawnEditableObjects();

            int startIdx = drawnObjects.Count() - 1;
            if (!o2EditorSceneScreen.GetSelectedObjects().IsEmpty())
                startIdx = drawnObjects.IndexOf(o2EditorSceneScreen.GetSelectedObjects().Last()) - 1;

            for (int i = startIdx; i >= 0; i--)
            {
                auto object = drawnObjects[i].Lock();
                if (!object->IsLockedInHierarchy() && object->GetTransform().IsPointInside(sceneSpaceCursor))
                {
                    mBeforeSelectingObjects = o2EditorSceneScreen.GetSelectedObjects();

                    if (!o2Input.IsKeyDown(VK_CONTROL))
                        o2EditorSceneScreen.ClearSelectionWithoutAction(false);

                    o2EditorSceneScreen.SelectObjectWithoutAction(object);
                    o2EditorTree.HighlightObjectTreeNode(object);
                    selected = true;

                    auto selectionAction = mmake<SelectAction>(o2EditorSceneScreen.GetSelectedObjects(),
                                                               mBeforeSelectingObjects);
                    o2EditorApplication.DoneAction(selectionAction);
                    break;
                }
            }

            if (!o2Input.IsKeyDown(VK_CONTROL) && !selected)
                o2EditorSceneScreen.ClearSelection();
        }
    }

    void SelectionTool::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        if (mSelectingObjects)
        {
            mSelectingObjects = false;
            mCurrentSelectingObjects.Clear();
        }
    }

    void SelectionTool::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (!mSelectingObjects && (mPressPoint - cursor.position).Length() > 5.0f)
        {
            mSelectingObjects = true;

            mBeforeSelectingObjects = o2EditorSceneScreen.GetSelectedObjects();

            if (!o2Input.IsKeyDown(VK_CONTROL))
                o2EditorSceneScreen.ClearSelectionWithoutAction();
        }

        if (mSelectingObjects && cursor.delta.Length() > 0.1f)
        {
            mSelectionSprite->SetRect(RectF(mPressPoint, cursor.position));
            RectF selectionRect(cursor.position, mPressPoint);

            auto currentSelectedObjects = mCurrentSelectingObjects;
            mCurrentSelectingObjects.Clear();
            for (auto& object : currentSelectedObjects)
            {
                if (object->GetTransform().AABB().IsIntersects(selectionRect))
                    mCurrentSelectingObjects.Add(object);
            }

            auto& drawnObjects = o2Scene.GetDrawnEditableObjects();
            for (auto& objectWeak : drawnObjects)
            {
                auto object = objectWeak.Lock();

                if (mCurrentSelectingObjects.Contains(object))
                    continue;

                if (!object->IsLockedInHierarchy() && object->GetTransform().AABB().IsIntersects(selectionRect))
                    mCurrentSelectingObjects.Add(object);
            }

            mNeedRedraw = true;
        }
    }

    void SelectionTool::OnCursorMoved(const Input::Cursor& cursor)
    {}

    void SelectionTool::OnKeyPressed(const Input::Key& key)
    {
        if (key == VK_ESCAPE)
            o2EditorSceneScreen.ClearSelection();

        if (key == 'A' && o2Input.IsKeyDown(VK_CONTROL))
            o2EditorSceneScreen.SelectAllObjects();
    }

}
// --- META ---

DECLARE_CLASS(Editor::SelectionTool, Editor__SelectionTool);
// --- END META ---
