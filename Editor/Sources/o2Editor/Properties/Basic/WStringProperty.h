#pragma once

#include "o2/Utils/Property.h"
#include "o2Editor/Properties/IPropertyField.h"

using namespace o2;

namespace o2
{
    class EditBox;
}

namespace Editor
{
    // -------------------------------
    // Editor string property edit box
    // -------------------------------
    class WStringProperty: public TPropertyField<WString>
    {
    public:
        // Default constructor
        WStringProperty(RefCounter* refCounter);

        // Copy constructor
        WStringProperty(RefCounter* refCounter, const WStringProperty& other);

        // Copy operator
        WStringProperty& operator=(const WStringProperty& other);

        // Changes the text of every target by the function, as one undoable change of the user
        void TransformByUser(const Function<WString(const WString&)>& transform);

        // Returns the text of every target
        Vector<WString> GetValues() const;

        SERIALIZABLE(WStringProperty);
        CLONEABLE_REF(WStringProperty);

    protected:
        Ref<EditBox> mEditBox; // Edit box 

    protected:
        // Updates value view
        void UpdateValueView() override;

        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Edit box change event
        void OnEdited(const WString& data);
    };
}
// --- META ---

CLASS_BASES_META(Editor::WStringProperty)
{
    BASE_CLASS(Editor::TPropertyField<WString>);
}
END_META;
CLASS_FIELDS_META(Editor::WStringProperty)
{
    FIELD().PROTECTED().NAME(mEditBox);
}
END_META;
CLASS_METHODS_META(Editor::WStringProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const WStringProperty&);
    FUNCTION().PUBLIC().SIGNATURE(void, TransformByUser, const Function<WString(const WString&)>&);
    FUNCTION().PUBLIC().SIGNATURE(Vector<WString>, GetValues);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateValueView);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEdited, const WString&);
}
END_META;
// --- END META ---
