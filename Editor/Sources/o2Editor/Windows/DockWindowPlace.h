#pragma once

#include "o2/Events/CursorEventsArea.h"
#include "o2/Events/DrawableCursorEventsListener.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Math/Layout.h"
#include "o2/Utils/Math/Rect.h"

using namespace o2;

namespace Editor
{
    FORWARD_CLASS_REF(DockableWindow);

    // ------------------------------------
    // Dockable windows place for attaching
    // ------------------------------------
    class DockWindowPlace: public Widget, public DrawableCursorEventsListener
    {
    public:
        // Default constructor
        DockWindowPlace(RefCounter* refCounter);

        // Copy-constructor
        DockWindowPlace(RefCounter* refCounter, const DockWindowPlace& other);

        // Destructor
        ~DockWindowPlace();

        // Copy-operator
        DockWindowPlace& operator=(const DockWindowPlace& other);

        // Draws widget
        void Draw() override;

        // Sets resizible side and configures drag handle when draggable is true
        void SetResizibleDir(TwoDirection dir, float border,
                             const Ref<DockWindowPlace>& neighborMin, const Ref<DockWindowPlace>& neighborMax);

        // Returns resizible side
        TwoDirection GetResizibleDir() const;

        // Arranging child windows as tabs
        void ArrangeChildWindows();

        // Sets target window as active tab
        void SetActiveTab(const Ref<DockableWindow>& window);

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Updates layout
        void UpdateSelfTransform() override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(DockWindowPlace);
        CLONEABLE_REF(DockWindowPlace);

    protected:
        TwoDirection mResizibleDir; // Resizible dragable side

        WeakRef<DockWindowPlace> mNeighborMin;         // Resizing neighbor, using when dragging this side
        Ref<CursorEventsArea>    mDragHandleMin;       // Separator drag handle
        Layout                   mDragHandleLayoutMin; // Separator drag handle layout
        RectF                    mDragHandleAreaMin;   // Separator drag handle area calculated from mDragHandleLayout

        WeakRef<DockWindowPlace> mNeighborMax;         // Resizing neighbor, using when dragging this side
        Ref<CursorEventsArea>    mDragHandleMax;       // Separator drag handle
        Layout                   mDragHandleLayoutMax; // Separator drag handle layout
        RectF                    mDragHandleAreaMax;   // Separator drag handle area calculated from mDragHandleLayout

    protected:
        // Called when cursor drag handle was moved
        void OnDragHandleMinMoved(const Vec2F& delta);

        // Called when cursor drag handle was moved
        void OnDragHandleMaxMoved(const Vec2F& delta);

        // Checks interactable. If childs more than 0 this will be not interactable
        void CheckInteractable();

        // Initializes drag handle
        void InitializeDragHandle();

        REF_COUNTERABLE_IMPL(Widget);

        friend class DockableWindow;
        friend class WindowsLayout;
    };
}
// --- META ---

CLASS_BASES_META(Editor::DockWindowPlace)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::DrawableCursorEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::DockWindowPlace)
{
    FIELD().PROTECTED().NAME(mResizibleDir);
    FIELD().PROTECTED().NAME(mNeighborMin);
    FIELD().PROTECTED().NAME(mDragHandleMin);
    FIELD().PROTECTED().NAME(mDragHandleLayoutMin);
    FIELD().PROTECTED().NAME(mDragHandleAreaMin);
    FIELD().PROTECTED().NAME(mNeighborMax);
    FIELD().PROTECTED().NAME(mDragHandleMax);
    FIELD().PROTECTED().NAME(mDragHandleLayoutMax);
    FIELD().PROTECTED().NAME(mDragHandleAreaMax);
}
END_META;
CLASS_METHODS_META(Editor::DockWindowPlace)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const DockWindowPlace&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, SetResizibleDir, TwoDirection, float, const Ref<DockWindowPlace>&, const Ref<DockWindowPlace>&);
    FUNCTION().PUBLIC().SIGNATURE(TwoDirection, GetResizibleDir);
    FUNCTION().PUBLIC().SIGNATURE(void, ArrangeChildWindows);
    FUNCTION().PUBLIC().SIGNATURE(void, SetActiveTab, const Ref<DockableWindow>&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragHandleMinMoved, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragHandleMaxMoved, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckInteractable);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeDragHandle);
}
END_META;
// --- END META ---
