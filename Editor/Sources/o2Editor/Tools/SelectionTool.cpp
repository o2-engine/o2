#include "o2Editor/stdafx.h"
#include "SelectionTool.h"

#include "o2/Application/VKCodes.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Editor/SceneEditableObject.h"
#include "o2Editor/Actions/Select.h"
#include "o2Editor/Tools/ITransformTool.h"
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

		if (mSelectingByFrame && !o2EditorSceneScreen.IsView3DMode())
			mSelectionSprite->Draw();
    }

    void SelectionTool::DrawScreen()
    {
        // In 3D mode the selection rectangle is screen space, drawn in the overlay pass
        if (mSelectingByFrame && o2EditorSceneScreen.IsView3DMode())
            mSelectionSprite->Draw();
    }

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
        mPressScreenPoint = o2Input.GetCursorPos();
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
            bool selected = o2EditorSceneScreen.IsView3DMode()
                ? SelectByClick3D(cursor)
                : SelectByClick2D(cursor);

            if (!o2Input.IsKeyDown(VK_CONTROL) && !selected)
                o2EditorSceneScreen.ClearSelection();
        }
    }

    void SelectionTool::ApplyClickSelection(const Ref<SceneEditableObject>& object)
    {
        mBeforeSelectingObjects = o2EditorSceneScreen.GetSelectedObjects();

        if (!o2Input.IsKeyDown(VK_CONTROL))
            o2EditorSceneScreen.ClearSelectionWithoutAction(false);

        o2EditorSceneScreen.SelectObjectWithoutAction(object);
        o2EditorTree.HighlightObjectTreeNode(object);

        auto selectionAction = mmake<SelectAction>(o2EditorSceneScreen.GetSelectedObjects(),
                                                   mBeforeSelectingObjects);
        o2EditorSceneWindow.DoneAction(selectionAction);
    }

    bool SelectionTool::SelectByClick2D(const Input::Cursor& cursor)
    {
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
                ApplyClickSelection(object);
                return true;
            }
        }

        return false;
    }

    bool SelectionTool::SelectByClick3D(const Input::Cursor& cursor)
    {
        Vec3F rayOrigin, rayDirection;
        bool hasRay = o2EditorSceneScreen.ScreenToWorldRay(o2Input.GetCursorPos(), rayOrigin, rayDirection);

        // Ray picking against oriented 3D bounds; flat 2D content falls back to the plane point test.
        // Hits are ordered by ray distance, the nearest wins
        Vector<Pair<float, Ref<SceneEditableObject>>> hits;
        for (auto& objectWeak : o2Scene.GetDrawnEditableObjects())
        {
            auto object = objectWeak.Lock();
            if (!object || object->IsLockedInHierarchy())
                continue;

            float distance;
            if (hasRay && ITransformTool::RayIntersectsObject3D(object, rayOrigin, rayDirection, distance))
                hits.Add({ distance, object });
            else if (object->GetTransform().IsPointInside(cursor.position))
            {
                float planeDistance = hasRay ? (Vec3F(cursor.position.x, cursor.position.y, 0.0f) - rayOrigin).Length()
                                             : FLT_MAX;
                hits.Add({ planeDistance, object });
            }
        }

        if (hits.IsEmpty())
            return false;

        hits.SortBy<float>([](auto& hit) { return hit.first; });

        // Repeated clicks over the same spot cycle through the hits under the cursor
        int pickIndex = 0;
        auto& selectedObjects = o2EditorSceneScreen.GetSelectedObjects();
        if (!selectedObjects.IsEmpty())
        {
            int lastIndex = hits.IndexOf([&](auto& hit) { return hit.second == selectedObjects.Last(); });
            if (lastIndex >= 0)
                pickIndex = (lastIndex + 1)%hits.Count();
        }

        ApplyClickSelection(hits[pickIndex].second);
        return true;
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
        bool is3DMode = o2EditorSceneScreen.IsView3DMode();
        Vec2F screenCursor = o2Input.GetCursorPos();

        float dragDistance = is3DMode
            ? (mPressScreenPoint - screenCursor).Length()
            : (mPressPoint - cursor.position).Length();

        if (!mSelectingByFrame && dragDistance > 5.0f)
        {
            mSelectingByFrame = true;

            mBeforeSelectingObjects = o2EditorSceneScreen.GetSelectedObjects();

            if (!o2Input.IsKeyDown(VK_CONTROL))
                o2EditorSceneScreen.ClearSelectionWithoutAction();
        }

        if (mSelectingByFrame && is3DMode)
        {
            // Screen space rect; an object is selected when its projected world position falls inside
            mSelectionSprite->SetRect(RectF(mPressScreenPoint, screenCursor));
            RectF selectionRect(screenCursor, mPressScreenPoint);

            mCurrentSelectingObjects.Clear();

            // Unity-like frame selection: the projected screen rectangle of the object's
            // 3D bounds corners intersecting the frame selects the object
            Function<Vec2F(const Vec3F&)> projector = [](const Vec3F& worldPoint)
            {
                return o2EditorSceneScreen.World3DToScreenPoint(worldPoint);
            };

            auto& drawnObjects = o2Scene.GetDrawnEditableObjects();
            for (auto& objectWeak : drawnObjects)
            {
                auto object = objectWeak.Lock();
                if (object->IsLockedInHierarchy())
                    continue;

                RectF objectScreenRect;
                if (ITransformTool::GetObjectScreenRect3D(object, projector, objectScreenRect))
                {
                    if (selectionRect.IsIntersects(objectScreenRect))
                        mCurrentSelectingObjects.Add(object);
                }
                else if (selectionRect.IsInside(o2EditorSceneScreen.SceneToScreenPoint(object->GetPivot())))
                    mCurrentSelectingObjects.Add(object);
            }

            mNeedRedraw = true;
            return;
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
