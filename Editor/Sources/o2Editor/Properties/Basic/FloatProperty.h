#pragma once

#include "o2/Events/CursorEventsArea.h"
#include "o2/Events/KeyboardEventsListener.h"
#include "o2Editor/Properties/IPropertyField.h"

using namespace o2;

namespace o2
{
    class EditBox;
}

namespace Editor
{
    // ------------------------------
    // Editor float property edit box
    // ------------------------------
    class FloatProperty: public TPropertyField<float>, public KeyboardEventsListener
    {
    public:
        // Default constructor
        FloatProperty(RefCounter* refCounter);

        // Copy constructor
        FloatProperty(RefCounter* refCounter, const FloatProperty& other);

        // Copy operator
        FloatProperty& operator=(const FloatProperty& other);

        // Returns edit box
        const Ref<EditBox>& GetEditBox() const;

        // Specializes field info, checks range attribute
        void SetFieldInfo(const FieldInfo* fieldInfo) override;

        // Enables range mode and sets min and max
        void SetRange(float minRange, float maxRange);

        // Disables range mode
        void DisableRange();

        SERIALIZABLE(FloatProperty);
        CLONEABLE_REF(FloatProperty);

    protected:
        Ref<EditBox>          mEditBox;    // Edit box for regular values
        Ref<CursorEventsArea> mDragHangle; // Value changing drag handle

        Ref<HorizontalProgress> mProgress;          // Progress bar for range values
        bool                    mUsesRange = false; // Is value used with range
        float                   mMinRange = 0.0f;   // Min range value
        float                   mMaxRange = 1.0f;   // Max range value

    protected:
        // Updates value view
        void UpdateValueView() override;

        // Called when key was released
        void OnKeyReleased(const Input::Key& key) override;

        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Edit box change event
        void OnEdited(const WString& data);

        // On value changed by progress
        void OnEditedValue(float value);

        // Called when drag handle was moved and changes the property value
        void OnDragHandleMoved(const Input::Cursor& cursor);

        // Called when change value move handle pressed, sets cursor infinite mode and stores value to data
        void OnMoveHandlePressed(const Input::Cursor& cursor);

        // Called when change value move handle  released, turns off cursor infinite mode, 
        // checks value was changed then calls value change completed event
        void OnMoveHandleReleased(const Input::Cursor& cursor);

        REF_COUNTERABLE_IMPL(TPropertyField<float>);
    };
}
// --- META ---

CLASS_BASES_META(Editor::FloatProperty)
{
    BASE_CLASS(Editor::TPropertyField<float>);
    BASE_CLASS(o2::KeyboardEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::FloatProperty)
{
    FIELD().PROTECTED().NAME(mEditBox);
    FIELD().PROTECTED().NAME(mDragHangle);
    FIELD().PROTECTED().NAME(mProgress);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mUsesRange);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mMinRange);
    FIELD().PROTECTED().DEFAULT_VALUE(1.0f).NAME(mMaxRange);
}
END_META;
CLASS_METHODS_META(Editor::FloatProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const FloatProperty&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<EditBox>&, GetEditBox);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFieldInfo, const FieldInfo*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRange, float, float);
    FUNCTION().PUBLIC().SIGNATURE(void, DisableRange);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateValueView);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyReleased, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEdited, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEditedValue, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragHandleMoved, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMoveHandlePressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMoveHandleReleased, const Input::Cursor&);
}
END_META;
// --- END META ---
