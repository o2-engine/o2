#pragma once

#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Scene/UI/Widget.h"

namespace o2
{
    // -----------------------------------------------------
    // Scrolling area with scrollbars and clipping rectangle
    // -----------------------------------------------------
    class ScrollArea: public Widget, public CursorAreaEventsListener
    {
    public:
        PROPERTIES(ScrollArea);
        PROPERTY(Vec2F, scroll, SetScroll, GetScroll);                        // Scroll position property
        PROPERTY(float, horScroll, SetHorizontalScroll, GetHorizontalScroll); // Horizontal scroll position property
        PROPERTY(float, verScroll, SetVerticalScroll, GetVerticalScroll);     // Vertical scroll position property

    public:
        Function<void(const Vec2F&)> onScrolled; // Scrolling event. Scroll position is parameter

    public:
        // Default constructor
        explicit ScrollArea(RefCounter* refCounter);

        // Copy-constructor
        ScrollArea(RefCounter* refCounter, const ScrollArea& other);

        // Destructor
        ~ScrollArea();

        // Copy-operator
        ScrollArea& operator=(const ScrollArea& other);

        // Draws widget
        void Draw() override;

        // Updates widget
        void Update(float dt) override;

        // Updates children
        void UpdateChildren(float dt) override;

        // Sets scroll position
        void SetScroll(const Vec2F& scroll);

        // Sets scroll position
        void SetScrollForcible(const Vec2F& scroll);

        // Returns scroll position
        Vec2F GetScroll() const;

        // Returns scroll range by width and height
        RectF GetScrollRange() const;

        // Resets scroll to left top corner
        void ResetScroll();

        // Sets horizontal scroll position
        void SetHorizontalScroll(float scroll);

        // Returns horizontal scroll position
        float GetHorizontalScroll() const;

        // Sets vertical scroll position
        void SetVerticalScroll(float scroll);

        // Returns vertical scroll position
        float GetVerticalScroll() const;

        // Sets horizontal scroll bar
        void SetHorizontalScrollBar(const Ref<HorizontalScrollBar>& scrollbar, bool owner = true);

        // Returns horizontal scroll bar
        const Ref<HorizontalScrollBar>& GetHorizontalScrollbar() const;

        // Sets Vertical scroll bar
        void SetVerticalScrollBar(const Ref<VerticalScrollBar>& scrollbar, bool owner = true);

        // Returns Vertical scroll bar
        const Ref<VerticalScrollBar>& GetVerticalScrollbar() const;

        // Sets scroll bars hiding
        void SetEnableScrollsHiding(bool hideScrolls);

        // Returns is scroll bars hiding
        bool IsScrollsHiding();

        // Sets whether scroll bars will be shown when cursor hovers them
        void SetScrollBarsShowingByCursor(bool showByCursor);

        // Returns whether scroll bars will be shown when cursor hovers them
        bool IsScrollBarsShowingByCursor();

        // Sets clipping layout
        void SetClippingLayout(const Layout& clipLayout);

        // Returns clipping layout
        Layout GetClippingLayout() const;

        // Sets view layout
        void SetViewLayout(const Layout& viewLayout);

        // Returns view layout
        Layout GetViewLayout() const;

        // Updates children transforms, calculates scroll rectangle and updates scrollbars
        void UpdateChildrenTransforms() override;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns is listener scrollable
        bool IsScrollable() const override;

        // Returns true when input events can be handled by down listeners
        bool IsInputTransparent() const override;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(ScrollArea);
        CLONEABLE_REF(ScrollArea);

    protected:
        Ref<HorizontalScrollBar> mHorScrollBar; // horizontal scroll bar
        Ref<VerticalScrollBar>   mVerScrollBar; // Vertical scroll bar

        bool mOwnHorScrollBar = false; // True, if this widget is owner of mHorScrollBar
        bool mOwnVerScrollBar = false; // True, if this widget is owner of mVerScrollBar

        bool mLayoutUpdated = false; // This is true when layout was updated on current frame

        Layout mViewAreaLayout = Layout::BothStretch(); // Children view area layout with disabled bars @SERIALIZABLE
        RectF  mAbsoluteViewArea;                       // View area

        Layout mClipAreaLayout = Layout::BothStretch(); // Clipping area layout (relative to this widget layout) @SERIALIZABLE
        RectF  mAbsoluteClipArea;                       // Clipping area

        Vec2F mScrollPos;               // Scroll position @SERIALIZABLE
        bool  mHasScrolled = false;     // True when area has scrolled at this frame, resets to false on update
        Vec2F mScrollSpeed;             // Scrolling speed
        float mScrollSpeedDamp = 7.0f;  // Scroll speed damping  @SERIALIZABLE
        RectF mScrollArea;              // Maximum scroll area size
        RectF mScrollRange;             // Scroll range by width and height
        bool  mEnableHorScroll = false; // Is horizontal scroll enabled (range > 0)
        bool  mEnableVerScroll = false; // Is vertical scroll enabled (range > 0)

        bool  mUnderCursor = false;   // True, when widget is under cursor
        bool  mPressedCursor = false; // True, when widget was pressed by cursor
        Vec2F mPressedScroll;         // Scroll position when cursor was pressed
        Vec2F mPressedCursorPos;      // Cursor position, when was pressed

        float mSpeedUpdTime = 0.0f; // Elapsed time from last scroll speed updating

        bool  mEnableScrollsHiding = true;      // Enables scroll hiding when they are stopped @SERIALIZABLE
        bool  mShowScrollBarsByCursor = false;  // Enables showing scroll bars when cursor hovers them @SERIALIZABLE
        float mLastHorScrollChangeTime = -1.0f; // Last time when horizontal scroll bar was changed
        float mLastVerScrollChangeTime = -1.0f; // Last time when horizontal scroll bar was changed

    protected:
        // Called when listener was drawn
        void OnDrawn() override;

        // Completion deserialization callback
        void OnDeserialized(const DataValue& node) override;

        // Called when child widget was added
        void OnChildAdded(const Ref<Widget>& child) override;

        // Called when child widget was removed
        void OnChildRemoved(const Ref<Widget>& child) override;

        // Sets children world rectangle. Moves children rectangle to scroll position offset
        void SetChildrenWorldRect(const RectF& childrenWorldRect) override;

        // Checks widget clipping by area
        void CheckClipping(const RectF& clipArea) override;

        // Called when scrolling
        void OnScrolled(float scroll) override;

        // Moves scroll position and updates children widgets clipping and layout
        virtual void MoveScrollPosition(const Vec2F& delta);

        // Calculates scroll area
        virtual void CalculateScrollArea();

        // Calculates mScrollArea bounds by new childrenRectangle
        void RecalculateScrollAreaRect(const RectF& childRect, const Vec2F& offset);

        // Initializes mScrollArea by default rectangle and calculates offset
        void InitializeScrollAreaRectCalculation(Vec2F& offset);

        // Updates scroll parameters: clip area, scroll size
        virtual void UpdateScrollParams();

        //Calls when widget was scrolled
        virtual void OnScrolled();

        // Check scroll bars showing
        void CheckScrollBarsVisibility();

        // Checks children clippings
        void CheckChildrenClipping();

        // Called when horizontal scroll bar value was changed
        void OnHorScrollChanged(float value);

        // Called when vertical scroll bar value was changed
        void OnVerScrollChanged(float value);

        REF_COUNTERABLE_IMPL(Widget);

        friend class CustomDropDown;
    };
}
// --- META ---

CLASS_BASES_META(o2::ScrollArea)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::CursorAreaEventsListener);
}
END_META;
CLASS_FIELDS_META(o2::ScrollArea)
{
    FIELD().PUBLIC().NAME(scroll);
    FIELD().PUBLIC().NAME(horScroll);
    FIELD().PUBLIC().NAME(verScroll);
    FIELD().PUBLIC().NAME(onScrolled);
    FIELD().PROTECTED().NAME(mHorScrollBar);
    FIELD().PROTECTED().NAME(mVerScrollBar);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mOwnHorScrollBar);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mOwnVerScrollBar);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mLayoutUpdated);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Layout::BothStretch()).NAME(mViewAreaLayout);
    FIELD().PROTECTED().NAME(mAbsoluteViewArea);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Layout::BothStretch()).NAME(mClipAreaLayout);
    FIELD().PROTECTED().NAME(mAbsoluteClipArea);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mScrollPos);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHasScrolled);
    FIELD().PROTECTED().NAME(mScrollSpeed);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(7.0f).NAME(mScrollSpeedDamp);
    FIELD().PROTECTED().NAME(mScrollArea);
    FIELD().PROTECTED().NAME(mScrollRange);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mEnableHorScroll);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mEnableVerScroll);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mUnderCursor);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPressedCursor);
    FIELD().PROTECTED().NAME(mPressedScroll);
    FIELD().PROTECTED().NAME(mPressedCursorPos);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mSpeedUpdTime);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mEnableScrollsHiding);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mShowScrollBarsByCursor);
    FIELD().PROTECTED().DEFAULT_VALUE(-1.0f).NAME(mLastHorScrollChangeTime);
    FIELD().PROTECTED().DEFAULT_VALUE(-1.0f).NAME(mLastVerScrollChangeTime);
}
END_META;
CLASS_METHODS_META(o2::ScrollArea)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const ScrollArea&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateChildren, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScroll, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScrollForcible, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetScroll);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetScrollRange);
    FUNCTION().PUBLIC().SIGNATURE(void, ResetScroll);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHorizontalScroll, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetHorizontalScroll);
    FUNCTION().PUBLIC().SIGNATURE(void, SetVerticalScroll, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetVerticalScroll);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHorizontalScrollBar, const Ref<HorizontalScrollBar>&, bool);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<HorizontalScrollBar>&, GetHorizontalScrollbar);
    FUNCTION().PUBLIC().SIGNATURE(void, SetVerticalScrollBar, const Ref<VerticalScrollBar>&, bool);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<VerticalScrollBar>&, GetVerticalScrollbar);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnableScrollsHiding, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsScrollsHiding);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScrollBarsShowingByCursor, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsScrollBarsShowingByCursor);
    FUNCTION().PUBLIC().SIGNATURE(void, SetClippingLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(Layout, GetClippingLayout);
    FUNCTION().PUBLIC().SIGNATURE(void, SetViewLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(Layout, GetViewLayout);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateChildrenTransforms);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsScrollable);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsInputTransparent);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDrawn);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildAdded, const Ref<Widget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildRemoved, const Ref<Widget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetChildrenWorldRect, const RectF&);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckClipping, const RectF&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnScrolled, float);
    FUNCTION().PROTECTED().SIGNATURE(void, MoveScrollPosition, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, CalculateScrollArea);
    FUNCTION().PROTECTED().SIGNATURE(void, RecalculateScrollAreaRect, const RectF&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeScrollAreaRectCalculation, Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateScrollParams);
    FUNCTION().PROTECTED().SIGNATURE(void, OnScrolled);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckScrollBarsVisibility);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckChildrenClipping);
    FUNCTION().PROTECTED().SIGNATURE(void, OnHorScrollChanged, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnVerScrollChanged, float);
}
END_META;
// --- END META ---
