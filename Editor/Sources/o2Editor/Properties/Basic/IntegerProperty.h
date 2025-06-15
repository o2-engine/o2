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
    // --------------------------------
    // Editor integer property edit box
    // --------------------------------
    class IntegerProperty: public TPropertyField<int>, public KeyboardEventsListener
    {
    public:
        // Default constructor
        IntegerProperty(RefCounter* refCounter);

        // Copy constructor
        IntegerProperty(RefCounter* refCounter, const IntegerProperty& other);

        // Copy operator
        IntegerProperty& operator=(const IntegerProperty& other);

        SERIALIZABLE(IntegerProperty);
        CLONEABLE_REF(IntegerProperty);

    protected:
        Ref<EditBox>          mEditBox;    // Edit box 
        Ref<CursorEventsArea> mDragHangle; // Value changing drag handle

    protected:
        // Updates value view
        void UpdateValueView() override;

        // Called when key was released
        void OnKeyReleased(const Input::Key& key) override;

        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Edit box change event
        void OnEdited(const WString& data);

        // Called when drag handle was moved and changes the property value
        void OnDragHandleMoved(const Input::Cursor& cursor);

        // Called when change value move handle pressed, sets cursor infinite mode and stores value to data
        void OnMoveHandlePressed(const Input::Cursor& cursor);

        // Called when change value move handle  released, turns off cursor infinite mode, 
        // checks value was changed then calls value change completed event
        void OnMoveHandleReleased(const Input::Cursor& cursor);

        REF_COUNTERABLE_IMPL(TPropertyField<int>);
    };
}
// --- META ---

CLASS_BASES_META(Editor::IntegerProperty)
{
    BASE_CLASS(Editor::TPropertyField<int>);
    BASE_CLASS(o2::KeyboardEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::IntegerProperty)
{
    FIELD().PROTECTED().NAME(mEditBox);
    FIELD().PROTECTED().NAME(mDragHangle);
}
END_META;
CLASS_METHODS_META(Editor::IntegerProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const IntegerProperty&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateValueView);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyReleased, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEdited, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragHandleMoved, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMoveHandlePressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMoveHandleReleased, const Input::Cursor&);
}
END_META;
// --- END META ---
