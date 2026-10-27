#pragma once

#include "o2/Scene/UI/Widget.h"
#include "o2/Events/DrawableCursorEventsListener.h"

namespace o2
{
    // ---------------------
    // Vertical progress bar
    // ---------------------
    class VerticalScrollBar: public Widget, public DrawableCursorEventsListener
    {
    public:
        PROPERTIES(VerticalScrollBar);
        PROPERTY(float, value, SetValue, GetValue); // Current value property

        PROPERTY(float, minValue, SetMinValue, GetMinValue); // Minimal value property
        PROPERTY(float, maxValue, SetMaxValue, GetMaxValue); // Maximal value property

        PROPERTY(float, scrollSense, SetScrollSense, GetScrollSense);          // Scroll sense coefficient
        PROPERTY(float, scrollSize, SetScrollHandleSize, GetScrollHandleSize); // Scroll handle size

    public:
        Function<void(float)> onChange;       // On value changing event @SCRIPTABLE
        Function<void(float)> onChangeByUser; // On Value changing from user event @SCRIPTABLE
        Function<void(float)> onSmoothChange; // On smooth value changing event

    public:
        // Constructor
        explicit VerticalScrollBar(RefCounter* refCounter);

        // Copy-constructor
        VerticalScrollBar(RefCounter* refCounter, const VerticalScrollBar& other);

        // Destructor
        ~VerticalScrollBar();

        // Copy-operator
        VerticalScrollBar& operator=(const VerticalScrollBar& other);

        // Updates widget and smooth value changing
        void Update(float dt) override;

        // Sets value @SCRIPTABLE
        void SetValue(float value);

        // Sets value forcible, without smoothing @SCRIPTABLE
        void SetValueForcible(float value);

        // Returns value @SCRIPTABLE
        float GetValue() const;

        // Returns smooth value
        float GetSmoothValue() const;

        // Sets minimal value
        void SetMinValue(float minValue);

        // Returns minimal value @SCRIPTABLE
        float GetMinValue() const;

        // Sets maximal value
        void SetMaxValue(float maxValue);

        // Returns maximal value @SCRIPTABLE
        float GetMaxValue() const;

        // Sets value range @SCRIPTABLE
        void SetValueRange(float minValue, float maxValue);

        // Sets scroll sense
        void SetScrollSense(float coef);

        // Returns scroll sense
        float GetScrollSense() const;

        // Sets scroll handle size @SCRIPTABLE
        void SetScrollHandleSize(float size);

        // Returns scroll handle size @SCRIPTABLE
        float GetScrollHandleSize() const;

        // Sets scroll handle minimum size in pixels @SCRIPTABLE
        void SetMinimalScrollHandleSize(float pixelSize);

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns is listener scrollable
        bool IsScrollable() const override;

        // Updates layout
        void UpdateSelfTransform() override;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(VerticalScrollBar);
        CLONEABLE_REF(VerticalScrollBar);

    protected:
        float mValue = 0.0f;       // Current value @SERIALIZABLE
        float mSmoothValue = 0.0f; // Smooth value

        float mMinValue = 0.0f; // Minimum value @SERIALIZABLE
        float mMaxValue = 1.0f; // Maximum value @SERIALIZABLE

        float mScrollSense = 1.0f;           // Scroll sense coefficient @SERIALIZABLE
        float mScrollHandleSize = 0.2f;      // Scroll handle size (in value range) @SERIALIZABLE
        float mScrollhandleMinPxSize = 5.0f; // Minimal scroll size in pixels @SERIALIZABLE

        float mPressHandleOffset = 0.0f; // Value offset when handle was pressed
        bool  mHandlePressed = false;    // True, when handle was pressed

       Ref<WidgetLayer> mHandleLayer; // Handle layer
       Ref<WidgetLayer> mBackLayer;   // Background layer

    protected:
        // Updates layers layouts, calls after updating widget layout
        void UpdateLayersLayouts() override;

        // Called when new layer was added. Here searching bar, back and handle layers
        void OnLayerAdded(const Ref<WidgetLayer>& layer) override;

        // Called when deserialized
        void OnDeserialized(const DataValue& node) override;

        // Called when visible was changed
        void OnEnabled() override;

        // Called when visible was changed
        void OnDisabled() override;

        // Updates bar, back and handle layers layout by value
        void UpdateProgressLayersLayouts();

        // Gets value from cursor position, depends on orientation
        float GetValueFromCursor(const Input::Cursor& cursor);

        // Sets value and calls callback onUserChange
        void SetValueFromUser(float value);

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor moved on this (or moved outside when this was pressed)
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor enters this object
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Called when cursor exits this object
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Called when scrolling
        void OnScrolled(float scroll) override;

        REF_COUNTERABLE_IMPL(Widget);

        friend class ContextMenu;
        friend class CustomList;
        friend class EditBox;
        friend class LongList;
        friend class ScrollArea;
        friend class Tree;
    };
}
// --- META ---

CLASS_BASES_META(o2::VerticalScrollBar)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::DrawableCursorEventsListener);
}
END_META;
CLASS_FIELDS_META(o2::VerticalScrollBar)
{
    FIELD().PUBLIC().NAME(value);
    FIELD().PUBLIC().NAME(minValue);
    FIELD().PUBLIC().NAME(maxValue);
    FIELD().PUBLIC().NAME(scrollSense);
    FIELD().PUBLIC().NAME(scrollSize);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(onChange);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(onChangeByUser);
    FIELD().PUBLIC().NAME(onSmoothChange);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mValue);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mSmoothValue);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mMinValue);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mMaxValue);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mScrollSense);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.2f).NAME(mScrollHandleSize);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(5.0f).NAME(mScrollhandleMinPxSize);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mPressHandleOffset);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHandlePressed);
    FIELD().PROTECTED().NAME(mHandleLayer);
    FIELD().PROTECTED().NAME(mBackLayer);
}
END_META;
CLASS_METHODS_META(o2::VerticalScrollBar)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const VerticalScrollBar&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetValue, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetValueForcible, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetValue);
    FUNCTION().PUBLIC().SIGNATURE(float, GetSmoothValue);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMinValue, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetMinValue);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaxValue, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetMaxValue);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetValueRange, float, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScrollSense, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetScrollSense);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetScrollHandleSize, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetScrollHandleSize);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetMinimalScrollHandleSize, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsScrollable);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLayersLayouts);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLayerAdded, const Ref<WidgetLayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateProgressLayersLayouts);
    FUNCTION().PROTECTED().SIGNATURE(float, GetValueFromCursor, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetValueFromUser, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnScrolled, float);
}
END_META;
// --- END META ---
