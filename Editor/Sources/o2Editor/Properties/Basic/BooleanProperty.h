#pragma once

#include "o2/Utils/Property.h"
#include "o2Editor/Properties/IPropertyField.h"

using namespace o2;

namespace o2
{
    class Toggle;
}

namespace Editor
{
    // -----------------------
    // Editor boolean property
    // -----------------------
    class BooleanProperty: public TPropertyField<bool>
    {
    public:
        // Default constructor
        BooleanProperty(RefCounter* refCounter);

        // Copy constructor
        BooleanProperty(RefCounter* refCounter, const BooleanProperty& other);

        // Copy operator
        BooleanProperty& operator=(const BooleanProperty& other);

        SERIALIZABLE(BooleanProperty);
        CLONEABLE_REF(BooleanProperty);

    protected:
        Ref<Toggle> mToggle; // Toggle 

    protected:
        // Updates value view
        void UpdateValueView() override;

        // Searches controls widgets and layers and initializes them
        void InitializeControls();
    };
}
// --- META ---

CLASS_BASES_META(Editor::BooleanProperty)
{
    BASE_CLASS(Editor::TPropertyField<bool>);
}
END_META;
CLASS_FIELDS_META(Editor::BooleanProperty)
{
    FIELD().PROTECTED().NAME(mToggle);
}
END_META;
CLASS_METHODS_META(Editor::BooleanProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const BooleanProperty&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateValueView);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
}
END_META;
// --- END META ---
