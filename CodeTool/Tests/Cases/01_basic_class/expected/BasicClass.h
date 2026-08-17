#pragma once
#include "o2/Utils/Serialization/Serializable.h"

namespace test
{
    // Simple serializable class with a couple of fields
    class BasicClass: public o2::ISerializable
    {
    public:
        int   publicField = 5;   // @SERIALIZABLE
        float otherField;

        BasicClass();

        void DoSomething();
        int GetValue() const;

        SERIALIZABLE(BasicClass);

    protected:
        bool mProtectedField = false; // @SERIALIZABLE

    private:
        double mPrivateField;
    };
}
// --- META ---

CLASS_BASES_META(test::BasicClass)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(test::BasicClass)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(5).NAME(publicField);
    FIELD().PUBLIC().NAME(otherField);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mProtectedField);
    FIELD().PRIVATE().NAME(mPrivateField);
}
END_META;
CLASS_METHODS_META(test::BasicClass)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, DoSomething);
    FUNCTION().PUBLIC().SIGNATURE(int, GetValue);
}
END_META;
// --- END META ---
