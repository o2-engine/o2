#pragma once

#include "o2/Scene/UI/Widget.h"
#include "o2/Events/DrawableCursorEventsListener.h"

namespace o2
{
    // -----------------------
    // Horizontal progress bar
    // -----------------------
    class HorizontalProgress: public Widget, public DrawableCursorEventsListener
    {
    public:
        enum class Orientation { Right, Left };

    public:
        PROPERTIES(HorizontalProgress);
        PROPERTY(float, value, SetValue, GetValue);          // Current value property

        PROPERTY(float, minValue, SetMinValue, GetMinValue); // Minimal value property
        PROPERTY(float, maxValue, SetMaxValue, GetMaxValue); // Maximal value property

        PROPERTY(float, scrollSense, SetScrollSense, GetScrollSense); // Scroll sense coefficient

    public:
        Function<void(float)> onChange;      // On Value changed event
        Function<void(float)> onChangeByUser; // On Value changed by user event

    public:
        // Constructor
        explicit HorizontalProgress(RefCounter* refCounter);

        // Copy-constructor
        HorizontalProgress(RefCounter* refCounter, const HorizontalProgress& other);

        // Destructor
        ~HorizontalProgress();

        // Copy-operator
        HorizontalProgress& operator=(const HorizontalProgress& other);

        // Updates widget and smooth value changing
        void Update(float dt) override;

        // Sets value @SCRIPTABLE
        void SetValue(float value, bool byUser = false);

        // Sets value forcible, without smoothing @SCRIPTABLE
        void SetValueForcible(float value);

        // Returns value @SCRIPTABLE
        float GetValue() const;

        // Sets minimal value
        void SetMinValue(float minValue);

        // Returns minimal value
        float GetMinValue() const;

        // Sets maximal value
        void SetMaxValue(float maxValue);

        // Returns maximal value
        float GetMaxValue() const;

        // Sets value range
        void SetValueRange(float minValue, float maxValue);

        // Sets scroll sense
        void SetScrollSense(float coef);

        // Returns scroll sense
        float GetScrollSense() const;

        // Sets bar orientation
        void SetOrientation(Orientation orientation);

        // Returns bar orientation
        Orientation GetOrientation() const;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns is listener scrollable
        bool IsScrollable() const override;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(HorizontalProgress);
        CLONEABLE_REF(HorizontalProgress);

    protected:
        float        mValue = 0.0f;                     // Current value @SERIALIZABLE
        float        mSmoothValue = 0.0f;               // Current smoothed value
        float        mMinValue = 0.0f;                  // Minimal value @SERIALIZABLE
        float        mMaxValue = 1.0f;                  // Maximal value @SERIALIZABLE
        float        mScrollSense = 1.0f;               // Scroll sense coefficient @SERIALIZABLE
        Orientation  mOrientation = Orientation::Right; // Bar orientation @SERIALIZABLE

        Ref<WidgetLayer> mBarLayer;  // Bar layer
        Ref<WidgetLayer> mBackLayer; // background layer

    protected:
        // Called when new layer was added. Here searching bar, back and handle layers
        void OnLayerAdded(const Ref<WidgetLayer>& layer) override;

        // Called when deserialized
        void OnDeserialized(const DataValue& node) override;

        // Called when visible was changed
        void OnEnabled() override;

        // Called when visible was changed
        void OnDisabled() override;

        // Updates layers layouts, calls after updating widget layout
        void UpdateLayersLayouts() override;

        // Updates bar, back and handle layers layout by value
        void UpdateProgressLayersLayouts();

        // Gets value from cursor position, depends on orientation
        void GetValueFromCursor(const Input::Cursor& cursor);

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
    };
}
// --- META ---

PRE_ENUM_META(o2::HorizontalProgress::Orientation);

CLASS_BASES_META(o2::HorizontalProgress)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::DrawableCursorEventsListener);
}
END_META;
CLASS_FIELDS_META(o2::HorizontalProgress)
{
    FIELD().PUBLIC().NAME(value);
    FIELD().PUBLIC().NAME(minValue);
    FIELD().PUBLIC().NAME(maxValue);
    FIELD().PUBLIC().NAME(scrollSense);
    FIELD().PUBLIC().NAME(onChange);
    FIELD().PUBLIC().NAME(onChangeByUser);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mValue);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mSmoothValue);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mMinValue);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mMaxValue);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mScrollSense);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Orientation::Right).NAME(mOrientation);
    FIELD().PROTECTED().NAME(mBarLayer);
    FIELD().PROTECTED().NAME(mBackLayer);
}
END_META;
CLASS_METHODS_META(o2::HorizontalProgress)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const HorizontalProgress&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetValue, float, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetValueForcible, float);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, GetValue);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMinValue, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetMinValue);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaxValue, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetMaxValue);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueRange, float, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScrollSense, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetScrollSense);
    FUNCTION().PUBLIC().SIGNATURE(void, SetOrientation, Orientation);
    FUNCTION().PUBLIC().SIGNATURE(Orientation, GetOrientation);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsScrollable);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLayerAdded, const Ref<WidgetLayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLayersLayouts);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateProgressLayersLayouts);
    FUNCTION().PROTECTED().SIGNATURE(void, GetValueFromCursor, const Input::Cursor&);
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
