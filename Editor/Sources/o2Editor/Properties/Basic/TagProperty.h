#pragma once

#include "o2/Scene/Tags.h"
#include "o2Editor/Properties/IPropertyField.h"

namespace o2
{
    class Button;
    class ContextMenu;
    class EditBox;
    class Widget;
}

namespace Editor
{
    // ------------------------
    // Editor tags property box
    // ------------------------
    class TagsProperty: public TPropertyField<TagGroup>
    {
    public:
        // Default constructor
        TagsProperty(RefCounter* refCounter);

        // Copy constructor
        TagsProperty(RefCounter* refCounter, const TagsProperty& other);

        // Copy operator
        TagsProperty& operator=(const TagsProperty& other);

        SERIALIZABLE(TagsProperty);
        CLONEABLE_REF(TagsProperty);

    protected:
        Ref<EditBox>     mEditBox;            // Edit box 
        Ref<ContextMenu> mTagsContext;        // tags context
        bool             mPushingTag = false; // Is pushing tag and we don't need to check edit text

    protected:
        // Updates value view
        void UpdateValueView() override;

        // Sets common value
        void SetCommonValue(const TagGroup& value, bool byUser) override;

        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Updates context menu data with filter
        void UpdateContextData(const WString& filter);

        // Called when edit box changed
        void OnEditBoxChanged(const WString& text);

        // Called when edit box changed
        void OnEditBoxChangeCompleted(const WString& text);

        // Sets tags from string
        void SetTags(const WString& text);

        // Push tag at the end
        void PushTag(String name);
    };
}
// --- META ---

CLASS_BASES_META(Editor::TagsProperty)
{
    BASE_CLASS(Editor::TPropertyField<TagGroup>);
}
END_META;
CLASS_FIELDS_META(Editor::TagsProperty)
{
    FIELD().PROTECTED().NAME(mEditBox);
    FIELD().PROTECTED().NAME(mTagsContext);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPushingTag);
}
END_META;
CLASS_METHODS_META(Editor::TagsProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const TagsProperty&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateValueView);
    FUNCTION().PROTECTED().SIGNATURE(void, SetCommonValue, const TagGroup&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateContextData, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEditBoxChanged, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEditBoxChangeCompleted, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetTags, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, PushTag, String);
}
END_META;
// --- END META ---
