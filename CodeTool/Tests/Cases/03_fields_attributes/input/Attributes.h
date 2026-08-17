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
