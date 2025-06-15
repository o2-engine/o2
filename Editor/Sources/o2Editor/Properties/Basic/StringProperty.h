#pragma once

#include "o2Editor/Properties/IPropertyField.h"

using namespace o2;

namespace o2
{
    class Button;
    class EditBox;
    class EditBoxDropDown;
    class Widget;
}

namespace Editor
{
    // -------------------------------
    // Editor string property edit box
    // -------------------------------
    class StringProperty: public TPropertyField<String>
    {
    public:
        // Default constructor
        StringProperty(RefCounter* refCounter);

        // Copy constructor
        StringProperty(RefCounter* refCounter, const StringProperty& other);

        // Copy operator
        StringProperty& operator=(const StringProperty& other);

        // Specializes field info, processing attributes
        void SetFieldInfo(const FieldInfo* fieldInfo) override;
        
        // Refreshes field
        void Refresh(bool forcible = false) override;

        SERIALIZABLE(StringProperty);
        CLONEABLE_REF(StringProperty);

    protected:
        Ref<EditBox>         mEditBox;               // Edit box (used when no ItemsSource)
        Ref<EditBoxDropDown> mEditBoxDropDown;       // Edit box with dropdown (used with ItemsSource)
        bool                 mUsingDropDown = false; // Flag indicating if using dropdown
        Vector<String>       mCachedItems;           // Cached dropdown items list

    protected:
        // Updates value view
        void UpdateValueView() override;

        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Edit box change event
        void OnEdited(const WString& data);

        // Updates dropdown items from function in ItemsSource attribute
        void UpdateDropDownItems();
    };
}
// --- META ---

CLASS_BASES_META(Editor::StringProperty)
{
    BASE_CLASS(Editor::TPropertyField<String>);
}
END_META;
CLASS_FIELDS_META(Editor::StringProperty)
{
    FIELD().PROTECTED().NAME(mEditBox);
    FIELD().PROTECTED().NAME(mEditBoxDropDown);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mUsingDropDown);
    FIELD().PROTECTED().NAME(mCachedItems);
}
END_META;
CLASS_METHODS_META(Editor::StringProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const StringProperty&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFieldInfo, const FieldInfo*);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateValueView);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEdited, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateDropDownItems);
}
END_META;
// --- END META ---
