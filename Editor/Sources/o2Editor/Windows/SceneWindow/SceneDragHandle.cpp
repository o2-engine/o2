#include "o2Editor/stdafx.h"
#include "SceneDragHandle.h"

#include "o2/Application/Application.h"
#include "o2/Render/IRectDrawable.h"
#include "o2Editor/Windows/WindowsManager.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"

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
        // In 3D mode cursor events come in screen space, handle position stays in plane space
        if (SceneEditScreen::IsSingletonInitialzed() && o2EditorSceneScreen.IsView3DMode())
            return o2EditorSceneScreen.ScreenToScenePoint(point);

        return point;
    }

    Vec2F SceneDragHandle::LocalToScreen(const Vec2F& point)
    {
        if (SceneEditScreen::IsSingletonInitialzed() && o2EditorSceneScreen.IsView3DMode())
            return o2EditorSceneScreen.World3DToScreenPoint(Vec3F(point.x, point.y, mPositionZ));

        return point;
    }

    void SceneDragHandle::SetPositionZ(float z)
    {
        if (Math::Equals(mPositionZ, z))
            return;

        mPositionZ = z;
        UpdateScreenPosition();
    }

    float SceneDragHandle::GetPositionZ() const
    {
        return mPositionZ;
    }

    void SceneDragHandle::Draw()
    {
        Vec2F cameraScale = o2EditorSceneScreen.GetCameraScale();
        Vec2F drawablesScale(cameraScale.x, cameraScale.y);

        if (mRegularDrawable)
            mRegularDrawable->scale2D = drawablesScale;

        if (mHoverDrawable)
            mHoverDrawable->scale2D = drawablesScale;

        if (mPressedDrawable)
            mPressedDrawable->scale2D = drawablesScale;  

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

DECLARE_CLASS(Editor::SceneDragHandle, Editor__SceneDragHandle);
// --- END META ---
