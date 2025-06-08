#include "o2Editor/stdafx.h"
#include "DockWindowPlace.h"

#include "o2/Render/Render.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2Editor/Windows/DockableWindow.h"

namespace Editor
{
    DockWindowPlace::DockWindowPlace(RefCounter* refCounter):
        Widget(refCounter), DrawableCursorEventsListener(this), mDragHandleLayoutMin(Vec2F(), Vec2F(), Vec2F(), Vec2F()),
        mDragHandleLayoutMax(Vec2F(), Vec2F(), Vec2F(), Vec2F()), mNeighborMax(nullptr), mNeighborMin(nullptr)
    {
        InitializeDragHandle();
        RetargetStatesAnimations();
        SetLayoutDirty();
    }

    DockWindowPlace::DockWindowPlace(RefCounter* refCounter, const DockWindowPlace& other):
        Widget(refCounter, other), DrawableCursorEventsListener(this), mNeighborMax(nullptr), mNeighborMin(nullptr)
    {
        InitializeDragHandle();
        RetargetStatesAnimations();
        SetLayoutDirty();
    }

    DockWindowPlace::~DockWindowPlace()
    {}

    DockWindowPlace& DockWindowPlace::operator=(const DockWindowPlace& other)
    {
        Widget::operator=(other);
        mDragHandleLayoutMin = other.mDragHandleLayoutMin;
        return *this;
    }

    void DockWindowPlace::Draw()
    {
        Widget::Draw();

        mDragHandleMin->OnDrawn();
        mDragHandleMax->OnDrawn();

        if (o2Input.IsKeyDown(VK_F1))
        {
            if (mDragHandleMin->IsInteractable())
                o2Render.DrawAARectFrame(mDragHandleAreaMin, Color4::Blue());

            if (mDragHandleMax->IsInteractable())
                o2Render.DrawAARectFrame(mDragHandleAreaMax, Color4::Red());

            if (mNeighborMin)
            {
                Vec2F c1 = layout->GetWorldRect().Center();
                Vec2F c2 = mNeighborMin.Lock()->layout->GetWorldRect().Center();
                Vec2F n = (c2 - c1).Normalized().Perpendicular()*30.0f;
                o2Render.DrawAABezierCurveArrow(c1, c1 + n, c2 + n, c2, Color4::Blue());
            }

            if (mNeighborMax)
            {
                Vec2F c1 = layout->GetWorldRect().Center();
                Vec2F c2 = mNeighborMax.Lock()->layout->GetWorldRect().Center();
                Vec2F n = (c2 - c1).Normalized().Perpendicular()*30.0f;
                o2Render.DrawAABezierCurveArrow(c1, c1 + n, c2 + n, c2, Color4::Red());
            }
        }
    }

    bool DockWindowPlace::IsUnderPoint(const Vec2F& point)
    {
        return layout->GetWorldRect().IsInside(point);
    }

    void DockWindowPlace::SetResizibleDir(TwoDirection dir, float border,
                                          const Ref<DockWindowPlace>& neighborMin, const Ref<DockWindowPlace>& neighborMax)
    {
        mResizibleDir = dir;
        float border2 = border*2.0f;

        mDragHandleMin->interactable = neighborMin != nullptr;
        mNeighborMin = neighborMin;

        mNeighborMax = neighborMax;
        mDragHandleMax->interactable = neighborMax != nullptr;

        if (dir == TwoDirection::Horizontal)
        {
            mDragHandleLayoutMin = Layout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(-border2, 0), Vec2F(0, 0));
            mDragHandleLayoutMax = Layout(Vec2F(1, 0), Vec2F(1, 1), Vec2F(border2, 0), Vec2F(0, 0));
            mDragHandleMin->cursorType = CursorType::SizeWE;
            mDragHandleMax->cursorType = CursorType::SizeWE;

            if (neighborMin != nullptr)
                layout->offsetLeft = border;

            if (neighborMax != nullptr)
                layout->offsetRight = -border;
        }
        else
        {
            mDragHandleLayoutMin = Layout(Vec2F(0, 0), Vec2F(1, 0), Vec2F(0, -border2), Vec2F(0, 0));
            mDragHandleLayoutMax = Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(0, 0), Vec2F(0, border2));
            mDragHandleMin->cursorType = CursorType::SizeNS;
            mDragHandleMax->cursorType = CursorType::SizeNS;

            if (neighborMin != nullptr)
                layout->offsetBottom = border;

            if (neighborMax != nullptr)
                layout->offsetTop = -border;
        }

        mDragHandleAreaMin = mDragHandleLayoutMin.Calculate(layout->GetWorldRect());
        mDragHandleAreaMax = mDragHandleLayoutMax.Calculate(layout->GetWorldRect());
    }

    TwoDirection DockWindowPlace::GetResizibleDir() const
    {
        return mResizibleDir;
    }

    void DockWindowPlace::ArrangeChildWindows()
    {
        Vector<Ref<DockableWindow>> windows;
        for (auto& child : mChildren)
        {
            if (child->GetType() == TypeOf(DockableWindow))
                windows.Add(DynamicCast<DockableWindow>(child));
        }

        windows.Sort([](auto& a, auto& b) { return a->mTabPosition < b->mTabPosition; });

        if (windows.Count() == 1)
        {
            windows[0]->SetNonTabState();
        }
        else
        {
            float offset = 0;
            int pos = 0;
            for (auto& window : windows)
            {
                window->SetTabState(offset, pos, mChildren[0] == window);
                offset += window->GetTabWidth();

                pos++;
            }

            if (!windows.IsEmpty())
            {
                if (!windows.Any([&](auto& x) { return x->mTabActive; }))
                    SetActiveTab(windows[0]);
            }
        }
    }

    void DockWindowPlace::SetActiveTab(const Ref<DockableWindow>& window)
    {
        Vector<Ref<DockableWindow>> tabWindows;
        for (auto& child : mChildren)
        {
            if (child->GetType() == TypeOf(DockableWindow))
                tabWindows.Add(DynamicCast<DockableWindow>(child));
        }

        mChildrenInheritedDepth.SortBy<int>([](auto& child) { return DynamicCast<DockableWindow>(child)->mTabPosition; });

        for (auto& tabWindow : tabWindows)
        {
            if (tabWindow->mTabActive)
            {
                tabWindow->mTabActive = false;

                if (auto state = tabWindow->GetStateObject("tabActive"))
                    state->SetState(false);
            }
        }

        if (window)
        {
            window->mTabActive = true;

            if (auto state = window->GetStateObject("tabActive"))
                state->SetState(true);
        }
    }

    void DockWindowPlace::UpdateSelfTransform()
{
        Widget::UpdateSelfTransform();
        mDragHandleAreaMin = mDragHandleLayoutMin.Calculate(layout->GetWorldRect());
        mDragHandleAreaMax = mDragHandleLayoutMax.Calculate(layout->GetWorldRect());
    }

    String DockWindowPlace::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void DockWindowPlace::OnDragHandleMinMoved(const Vec2F& delta)
    {
        if (mResizibleDir == TwoDirection::Horizontal)
        {
            float anchorDelta = delta.x / mParentWidget.Lock()->layout->width;
            layout->anchorLeft += anchorDelta;

            mNeighborMin.Lock()->layout->anchorRight += anchorDelta;
        }
        else
        {
            float anchorDelta = delta.y / mParentWidget.Lock()->layout->height;
            layout->anchorBottom += anchorDelta;
            mNeighborMin.Lock()->layout->anchorTop += anchorDelta;
        }
    }

    void DockWindowPlace::OnDragHandleMaxMoved(const Vec2F& delta)
    {
        if (mResizibleDir == TwoDirection::Horizontal)
        {
            float anchorDelta = delta.x / mParentWidget.Lock()->layout->width;
            layout->anchorRight += anchorDelta;

            mNeighborMax.Lock()->layout->anchorLeft += anchorDelta;
        }
        else
        {
            float anchorDelta = delta.y / mParentWidget.Lock()->layout->height;
            layout->anchorTop += anchorDelta;
            mNeighborMax.Lock()->layout->anchorBottom += anchorDelta;
        }
    }

    void DockWindowPlace::CheckInteractable()
    {
        interactable = mChildren.Count([](auto x) { return x->GetType() == TypeOf(DockWindowPlace); }) == 0;

        for (auto& child : mChildren)
        {
            if (child->GetType() == TypeOf(DockWindowPlace))
                DynamicCast<DockWindowPlace>(child)->CheckInteractable();
        }
    }

    void DockWindowPlace::InitializeDragHandle()
    {
        mDragHandleMin = mmake<CursorEventsArea>();
        mDragHandleMin->isUnderPoint = [&](const Vec2F& point) { return mDragHandleAreaMin.IsInside(point); };
        mDragHandleMin->onMoved = [&](const Input::Cursor& cursor) { OnDragHandleMinMoved(cursor.delta); };

        mDragHandleMax = mmake<CursorEventsArea>();
        mDragHandleMax->isUnderPoint = [&](const Vec2F& point) { return mDragHandleAreaMax.IsInside(point); };
        mDragHandleMax->onMoved = [&](const Input::Cursor& cursor) { OnDragHandleMaxMoved(cursor.delta); };
    }

}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::DockWindowPlace>);
// --- META ---

DECLARE_CLASS(Editor::DockWindowPlace, Editor__DockWindowPlace);
// --- END META ---
