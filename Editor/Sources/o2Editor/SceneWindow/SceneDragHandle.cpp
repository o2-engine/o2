#include "o2Editor/stdafx.h"
#include "SceneDragHandle.h"

#include "o2/Application/Application.h"
#include "o2/Render/RectDrawable.h"
#include "o2Editor/Core/WindowsSystem/WindowsManager.h"
#include "o2Editor/SceneWindow/SceneEditScreen.h"
#include "o2Editor/SceneWindow/SceneWindow.h"

namespace Editor
{
    SceneDragHandle::SceneDragHandle(RefCounter* refCounter):
        DragHandle(refCounter)
    {
        if (WindowsManager::IsSingletonInitialzed())
        {
            o2EditorSceneScreen.mDragHandles.Add(Ref(this));
            messageFallDownListener = (ScrollView*)SceneEditScreen::InstancePtr();
        }
    }

    SceneDragHandle::SceneDragHandle(RefCounter* refCounter, const Ref<IRectDrawable>& regular, const Ref<IRectDrawable>& hover /*= nullptr*/,
                                     const Ref<IRectDrawable>& pressed /*= nullptr*/):
        DragHandle(refCounter, regular, hover, pressed)
    {
        if (WindowsManager::IsSingletonInitialzed())
        {
            o2EditorSceneScreen.mDragHandles.Add(Ref(this));
            messageFallDownListener = (ScrollView*)SceneEditScreen::InstancePtr();
        }
    }

    SceneDragHandle::SceneDragHandle(RefCounter* refCounter, const SceneDragHandle& other):
        DragHandle(refCounter, other)
    {
        if (WindowsManager::IsSingletonInitialzed())
        {
            o2EditorSceneScreen.mDragHandles.Add(Ref(this));
            messageFallDownListener = (ScrollView*)SceneEditScreen::InstancePtr();
        }
    }

    SceneDragHandle::~SceneDragHandle()
    {
        if (WindowsManager::IsSingletonInitialzed())
            o2EditorSceneScreen.mDragHandles.RemoveFirst([&](auto& x) { return x == this; });
    }

    SceneDragHandle& SceneDragHandle::operator=(const SceneDragHandle& other)
    {
        DragHandle::operator=(other);
        return *this;
    }

    Vec2F SceneDragHandle::ScreenToLocal(const Vec2F& point)
    {
        return o2EditorSceneScreen.ScreenToScenePoint(DragHandle::ScreenToLocal(point));
    }

    Vec2F SceneDragHandle::LocalToScreen(const Vec2F& point)
    {
        return o2EditorSceneScreen.SceneToScreenPoint(DragHandle::LocalToScreen(point));
    }

    void SceneDragHandle::Draw()
    {
        if (mode == Mode::SceneSpace)
        {
            Vec2F cameraScale = o2EditorSceneScreen.GetCameraScale();
            Vec2F drawablesScale(1.0f/cameraScale.x, 1.0f/cameraScale.y);

            if (mRegularDrawable)
                mRegularDrawable->scale = drawablesScale;

            if (mHoverDrawable)
                mHoverDrawable->scale = drawablesScale;

            if (mPressedDrawable)
                mPressedDrawable->scale = drawablesScale;
        }

        DragHandle::Draw();
    }

    void SceneDragHandle::SetEnabled(bool enable)
    {
        if (mEnabled == enable)
            return;

        DragHandle::SetEnabled(enable);

        if (mEnabled)
            o2EditorSceneScreen.mDragHandles.Add(Ref(this));
        else
            o2EditorSceneScreen.mDragHandles.Remove(Ref(this));
    }
}
// --- META ---

ENUM_META(Editor::SceneDragHandle::Mode)
{
    ENUM_ENTRY(SceneSpace);
    ENUM_ENTRY(ScreenSpace);
}
END_ENUM_META;

DECLARE_CLASS(Editor::SceneDragHandle, Editor__SceneDragHandle);
// --- END META ---
