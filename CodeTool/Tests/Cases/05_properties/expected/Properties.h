#pragma once
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Property.h"

namespace test
{
    class Props: public o2::ISerializable
    {
    public:
        PROPERTIES(Props);
        PROPERTY(int, value, SetValue, GetValue);          // Value property
        GETTER(float, readOnly, GetReadOnly);              // Read only getter
        SETTER(bool, writeOnly, SetWriteOnly);             // Write only setter

        void SetValue(int v);
        int GetValue() const;
        float GetReadOnly() const;
        void SetWriteOnly(bool v);

        SERIALIZABLE(Props);

    protected:
        int mValue = 0; // @SERIALIZABLE
    };
}
// --- META ---

CLASS_BASES_META(test::Props)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(test::Props)
{
    FIELD().PUBLIC().NAME(value);
    FIELD().PUBLIC().NAME(readOnly);
    FIELD().PUBLIC().NAME(writeOnly);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(mValue);
}
END_META;
CLASS_METHODS_META(test::Props)
{

    FUNCTION().PUBLIC().SIGNATURE(void, SetValue, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetValue);
    FUNCTION().PUBLIC().SIGNATURE(float, GetReadOnly);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWriteOnly, bool);
}
END_META;
// --- END META ---
