#include "o2Editor/stdafx.h"
#include "SelectionTool.h"

#include "o2/Application/VKCodes.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Editor/SceneEditableObject.h"
#include "o2Editor/Actions/Select.h"
#include "o2Editor/EditorApplication.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"
#include "o2Editor/Windows/TreeWindow/TreeWindow.h"
#include "o2Editor/Windows/WindowsManager.h"

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
        return ShortcutKeys({VK_Q});
    }

    void SelectionTool::DrawScene()
    {
        for (auto& object : mCurrentSelectingObjects)
			o2EditorSceneScreen.DrawObjectSelection(object, o2EditorSceneScreen.GetManyObjectsSelectionColor());

		if (mSelectingByFrame)
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
        mSelectingByFrame = false;
    }

    void SelectionTool::OnObjectsSelectionChanged(const Vector<Ref<SceneEditableObject>>& objects)
    {}

    void SelectionTool::OnCursorPressed(const Input::Cursor& cursor)
    {
        mPressPoint = cursor.position;
    }

    void SelectionTool::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (mSelectingByFrame)
        {
            o2EditorSceneScreen.SelectObjectsWithoutAction(mCurrentSelectingObjects, true);
            mCurrentSelectingObjects.Clear();
            mSelectingByFrame = false;

            auto selectionAction = mmake<SelectAction>(o2EditorSceneScreen.GetSelectedObjects(), mBeforeSelectingObjects);
            o2EditorSceneWindow.DoneAction(selectionAction);
        }
        else
        {
            bool selected = false;
            Vec2F sceneSpaceCursor = cursor.position;
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
                    o2EditorSceneWindow.DoneAction(selectionAction);
                    break;
                }
            }

            if (!o2Input.IsKeyDown(VK_CONTROL) && !selected)
                o2EditorSceneScreen.ClearSelection();
        }
    }

    void SelectionTool::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        if (mSelectingByFrame)
        {
            mSelectingByFrame = false;
            mCurrentSelectingObjects.Clear();
        }
    }

    void SelectionTool::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (!mSelectingByFrame && (mPressPoint - cursor.position).Length() > 5.0f)
        {
            mSelectingByFrame = true;

            mBeforeSelectingObjects = o2EditorSceneScreen.GetSelectedObjects();

            if (!o2Input.IsKeyDown(VK_CONTROL))
                o2EditorSceneScreen.ClearSelectionWithoutAction();
        }

        if (mSelectingByFrame && cursor.delta.Length() > 0.1f)
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
