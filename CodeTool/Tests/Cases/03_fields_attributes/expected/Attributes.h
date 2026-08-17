#pragma once
#include "o2/Utils/Serialization/Serializable.h"

namespace test
{
    class Attributes: public o2::ISerializable
    {
    public:
        int serializableField = 1;    // @SERIALIZABLE
        int editorProperty = 2;       // @EDITOR_PROPERTY
        int ignoredField = 3;         // @IGNORE
        int editorIgnored = 4;        // @EDITOR_IGNORE
        int animatableField = 5;      // @SERIALIZABLE @ANIMATABLE
        int scriptableField = 6;      // @SCRIPTABLE

        o2::String  stringField = "text";  // @SERIALIZABLE
        o2::Vector<int> vectorField;       // @SERIALIZABLE

        SERIALIZABLE(Attributes);

    protected:
        int mProtectedEditorProperty = 7; // @EDITOR_PROPERTY
    };
}
// --- META ---

CLASS_BASES_META(test::Attributes)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(test::Attributes)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1).NAME(serializableField);
    FIELD().PUBLIC().EDITOR_PROPERTY_ATTRIBUTE().DEFAULT_VALUE(2).NAME(editorProperty);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().DEFAULT_VALUE(4).NAME(editorIgnored);
    FIELD().PUBLIC().ANIMATABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(5).NAME(animatableField);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().DEFAULT_VALUE(6).NAME(scriptableField);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE("text").NAME(stringField);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(vectorField);
    FIELD().PROTECTED().EDITOR_PROPERTY_ATTRIBUTE().DEFAULT_VALUE(7).NAME(mProtectedEditorProperty);
}
END_META;
CLASS_METHODS_META(test::Attributes)
{
}
END_META;
// --- END META ---
