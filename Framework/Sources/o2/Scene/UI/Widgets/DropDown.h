#pragma once

#include "o2/Scene/UI/Widgets/CustomDropDown.h"

namespace o2
{
    class Text;

    // --------------
    // Text drop down
    // --------------
    class DropDown: public CustomDropDown
    {
    public:
        PROPERTIES(DropDown);
        PROPERTY(WString, value, SelectItemText, GetSelectedItemText); // Current selected item text

    public:
        Function<void(const WString&)> onSelectedText; // Change text selected event

    public:
        // Default constructor
        explicit DropDown(RefCounter* refCounter);

        // Copy-constructor
        DropDown(RefCounter* refCounter, const DropDown& other);

        // Destructor
        ~DropDown();

        // Copy operator
        DropDown& operator=(const DropDown& other);

        // Draws widget
        void Draw() override;

        // Adds new text item and returns position
        int AddItem(const WString& text);

        // Add new text item at position and returns this position
        int AddItem(const WString& text, int position);

        // Adds array of text items
        void AddItems(const Vector<WString>& data);

        // Removes item, if exist
        void RemoveItem(const WString& text);

        // Returns position of item. Returns -1 if can't find item
        int FindItem(const WString& text);

        // Returns item text by position
        WString GetItemText(int position);

        // Returns array of all text items
        Vector<WString> GetAllItemsText() const;

        // Returns current selected text item
        WString GetSelectedItemText();

        // Selects item text
        void SelectItemText(const WString& text);

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        // Returns the icon of the type in the editor
        static String GetEditorIcon();

        SERIALIZABLE(DropDown);
        CLONEABLE_REF(DropDown);

    protected:
        Ref<Text> mSelectedText; // Selected text label, draws separately

    protected:
        // Called when selected item index was changed
        void OnSelectionChanged() override;

        // Called when layer added and updates drawing sequence, searches selected text drawable
        void OnLayerAdded(const Ref<WidgetLayer>& layer) override;
    };
}
// --- META ---

CLASS_BASES_META(o2::DropDown)
{
    BASE_CLASS(o2::CustomDropDown);
}
END_META;
CLASS_FIELDS_META(o2::DropDown)
{
    FIELD().PUBLIC().NAME(value);
    FIELD().PUBLIC().NAME(onSelectedText);
    FIELD().PROTECTED().NAME(mSelectedText);
}
END_META;
CLASS_METHODS_META(o2::DropDown)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const DropDown&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(int, AddItem, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(int, AddItem, const WString&, int);
    FUNCTION().PUBLIC().SIGNATURE(void, AddItems, const Vector<WString>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveItem, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(int, FindItem, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetItemText, int);
    FUNCTION().PUBLIC().SIGNATURE(Vector<WString>, GetAllItemsText);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetSelectedItemText);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectItemText, const WString&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectionChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLayerAdded, const Ref<WidgetLayer>&);
}
END_META;
// --- END META ---
