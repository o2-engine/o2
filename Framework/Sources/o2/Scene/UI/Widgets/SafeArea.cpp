#include "o2/stdafx.h"
#include "SafeArea.h"

#include "o2/Application/Application.h"
#include "o2/Render/Render.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    SafeArea::SafeArea(RefCounter* refCounter):
        Widget(refCounter)
    {}

    SafeArea::SafeArea(RefCounter* refCounter, const SafeArea& other):
        Widget(refCounter, other), left(other.left), right(other.right), top(other.top), bottom(other.bottom)
    {}

    SafeArea& SafeArea::operator=(const SafeArea& other)
    {
        Widget::operator=(other);

        left = other.left;
        right = other.right;
        top = other.top;
        bottom = other.bottom;

        return *this;
    }

    Ref<CameraActor> SafeArea::GetCamera() const
    {
        for (auto parent = mParent.Lock(); parent; parent = parent->GetParent().Lock())
        {
            if (auto camera = DynamicCast<CameraActor>(parent))
                return camera;
        }

        return nullptr;
    }

    RectF SafeArea::GetSafeWorldRect() const
    {
        auto camera = GetCamera();
        if (!camera)
            return RectF();

        RectF frame = camera->transform->GetWorldRect();
        Vec2F resolution = camera->GetRenderResolution();
        if (resolution.x < 1.0f || resolution.y < 1.0f)
        {
            if (!Render::IsSingletonInitialzed())
                return frame;

            resolution = o2Render.GetResolution();
            if (resolution.x < 1.0f || resolution.y < 1.0f)
                return frame;
        }

        // the insets come in the units of the content, the camera stretches its frame over it
        Vec2F unit(frame.Width()/resolution.x, frame.Height()/resolution.y);
        BorderF insets = Integration::IsSingletonInitialzed() ? o2Integration.GetSafeAreaInsets() : BorderF();

        return RectF(frame.left + (left ? insets.left*unit.x : 0.0f),
                     frame.bottom + (bottom ? insets.bottom*unit.y : 0.0f),
                     frame.right - (right ? insets.right*unit.x : 0.0f),
                     frame.top - (top ? insets.top*unit.y : 0.0f));
    }

    void SafeArea::Fit()
    {
        if (!mParent)
            return;

        // in a prototype opened alone there is no camera: the area keeps the layout it was given there
        if (!GetCamera())
        {
            if (!mWarnedNoCamera && !o2Integration.IsEditor())
            {
                o2Debug.LogWarning("Safe area \"" + mName + "\" is not inside a camera actor: it keeps its own layout");
                mWarnedNoCamera = true;
            }

            return;
        }

        // anchored to the parent's corners, the offsets are the distances to its sides; an empty area starts from
        // the whole parent, its layout space must have a size to map world points into
        bool emptyArea = layout->mWorldBox.ToRect().Width() < 1.0f || layout->mWorldBox.ToRect().Height() < 1.0f;
        if (layout->GetAnchorMin() != Vec2F() || layout->GetAnchorMax() != Vec2F(1, 1) || emptyArea)
        {
            layout->SetAnchorMin(Vec2F());
            layout->SetAnchorMax(Vec2F(1, 1));
            if (emptyArea)
            {
                layout->SetOffsetMin(Vec2F());
                layout->SetOffsetMax(Vec2F());
            }
        }

        if (layout->IsDirty())
            UpdateTransform();

        // the parents may be scaled (a screen designed in its own units): the safe rectangle goes into the layout
        // space of the area, which maps onto the world by their scales
        RectF safe = GetSafeWorldRect();
        Vec2F a = layout->WorldToLayoutPoint(safe.LeftBottom()), b = layout->WorldToLayoutPoint(safe.RightTop());
        Vec2F min(Math::Min(a.x, b.x), Math::Min(a.y, b.y)), max(Math::Max(a.x, b.x), Math::Max(a.y, b.y));

        // scene layouts are floored to whole units: closer than one is in place
        RectF current = layout->mWorldBox.ToRect();
        if (Math::Abs(min.x - current.left) < 1.0f && Math::Abs(min.y - current.bottom) < 1.0f &&
            Math::Abs(max.x - current.right) < 1.0f && Math::Abs(max.y - current.top) < 1.0f)
        {
            return;
        }

        layout->SetOffsetMin(layout->GetOffsetMin() + min - current.LeftBottom());
        layout->SetOffsetMax(layout->GetOffsetMax() + max - current.RightTop());
        UpdateTransform();
    }

    void SafeArea::OnUpdate(float dt)
    {
        Fit();
        Widget::OnUpdate(dt);
    }

    String SafeArea::GetCreateMenuGroup()
    {
        return "Layout";
    }

    String SafeArea::GetEditorIcon()
    {
        return "ui/UI4_actor_safearea_icon.svg";
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::SafeArea>);
// --- META ---

DECLARE_CLASS(o2::SafeArea, o2__SafeArea);
// --- END META ---
